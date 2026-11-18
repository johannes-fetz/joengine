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
#ifdef JO_COMPILE_WITH_AUDIO_SUPPORT
# include "jo/audio.h"
# include "internal/audio.h"
#endif

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
/** @brief Audio ring buffer in sound RAM (with the audio module: the place it gives, see jo_video_queue_audio()) */
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
/** @brief SCSP slots used for the movie audio (left and right: the slots of the last channel of the audio module) */
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
/** @brief The sound chip stopped this many VBLANKs after the end of the sound of the movie (in the zeros after it, see
 *  jo_video_task()): the time played is right within a VBLANK */
# define JO_VIDEO_AUDIO_GUARD_TICKS         (2)
/** @brief The sound queued this many VBLANKs before it plays, ahead of its frames when they are decoded late (see
 *  jo_video_queue_audio_ahead()) */
# define JO_VIDEO_AUDIO_AHEAD_TICKS         (8)

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
    unsigned int                next_audio; // The sounds before it are queued (see jo_video_queue_audio_ahead())
    bool                        playing;
    bool                        paused;
    bool                        frame_ready;
    bool                        ended;
    volatile unsigned int       ticks; // VBLANK count since start
    unsigned int                fields_per_second;
    // Audio
    unsigned char               *pcm; // In sound RAM
    unsigned int                pcm_size;
    bool                        audio_started;
    // The sound played: the ticks at the key on, the samples per tick x 65536, the samples (per channel) written since the
    // key on and the samples cleared after them (see jo_video_queue_audio())
    unsigned int                audio_ticks;
    unsigned int                audio_rate;
    unsigned int                audio_queued;
    unsigned int                audio_cleared;
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
    unsigned int                count;
    int                         read;

    unsigned int                start;

    m = &__jo_video_movie;
    // (an offset or a size of a damaged file near 4 GB: offset + size wrapped around 32 bits, a pointer far from the buffer
    // was returned without reading anything)
    if (offset < m->window_start || size > m->buffer_size - JO_VIDEO_SECTOR_SIZE ||
        offset > 0xFFFFFFFFu - JO_VIDEO_SECTOR_SIZE - size)
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
        // All the sectors missing in one read: one command of the CD block for a frame instead of one per sector (there
        // is room for them: offset + size + a sector fits in the window, checked above)
        count = (offset + size - m->window_end + JO_VIDEO_SECTOR_SIZE - 1) / JO_VIDEO_SECTOR_SIZE;
        __jo_fs_seek_sector(m->file, (int)m->file_sector);
        read = __jo_fs_read_sectors(m->file, (int)count, m->buffer + (m->window_end - m->window_start),
                                    (int)(count * JO_VIDEO_SECTOR_SIZE));
        if (read <= 0)
            return (JO_NULL);
        m->window_end += read;
        m->file_sector += count;
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
        if (chunk[0] == 'F' && chunk[1] == 'D' && chunk[2] == 'S' && chunk[3] == 'C' && pos + 26 <= size)
        {
            m->header.height = jo_video_be32(chunk + 12);
            m->header.width = jo_video_be32(chunk + 16);
            m->header.bpp = chunk[20];
            m->header.audio_channels = chunk[21];
            m->header.audio_bits = chunk[22];
            m->header.audio_frequency = jo_video_be16(chunk + 24);
        }
        else if (chunk[0] == 'S' && chunk[1] == 'T' && chunk[2] == 'A' && chunk[3] == 'B' && pos + 16 <= size)
        {
            m->time_base = jo_video_be32(chunk + 8);
            m->sample_count = jo_video_be32(chunk + 12);
            m->stab = chunk + 16;
            // (the entries of 16 bytes in the header: a damaged count read the entries after it, out of the buffer)
            if (m->sample_count > (size - pos - 16) / 16)
                m->sample_count = (size - pos - 16) / 16;
        }
        // (a length past the header: pos wrapped around 32 bits, the same chunks were read again without end)
        if (length < 8 || length > size - pos)
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
    JO_ZERO(__jo_video_movie.audio_queued);
    JO_ZERO(__jo_video_movie.audio_cleared);
}

