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
#include "jo/math.h"
#include "jo/fs.h"
#include "jo/tools.h"
#include "jo/malloc.h"
#include "internal/tools.h"
#include "internal/fs.h"

/** @brief Read retry
 */
# define JO_READ_RETRY_COUNT                (10)

/** @brief Spin wait after each retry
 */
# define JO_READ_SPIN_WAIT                  (16)

/** @brief Sector size
 */
# define JO_SECTOR_SIZE                     (2048)

/** @brief Max file opened at the same time
 */

#if JO_MAX_FS_BACKGROUND_JOBS > 0
# define JO_OPEN_MAX						(JO_MAX_FS_BACKGROUND_JOBS)
#else
# define JO_OPEN_MAX						(1)
#endif

/** @brief Sectors taken at most by each call to jo_fs_do_background_jobs() (the ones already read by the drive)
 */
# define JO_MAXIMUM_SECTOR_FETCHED_ONCE_ASYNC     (8)

#ifdef JO_COMPILE_WITH_FS_SUPPORT

/*
 ██████╗██████╗     ██████╗ ██╗      ██████╗  ██████╗██╗  ██╗
██╔════╝██╔══██╗    ██╔══██╗██║     ██╔═══██╗██╔════╝██║ ██╔╝
██║     ██║  ██║    ██████╔╝██║     ██║   ██║██║     █████╔╝
██║     ██║  ██║    ██╔══██╗██║     ██║   ██║██║     ██╔═██╗
╚██████╗██████╔╝    ██████╔╝███████╗╚██████╔╝╚██████╗██║  ██╗
 ╚═════╝╚═════╝     ╚═════╝ ╚══════╝ ╚═════╝  ╚═════╝╚═╝  ╚═╝
*/

/*
** CD block registers
*/
# define JO_CD_HIRQ                         (*(volatile unsigned short *)0x25890008)
# define JO_CD_CR1                          (*(volatile unsigned short *)0x25890018)
# define JO_CD_CR2                          (*(volatile unsigned short *)0x2589001C)
# define JO_CD_CR3                          (*(volatile unsigned short *)0x25890020)
# define JO_CD_CR4                          (*(volatile unsigned short *)0x25890024)
# define JO_CD_DATA                         (*(volatile unsigned int *)0x25818000)
# define JO_CD_INFO                         (*(volatile unsigned short *)0x25898000)

/*
** HIRQ flags
*/
# define JO_CD_HIRQ_CMOK                    (0x0001)
# define JO_CD_HIRQ_DRDY                    (0x0002)
# define JO_CD_HIRQ_ESEL                    (0x0040)

/** @brief Rejected command status */
# define JO_CD_STATUS_REJECT                (0xff)
/** @brief Drive status (CR1 high byte, bits 0-3) */
# define JO_CD_STATUS_MASK                  (0x0f)
/** @brief Get the drive status from the CR1 register value */
# define JO_CD_STATUS(CR1)                  (((CR1) >> 8) & JO_CD_STATUS_MASK)
/** @brief Get the current frame address from the CR3 and CR4 register values (Get status command) */
# define JO_CD_STATUS_FAD(CR3, CR4)         ((((CR3) & 0xff) << 16) | (CR4))
/** @brief Play mode: keep the current play mode */
# define JO_CD_PLAY_MODE_NO_CHANGE          (0x7f)

/*
** CD block commands (CR1 high byte)
*/
# define JO_CD_COMMAND_GET_STATUS           (0x00)
# define JO_CD_COMMAND_GET_TOC              (0x02)
# define JO_CD_COMMAND_INITIALIZE           (0x04)
# define JO_CD_COMMAND_END_TRANSFER         (0x06)
# define JO_CD_COMMAND_PLAY                 (0x10)
# define JO_CD_COMMAND_SEEK                 (0x11)
# define JO_CD_COMMAND_SET_DEVICE_CONNECTION (0x30)
# define JO_CD_COMMAND_SET_FILTER_RANGE     (0x40)
# define JO_CD_COMMAND_SET_FILTER_CONNECTION (0x46)
# define JO_CD_COMMAND_RESET_SELECTOR       (0x48)
# define JO_CD_COMMAND_GET_SECTOR_NUMBER    (0x51)
# define JO_CD_COMMAND_SET_SECTOR_LENGTH    (0x60)
# define JO_CD_COMMAND_GET_THEN_DELETE      (0x63)

/** @brief Command register 1: command and 8 bits parameter */
# define JO_CD_CR1_VALUE(COMMAND, PARAM)    ((unsigned short)(((COMMAND) << 8) | ((PARAM) & 0xff)))
/** @brief High 8 bits of a 24 bits position (stored in the low byte of CR1 or CR3) */
# define JO_CD_POSITION_HIGH(POSITION)      (((POSITION) >> 16) & 0xff)
/** @brief Low 16 bits of a 24 bits position */
# define JO_CD_POSITION_LOW(POSITION)       ((POSITION) & 0xffff)
/** @brief Position: frame address (bit 23 set) */
# define JO_CD_POSITION_FAD(FAD)            (0x800000 | (FAD))
/** @brief Position: track and index (bit 23 clear) */
# define JO_CD_POSITION_TRACK(TRACK, INDEX) (((TRACK) << 8) | (INDEX))
/** @brief Position: pause at the current position (seek command) */
# define JO_CD_POSITION_PAUSE               (0xffffff)
/** @brief First index of a track */
# define JO_CD_FIRST_INDEX                  (1)
/** @brief Last index of a track (play until the end of the track) */
# define JO_CD_LAST_INDEX                   (99)

/** @brief Initialization: ECC check count */
# define JO_CD_INIT_ECC                     (5)
/** @brief Initialization: retry count */
# define JO_CD_INIT_RETRY                   (15)
/** @brief Reset selector flags: reset all partitions, filter conditions and connections */
# define JO_CD_RESET_SELECTOR_ALL           (0xFC)
/** @brief Set filter connection flag: set the "true" connection */
# define JO_CD_FILTER_TRUE_CONNECTION       (0x01)
/** @brief Filter and partition used for data reads */
# define JO_CD_FILTER                       (0)
# define JO_CD_PARTITION                    (0)
/** @brief Table of contents: 99 tracks + first track, last track and lead-out */
# define JO_CD_TOC_ENTRIES                  (102)
/** @brief Timeout while waiting a sector (loop count) */
# define JO_CD_SECTOR_TIMEOUT               (0x200000)

/*
** ISO9660 (ECMA-119)
*/
/** @brief Sector of the primary volume descriptor */
# define JO_ISO9660_PVD_SECTOR              (16)
/** @brief Offset of the root directory record in the primary volume descriptor */
# define JO_ISO9660_PVD_ROOT_RECORD         (156)
/** @brief Directory record: length */
# define JO_ISO9660_RECORD_LENGTH           (0)
/** @brief Directory record: extent location (little endian) */
# define JO_ISO9660_RECORD_EXTENT           (2)
/** @brief Directory record: data length (little endian) */
# define JO_ISO9660_RECORD_SIZE             (10)
/** @brief Directory record: flags */
# define JO_ISO9660_RECORD_FLAGS            (25)
/** @brief Directory record: name length */
# define JO_ISO9660_RECORD_NAME_LENGTH      (32)
/** @brief Directory record: name */
# define JO_ISO9660_RECORD_NAME             (33)
/** @brief Directory record flag: directory */
# define JO_ISO9660_FLAG_DIRECTORY          (0x02)
/** @brief Name of the current directory record */
# define JO_ISO9660_CURRENT_DIRECTORY       (0x00)
/** @brief Name of the parent directory record */
# define JO_ISO9660_PARENT_DIRECTORY        (0x01)

/** @brief Frame address of the first sector of the disc */
# define JO_CD_FIRST_FAD                    (150)

/** @brief Command timeout (loop count) */
# define JO_CD_TIMEOUT                      (0x400000)

/** @brief A directory: this bit of jo_fs_dir_entry.fad (the frame addresses are under 2^24: 20 bytes an entry, not 24,
 *  1 KB of RAM less) */
# define JO_FS_DIRECTORY_FAD_FLAG           (0x80000000u)
/** @brief The frame address of an entry of the directory */
# define JO_FS_ENTRY_FAD(E)                 ((E)->fad & ~JO_FS_DIRECTORY_FAD_FLAG)

/** @brief Maximum length of a filename */
# define JO_FS_FILENAME_LENGTH              (12)

typedef struct
{
    unsigned short              cr[4];
}                               jo_cd_response;

typedef struct
{
    char                        name[JO_FS_FILENAME_LENGTH];
    unsigned int                fad;            // (and JO_FS_DIRECTORY_FAD_FLAG for a directory)
    unsigned int                size;
}                               jo_fs_dir_entry;

typedef struct
{
    bool                        used;
    unsigned int                fad;
    unsigned int                size;
    int                         sector_count;
    int                         position; // In sector
}                               jo_fs_handle;

