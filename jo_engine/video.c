/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2026, Johannes Fetz (johannesfetz@gmail.com)
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of the Johannes Fetz nor the
**       names of its contributors may be used to endorse or promote products
**       derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL Johannes Fetz BE LIABLE FOR ANY
** DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
** INCLUDES
*/
#include <stdbool.h>
#include "jo/conf.h"
#include "jo/types.h"
#include "jo/sega_saturn.h"
#include "jo/smpc.h"
#include "jo/core.h"
#include "jo/tools.h"
#include "jo/malloc.h"
#include "jo/fs.h"
#include "jo/video.h"
#include "jo/colors.h"
#include "jo/image.h"
#include "jo/vdp1_command_pipeline.h"
#include "jo/sprites.h"
#include "internal/sega_saturn.h"
#include "internal/fs.h"
#include "internal/sprites.h"

#ifdef JO_COMPILE_WITH_VIDEO_SUPPORT

/*
** Movie player: FILM container with Cinepak video and raw PCM audio.
**
** FILM file:
**   "FILM" header length (4) version (4) reserved (4)
**   "FDSC" length (4) "cvid" height (4) width (4) bpp (1) audio channels (1)
**          audio bits (1) audio compression (1) audio frequency (2) ...
**   "STAB" length (4) time base (4) sample count (4)
**          sample count * { offset (4) length (4) time stamp (4) duration (4) }
**   data (offsets are relative to the end of the header)
** Audio samples have a time stamp of 0xFFFFFFFF. In a stereo audio sample, the
** left channel data is followed by the right channel data.
**
** Cinepak: the picture is divided in horizontal strips of 4x4 pixels blocks.
** Each block uses 1 (V1) or 4 (V4) entries of a 256 entries codebook. An entry
** is 4 luminance values and 2 chrominance values (2x2 pixels).
*/

/*
** INTERNAL MACROS
*/
/** @brief Audio ring buffer in sound RAM */
# define JO_VIDEO_PCM_ADDR                  ((unsigned char *)(JO_SOUND_RAM + 0x20000))
# define JO_VIDEO_PCM_SIZE                  (4096L * 16)
/** @brief Buffer for the movie file */
# define JO_VIDEO_BUFFER_SIZE               (1024L * 100)
/** @brief Maximum strip count in a frame */
# define JO_VIDEO_MAX_STRIPS                (4)
/** @brief Size of a codebook (256 entries of 2x2 pixels) */
# define JO_VIDEO_CODEBOOK_BYTES            (256 * 4 * 2)
/** @brief Work buffer: codebooks and FILM header */
# define JO_VIDEO_WORK_SIZE                 (40 * 1024)
/** @brief Maximum size of the FILM header */
# define JO_VIDEO_MAX_HEADER_BYTES          (JO_VIDEO_WORK_SIZE - JO_VIDEO_MAX_STRIPS * JO_VIDEO_CODEBOOK_BYTES * 2 - 256)
/** @brief SCSP slots used for the movie audio (left and right) */
# define JO_VIDEO_AUDIO_SLOT                (30)
/** @brief Time stamp of an audio sample in the STAB chunk */
# define JO_VIDEO_AUDIO_SAMPLE              (0xFFFFFFFF)
/** @brief Time stamp bits (bit 31 marks a key frame) */
# define JO_VIDEO_TIME_STAMP_MASK           (0x7FFFFFFF)
/** @brief First bit of a 32 bits flags word (Cinepak) */
# define JO_VIDEO_FLAGS_FIRST_BIT           (0x80000000)
/** @brief Cinepak strip: key frame strip */
# define JO_VIDEO_STRIP_INTRA               (0x10)
/** @brief Cinepak strip: inter frame strip */
# define JO_VIDEO_STRIP_INTER               (0x11)
/*
** Cinepak chunk identifiers
*/
# define JO_VIDEO_CHUNK_V4_CODEBOOK                 (0x2000)
# define JO_VIDEO_CHUNK_V4_CODEBOOK_PARTIAL         (0x2100)
# define JO_VIDEO_CHUNK_V1_CODEBOOK                 (0x2200)
# define JO_VIDEO_CHUNK_V1_CODEBOOK_PARTIAL         (0x2300)
# define JO_VIDEO_CHUNK_V4_CODEBOOK_GREY            (0x2400)
# define JO_VIDEO_CHUNK_V4_CODEBOOK_GREY_PARTIAL    (0x2500)
# define JO_VIDEO_CHUNK_V1_CODEBOOK_GREY            (0x2600)
# define JO_VIDEO_CHUNK_V1_CODEBOOK_GREY_PARTIAL    (0x2700)
/** @brief Vectors of a key frame (V1 and V4 blocks) */
# define JO_VIDEO_CHUNK_VECTORS_INTRA               (0x3000)
/** @brief Vectors of an inter frame (updated blocks only) */
# define JO_VIDEO_CHUNK_VECTORS_INTER               (0x3100)
/** @brief Vectors (V1 blocks only) */
# define JO_VIDEO_CHUNK_VECTORS_V1_ONLY             (0x3200)
/** @brief Sector size */
# define JO_VIDEO_SECTOR_SIZE               (2048)