/** @brief The samples (per channel) played since the key on, from the time of the movie */
static unsigned int             jo_video_audio_played(const jo_video_movie * const m)
{
    return ((unsigned int)(((unsigned long long)(m->ticks - m->audio_ticks) * m->audio_rate) >> 16));
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
    if (strips && p + 3 <= end && (p[0] != JO_VIDEO_STRIP_INTRA && p[0] != JO_VIDEO_STRIP_INTER) &&
        (p[2] == JO_VIDEO_STRIP_INTRA || p[2] == JO_VIDEO_STRIP_INTER))
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

/** @brief Clear bytes of the audio ring (an odd first byte of 8 bits samples left)
 *  @param dst First byte
 *  @param n Bytes
 */
static void                     jo_video_clear_audio(unsigned char *dst, unsigned int n)
{
    if (((unsigned int)dst & 1) && n > 0)
    {
        ++dst;
        --n;
    }
    // (16 bits writes at the ends, the words of 32 bits between them by the DMA controller: jo_dma_fill32() writes words)
    if (((unsigned int)dst & 2) && n >= 2)
    {
        *(volatile unsigned short *)dst = 0;
        dst += 2;
        n -= 2;
    }
    if (n >= 4)
    {
        jo_dma_fill32(dst, 0, n & ~3u);
        dst += n & ~3u;
    }
    if (n & 2)
        *(volatile unsigned short *)dst = 0;
}

/** @brief Write samples in the audio ring, or zeros (each channel)
 *  @param data Samples of the channels one after the other (per_channel bytes each), JO_NULL: zeros
 *  @param per_channel Bytes of a channel in data
 *  @param from First sample written (per channel, since the key on)
 *  @param count Samples written (per channel)
 */
static void                     jo_video_write_audio(const unsigned char * const data, const unsigned int per_channel,
                                                     const unsigned int from, const unsigned int count, const unsigned int channels,
                                                     const unsigned int half, const unsigned int bytes)
{
    jo_video_movie              *m;
    unsigned int                c;
    unsigned int                n;
    unsigned int                pos;
    unsigned int                done;
    unsigned char               *dst;

    m = &__jo_video_movie;
    for (JO_ZERO(c); c < channels; ++c)
    {
        pos = (from * bytes) % half;
        for (JO_ZERO(done); done < count * bytes; done += n)
        {
            n = JO_MIN(count * bytes - done, half - pos);
            dst = m->pcm + c * half + pos;
            if (data != JO_NULL)
                jo_dma_copy(data + c * per_channel + done, dst, n);
            else
                jo_video_clear_audio(dst, n);
            pos += n;
            if (pos >= half)
                JO_ZERO(pos);
        }
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
    unsigned int                slot;
    unsigned int                offset;
    unsigned int                samples;
    unsigned int                count;
    unsigned int                bytes;
    unsigned int                skip;
    unsigned int                played;
    unsigned int                target;
    unsigned int                end;
    bool                        bits8;

    m = &__jo_video_movie;
#ifdef JO_COMPILE_WITH_AUDIO_SUPPORT
    // The place of the samples and the last channel taken from the audio module at the first sound of the movie (its
    // copied sounds and its channels kept away: they played the samples of the movie). No room: no sound
    if (m->pcm == JO_NULL)
    {
        if (m->pcm_size == 0)
            return;
        if ((offset = __jo_audio_reserve_movie(m->pcm_size)) == 0)
        {
            JO_ZERO(m->pcm_size);
# ifdef JO_DEBUG
            jo_core_error("No room in the sound RAM for the sound of the movie: fewer resident sounds");
# endif
            return;
        }
        m->pcm = (unsigned char *)(JO_SOUND_RAM + offset);
    }
#endif
    channels = m->header.audio_channels ? m->header.audio_channels : 1;
    if (channels > 2)
        channels = 2;
    half = (m->pcm_size / channels) & ~3;
    per_channel = size / channels;
    bits8 = (m->header.audio_bits == 8);
    bytes = bits8 ? 1 : 2;
    samples = half / bytes;
    count = per_channel / bytes;
    // Late (the window of the file too small to queue it ahead): the samples already played dropped (the estimate is right
    // within a VBLANK), the next ones written before they play. Written behind the sound chip, they were heard one turn of
    // the ring later
    JO_ZERO(skip);
    JO_ZERO(played);
    if (m->audio_started)
    {
        played = jo_video_audio_played(m);
        target = m->audio_queued + (m->audio_rate >> 16);
        if (played > target)
            skip = JO_MIN(played - target, count);
    }
    jo_video_write_audio(data + skip * bytes, per_channel, m->audio_queued + skip, count - skip, channels, half, bytes);
    m->audio_queued += count;
    // Zeros after the samples written (a quarter of the ring): played instead of the sound of the last turn of the ring when
    // the movie is late, and at its end before the key off (it cut the sound: a click). Not over the samples not played yet
    if (m->audio_cleared < m->audio_queued)
        m->audio_cleared = m->audio_queued;
    end = m->audio_queued + samples / 4;
    if (m->audio_queued - JO_MIN(played, m->audio_queued) + samples / 4 <= samples && end > m->audio_cleared)
    {
        jo_video_write_audio(JO_NULL, 0, m->audio_cleared, end - m->audio_cleared, channels, half, bytes);
        m->audio_cleared = end;
    }
    if (m->audio_started || !m->header.audio_frequency)
        return;
    m->audio_ticks = m->ticks;
    m->audio_rate = jo_uint_div((unsigned int)m->header.audio_frequency << 16, m->fields_per_second);
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

/** @brief Queue the sound JO_VIDEO_AUDIO_AHEAD_TICKS before it plays, before the frames that come first in the file: the
 *  sound is stored at its time in the file, it was late after heavy frames decoded late (the sound of the last turn of the
 *  ring played). The frames before the sound read with it (kept in the window of the file, if it is big enough)
 */
static void                     jo_video_queue_audio_ahead(void)
{
    jo_video_movie              *m;
    const unsigned char         *entry;
    const unsigned char         *data;
    unsigned int                offset;
    unsigned int                length;
    unsigned int                limit;
    unsigned int                samples;
    unsigned int                first;

    m = &__jo_video_movie;
    if (!m->audio_started)
        return;
    samples = (m->pcm_size / (m->header.audio_channels > 1 ? 2 : 1)) / (m->header.audio_bits == 8 ? 1 : 2);
    limit = jo_video_audio_played(m) + JO_MIN(JO_VIDEO_AUDIO_AHEAD_TICKS * (m->audio_rate >> 16), samples / 4);
    if (m->next_audio < m->next_sample)
        m->next_audio = m->next_sample;
    for (; m->audio_queued < limit && m->next_audio < m->sample_count; ++m->next_audio)
    {
        entry = m->stab + JO_MULT_BY_16(m->next_audio);
        if (jo_video_be32(entry + 8) != JO_VIDEO_AUDIO_SAMPLE)
            continue;
        offset = jo_video_be32(entry) + m->header_size;
        length = jo_video_be32(entry + 4);
        // (the data of the file from the next frame: the data before it is not used anymore)
        first = jo_video_be32(m->stab + JO_MULT_BY_16(m->next_sample)) + m->header_size;
        if (offset < first || offset - first + length + JO_VIDEO_SECTOR_SIZE > m->buffer_size ||
            (data = jo_video_file_data(first, offset - first + length)) == JO_NULL)
            return;
        jo_video_queue_audio(data + (offset - first), length);
    }
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
    jo_video_queue_audio_ahead();
    while (m->next_sample < m->sample_count)
    {
        entry = m->stab + JO_MULT_BY_16(m->next_sample);
        offset = jo_video_be32(entry) + m->header_size;
        length = jo_video_be32(entry + 4);
        stamp = jo_video_be32(entry + 8);
        if (stamp == JO_VIDEO_AUDIO_SAMPLE)
        {
            // (queued ahead)
            if (m->next_sample < m->next_audio)
            {
                ++m->next_sample;
                continue;
            }
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
        // (the sound queued played to its end first: stopped in the zeros after it, without a click)
        if (m->audio_started && jo_video_audio_played(m) < m->audio_queued + JO_VIDEO_AUDIO_GUARD_TICKS * (m->audio_rate >> 16))
            return;
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
#ifdef JO_COMPILE_WITH_AUDIO_SUPPORT
    if (m->pcm != JO_NULL)
    {
        __jo_audio_release_movie();
        m->pcm = JO_NULL;
    }
#endif
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

void                            __jo_video_sprites_freed(const int sprite_id)
{
    if (__jo_video_sprite_id >= sprite_id)
        __jo_video_sprite_id = -1;
    if (__jo_video_back_sprite_id >= sprite_id)
        __jo_video_back_sprite_id = -1;
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
        // (the sprites freed while the movie plays: the frames are not displayed)
        if (__jo_video_back_sprite_id >= 0 && __jo_video_sprite_id >= 0)
        {
            // The new frame goes in the hidden sprite, then the sprites are swapped
            __jo_video_movie.img.data = __jo_video_movie.decode_buffer;
            // The frame is copied by the DMA controller while the game continues (jo_dma_wait() before the next decoding
            // and before the end of the frame)
            __jo_sprite_replace(&__jo_video_movie.img, __jo_video_back_sprite_id, true);
            frame_sprite_id = __jo_video_back_sprite_id;
            __jo_video_back_sprite_id = __jo_video_sprite_id;
            __jo_video_sprite_id = frame_sprite_id;
        }
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
#ifdef JO_COMPILE_WITH_AUDIO_SUPPORT
    m->pcm = JO_NULL;
#else
    m->pcm = JO_VIDEO_PCM_ADDR;
#endif
    m->pcm_size = JO_VIDEO_PCM_SIZE;
    m->fields_per_second = jo_core_get_refresh_rate();
    m->window_start = 0;
    m->window_end = 0;
    m->file_sector = 0;
    m->decode = JO_NULL;
    m->next_sample = 0;
    m->next_audio = 0;
    m->playing = false;
    m->paused = false;
    m->frame_ready = false;
    m->ended = false;
    m->ticks = 0;
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
    // The size of a sprite of the VDP1 (a width multiple of 8 up to 504, up to 255 lines). Checked without JO_DEBUG too: the
    // blocks are drawn 32 bits at a time (an address error with another width), and the size of a damaged header wrapped
    // the size of the decode buffer around 32 bits (the frames were written far past a small block)
    if (m->header.width == 0 || JO_MOD_POW2(m->header.width, 8) != 0 || m->header.width > 504 ||
        m->header.height == 0 || m->header.height > 255)
    {
#ifdef JO_DEBUG
        jo_core_error("%s: Video width must be a multiple of 8 (up to 504), height up to 255", filename);
#endif
        jo_video_stop();
        return (false);
    }
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
        // (the sounds queued ahead are queued again)
        __jo_video_movie.next_audio = __jo_video_movie.next_sample;
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