/*
** GLOBALS
*/
static unsigned int             __jo_cd_play_next_fad = 0; // Next sector expected from the current read
static unsigned int             __jo_cd_play_end_fad = 0; // End of the current read (exclusive)
static bool                     __jo_cd_initialized = false;
static unsigned char            __jo_cd_sector_buffer[JO_SECTOR_SIZE] __attribute__((aligned(4)));
/** @brief The end of the files of the disc (the read ahead stops there, see __jo_fs_read_sectors()) */
static unsigned int             __jo_fs_data_end_fad = 0;
/** @brief A read a little after the position of the drive: the sectors between are read and dropped (no seek) */
# define JO_CD_SKIP_MAX                 (48)
static jo_fs_dir_entry          __jo_fs_dir[JO_FS_MAX_FILES];
static int                      __jo_fs_dir_count = 0;
/** @brief The directory loaded in __jo_fs_dir (loaded again after a failed jo_fs_cd(): the table was half replaced) */
static unsigned int             __jo_fs_dir_fad = 0;
static unsigned int             __jo_fs_dir_size = 0;
static jo_fs_handle             __jo_fs_handles[JO_OPEN_MAX];
/** @brief The current read of the drive belongs to the stream (__jo_fs_stream_read()) */
static bool                     __jo_cd_stream_owner = false;
/** @brief Called by the reads of files while they wait for the drive (the music keeps its buffer filled) */
void                            (*__jo_fs_stream_service)(void) = JO_NULL;
static bool                     __jo_fs_stream_service_running = false;
/** @brief The reads of the music at each frame (core.c), done while jo_fs_wait_background_jobs() waits */
extern void                     (*__jo_core_music_update)(void);

/** @brief Send a command to the CD block
 *  @param cr1 Command register 1 (JO_CD_CR1_VALUE())
 *  @param cr2 Command register 2
 *  @param cr3 Command register 3
 *  @param cr4 Command register 4
 *  @param wait_flags HIRQ flags to wait for after the command (0: no wait)
 *  @param response Response registers (output, JO_NULL if not needed)
 *  @return 0 if succeed, -1 if timeout or rejected
 */
static int                      jo_cd_command(const unsigned short cr1, const unsigned short cr2, const unsigned short cr3, const unsigned short cr4,
                                              const unsigned short wait_flags, jo_cd_response *response)
{
    register unsigned int       timeout;

    for (JO_ZERO(timeout); !(JO_CD_HIRQ & JO_CD_HIRQ_CMOK); ++timeout)
        if (timeout > JO_CD_TIMEOUT)
            return (-1);
    JO_CD_HIRQ = ~(JO_CD_HIRQ_CMOK | wait_flags);
    JO_CD_CR1 = cr1;
    JO_CD_CR2 = cr2;
    JO_CD_CR3 = cr3;
    JO_CD_CR4 = cr4;
    for (JO_ZERO(timeout); !(JO_CD_HIRQ & JO_CD_HIRQ_CMOK); ++timeout)
        if (timeout > JO_CD_TIMEOUT)
            return (-1);
    if (wait_flags)
    {
        for (JO_ZERO(timeout); (JO_CD_HIRQ & wait_flags) != wait_flags; ++timeout)
            if (timeout > JO_CD_TIMEOUT)
                return (-1);
    }
    if (response != JO_NULL)
    {
        response->cr[0] = JO_CD_CR1;
        response->cr[1] = JO_CD_CR2;
        response->cr[2] = JO_CD_CR3;
        response->cr[3] = JO_CD_CR4;
    }
    if ((JO_CD_CR1 >> 8) == JO_CD_STATUS_REJECT)
        return (-1);
    return (0);
}

/** @brief End the current data transfer
 *  @return 0 if succeed, -1 if failed
 */
static  __jo_force_inline int   jo_cd_end_transfer(void)
{
    return (jo_cd_command(JO_CD_CR1_VALUE(JO_CD_COMMAND_END_TRANSFER, 0), 0, 0, 0, 0, JO_NULL));
}

/** @brief Delete the sectors of the buffer partition
 *  @return 0 if succeed, -1 if failed
 */
static  __jo_force_inline int   jo_cd_reset_partition(void)
{
    return (jo_cd_command(JO_CD_CR1_VALUE(JO_CD_COMMAND_RESET_SELECTOR, 0), 0, JO_CD_PARTITION << 8, 0, JO_CD_HIRQ_ESEL, JO_NULL));
}

/** @brief Get the number of sectors in the buffer partition
 *  @return Sector count or -1 if failed
 */
static int                      jo_cd_sector_count(void)
{
    jo_cd_response              r;

    if (jo_cd_command(JO_CD_CR1_VALUE(JO_CD_COMMAND_GET_SECTOR_NUMBER, 0), 0, JO_CD_PARTITION << 8, 0, 0, &r) < 0)
        return (-1);
    return (r.cr[3]);
}

/** @brief Initialize the CD block once
 *  @return 0 if succeed, -1 if failed
 *  @remarks Reads the table of contents (needed to play audio tracks)
 */
static int                      jo_cd_init(void)
{
    register unsigned int       i;

    if (__jo_cd_initialized)
        return (0);
    // Abort any pending transfer, initialize (standby time, ECC and retry count), 2048 bytes sectors
    jo_cd_end_transfer();
    if (jo_cd_command(JO_CD_CR1_VALUE(JO_CD_COMMAND_INITIALIZE, 0), 0, 0, (JO_CD_INIT_ECC << 8) | JO_CD_INIT_RETRY, 0, JO_NULL) < 0)
        return (-1);
    // Sector length: 2048 bytes for reads and writes
    if (jo_cd_command(JO_CD_CR1_VALUE(JO_CD_COMMAND_SET_SECTOR_LENGTH, 0), 0, 0, 0, JO_CD_HIRQ_ESEL, JO_NULL) < 0)
        return (-1);
    // Reset partitions data, filter conditions and connections
    jo_cd_command(JO_CD_CR1_VALUE(JO_CD_COMMAND_RESET_SELECTOR, JO_CD_RESET_SELECTOR_ALL), 0, 0, 0, JO_CD_HIRQ_ESEL, JO_NULL);
    // Read the table of contents (the drive needs it to play tracks)
    if (jo_cd_command(JO_CD_CR1_VALUE(JO_CD_COMMAND_GET_TOC, 0), 0, 0, 0, JO_CD_HIRQ_DRDY, JO_NULL) == 0)
    {
        // (read and dropped: the transfer of the table must be done, nothing uses it; 408 bytes of RAM kept before)
        for (JO_ZERO(i); i < JO_CD_TOC_ENTRIES; ++i)
        {
            (void)JO_CD_INFO;
            (void)JO_CD_INFO;
        }
        jo_cd_end_transfer();
    }
    // CD device output goes to the filter, the filter true output goes to the partition
    jo_cd_command(JO_CD_CR1_VALUE(JO_CD_COMMAND_SET_FILTER_CONNECTION, JO_CD_FILTER_TRUE_CONNECTION), JO_CD_PARTITION << 8, JO_CD_FILTER << 8, 0, JO_CD_HIRQ_ESEL, JO_NULL);
    jo_cd_command(JO_CD_CR1_VALUE(JO_CD_COMMAND_SET_DEVICE_CONNECTION, 0), 0, JO_CD_FILTER << 8, 0, JO_CD_HIRQ_ESEL, JO_NULL);
    __jo_cd_initialized = true;
    return (0);
}

/** @brief Stop the current data read and drop the buffered sectors */
static void                     jo_cd_stop_read(void)
{
    jo_cd_response              r;

    if (!__jo_cd_play_end_fad)
        return;
    // Pause the drive at the current position and drop the buffered sectors
    jo_cd_command(JO_CD_CR1_VALUE(JO_CD_COMMAND_SEEK, JO_CD_POSITION_HIGH(JO_CD_POSITION_PAUSE)), JO_CD_POSITION_LOW(JO_CD_POSITION_PAUSE), 0, 0, 0, &r);
    jo_cd_reset_partition();
    JO_ZERO(__jo_cd_play_next_fad);
    JO_ZERO(__jo_cd_play_end_fad);
    __jo_cd_stream_owner = false;
}

/** @brief Start reading sectors in the buffer partition
 *  @param fad First sector (frame address)
 *  @param count Sector count
 *  @return 0 if succeed, -1 if failed
 */
static int                      jo_cd_start_read(const unsigned int fad, const unsigned int count)
{
    jo_cd_stop_read();
    if (jo_cd_reset_partition() < 0)
        return (-1);
    // Filter range, all the sectors go to the partition
    jo_cd_command(JO_CD_CR1_VALUE(JO_CD_COMMAND_SET_FILTER_RANGE, JO_CD_POSITION_HIGH(fad)), JO_CD_POSITION_LOW(fad),
                  (JO_CD_FILTER << 8) | JO_CD_POSITION_HIGH(count), JO_CD_POSITION_LOW(count), JO_CD_HIRQ_ESEL, JO_NULL);
    jo_cd_command(JO_CD_CR1_VALUE(JO_CD_COMMAND_SET_FILTER_CONNECTION, JO_CD_FILTER_TRUE_CONNECTION), JO_CD_PARTITION << 8, JO_CD_FILTER << 8, 0, JO_CD_HIRQ_ESEL, JO_NULL);
    jo_cd_command(JO_CD_CR1_VALUE(JO_CD_COMMAND_SET_DEVICE_CONNECTION, 0), 0, JO_CD_FILTER << 8, 0, JO_CD_HIRQ_ESEL, JO_NULL);
    // Play from fad, count sectors (the end position is a sector count when the FAD flag is set)
    if (jo_cd_command(JO_CD_CR1_VALUE(JO_CD_COMMAND_PLAY, JO_CD_POSITION_HIGH(JO_CD_POSITION_FAD(fad))), JO_CD_POSITION_LOW(fad),
                      JO_CD_POSITION_HIGH(JO_CD_POSITION_FAD(count)), JO_CD_POSITION_LOW(count), 0, JO_NULL) < 0)
        return (-1);
    __jo_cd_play_next_fad = fad;
    __jo_cd_play_end_fad = fad + count;
    __jo_cd_stream_owner = false;
    return (0);
}