typedef struct
{
    unsigned int                width;
    unsigned int                height;
    unsigned char               bpp;
    unsigned char               audio_channels;
    unsigned char               audio_bits;
    unsigned short              audio_frequency;
}                               jo_video_header;

typedef struct
{
    void                        *file;
    jo_video_header             header;
    unsigned char               *header_data;
    unsigned int                header_size;
    unsigned int                time_base;
    unsigned int                sample_count;
    const unsigned char         *stab;
    unsigned short              *codebooks; // [strip][V1/V4][256][4]
    // File window
    unsigned char               *buffer;
    unsigned int                buffer_size;
    unsigned int                window_start; // File offset of buffer[0]
    unsigned int                window_end; // File offset after the last byte read
    unsigned int                file_sector; // Next sector to read
    // Playback
    unsigned short              *decode;
    unsigned int                decode_pitch; // In pixels
    unsigned int                next_sample;
    bool                        playing;
    bool                        paused;
    bool                        frame_ready;
    bool                        ended;
    volatile unsigned int       ticks; // VBLANK count since start
    unsigned int                fields_per_second;
    // Audio
    unsigned char               *pcm; // In sound RAM
    unsigned int                pcm_size;
    unsigned int                pcm_write; // Per channel write position (bytes)
    bool                        audio_started;
    // Jo Engine
    void                        *movie_buffer;
    void                        *decode_buffer;
    void                        *work_buffer;
    jo_img                      img;
    int                         event_id;
    int                         vblank_id;
    jo_video_onstop_callback    onstop;
}                               jo_video_movie;

int                             __jo_video_sprite_id;
/** @brief Sprite updated with the next frame (double buffering: the VDP1 can still draw the displayed sprite) */
static int                      __jo_video_back_sprite_id;
static jo_video_movie           __jo_video_movie;

/** @brief Read a big endian 32 bits value
 *  @param p Data
 *  @return Value
 */
static  __jo_force_inline unsigned int  jo_video_be32(const unsigned char *p)
{
    return ((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]);
}

/** @brief Read a big endian 16 bits value
 *  @param p Data
 *  @return Value
 */
static  __jo_force_inline unsigned int  jo_video_be16(const unsigned char *p)
{
    return ((p[0] << 8) | p[1]);
}

/** @brief Get movie data (read the sectors if needed)
 *  @param offset Offset in the file
 *  @param size Data size
 *  @return Pointer to the data or JO_NULL if failed
 *  @remarks The data before offset may be dropped from the buffer
 */
static const unsigned char      *jo_video_file_data(const unsigned int offset, const unsigned int size)
{
    jo_video_movie              *m;
    unsigned int                keep;
    int                         read;

    unsigned int                start;

    m = &__jo_video_movie;
    if (offset < m->window_start)
        return (JO_NULL);
    if (offset + size + JO_VIDEO_SECTOR_SIZE > m->window_start + m->buffer_size)
    {
        // Drop what is before offset. The window starts on a 4 bytes boundary (if there is enough room) so the copy
        // below and the sectors read from the CD stay 32 bits aligned
        start = (size + (offset & 3) <= m->buffer_size - JO_VIDEO_SECTOR_SIZE) ? offset & ~3 : offset;
        keep = m->window_end > start ? m->window_end - start : 0;
        if (keep)
            jo_memcpy(m->buffer, m->buffer + (start - m->window_start), keep);
        m->window_start = start;
        m->window_end = start + keep;
        if (size > m->buffer_size - JO_VIDEO_SECTOR_SIZE)
            return (JO_NULL);
    }
    while (m->window_end < offset + size)
    {
        // The window end is always on a sector boundary of the file, except after compaction
        if (m->file_sector * JO_VIDEO_SECTOR_SIZE < m->window_end)
        {
            // Already partially read: continue from the next sector
            ++m->file_sector;
            continue;
        }
        __jo_fs_seek_sector(m->file, m->file_sector);
        read = __jo_fs_read_sectors(m->file, 1, m->buffer + (m->window_end - m->window_start), JO_VIDEO_SECTOR_SIZE);
        if (read <= 0)
            return (JO_NULL);
        m->window_end += read;
        ++m->file_sector;
    }
    return (m->buffer + (offset - m->window_start));
}

/** @brief Read the FILM header (FDSC and STAB chunks)
 *  @return false if the file is not a FILM file or if the header is too big
 */
static bool                     jo_video_read_header(void)
{
    jo_video_movie              *m;
    const unsigned char         *p;
    const unsigned char         *chunk;
    unsigned int                size;
    unsigned int                pos;
    unsigned int                length;

    m = &__jo_video_movie;
    p = jo_video_file_data(0, 16);
    if (p == JO_NULL || p[0] != 'F' || p[1] != 'I' || p[2] != 'L' || p[3] != 'M')
        return (false);
    size = jo_video_be32(p + 4);
    if (size > JO_VIDEO_MAX_HEADER_BYTES)
        return (false);
    if ((p = jo_video_file_data(0, size)) == JO_NULL)
        return (false);
    jo_memcpy(m->header_data, p, size);
    m->header_size = size;
    for (pos = 16; pos + 8 <= size;)
    {
        chunk = m->header_data + pos;
        length = jo_video_be32(chunk + 4);
        if (chunk[0] == 'F' && chunk[1] == 'D' && chunk[2] == 'S' && chunk[3] == 'C')
        {
            m->header.height = jo_video_be32(chunk + 12);
            m->header.width = jo_video_be32(chunk + 16);
            m->header.bpp = chunk[20];
            m->header.audio_channels = chunk[21];
            m->header.audio_bits = chunk[22];
            m->header.audio_frequency = jo_video_be16(chunk + 24);
        }
        else if (chunk[0] == 'S' && chunk[1] == 'T' && chunk[2] == 'A' && chunk[3] == 'B')
        {
            m->time_base = jo_video_be32(chunk + 8);
            m->sample_count = jo_video_be32(chunk + 12);
            m->stab = chunk + 16;
        }
        if (length < 8)
            break;
        pos += length;
    }
    if (m->time_base == 0)
        m->time_base = 1;
    return (true);
}

/** @brief Stop the movie audio */
static void                     jo_video_stop_audio(void)
{
    if (__jo_video_movie.audio_started)
    {
        // Key off
        JO_SCSP_SLOT(JO_VIDEO_AUDIO_SLOT, JO_SCSP_SLOT_CONTROL) = JO_SCSP_KEY_ON_EXECUTE;
        JO_SCSP_SLOT(JO_VIDEO_AUDIO_SLOT + 1, JO_SCSP_SLOT_CONTROL) = JO_SCSP_KEY_ON_EXECUTE;
    }
    __jo_video_movie.audio_started = false;
}

/** @brief Movie timer (VBLANK callback) */
static void                     jo_video_vblank(void)
{
    if (__jo_video_movie.playing && !__jo_video_movie.paused)
        ++__jo_video_movie.ticks;
}

/*
** Cinepak
*/

/** @brief 2 pixels (may alias the unsigned short pixels of the decode buffer and of the codebooks) */
typedef unsigned int            __attribute__((may_alias)) jo_video_pixels;

/** @brief Convert a Cinepak color to a Saturn color
 *  @param y Luminance (0 to 255)
 *  @param u Chrominance U (-128 to 127)
 *  @param v Chrominance V (-128 to 127)
 *  @return Saturn RGB color
 */
/** @brief Offset of the value 0 in __jo_video_clip (the components are between -256 and 509) */
# define JO_VIDEO_CLIP_OFFSET           (256)

/** @brief Saturn color component (5 bits) of a clamped component: __jo_video_clip[JO_VIDEO_CLIP_OFFSET + x] = clamp(x, 0, 255) >> 3 */
static unsigned char            __jo_video_clip[JO_VIDEO_CLIP_OFFSET + 512];

/** @brief Fill __jo_video_clip */
static void                     jo_video_init_clip_table(void)
{
    register int                i;
    int                         x;

    for (JO_ZERO(i); i < (int)sizeof(__jo_video_clip); ++i)
    {
        x = i - JO_VIDEO_CLIP_OFFSET;
        __jo_video_clip[i] = JO_COLOR_SATURN(x < 0 ? 0 : (x > 255 ? 255 : x));
    }
}

static unsigned short           jo_video_yuv_to_rgb(const int y, const int u, const int v)
{
    int                         r;
    int                         g;
    int                         b;

    // Same as JO_COLOR_RGB() with the components clamped between 0 and 255, without branches
    r = y + (v << 1);
    g = y - (u >> 1) - v;
    b = y + (u << 1);
    return (JO_COLOR_SATURN_RGB(__jo_video_clip[JO_VIDEO_CLIP_OFFSET + r], __jo_video_clip[JO_VIDEO_CLIP_OFFSET + g], __jo_video_clip[JO_VIDEO_CLIP_OFFSET + b]));
}

/** @brief Read a Cinepak codebook
 *  @param book Codebook (256 entries of 4 colors)
 *  @param p Chunk data
 *  @param end End of the chunk
 *  @param partial Only the entries flagged are updated
 *  @param greyscale Entries without chrominance
 */