/** @brief Statistics of the reads of the CD (see jo_fs_get_read_stats()) */
static unsigned int     __jo_fs_stat_vblanks = 0;
static unsigned int     __jo_fs_stat_bytes = 0;
static unsigned int     __jo_fs_stat_files = 0;

/** @brief Transfer n sectors of the buffer partition (they are there: jo_cd_sector_count())
 *  @param n Sector count
 *  @param out Output buffer (moved after the bytes stored)
 *  @param out_bytes Bytes to store (the rest of the sectors is skipped; decreased by the bytes stored)
 *  @return 0 if succeed, -1 if failed
 */
static int                      jo_cd_get_sectors(const unsigned int n, unsigned char **out, unsigned int *out_bytes)
{
    unsigned char               *buf;
    unsigned int                bytes;
    register unsigned int       i;
    register unsigned int       w;
    register unsigned int       b;
    unsigned int                value;

    // Get then delete n sectors from partition 0
    if (jo_cd_command(JO_CD_CR1_VALUE(JO_CD_COMMAND_GET_THEN_DELETE, 0), 0, JO_CD_PARTITION << 8, n, JO_CD_HIRQ_DRDY, JO_NULL) < 0)
    {
        jo_cd_stop_read();
        return (-1);
    }
    buf = *out;
    bytes = *out_bytes;
    JO_ZERO(i);
    // Fast path: the n sectors fit in an aligned buffer, the DMA controller reads them
    if (bytes >= n * JO_SECTOR_SIZE && ((unsigned int)buf & 3) == 0)
    {
        __jo_dma_read_port32(&JO_CD_DATA, buf, n * JO_SECTOR_SIZE);
        buf += n * JO_SECTOR_SIZE;
        bytes -= n * JO_SECTOR_SIZE;
        i = n;
    }
    for (; i < n; ++i)
    {
        // Fast path: a whole sector in an aligned buffer (4 words per iteration)
        if (bytes >= JO_SECTOR_SIZE && ((unsigned int)buf & 3) == 0)
        {
            unsigned int    *d32 = (unsigned int *)buf;

            for (JO_ZERO(w); w < JO_SECTOR_SIZE / 4; w += 4)
            {
                d32[0] = JO_CD_DATA;
                d32[1] = JO_CD_DATA;
                d32[2] = JO_CD_DATA;
                d32[3] = JO_CD_DATA;
                d32 += 4;
            }
            buf += JO_SECTOR_SIZE;
            bytes -= JO_SECTOR_SIZE;
            continue;
        }
        for (JO_ZERO(w); w < JO_SECTOR_SIZE / 4; ++w)
        {
            value = JO_CD_DATA;
            if (bytes >= 4)
            {
                if (((unsigned int)buf & 3) == 0)
                    *(unsigned int *)buf = value;
                else
                {
                    buf[0] = value >> 24;
                    buf[1] = value >> 16;
                    buf[2] = value >> 8;
                    buf[3] = value;
                }
                buf += 4;
                bytes -= 4;
            }
            else
            {
                for (JO_ZERO(b); b < bytes; ++b)
                    buf[b] = value >> (24 - JO_MULT_BY_8(b));
                buf += bytes;
                JO_ZERO(bytes);
            }
        }
    }
    jo_cd_end_transfer();
    *out = buf;
    *out_bytes = bytes;
    __jo_cd_play_next_fad += n;
    return (0);
}

/** @brief Read sectors
 *  @param fad First sector (frame address)
 *  @param count Sector count
 *  @param buf Output buffer
 *  @param bytes Bytes to store in buf (the rest of the sectors is skipped)
 *  @param read_ahead_end Last sector (exclusive) that can be read in advance
 *  @return 0 if succeed, -1 if failed
 *  @remarks Sequential reads continue the current read without seeking
 */
static int                      jo_cd_read_sectors(const unsigned int fad, const unsigned int count, unsigned char *buf, unsigned int bytes,
                                                   unsigned int read_ahead_end)
{
    unsigned int                done;
    unsigned int                timeout;
    int                         available;
    unsigned int                n;
    unsigned int                start;

    if (jo_cd_init() < 0)
        return (-1);
    if (count == 0)
        return (0);
    start = jo_core_get_vblank_count();
    __jo_fs_stat_bytes += count * JO_SECTOR_SIZE;
    if (read_ahead_end < fad + count)
        read_ahead_end = fad + count;
    // A little after the sectors read in advance: the sectors between are dropped (a seek costs more, about 10 frames)
    if (__jo_cd_play_end_fad && !__jo_cd_stream_owner && fad > __jo_cd_play_next_fad && fad - __jo_cd_play_next_fad <= JO_CD_SKIP_MAX &&
        fad + count <= __jo_cd_play_end_fad)
    {
        while (__jo_cd_play_next_fad < fad)
            if (jo_cd_read_sectors(__jo_cd_play_next_fad, 1, __jo_cd_sector_buffer, JO_SECTOR_SIZE, read_ahead_end) < 0)
                return (-1);
    }
    if (__jo_cd_stream_owner || fad != __jo_cd_play_next_fad || fad + count > __jo_cd_play_end_fad)
    {
        if (jo_cd_start_read(fad, read_ahead_end - fad) < 0)
            return (-1);
    }
    JO_ZERO(done);
    JO_ZERO(timeout);
    while (done < count)
    {
        if (__jo_fs_stream_service != JO_NULL && !__jo_fs_stream_service_running)
        {
            __jo_fs_stream_service_running = true;
            __jo_fs_stream_service();
            __jo_fs_stream_service_running = false;
            if ((__jo_cd_stream_owner || __jo_cd_play_next_fad != fad + done) &&
                jo_cd_start_read(fad + done, read_ahead_end - (fad + done)) < 0)
                return (-1);
        }
        if ((available = jo_cd_sector_count()) < 0)
            return (-1);
        if (available == 0)
        {
            if (++timeout > JO_CD_SECTOR_TIMEOUT)
            {
                jo_cd_stop_read();
                return (-1);
            }
            continue;
        }
        JO_ZERO(timeout);
        n = (unsigned int)available;
        if (n > count - done)
            n = count - done;
        if (jo_cd_get_sectors(n, &buf, &bytes) < 0)
            return (-1);
        done += n;
    }
    if (__jo_cd_play_next_fad >= __jo_cd_play_end_fad)
    {
        JO_ZERO(__jo_cd_play_next_fad);
        JO_ZERO(__jo_cd_play_end_fad);
    }
    __jo_fs_stat_vblanks += jo_core_get_vblank_count() - start;
    return (0);
}

/** @brief The calls of jo_cd_take_sectors() without a sector: the read is started again after (a lost command) */
# define JO_CD_TAKE_RETRY               (300)
static unsigned int             __jo_cd_take_idle = 0;

/** @brief Take the sectors already read by the drive, without waiting for them (see jo_fs_do_background_jobs())
 *  @param fad First sector (frame address)
 *  @param count Sector count wanted (at most)
 *  @param buf Output buffer
 *  @param bytes Bytes to store in buf (the rest of the sectors is skipped)
 *  @param read_ahead_end Last sector (exclusive) that can be read in advance
 *  @return The sectors stored (0: not read yet, or the music reads its own sectors), -1 if failed
 *  @remarks The read of the music (__jo_fs_stream_read()) is not stopped: its sectors come first, the drive is taken
 *  back when they are read (its buffer is filled, the file has the drive until the music needs it again)
 */