static void                     jo_video_read_codebook(unsigned short *book, const unsigned char *p, const unsigned char *end,
                                                       const bool partial, const bool greyscale)
{
    unsigned int                flags;
    unsigned int                mask;
    register unsigned int       i;
    register unsigned int       k;
    int                         y[4];
    int                         u;
    int                         v;

    JO_ZERO(flags);
    JO_ZERO(mask);
    for (JO_ZERO(i); i < 256; ++i)
    {
        if (partial)
        {
            if (!mask)
            {
                if (p + 4 > end)
                    return;
                flags = jo_video_be32(p);
                p += 4;
                mask = JO_VIDEO_FLAGS_FIRST_BIT;
            }
            if (!(flags & mask))
            {
                mask >>= 1;
                continue;
            }
            mask >>= 1;
        }
        if (p + (greyscale ? 4 : 6) > end)
            return;
        for (JO_ZERO(k); k < 4; ++k)
            y[k] = p[k];
        if (greyscale)
        {
            JO_ZERO(u);
            JO_ZERO(v);
            p += 4;
        }
        else
        {
            u = (signed char)p[4];
            v = (signed char)p[5];
            p += 6;
        }
        for (JO_ZERO(k); k < 4; ++k)
            book[(i << 2) + k] = jo_video_yuv_to_rgb(y[k], u, v);
    }
}

/** @brief Draw a 4x4 block from one codebook entry (each color covers 2x2 pixels)
 *  @param dst Top left pixel
 *  @param pitch Line size in pixels
 *  @param entry Codebook entry
 */
static void                     jo_video_draw_v1(unsigned short *dst, const unsigned int pitch, const unsigned short *entry)
{
    jo_video_pixels             *d;
    jo_video_pixels             c;
    const unsigned int          pitch2 = JO_DIV_BY_2(pitch);

    // 2 pixels per 32 bits write (dst is 4 bytes aligned: x is a multiple of 4 and the width a multiple of 8)
    d = (jo_video_pixels *)dst;
    c = ((unsigned int)entry[0] << 16) | entry[0];
    d[0] = c;
    d[pitch2] = c;
    c = ((unsigned int)entry[1] << 16) | entry[1];
    d[1] = c;
    d[pitch2 + 1] = c;
    d += pitch;
    c = ((unsigned int)entry[2] << 16) | entry[2];
    d[0] = c;
    d[pitch2] = c;
    c = ((unsigned int)entry[3] << 16) | entry[3];
    d[1] = c;
    d[pitch2 + 1] = c;
}

/** @brief Draw a 4x4 block from four codebook entries
 *  @param dst Top left pixel
 *  @param pitch Line size in pixels
 *  @param v4 V4 codebook
 *  @param idx 4 entry indexes
 */
static void                     jo_video_draw_v4(unsigned short *dst, const unsigned int pitch, const unsigned short *v4, const unsigned char *idx)
{
    jo_video_pixels             *d;
    const jo_video_pixels       *e;
    const unsigned int          pitch2 = JO_DIV_BY_2(pitch);

    // An entry is 2x2 pixels: 2 pixels of a line per 32 bits (big endian: the left pixel is the high half)
    d = (jo_video_pixels *)dst;
    e = (const jo_video_pixels *)&v4[idx[0] << 2];
    d[0] = e[0];
    d[pitch2] = e[1];
    e = (const jo_video_pixels *)&v4[idx[1] << 2];
    d[1] = e[0];
    d[pitch2 + 1] = e[1];
    d += pitch;
    e = (const jo_video_pixels *)&v4[idx[2] << 2];
    d[0] = e[0];
    d[pitch2] = e[1];
    e = (const jo_video_pixels *)&v4[idx[3] << 2];
    d[1] = e[0];
    d[pitch2 + 1] = e[1];
}

/** @brief Decode the blocks of a strip
 *  @param p Chunk data
 *  @param end End of the chunk
 *  @param id Chunk identifier (JO_VIDEO_CHUNK_VECTORS_xxx)
 *  @param v1 V1 codebook
 *  @param v4 V4 codebook
 *  @param top First line of the strip
 *  @param bottom Last line of the strip (exclusive)
 *  @param width Frame width
 */
static void                     jo_video_decode_vectors(const unsigned char *p, const unsigned char *end, const unsigned int id,
                                                        const unsigned short *v1, const unsigned short *v4, const int top, const int bottom, const int width)
{
    jo_video_movie              *m;
    unsigned int                flags;
    unsigned int                mask;
    int                         x;
    int                         y;
    unsigned short              *dst;

    m = &__jo_video_movie;
    JO_ZERO(flags);
    JO_ZERO(mask);
    for (y = top; y < bottom; y += 4)
    {
        for (JO_ZERO(x); x < width; x += 4)
        {
            dst = m->decode + y * m->decode_pitch + x;
            if (id == JO_VIDEO_CHUNK_VECTORS_INTER)
            {
                // Inter frame: first bit tells if the block is updated
                if (!mask)
                {
                    if (p + 4 > end)
                        return;
                    flags = jo_video_be32(p);
                    p += 4;
                    mask = JO_VIDEO_FLAGS_FIRST_BIT;
                }
                if (!(flags & mask))
                {
                    mask >>= 1;
                    continue;
                }
                mask >>= 1;
            }
            if (id != JO_VIDEO_CHUNK_VECTORS_V1_ONLY)
            {
                if (!mask)
                {
                    if (p + 4 > end)
                        return;
                    flags = jo_video_be32(p);
                    p += 4;
                    mask = JO_VIDEO_FLAGS_FIRST_BIT;
                }
                if (flags & mask)
                {
                    mask >>= 1;
                    if (p + 4 > end)
                        return;
                    if (y + 3 < (int)m->header.height)
                        jo_video_draw_v4(dst, m->decode_pitch, v4, p);
                    p += 4;
                    continue;
                }
                mask >>= 1;
            }
            if (p + 1 > end)
                return;
            if (y + 3 < (int)m->header.height)
                jo_video_draw_v1(dst, m->decode_pitch, &v1[(*p) << 2]);
            ++p;
        }
    }
}

/** @brief Decode a Cinepak frame in the decode buffer
 *  @param p Frame data
 *  @param size Frame size
 */
static void                     jo_video_decode_frame(const unsigned char *p, const unsigned int size)
{
    jo_video_movie              *m;
    const unsigned char         *end;
    const unsigned char         *strip_end;
    const unsigned char         *chunk_end;
    unsigned int                frame_flags;
    unsigned int                strips;
    unsigned int                strip_size;
    unsigned int                id;
    unsigned int                chunk_size;
    register unsigned int       s;
    int                         y;
    int                         width;
    int                         strip_height;
    unsigned short              *v1;
    unsigned short              *v4;

    m = &__jo_video_movie;
    if (size < 10 || m->decode == JO_NULL)
        return;
    // The previous frame may still be copied in VRAM by the DMA controller
    jo_dma_wait();
    end = p + size;
    frame_flags = p[0];
    width = jo_video_be16(p + 4);
    if (width > (int)m->header.width)
        width = m->header.width;
    strips = jo_video_be16(p + 8);
    p += 10;
    // Some FILM files have 2 more bytes before the first strip
    if (strips && (p[0] != JO_VIDEO_STRIP_INTRA && p[0] != JO_VIDEO_STRIP_INTER) && (p[2] == JO_VIDEO_STRIP_INTRA || p[2] == JO_VIDEO_STRIP_INTER))
        p += 2;
    JO_ZERO(y);
    for (JO_ZERO(s); s < strips && s < JO_VIDEO_MAX_STRIPS && p + 12 <= end; ++s)
    {
        v1 = m->codebooks + s * 2 * 256 * 4;
        v4 = v1 + 256 * 4;
        // Without the flag, a strip starts with the codebooks of the previous one
        if (s > 0 && !(frame_flags & 1))
            jo_memcpy(v1, v1 - 2 * 256 * 4, 2 * JO_VIDEO_CODEBOOK_BYTES);
        strip_size = jo_video_be16(p + 2);
        strip_height = jo_video_be16(p + 8) - jo_video_be16(p + 4);
        // (a damaged file: a strip out of the frame wrote before or after the decode buffer)
        if (strip_height <= 0 || y + strip_height > (int)m->header.height)
            break;
        strip_end = p + strip_size;
        if (strip_end > end)
            strip_end = end;
        p += 12;
        while (p + 4 <= strip_end)
        {
            id = jo_video_be16(p);
            chunk_size = jo_video_be16(p + 2);
            if (chunk_size < 4)
                break;
            chunk_end = p + chunk_size;
            if (chunk_end > strip_end)
                chunk_end = strip_end;
            switch (id)
            {
                case JO_VIDEO_CHUNK_V4_CODEBOOK: jo_video_read_codebook(v4, p + 4, chunk_end, false, false); break;
                case JO_VIDEO_CHUNK_V4_CODEBOOK_PARTIAL: jo_video_read_codebook(v4, p + 4, chunk_end, true, false); break;
                case JO_VIDEO_CHUNK_V1_CODEBOOK: jo_video_read_codebook(v1, p + 4, chunk_end, false, false); break;
                case JO_VIDEO_CHUNK_V1_CODEBOOK_PARTIAL: jo_video_read_codebook(v1, p + 4, chunk_end, true, false); break;
                case JO_VIDEO_CHUNK_V4_CODEBOOK_GREY: jo_video_read_codebook(v4, p + 4, chunk_end, false, true); break;
                case JO_VIDEO_CHUNK_V4_CODEBOOK_GREY_PARTIAL: jo_video_read_codebook(v4, p + 4, chunk_end, true, true); break;
                case JO_VIDEO_CHUNK_V1_CODEBOOK_GREY: jo_video_read_codebook(v1, p + 4, chunk_end, false, true); break;
                case JO_VIDEO_CHUNK_V1_CODEBOOK_GREY_PARTIAL: jo_video_read_codebook(v1, p + 4, chunk_end, true, true); break;
                case JO_VIDEO_CHUNK_VECTORS_INTRA:
                case JO_VIDEO_CHUNK_VECTORS_INTER:
                case JO_VIDEO_CHUNK_VECTORS_V1_ONLY:
                    jo_video_decode_vectors(p + 4, chunk_end, id, v1, v4, y, y + strip_height, width);
                    break;
                default:
                    break;
            }
            p = chunk_end;
        }
        y += strip_height;
        p = strip_end;
    }
}