static int                      jo_cd_take_sectors(const unsigned int fad, unsigned int count, unsigned char *buf, unsigned int bytes,
                                                   unsigned int read_ahead_end)
{
    int                         available;
    unsigned int                n;

    if (jo_cd_init() < 0)
        return (-1);
    if (count == 0)
        return (0);
    if (__jo_cd_stream_owner && __jo_fs_stream_service != JO_NULL)
        return (0);
    if (read_ahead_end < fad + count)
        read_ahead_end = fad + count;
    // (a little after the sectors read in advance (the next file on the disc): the sectors between are dropped, a seek
    // costs more)
    while (__jo_cd_play_end_fad && !__jo_cd_stream_owner && fad > __jo_cd_play_next_fad &&
           fad - __jo_cd_play_next_fad <= JO_CD_SKIP_MAX && fad + count <= __jo_cd_play_end_fad)
    {
        unsigned char           *dropped;
        unsigned int            none;

        if ((available = jo_cd_sector_count()) < 0)
            return (-1);
        if (available == 0)
            return (0);
        n = JO_MIN((unsigned int)available, fad - __jo_cd_play_next_fad);
        dropped = __jo_cd_sector_buffer;
        JO_ZERO(none);
        if (jo_cd_get_sectors(n, &dropped, &none) < 0)
            return (-1);
    }
    if (__jo_cd_stream_owner || fad != __jo_cd_play_next_fad || fad + count > __jo_cd_play_end_fad)
    {
        if (jo_cd_start_read(fad, read_ahead_end - fad) < 0)
            return (-1);
        JO_ZERO(__jo_cd_take_idle);
    }
    if ((available = jo_cd_sector_count()) < 0)
        return (-1);
    if (available == 0)
    {
        if (++__jo_cd_take_idle > JO_CD_TAKE_RETRY)
            jo_cd_stop_read();
        return (0);
    }
    JO_ZERO(__jo_cd_take_idle);
    n = (unsigned int)available;
    if (n > count)
        n = count;
    if (jo_cd_get_sectors(n, &buf, &bytes) < 0)
        return (-1);
    __jo_fs_stat_bytes += n * JO_SECTOR_SIZE;
    if (__jo_cd_play_next_fad >= __jo_cd_play_end_fad)
    {
        JO_ZERO(__jo_cd_play_next_fad);
        JO_ZERO(__jo_cd_play_end_fad);
    }
    return ((int)n);
}

/*
** CD audio
** Track position: track << 8 | index
*/
void                            __jo_cd_play_tracks(const int from_track, const int to_track, const unsigned char play_mode)
{
    jo_cd_response              r;

    // Any data read in progress is cancelled (and its buffered sectors dropped)
    jo_cd_init();
    jo_cd_stop_read();
    jo_cd_reset_partition();
    // From index 1 of the first track up to the end of the last track (index 99)
    jo_cd_command(JO_CD_CR1_VALUE(JO_CD_COMMAND_PLAY, 0), JO_CD_POSITION_TRACK(from_track, JO_CD_FIRST_INDEX),
                  (play_mode << 8), JO_CD_POSITION_TRACK(to_track, JO_CD_LAST_INDEX), 0, &r);
}

/** @brief The last sector of a track: the sector before the next track (the lead-out after the last one), from the
 *  table of contents (read again: not kept in RAM)
 *  @return The frame address, 0 if the table of contents is not read */
static unsigned int             jo_cd_track_end_fad(const int track)
{
    register unsigned int       i;
    unsigned int                entry;
    unsigned int                next;
    unsigned int                lead_out;

    JO_ZERO(next);
    JO_ZERO(lead_out);
    if (jo_cd_command(JO_CD_CR1_VALUE(JO_CD_COMMAND_GET_TOC, 0), 0, 0, 0, JO_CD_HIRQ_DRDY, JO_NULL) != 0)
        return (0);
    // (the entries: the tracks 1 to 99 (0xFFFFFFFF: no track), the first track, the last track, the lead-out; control
    // and address in the high byte, the frame address in the low 24 bits)
    for (JO_ZERO(i); i < JO_CD_TOC_ENTRIES; ++i)
    {
        entry = (unsigned int)JO_CD_INFO << 16;
        entry |= (unsigned int)JO_CD_INFO;
        // (the entry 99 is the first track, not a track: the end of the track 99 is the lead-out)
        if (track < 99 && i == (unsigned int)track && entry != 0xFFFFFFFF)
            next = entry & 0xFFFFFF;
        if (i == JO_CD_TOC_ENTRIES - 1)
            lead_out = entry & 0xFFFFFF;
    }
    jo_cd_end_transfer();
    return ((next != 0 ? next : lead_out) - 1);
}

void                            __jo_cd_play_from_fad(const unsigned int fad, const int from_track, const int to_track,
                                                      const unsigned char play_mode)
{
    jo_cd_response              r;
    unsigned int                end;

    jo_cd_init();
    jo_cd_stop_read();
    jo_cd_reset_partition();
    // (the start and the end of a play are both frame addresses or both tracks (a frame address up to a track played
    // nothing in Mednafen); with frame addresses, the end is the count of sectors to play)
    // (paused at the very end, or the table of contents not read: the tracks again from their start)
    end = jo_cd_track_end_fad(to_track);
    if (end == 0 || end == 0xFFFFFFFF || fad >= end)
    {
        __jo_cd_play_tracks(from_track, to_track, play_mode);
        return;
    }
    jo_cd_command(JO_CD_CR1_VALUE(JO_CD_COMMAND_PLAY, JO_CD_POSITION_HIGH(JO_CD_POSITION_FAD(fad))), JO_CD_POSITION_LOW(fad),
                  (play_mode << 8) | JO_CD_POSITION_HIGH(JO_CD_POSITION_FAD(end - fad + 1)), JO_CD_POSITION_LOW(end - fad + 1), 0,
                  &r);
}

int                             __jo_cd_get_status(unsigned int *fad)
{
    jo_cd_response              r;

    if (jo_cd_init() < 0)
        return (-1);
    if (jo_cd_command(JO_CD_CR1_VALUE(JO_CD_COMMAND_GET_STATUS, 0), 0, 0, 0, 0, &r) < 0)
        return (-1);
    if (fad != JO_NULL)
        *fad = JO_CD_STATUS_FAD(r.cr[2], r.cr[3]);
    return (JO_CD_STATUS(r.cr[0]));
}

void                            __jo_cd_pause(void)
{
    jo_cd_response              r;

    jo_cd_init();
    jo_cd_stop_read();
    jo_cd_command(JO_CD_CR1_VALUE(JO_CD_COMMAND_SEEK, JO_CD_POSITION_HIGH(JO_CD_POSITION_PAUSE)), JO_CD_POSITION_LOW(JO_CD_POSITION_PAUSE), 0, 0, 0, &r);
}

/*
██╗███████╗ ██████╗  █████╗  ██████╗  ██████╗  ██████╗
██║██╔════╝██╔═══██╗██╔══██╗██╔════╝ ██╔════╝ ██╔═████╗
██║███████╗██║   ██║╚██████║███████╗ ███████╗ ██║██╔██║
██║╚════██║██║   ██║ ╚═══██║██╔═══██╗██╔═══██╗████╔╝██║
██║███████║╚██████╔╝ █████╔╝╚██████╔╝╚██████╔╝╚██████╔╝
╚═╝╚══════╝ ╚═════╝  ╚════╝  ╚═════╝  ╚═════╝  ╚═════╝
*/

/** @brief Read a little endian 32 bits value
 *  @param p Data
 *  @return Value
 */