/** @brief Copy an audio sample in the audio ring buffer
 *  @param data Audio data (left channel then right channel)
 *  @param size Data size
 *  @remarks The SCSP slots start with the first audio sample
 */
static void                     jo_video_queue_audio(const unsigned char *data, const unsigned int size)
{
    jo_video_movie              *m;
    unsigned int                channels;
    unsigned int                half;
    unsigned int                per_channel;
    unsigned int                c;
    unsigned int                n;
    unsigned int                pos;
    unsigned int                done;
    unsigned int                slot;
    unsigned int                offset;
    unsigned int                samples;
    bool                        bits8;

    m = &__jo_video_movie;
    channels = m->header.audio_channels ? m->header.audio_channels : 1;
    if (channels > 2)
        channels = 2;
    half = (m->pcm_size / channels) & ~3;
    per_channel = size / channels;
    for (JO_ZERO(c); c < channels; ++c)
    {
        pos = m->pcm_write;
        for (JO_ZERO(done); done < per_channel; done += n)
        {
            n = per_channel - done;
            if (n > half - pos)
                n = half - pos;
            jo_dma_copy(data + c * per_channel + done, m->pcm + c * half + pos, n);
            pos += n;
            if (pos >= half)
                JO_ZERO(pos);
        }
    }
    m->pcm_write = (m->pcm_write + per_channel) % half;
    if (m->audio_started || !m->header.audio_frequency)
        return;
    bits8 = (m->header.audio_bits == 8);
    samples = bits8 ? half : (half >> 1);
    for (JO_ZERO(c); c < channels; ++c)
    {
        slot = JO_VIDEO_AUDIO_SLOT + c;
        offset = ((unsigned int)m->pcm - JO_SOUND_RAM) + c * half;
        JO_SCSP_SLOT(slot, JO_SCSP_SLOT_CONTROL) = (bits8 ? JO_SCSP_PCM_8BITS : 0) | JO_SCSP_LOOP_NORMAL | JO_SCSP_START_ADDRESS_HIGH(offset);
        JO_SCSP_SLOT(slot, JO_SCSP_SLOT_START_ADDRESS) = JO_SCSP_START_ADDRESS_LOW(offset);
        JO_SCSP_SLOT(slot, JO_SCSP_SLOT_LOOP_START) = 0;
        JO_SCSP_SLOT(slot, JO_SCSP_SLOT_LOOP_END) = samples - 1;
        JO_SCSP_SLOT(slot, JO_SCSP_SLOT_ENVELOPE_ATTACK) = JO_SCSP_ATTACK_RATE_MAX;
        JO_SCSP_SLOT(slot, JO_SCSP_SLOT_ENVELOPE_RELEASE) = JO_SCSP_RELEASE_RATE_MAX;
        JO_SCSP_SLOT(slot, JO_SCSP_SLOT_TOTAL_LEVEL) = 0;
        JO_SCSP_SLOT(slot, JO_SCSP_SLOT_MODULATION) = 0;
        JO_SCSP_SLOT(slot, JO_SCSP_SLOT_PITCH) = __jo_scsp_pitch(m->header.audio_frequency);
        JO_SCSP_SLOT(slot, JO_SCSP_SLOT_LFO) = 0;
        JO_SCSP_SLOT(slot, JO_SCSP_SLOT_DSP_INPUT) = 0;
        JO_SCSP_SLOT(slot, JO_SCSP_SLOT_MIXER) = JO_SCSP_DIRECT_LEVEL(JO_SCSP_LEVEL_MAX) |
                                                 JO_SCSP_DIRECT_PAN(channels == 1 ? JO_SCSP_PAN_CENTER : (c == 0 ? JO_SCSP_PAN_LEFT : JO_SCSP_PAN_RIGHT));
        JO_SCSP_SLOT(slot, JO_SCSP_SLOT_CONTROL) = (JO_SCSP_SLOT(slot, JO_SCSP_SLOT_CONTROL) & JO_SCSP_KEY_MASK) | JO_SCSP_KEY_ON | JO_SCSP_KEY_ON_EXECUTE;
    }
    m->audio_started = true;
}

/** @brief Decode the next frames (called every frame)
 *  @remarks Audio samples are queued until the next video frame. A video frame is decoded when its time has come
 */
static void                     jo_video_task(void)
{
    jo_video_movie              *m;
    unsigned int                now;
    const unsigned char         *entry;
    const unsigned char         *data;
    unsigned int                offset;
    unsigned int                length;
    unsigned int                stamp;

    m = &__jo_video_movie;
    if (!m->playing || m->paused || m->ended)
        return;
    now = (unsigned int)(((unsigned long long)m->ticks * m->time_base) / m->fields_per_second);
    while (m->next_sample < m->sample_count)
    {
        entry = m->stab + JO_MULT_BY_16(m->next_sample);
        offset = jo_video_be32(entry) + m->header_size;
        length = jo_video_be32(entry + 4);
        stamp = jo_video_be32(entry + 8);
        if (stamp == JO_VIDEO_AUDIO_SAMPLE)
        {
            if ((data = jo_video_file_data(offset, length)) != JO_NULL)
                jo_video_queue_audio(data, length);
#ifdef JO_DEBUG
            else
                jo_core_error("Fail to read the audio sample %d (%d bytes)", m->next_sample, length);
#endif
            ++m->next_sample;
            continue;
        }
        if ((stamp & JO_VIDEO_TIME_STAMP_MASK) > now || m->frame_ready)
            return;
        if ((data = jo_video_file_data(offset, length)) != JO_NULL)
            jo_video_decode_frame(data, length);
#ifdef JO_DEBUG
        else
            jo_core_error("Fail to read the frame %d (%d bytes)", m->next_sample, length);
#endif
        m->frame_ready = true;
        ++m->next_sample;
        return;
    }
    if (!m->frame_ready)
    {
        m->playing = false;
        jo_video_stop_audio();
        m->ended = true;
    }
}

/*
** Jo Engine API
*/

void                            jo_video_stop(void)
{
    jo_video_movie              *m;
    jo_video_onstop_callback    onstop;

    m = &__jo_video_movie;
    m->playing = false;
    jo_video_stop_audio();
    if (m->event_id != -1)
    {
        jo_core_remove_callback(m->event_id);
        m->event_id = -1;
    }
    if (m->file != JO_NULL)
    {
        __jo_fs_close(m->file);
        m->file = JO_NULL;
    }
    if (m->decode_buffer != JO_NULL)
    {
        // The DMA controller may still read the buffer
        jo_dma_wait();
        jo_free(m->decode_buffer);
        m->decode_buffer = JO_NULL;
    }
    if (m->work_buffer != JO_NULL)
    {
        jo_free(m->work_buffer);
        m->work_buffer = JO_NULL;
    }
    if (m->movie_buffer != JO_NULL)
    {
        jo_free(m->movie_buffer);
        m->movie_buffer = JO_NULL;
    }
    if (m->onstop != JO_NULL)
    {
        jo_sprite_free_all();
        // The sprites must be created again by jo_video_open_file()
        __jo_video_sprite_id = -1;
        __jo_video_back_sprite_id = -1;
        // (forgotten before the call: a callback that opens the next movie stopped it again and called itself without end,
        // and the callback given to jo_video_play() for the next movie was cleared after)
        onstop = m->onstop;
        m->onstop = JO_NULL;
        onstop();
    }
}

/** @brief Movie callback (called every frame)
 *  @remarks Updates the sprite when a frame is ready and stops at the end of the movie
 */
static void			            jo_video_callback(void)
{
    int                         frame_sprite_id;

    jo_video_task();
    if (__jo_video_movie.frame_ready)
    {
        // The new frame goes in the hidden sprite, then the sprites are swapped
        __jo_video_movie.img.data = __jo_video_movie.decode_buffer;
        // The frame is copied by the DMA controller while the game continues (jo_dma_wait() before the next decoding
        // and before the end of the frame)
        __jo_sprite_replace(&__jo_video_movie.img, __jo_video_back_sprite_id, true);
        frame_sprite_id = __jo_video_back_sprite_id;
        __jo_video_back_sprite_id = __jo_video_sprite_id;
        __jo_video_sprite_id = frame_sprite_id;
        __jo_video_movie.frame_ready = false;
    }
    if (__jo_video_movie.ended)
        jo_video_stop();
}