static  __jo_force_inline unsigned int  jo_fs_read_le32(const unsigned char *p)
{
    return (p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}

/** @brief Load a directory in the current directory table
 *  @param fad First sector of the directory
 *  @param size Directory size in bytes
 *  @return Number of entries or -1 if failed
 *  @remarks Names are copied without the version (";1")
 */
static int                      jo_fs_load_directory(const unsigned int fad, const unsigned int size)
{
    unsigned int                sectors;
    unsigned int                s;
    unsigned int                pos;
    int                         count;
    const unsigned char         *record;
    unsigned int                record_length;
    unsigned int                name_length;
    jo_fs_dir_entry             *entry;
    register unsigned int       i;

    JO_ZERO(count);
    sectors = (size + JO_SECTOR_SIZE - 1) / JO_SECTOR_SIZE;
    for (JO_ZERO(s); s < sectors; ++s)
    {
        if (jo_cd_read_sectors(fad + s, 1, __jo_cd_sector_buffer, JO_SECTOR_SIZE, fad + sectors) < 0)
            return (-1);
        for (JO_ZERO(pos); pos < JO_SECTOR_SIZE;)
        {
            record = &__jo_cd_sector_buffer[pos];
            record_length = record[JO_ISO9660_RECORD_LENGTH];
            // (a record never crosses a sector: a damaged one was read after the buffer)
            if (record_length == 0 || pos + record_length > JO_SECTOR_SIZE ||
                (unsigned int)JO_ISO9660_RECORD_NAME + record[JO_ISO9660_RECORD_NAME_LENGTH] > record_length)
                break;
            if (count >= JO_FS_MAX_FILES)
                return (count);
            entry = &__jo_fs_dir[count];
            entry->fad = jo_fs_read_le32(&record[JO_ISO9660_RECORD_EXTENT]) + JO_CD_FIRST_FAD;
            entry->size = jo_fs_read_le32(&record[JO_ISO9660_RECORD_SIZE]);
            if (entry->fad + (entry->size + JO_SECTOR_SIZE - 1) / JO_SECTOR_SIZE > __jo_fs_data_end_fad)
                __jo_fs_data_end_fad = entry->fad + (entry->size + JO_SECTOR_SIZE - 1) / JO_SECTOR_SIZE;
            if (record[JO_ISO9660_RECORD_FLAGS] & JO_ISO9660_FLAG_DIRECTORY)
                entry->fad |= JO_FS_DIRECTORY_FAD_FLAG;
            name_length = record[JO_ISO9660_RECORD_NAME_LENGTH];
            if (name_length == 1 && record[JO_ISO9660_RECORD_NAME] == JO_ISO9660_CURRENT_DIRECTORY)
            {
                entry->name[0] = '.';
                entry->name[1] = '\0';
            }
            else if (name_length == 1 && record[JO_ISO9660_RECORD_NAME] == JO_ISO9660_PARENT_DIRECTORY)
            {
                entry->name[0] = '.';
                entry->name[1] = '.';
                entry->name[2] = '\0';
            }
            else
            {
                for (JO_ZERO(i); i < name_length && i < JO_FS_FILENAME_LENGTH && record[JO_ISO9660_RECORD_NAME + i] != ';'; ++i)
                    entry->name[i] = record[JO_ISO9660_RECORD_NAME + i];
                // Files without extension are stored as "NAME."
                if (i > 0 && entry->name[i - 1] == '.')
                    --i;
                // A 12 characters name is not terminated
                if (i < JO_FS_FILENAME_LENGTH)
                    entry->name[i] = '\0';
            }
            ++count;
            pos += record_length;
        }
    }
    return (count);
}

/** @brief Load the root directory of the disc
 *  @return Number of entries or -1 if failed
 */
static int                      jo_fs_load_root_directory(void)
{
    // Primary volume descriptor
    if (jo_cd_read_sectors(JO_CD_FIRST_FAD + JO_ISO9660_PVD_SECTOR, 1, __jo_cd_sector_buffer, JO_SECTOR_SIZE, 0) < 0)
        return (-1);
    // Standard identifier: "CD001"
    if (__jo_cd_sector_buffer[1] != 'C' || __jo_cd_sector_buffer[2] != 'D' || __jo_cd_sector_buffer[3] != '0')
        return (-1);
    __jo_fs_dir_fad = jo_fs_read_le32(&__jo_cd_sector_buffer[JO_ISO9660_PVD_ROOT_RECORD + JO_ISO9660_RECORD_EXTENT]) + JO_CD_FIRST_FAD;
    __jo_fs_dir_size = jo_fs_read_le32(&__jo_cd_sector_buffer[JO_ISO9660_PVD_ROOT_RECORD + JO_ISO9660_RECORD_SIZE]);
    return (jo_fs_load_directory(__jo_fs_dir_fad, __jo_fs_dir_size));
}

/*
** Internal file API
*/

int                             __jo_fs_get_file_id(const char * const filename)
{
    register int                i;
    register int                j;
    const char                  *name;

    if (filename == JO_NULL)
        return (-1);
    for (JO_ZERO(i); i < __jo_fs_dir_count; ++i)
    {
        name = __jo_fs_dir[i].name;
        for (JO_ZERO(j); j < JO_FS_FILENAME_LENGTH && name[j] == filename[j] && name[j]; ++j)
            ;
        if (j == JO_FS_FILENAME_LENGTH ? filename[j] == '\0' : name[j] == filename[j])
            return (i);
    }
    return (-1);
}

bool                            jo_fs_file_exists(const char *const filename)
{
    return (__jo_fs_get_file_id(filename) >= 0);
}

#ifdef JO_FS_TRACE
/* Test: the files opened, in their order (JO_FS_TRACE: the count kept), see jo_fs_trace_name() */
static short                    __jo_fs_trace[JO_FS_TRACE] __attribute__((used));
static int                      __jo_fs_trace_count = 0;

const char                      *jo_fs_trace_name(const int index)
{
    return (index >= 0 && index < __jo_fs_trace_count ? __jo_fs_dir[__jo_fs_trace[index]].name : JO_NULL);
}
#endif

void                            *__jo_fs_open(const int file_id)
{
    register int                i;
    jo_fs_handle                *handle;

    if (file_id < 0 || file_id >= __jo_fs_dir_count)
        return (JO_NULL);
    ++__jo_fs_stat_files;
#ifdef JO_FS_TRACE
    if (__jo_fs_trace_count < JO_FS_TRACE)
        __jo_fs_trace[__jo_fs_trace_count++] = (short)file_id;
#endif
    for (JO_ZERO(i); i < JO_OPEN_MAX; ++i)
    {
        handle = &__jo_fs_handles[i];
        if (handle->used)
            continue;
        handle->used = true;
        handle->fad = JO_FS_ENTRY_FAD(&__jo_fs_dir[file_id]);
        handle->size = __jo_fs_dir[file_id].size;
        handle->sector_count = (handle->size + JO_SECTOR_SIZE - 1) / JO_SECTOR_SIZE;
        JO_ZERO(handle->position);
        return (handle);
    }
    return (JO_NULL);
}

void                            __jo_fs_close(void *handle)
{
    if (handle != JO_NULL)
        ((jo_fs_handle *)handle)->used = false;
}

int                             __jo_fs_get_file_size(void *handle)
{
    return (((jo_fs_handle *)handle)->size);
}

static int                      __jo_fs_get_sector_count(void *handle)
{
    return (((jo_fs_handle *)handle)->sector_count);
}

unsigned int                    __jo_fs_get_fad(void *handle)
{
    return (((jo_fs_handle *)handle)->fad);
}

int                             __jo_fs_stream_read(const unsigned int fad, const unsigned int count)
{
    if (jo_cd_init() < 0 || count == 0 || jo_cd_start_read(fad, count) < 0)
        return (-1);
    __jo_cd_stream_owner = true;
    return (0);
}

int                             __jo_fs_stream_take(void *buf, const int max_sectors)
{
    int                         available;
    int                         n;

    if (!__jo_cd_stream_owner)
        return (-1);
    if ((available = jo_cd_sector_count()) < 0)
        return (-1);
    n = available < max_sectors ? available : max_sectors;
    if (n <= 0)
        return (0);
    if (jo_cd_command(JO_CD_CR1_VALUE(JO_CD_COMMAND_GET_THEN_DELETE, 0), 0, JO_CD_PARTITION << 8, (unsigned short)n, JO_CD_HIRQ_DRDY, JO_NULL) < 0)
    {
        jo_cd_stop_read();
        return (-1);
    }
    __jo_dma_read_port32(&JO_CD_DATA, buf, (unsigned int)n * JO_SECTOR_SIZE);
    jo_cd_end_transfer();
    __jo_cd_play_next_fad += (unsigned int)n;
    if (__jo_cd_play_next_fad >= __jo_cd_play_end_fad)
    {
        JO_ZERO(__jo_cd_play_next_fad);
        JO_ZERO(__jo_cd_play_end_fad);
        __jo_cd_stream_owner = false;
    }
    return (n);
}

bool                            __jo_fs_stream_is_reading(void)
{
    return (__jo_cd_stream_owner);
}

void                            __jo_fs_stream_stop(void)
{
    if (__jo_cd_stream_owner)
        jo_cd_stop_read();
}

bool                            __jo_fs_seek_sector(void *handle, const int sector)
{
    jo_fs_handle                *h;

    h = (jo_fs_handle *)handle;
    if (sector < 0 || sector > h->sector_count)
        return (false);
    h->position = sector;
    return (true);
}

int                             __jo_fs_read_sectors(void *handle, int sector_count, void *buf, const int buf_size)
{
    jo_fs_handle                *h;
    int                         remaining_bytes;
    int                         bytes;

    h = (jo_fs_handle *)handle;
    if (h->position >= h->sector_count || sector_count <= 0)
        return (0);
    if (sector_count > h->sector_count - h->position)
        sector_count = h->sector_count - h->position;
    remaining_bytes = h->size - h->position * JO_SECTOR_SIZE;
    bytes = sector_count * JO_SECTOR_SIZE;
    if (bytes > remaining_bytes)
        bytes = remaining_bytes;
    if (bytes > buf_size)
        bytes = buf_size;
    // (the read goes on after the file: the next file on the disc is often the next one read, see the order of the files of
    // the disc in the makefile (cd.sort). The buffer of the CD block holds 200 sectors: the drive waits when it is full)
    if (jo_cd_read_sectors(h->fad + h->position, sector_count, (unsigned char *)buf, bytes,
                           JO_MAX(h->fad + h->sector_count, __jo_fs_data_end_fad)) < 0)
        return (-1);
    h->position += sector_count;
    return (bytes);
}

/** @brief Read the sectors of a file already read by the drive, without waiting (see jo_cd_take_sectors())
 *  @return Bytes stored in buf (0: none yet), -1 if failed
 */
static int                      __jo_fs_take_sectors(jo_fs_handle * const h, int sector_count, void *buf, const int buf_size)
{
    int                         remaining_bytes;
    int                         bytes;
    int                         n;

    if (h->position >= h->sector_count || sector_count <= 0)
        return (0);
    if (sector_count > h->sector_count - h->position)
        sector_count = h->sector_count - h->position;
    remaining_bytes = h->size - h->position * JO_SECTOR_SIZE;
    bytes = sector_count * JO_SECTOR_SIZE;
    if (bytes > remaining_bytes)
        bytes = remaining_bytes;
    if (bytes > buf_size)
        bytes = buf_size;
    if ((n = jo_cd_take_sectors(h->fad + h->position, sector_count, (unsigned char *)buf, bytes,
                                JO_MAX(h->fad + h->sector_count, __jo_fs_data_end_fad))) <= 0)
        return (n);
    h->position += n;
    // (the bytes stored: whole sectors, the end of the file in the last one)
    return (JO_MIN(n * JO_SECTOR_SIZE, bytes));
}

/*
** Jo Engine file API
*/

typedef struct
{
    bool                        active;
    jo_fs_async_read_callback   callback;
    void                        *handle;
    int                         file_length;
    char                        *contents;
    char                        *ptr;
    int                         token;
}                               __jo_fs_background_job;

static __jo_fs_background_job   __jo_fs_background_jobs[JO_MAX_FS_BACKGROUND_JOBS];
unsigned int                    __jo_fs_background_job_count;
static int                      __jo_fs_read_buffer_size = JO_SECTOR_SIZE;

int								jo_fs_init()
{
    register int                i;

#ifdef JO_DEBUG
    if (!JO_IS_ARRAY_INSIDE_USER_RAM_AREA(__jo_fs_dir) ||
        !JO_IS_ARRAY_INSIDE_USER_RAM_AREA(__jo_fs_background_jobs))
    {
        jo_core_error("Please reduce JO_GLOBAL_MEMORY_SIZE_FOR_MALLOC");
        return (0);
    }
#endif
    JO_ZERO(__jo_fs_background_job_count);
    for (JO_ZERO(i); i < JO_MAX_FS_BACKGROUND_JOBS; ++i)
        __jo_fs_background_jobs[i].active = false;
    for (JO_ZERO(i); i < JO_OPEN_MAX; ++i)
        __jo_fs_handles[i].used = false;
    __jo_cd_initialized = false;
    if (jo_cd_init() < 0 || (__jo_fs_dir_count = jo_fs_load_root_directory()) <= 2)
    {
        if (__jo_fs_dir_count < 0)
            JO_ZERO(__jo_fs_dir_count);
#ifdef JO_DEBUG
        jo_core_error("Fail to read the CD");
#endif
        return (0);
    }
    return (1);
}

void                    jo_fs_do_background_jobs(void)
{
    register int        i;
    int                 nbyte;
    jo_fs_handle        *handle;
    __jo_fs_background_job *job;

    // (the first job only: the files one after the other, the drive does not go back and forth between them)
    for (JO_ZERO(i); i < JO_MAX_FS_BACKGROUND_JOBS; ++i)
    {
        job = &__jo_fs_background_jobs[i];
        if (!job->active)
            continue;
        handle = (jo_fs_handle *)job->handle;
        // (the sectors already read by the drive: no wait, the game goes on; a failed read is tried again next time)
        nbyte = __jo_fs_take_sectors(handle, JO_MAXIMUM_SECTOR_FETCHED_ONCE_ASYNC, job->ptr,
                                     job->file_length - (int)(job->ptr - job->contents));
        if (nbyte > 0)
            job->ptr += nbyte;
        if (handle->position >= handle->sector_count)
        {
            __jo_fs_close(handle);
            --__jo_fs_background_job_count;
            JO_ZERO(job->contents[job->file_length]);
            job->active = false;
            job->callback(job->contents, job->file_length, job->token);
        }
        return ;
    }
}

void                    jo_fs_wait_background_jobs(void)
{
    // (the music keeps its buffer filled: its read goes first, see jo_cd_take_sectors())
    while (__jo_fs_background_job_count)
    {
        if (__jo_core_music_update != JO_NULL)
            __jo_core_music_update();
        jo_fs_do_background_jobs();
    }
}

bool                    jo_fs_read_file_async_ptr(const char *const filename, jo_fs_async_read_callback callback, int optional_token, void *buf)
{
    register int        i;
    int			        fid;
    int                 nsct;

#ifdef JO_DEBUG
    if (callback == JO_NULL)
    {
        jo_core_error("callback is null");
        return (false);
    }
#endif
    for (JO_ZERO(i); i < JO_MAX_FS_BACKGROUND_JOBS; ++i)
    {
        if (__jo_fs_background_jobs[i].active)
            continue;
        fid = __jo_fs_get_file_id(filename);
        if (fid < 0)
        {
#ifdef JO_DEBUG
            jo_core_error("%s: File not found", filename);
#endif
            return (false);
        }
        if ((__jo_fs_background_jobs[i].handle = __jo_fs_open(fid)) == JO_NULL)
        {
#ifdef JO_DEBUG
            jo_core_error("%s: Too many opened files", filename);
#endif
            return (false);
        }
        __jo_fs_background_jobs[i].file_length = __jo_fs_get_file_size(__jo_fs_background_jobs[i].handle);
        nsct = __jo_fs_get_sector_count(__jo_fs_background_jobs[i].handle);
        if (buf != JO_NULL)
            __jo_fs_background_jobs[i].contents = buf;
        else if ((__jo_fs_background_jobs[i].contents = jo_malloc(JO_SECTOR_SIZE * nsct + 1)) == JO_NULL)
        {
#ifdef JO_DEBUG
            jo_core_error("%s: Out of memory", filename);
#endif
            __jo_fs_close(__jo_fs_background_jobs[i].handle);
            return (false);
        }
        __jo_fs_background_jobs[i].token = optional_token;
        __jo_fs_background_jobs[i].callback = callback;
        __jo_fs_background_jobs[i].ptr = __jo_fs_background_jobs[i].contents;
        __jo_fs_background_jobs[i].active = true;
        ++__jo_fs_background_job_count;
        return (true);
    }
#ifdef JO_DEBUG
    jo_core_error("%s: Too many background jobs", filename);
#endif
    return (false);
}

bool			        jo_fs_cd(const char *const sub_dir)
{
    int			        fid;
    int                 count;
    unsigned int        fad;
    unsigned int        size;

#ifdef JO_DEBUG
    if (sub_dir == JO_NULL)
    {
        jo_core_error("sub_dir is null");
        return (false);
    }
#endif
    fid = __jo_fs_get_file_id(sub_dir);
    if (fid < 0 || !(__jo_fs_dir[fid].fad & JO_FS_DIRECTORY_FAD_FLAG))
    {
#ifdef JO_DEBUG
        jo_core_error("%s: Directory not found", sub_dir);
#endif
        return (false);
    }
    fad = JO_FS_ENTRY_FAD(&__jo_fs_dir[fid]);
    size = __jo_fs_dir[fid].size;
    if ((count = jo_fs_load_directory(fad, size)) < 0)
    {
#ifdef JO_DEBUG
        jo_core_error("%s: Read failed", sub_dir);
#endif
        // (the table was half replaced by the entries read: the current directory is loaded again)
        count = jo_fs_load_directory(__jo_fs_dir_fad, __jo_fs_dir_size);
        __jo_fs_dir_count = count < 0 ? 0 : count;
        return (false);
    }
    __jo_fs_dir_fad = fad;
    __jo_fs_dir_size = size;
    __jo_fs_dir_count = count;
    return (true);
}

/** @brief Read a whole file
 *  @param file_id File identifier
 *  @param buf Output buffer
 *  @param buf_size Size of the output buffer
 *  @return Number of bytes read or -1 if failed
 */
/*
** Compressed files: "JOLZ", then the data compressed by Compiler/COMMON/jo_lz.py (its size, big endian, then LZSS: see
** jo_lz_decompress()). jo_fs_open() and jo_fs_read_file() read them decompressed, as they read the other files
*/

/** @brief The 4 bytes at the start of a compressed file */
# define JO_FS_LZ_MAGIC                 (0x4A4F4C5A)
/** @brief Size of the header of a compressed file (the magic, the size of the data) */
# define JO_FS_LZ_HEADER                (8)
/** @brief Bytes of the history of the LZSS (the longest distance of a copy) */
# define JO_FS_LZ_WINDOW                (4096)

/** @brief State of the decompression of a compressed file */
typedef struct
{
    unsigned char                       window[JO_FS_LZ_WINDOW];    // The last bytes decompressed (pos & (window - 1))
    unsigned int                        pos;                        // Bytes decompressed
    unsigned int                        flags;                      // Flags of the group of items (the high bit first)
    unsigned int                        bits;                       // Flags left in the group
    unsigned int                        copy_left;                  // Bytes left in the copy in progress
    unsigned int                        copy_from;                  // Position of its next byte
}                                       jo_fs_lz_state;

/** @brief Read the next sectors of a file in its read buffer
 *  @param file File
 *  @return false if the read failed
 */
static bool             jo_fs_fill_read_buffer(jo_file * const file);

/** @brief Read bytes of a file as they are on the CD
 *  @param file File
 *  @param buffer Output
 *  @param nbytes Bytes to read
 *  @param read The bytes already read (updated)
 *  @param size The size of the file on the CD
 *  @return Bytes read
 */
static int              jo_fs_read_raw(jo_file * const file, char *buffer, unsigned int nbytes, int * const read, const int size)
{
    int                 readed;
    unsigned int        n;

    JO_ZERO(readed);
    while (nbytes > 0 && *read < size)
    {
        // The read buffer is empty: read the next sectors
        if (file->read_index >= file->read_buffer_size && !jo_fs_fill_read_buffer(file))
            break;
        // Copy what is available in the read buffer at once
        n = file->read_buffer_size - file->read_index;
        if (n > nbytes)
            n = nbytes;
        if (n > (unsigned int)(size - *read))
            n = size - *read;
        jo_memcpy(buffer, file->read_buffer + file->read_index, n);
        buffer += n;
        file->read_index += n;
        nbytes -= n;
        *read += n;
        readed += n;
    }
    return (readed);
}

/** @brief The next byte of a compressed file on the CD
 *  @param file File
 *  @return The byte, or -1 at the end of the file (or if the read failed)
 */
static int              jo_fs_lz_next_raw_byte(jo_file * const file)
{
    if (file->raw_read >= file->raw_size)
        return (-1);
    if (file->read_index >= file->read_buffer_size && !jo_fs_fill_read_buffer(file))
        return (-1);
    ++file->raw_read;
    return ((unsigned char)file->read_buffer[file->read_index++]);
}

/** @brief Decompress the next bytes of a compressed file
 *  @param file File
 *  @param buffer Output (JO_NULL: the bytes are skipped)
 *  @param nbytes Bytes wanted
 *  @return Bytes decompressed (fewer at the end of the file, or if the read failed)
 */
static int              jo_fs_lz_read(jo_file * const file, unsigned char *buffer, unsigned int nbytes)
{
    jo_fs_lz_state      *s;
    unsigned char       *window;
    const unsigned char *in;
    const unsigned char *in_end;
    unsigned int        pos;
    unsigned int        flags;
    unsigned int        bits;
    unsigned int        from;
    unsigned int        n;
    unsigned int        k;
    unsigned int        v;
    unsigned int        b;
    int                 c;

    s = (jo_fs_lz_state *)file->lz;
    window = s->window;
    pos = s->pos;
    flags = s->flags;
    bits = s->bits;
    if (nbytes > (unsigned int)(file->size - file->read))
        nbytes = file->size - file->read;
    // (the bytes of the read buffer read through local pointers: a call per byte was 3 times slower; the last sectors of
    // the file can hold bytes after its end, in_end stops at its end)
    in = (const unsigned char *)file->read_buffer + file->read_index;
    in_end = in + JO_MIN(file->read_buffer_size - file->read_index, file->raw_size - file->raw_read);
# define JO_FS_LZ_NEXT(c)   do { if (in < in_end) c = *in++; else { \
                                 file->raw_read += (int)(in - ((const unsigned char *)file->read_buffer + file->read_index)); \
                                 file->read_index = (int)(in - (const unsigned char *)file->read_buffer); \
                                 c = jo_fs_lz_next_raw_byte(file); \
                                 in = (const unsigned char *)file->read_buffer + file->read_index; \
                                 in_end = in + JO_MIN(file->read_buffer_size - file->read_index, file->raw_size - file->raw_read); } } while (0)
    JO_ZERO(n);
    while (n < nbytes)
    {
        // Fast path: a whole group of 8 items (at most 17 bytes in, 144 bytes out), in the read buffer, into the output
        if (bits == 0 && s->copy_left == 0 && buffer != JO_NULL && nbytes - n >= 8 * 18 && in_end - in >= 17)
        {
            flags = *in++;
            for (k = 0; k < 8; ++k, flags <<= 1)
            {
                if (flags & 0x80)
                {
                    b = *in++;
                    window[pos++ & (JO_FS_LZ_WINDOW - 1)] = (unsigned char)b;
                    *buffer++ = (unsigned char)b;
                    ++n;
                    continue;
                }
                v = ((unsigned int)in[0] << 8) | in[1];
                in += 2;
                from = pos - ((v >> 4) + 1);
                v = (v & 15) + 3;
                n += v;
                do
                {
                    b = window[from++ & (JO_FS_LZ_WINDOW - 1)];
                    window[pos++ & (JO_FS_LZ_WINDOW - 1)] = (unsigned char)b;
                    *buffer++ = (unsigned char)b;
                }
                while (--v > 0);
            }
            continue;
        }
        // The copy in progress (it can read the bytes it writes: the window is written byte by byte)
        if (s->copy_left > 0)
        {
            k = JO_MIN(s->copy_left, nbytes - n);
            s->copy_left -= k;
            n += k;
            from = s->copy_from;
            s->copy_from += k;
            if (buffer != JO_NULL)
                while (k-- > 0)
                {
                    b = window[from++ & (JO_FS_LZ_WINDOW - 1)];
                    window[pos++ & (JO_FS_LZ_WINDOW - 1)] = (unsigned char)b;
                    *buffer++ = (unsigned char)b;
                }
            else
                while (k-- > 0)
                    window[pos++ & (JO_FS_LZ_WINDOW - 1)] = window[from++ & (JO_FS_LZ_WINDOW - 1)];
            continue;
        }
        if (bits == 0)
        {
            JO_FS_LZ_NEXT(c);
            if (c < 0)
                break;
            flags = (unsigned int)c;
            bits = 8;
        }
        --bits;
        if (flags & 0x80)
        {
            // A byte as it is
            flags <<= 1;
            JO_FS_LZ_NEXT(c);
            if (c < 0)
                break;
            window[pos++ & (JO_FS_LZ_WINDOW - 1)] = (unsigned char)c;
            if (buffer != JO_NULL)
                *buffer++ = (unsigned char)c;
            ++n;
            continue;
        }
        // A copy of 3 to 18 bytes from 1 to 4096 bytes before
        flags <<= 1;
        JO_FS_LZ_NEXT(c);
        if (c < 0)
            break;
        v = (unsigned int)c << 8;
        JO_FS_LZ_NEXT(c);
        if (c < 0)
            break;
        v |= (unsigned int)c;
        s->copy_from = pos - ((v >> 4) + 1);
        s->copy_left = (v & 15) + 3;
    }
# undef JO_FS_LZ_NEXT
    file->raw_read += (int)(in - ((const unsigned char *)file->read_buffer + file->read_index));
    file->read_index = (int)(in - (const unsigned char *)file->read_buffer);
    s->pos = pos;
    s->flags = flags;
    s->bits = bits;
    file->read += n;
    return ((int)n);
}

/** @brief Open a file of the CD for the reads of jo_fs_read_next_bytes(): its read buffer, its first sectors read (a
 *  compressed file: the state of its decompression)
 *  @param file File (id set)
 *  @param filename Its name (the errors)
 *  @return false if it failed
 */
static bool             jo_fs_open_id(jo_file * const file, const char *const filename)
{
    const unsigned char *h;

    file->lz = JO_NULL;
    if ((file->handle = __jo_fs_open(file->id)) == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("%s: Too many opened files", filename);
#else
        (void)filename;
#endif
        return (false);
    }
    file->size = __jo_fs_get_file_size(file->handle);
    // The buffer size is kept: jo_fs_set_read_sector_count() can be called while the file is open
    file->read_buffer_size = __jo_fs_read_buffer_size;
    file->read_index = file->read_buffer_size;
    JO_ZERO(file->read);
    if ((file->read_buffer = (char *)jo_malloc_with_behaviour(file->read_buffer_size * sizeof(*file->read_buffer),
                             JO_MALLOC_TRY_REUSE_BLOCK)) == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("%s: Out of memory", filename);
#endif
        __jo_fs_close(file->handle);
        file->handle = JO_NULL;
        return (false);
    }
    if (file->size < JO_FS_LZ_HEADER)
        return (true);
    // Compressed: its first bytes (read now: the reads of the CD go on from there)
    if (!jo_fs_fill_read_buffer(file))
    {
        jo_fs_close(file);
        return (false);
    }
    h = (const unsigned char *)file->read_buffer;
    if (jo_lz_size(h) != JO_FS_LZ_MAGIC)
        return (true);
    if ((file->lz = jo_malloc_with_behaviour(sizeof(jo_fs_lz_state), JO_MALLOC_TRY_REUSE_BLOCK)) == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("%s: Out of memory", filename);
#endif
        jo_fs_close(file);
        return (false);
    }
    JO_ZERO(((jo_fs_lz_state *)file->lz)->pos);
    JO_ZERO(((jo_fs_lz_state *)file->lz)->flags);
    JO_ZERO(((jo_fs_lz_state *)file->lz)->bits);
    JO_ZERO(((jo_fs_lz_state *)file->lz)->copy_left);
    JO_ZERO(((jo_fs_lz_state *)file->lz)->copy_from);
    file->raw_size = file->size;
    file->raw_read = JO_FS_LZ_HEADER;
    file->read_index = JO_FS_LZ_HEADER;
    file->size = (int)jo_lz_size(h + 4);
    return (true);
}