bool					        jo_video_open_file(const char *const filename)
{
    jo_video_movie              *m;
    int                         fid;

    m = &__jo_video_movie;
    jo_video_stop();
    fid = __jo_fs_get_file_id(filename);
    if (fid < 0)
    {
#ifdef JO_DEBUG
        jo_core_error("%s: File not found", filename);
#endif
        return (false);
    }
    if ((m->file = __jo_fs_open(fid)) == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("%s: Too many opened files", filename);
#endif
        return (false);
    }
    if ((m->movie_buffer = jo_malloc_with_behaviour(JO_VIDEO_BUFFER_SIZE, JO_MALLOC_TRY_REUSE_BLOCK)) == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("%s: Out of memory #1", filename);
#endif
        jo_video_stop();
        return (false);
    }
    if ((m->work_buffer = jo_malloc_with_behaviour(JO_VIDEO_WORK_SIZE, JO_MALLOC_TRY_REUSE_BLOCK)) == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("%s: Out of memory #2", filename);
#endif
        jo_video_stop();
        return (false);
    }
    m->header_data = JO_NULL;
    m->header_size = 0;
    m->time_base = 0;
    m->sample_count = 0;
    m->stab = JO_NULL;
    m->buffer = (unsigned char *)m->movie_buffer;
    m->buffer_size = JO_VIDEO_BUFFER_SIZE & ~(JO_VIDEO_SECTOR_SIZE - 1);
    m->codebooks = (unsigned short *)m->work_buffer;
    jo_video_init_clip_table();
    m->header_data = ((unsigned char *)m->work_buffer) + JO_VIDEO_MAX_STRIPS * JO_VIDEO_CODEBOOK_BYTES * 2;
    m->pcm = JO_VIDEO_PCM_ADDR;
    m->pcm_size = JO_VIDEO_PCM_SIZE;
    m->fields_per_second = jo_core_get_refresh_rate();
    m->window_start = 0;
    m->window_end = 0;
    m->file_sector = 0;
    m->decode = JO_NULL;
    m->next_sample = 0;
    m->playing = false;
    m->paused = false;
    m->frame_ready = false;
    m->ended = false;
    m->ticks = 0;
    m->pcm_write = 0;
    m->audio_started = false;
    jo_memset(&m->header, 0, sizeof(m->header));
    if (!jo_video_read_header())
    {
#ifdef JO_DEBUG
        jo_core_error("%s: Invalid FILM header", filename);
#endif
        jo_video_stop();
        return (false);
    }
#ifdef JO_DEBUG
    if (JO_MOD_POW2(m->header.width, 8) != 0)
    {
        jo_core_error("%s: Video width must be a multiple of 8", filename);
        jo_video_stop();
        return (false);
    }
#endif
    if ((m->decode_buffer = jo_malloc_with_behaviour(m->header.width * m->header.height * sizeof(jo_color), JO_MALLOC_TRY_REUSE_BLOCK)) == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("%s: Out of memory #3", filename);
#endif
        jo_video_stop();
        return (false);
    }
    m->img.width = m->header.width;
    m->img.height = m->header.height;
    m->img.data = m->decode_buffer;
    // (the sprites of the last movie, another size: a sprite is replaced by one of the same size only)
    if (__jo_video_sprite_id >= 0 && __jo_video_back_sprite_id >= 0 &&
        (jo_sprite_get_width(__jo_video_sprite_id) != m->img.width || jo_sprite_get_height(__jo_video_sprite_id) != m->img.height))
    {
        // (freed if they are the last sprites, else the sprites added after them would be freed too)
        if (JO_MAX(__jo_video_sprite_id, __jo_video_back_sprite_id) != jo_get_last_sprite_id())
        {
#ifdef JO_DEBUG
            jo_core_error("Movie of another size: free the sprites added after the last movie first");
#endif
            jo_video_stop();
            return (false);
        }
        jo_sprite_free_from(JO_MIN(__jo_video_sprite_id, __jo_video_back_sprite_id));
        __jo_video_sprite_id = -1;
        __jo_video_back_sprite_id = -1;
    }
    // 2 sprites: one is displayed while the next frame is copied in the other one
    if (__jo_video_sprite_id < 0)
        __jo_video_sprite_id = jo_sprite_add(&m->img);
    else
        __jo_video_sprite_id = jo_sprite_replace(&m->img, __jo_video_sprite_id);
    if (__jo_video_back_sprite_id < 0)
        __jo_video_back_sprite_id = jo_sprite_add(&m->img);
    else
        __jo_video_back_sprite_id = jo_sprite_replace(&m->img, __jo_video_back_sprite_id);
    if (__jo_video_sprite_id < 0 || __jo_video_back_sprite_id < 0)
    {
        jo_video_stop();
        return (false);
    }
    m->decode = (unsigned short *)m->decode_buffer;
    m->decode_pitch = m->header.width;
    return (true);
}

bool                            jo_video_pause(void)
{
    if (__jo_video_movie.file == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Video not ready");
#endif
        return (false);
    }
    __jo_video_movie.paused = !__jo_video_movie.paused;
    // (the sound chip looped the ring during the pause, then played out of sync: started again with the next sound queued)
    if (__jo_video_movie.paused)
    {
        jo_video_stop_audio();
        __jo_video_movie.pcm_write = 0;
    }
    return (true);
}

bool                            jo_video_play(const jo_video_onstop_callback onstop)
{
    if (__jo_video_movie.file == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Video not ready");
#endif
        return (false);
    }
    if (__jo_video_movie.event_id < 0)
    {
        __jo_video_movie.event_id = jo_core_add_callback(jo_video_callback);
        // (0 when it fails, not a negative id: the movie never played and jo_video_stop() removed the callback 0)
        if (__jo_video_movie.event_id == 0)
        {
            __jo_video_movie.event_id = -1;
#ifdef JO_DEBUG
            jo_core_error("Fail to create video callback");
#endif
            jo_video_stop();
            return (false);
        }
    }
    __jo_video_movie.onstop = onstop;
    __jo_video_movie.playing = true;
    __jo_video_movie.paused = false;
    __jo_video_movie.ended = false;
    return (true);
}

bool		                    jo_video_init(void)
{
    jo_memset(&__jo_video_movie, 0, sizeof(__jo_video_movie));
    __jo_video_movie.event_id = -1;
    __jo_video_sprite_id = -1;
    __jo_video_back_sprite_id = -1;
    __jo_video_movie.vblank_id = jo_core_add_vblank_callback(jo_video_vblank);
    return (true);
}

#endif /* !JO_COMPILE_WITH_VIDEO_SUPPORT */

/*
** END OF FILE
*/