static int		        jo_fs_load(const int file_id, void *buf, const int buf_size)
{
    void                *handle;
    int                 result;

    if ((handle = __jo_fs_open(file_id)) == JO_NULL)
        return (-1);
    result = __jo_fs_read_sectors(handle, __jo_fs_get_sector_count(handle), buf, buf_size);
    __jo_fs_close(handle);
    return (result);
}


void                    jo_fs_get_read_stats(unsigned int * const vblanks, unsigned int * const bytes, unsigned int * const files,
                                             const bool reset)
{
    if (vblanks != JO_NULL)
        *vblanks = __jo_fs_stat_vblanks;
    if (bytes != JO_NULL)
        *bytes = __jo_fs_stat_bytes;
    if (files != JO_NULL)
        *files = __jo_fs_stat_files;
    if (reset)
    {
        __jo_fs_stat_vblanks = 0;
        __jo_fs_stat_bytes = 0;
        __jo_fs_stat_files = 0;
    }
}

char                    *jo_fs_read_file_ptr(const char *const filename, void *buf, int *len)
{
    int			        fid;
    int			        fsize;
    int                 retry;
    char                *stream;

#ifdef JO_DEBUG
    if (filename == JO_NULL)
    {
        jo_core_error("filename is null");
        return (JO_NULL);
    }
#endif
    fid = __jo_fs_get_file_id(filename);
    if (fid < 0)
    {
#ifdef JO_DEBUG
        jo_core_error("%s: File not found", filename);
#endif
        return (JO_NULL);
    }
    fsize = __jo_fs_dir[fid].size;
    // Its first sectors read: a compressed file is read decompressed (see jo_fs_open()), the others go on with a read of
    // their next sectors in place
    {
        jo_file         file;
        int             first;

        file.id = fid;
        if (jo_fs_open_id(&file, filename))
        {
            if (file.lz != JO_NULL)
                fsize = file.size;
            stream = buf != JO_NULL ? (char *)buf : (char *)jo_malloc_with_behaviour((fsize + 1) * sizeof(*stream),
                                                                                     JO_MALLOC_TRY_REUSE_BLOCK);
            if (stream == JO_NULL)
            {
                jo_fs_close(&file);
#ifdef JO_DEBUG
                jo_core_error("%s: Out of memory", filename);
#endif
                return (JO_NULL);
            }
            if (file.lz != JO_NULL)
                first = jo_fs_lz_read(&file, (unsigned char *)stream, (unsigned int)fsize) == fsize ? fsize : -1;
            else
            {
                // (the bytes of the first sectors, then the next ones; a file of less than 8 bytes is read here)
                first = file.read_index == 0 ? JO_MIN(file.read_buffer_size, fsize) : 0;
                jo_memcpy(stream, file.read_buffer, (unsigned int)first);
                if (fsize > first && __jo_fs_read_sectors(file.handle, __jo_fs_get_sector_count(file.handle), stream + first,
                                                          fsize - first) < 1)
                    first = -1;
            }
            jo_fs_close(&file);
            if (first >= 0)
            {
                JO_ZERO(stream[fsize]);
                if (len != JO_NULL)
                    *len = fsize;
                return (stream);
            }
            if (buf == JO_NULL)
                jo_free(stream);
            // (a failed read: the file read again from its start, with the retries below)
            fsize = __jo_fs_dir[fid].size;
            if (file.lz != JO_NULL)
            {
#ifdef JO_DEBUG
                jo_core_error("%s: Read failed", filename);
#endif
                return (JO_NULL);
            }
        }
    }
    if (buf != JO_NULL)
        stream = (char *)buf;
    else if ((stream = (char *)jo_malloc_with_behaviour((fsize + 1) * sizeof(*stream), JO_MALLOC_TRY_REUSE_BLOCK)) == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("%s: Out of memory", filename);
#endif
        return (JO_NULL);
    }
    // An empty file returns 0 bytes
    for (retry = JO_READ_RETRY_COUNT; jo_fs_load(fid, stream, fsize) < (fsize > 0 ? 1 : 0); --retry)
    {
        if (!retry)
        {
            if (buf == JO_NULL)
                jo_free(stream);
#ifdef JO_DEBUG
            jo_core_error("%s: Read failed", filename);
#endif
            return (JO_NULL);
        }
        jo_spin_wait(JO_READ_SPIN_WAIT);
    }
    JO_ZERO(stream[fsize]);
    if (len != JO_NULL)
        *len = fsize;
    return stream;
}

char*					jo_fs_read_file_in_dir(const char *const filename, const char *const sub_dir, int *len)
{
    char                *stream;
    int                 fid;

    // (a directory not found: no read in the current directory, and no move to its parent after it)
    if (sub_dir != JO_NULL)
    {
        fid = __jo_fs_get_file_id(sub_dir);
        if (fid < 0 || !(__jo_fs_dir[fid].fad & JO_FS_DIRECTORY_FAD_FLAG))
        {
#ifdef JO_DEBUG
            jo_core_error("%s: Directory not found", sub_dir);
#endif
            return (JO_NULL);
        }
        // (a failed read: still in the current directory, no move to its parent after)
        if (!jo_fs_cd(sub_dir))
            return (JO_NULL);
    }
    stream = jo_fs_read_file(filename, len);
    if (sub_dir != JO_NULL)
        jo_fs_cd(JO_PARENT_DIR);
    return stream;
}

bool                    jo_fs_open(jo_file * const file, const char *const filename)
{
#ifdef JO_DEBUG
    if (file == JO_NULL)
    {
        jo_core_error("file is null");
        return (false);
    }
    if (filename == JO_NULL)
    {
        jo_core_error("filename is null");
        return (false);
    }
#endif
    file->id = __jo_fs_get_file_id(filename);
    file->lz = JO_NULL;
    file->read_buffer = JO_NULL;
    file->handle = JO_NULL;
    if (file->id < 0)
    {
#ifdef JO_DEBUG
        jo_core_error("%s: File not found", filename);
#endif
        return (false);
    }
    return (jo_fs_open_id(file, filename));
}

/** @brief Read the next sectors of a file in its read buffer
 *  @param file File
 *  @return false if the read failed
 */
static bool             jo_fs_fill_read_buffer(jo_file * const file)
{
    int                 retry;

    for (retry = JO_READ_RETRY_COUNT; __jo_fs_read_sectors(file->handle, file->read_buffer_size / JO_SECTOR_SIZE,
                                                            file->read_buffer, file->read_buffer_size) < 0; --retry)
    {
        if (!retry)
            return (false);
        jo_spin_wait(JO_READ_SPIN_WAIT);
    }
    JO_ZERO(file->read_index);
    return (true);
}

bool                    jo_fs_seek_forward(jo_file * const file, unsigned int nbytes)
{
    int                 remaining;
    int                 buffers;

#ifdef JO_DEBUG
    if (file == JO_NULL)
    {
        jo_core_error("file is null");
        return (false);
    }
#endif
    if (nbytes > (unsigned int)(file->size - file->read))
        nbytes = file->size - file->read;
    // A compressed file: the bytes decompressed and dropped
    if (file->lz != JO_NULL)
        return (jo_fs_lz_read(file, JO_NULL, nbytes) == (int)nbytes);
    // Bytes still in the read buffer
    remaining = file->read_buffer_size - file->read_index;
    if ((int)nbytes <= remaining)
    {
        file->read_index += nbytes;
        file->read += nbytes;
        return (true);
    }
    nbytes -= remaining;
    file->read += remaining;
    file->read_index = file->read_buffer_size;
    // Skip the whole buffers without reading them
    buffers = nbytes / file->read_buffer_size;
    if (buffers > 0 && !__jo_fs_seek_sector(file->handle, ((jo_fs_handle *)file->handle)->position + buffers * (file->read_buffer_size / JO_SECTOR_SIZE)))
    {
#ifdef JO_DEBUG
        jo_core_error("Seek failed");
#endif
        return (false);
    }
    file->read += buffers * file->read_buffer_size;
    nbytes %= file->read_buffer_size;
    if (!nbytes)
        return (true);
    if (!jo_fs_fill_read_buffer(file))
    {
#ifdef JO_DEBUG
        jo_core_error("Read failed");
#endif
        return (false);
    }
    file->read_index = nbytes;
    file->read += nbytes;
    return (true);
}

int                     jo_fs_read_next_bytes(jo_file * const file, char *buffer, unsigned int nbytes)
{
#ifdef JO_DEBUG
    if (file == JO_NULL)
    {
        jo_core_error("file is null");
        return (-1);
    }
    if (buffer == JO_NULL)
    {
        jo_core_error("buffer is null");
        return (-1);
    }
#endif
    if (file->lz != JO_NULL)
        return (jo_fs_lz_read(file, (unsigned char *)buffer, nbytes));
    return (jo_fs_read_raw(file, buffer, nbytes, &file->read, file->size));
}

void                    jo_fs_close(jo_file * const file)
{
#ifdef JO_DEBUG
    if (file == JO_NULL)
    {
        jo_core_error("file is null");
        return ;
    }
#endif
    if (file->lz != JO_NULL)
        jo_free(file->lz);
    if (file->read_buffer != JO_NULL)
        jo_free(file->read_buffer);
    if (file->handle != JO_NULL)
        __jo_fs_close(file->handle);
    file->lz = JO_NULL;
    file->read_buffer = JO_NULL;
    file->handle = JO_NULL;
}

void            jo_fs_set_read_sector_count(const int count)
{
#ifdef JO_DEBUG
    if (count < 1)
    {
        jo_core_error("count < 1");
        return ;
    }
#endif
    __jo_fs_read_buffer_size = count * JO_SECTOR_SIZE;
}

#endif /* !JO_COMPILE_WITH_FS_SUPPORT */

/*
** END OF FILE
*/
