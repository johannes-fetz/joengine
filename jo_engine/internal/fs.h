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
/** @file internal/fs.h
 *  @author Johannes Fetz
 *
 *  @brief File system and CD drive: internal functions of the engine (see jo/fs.h)
 *  @warning Not part of the API: only for the source files of the engine (included after jo/fs.h). The games
 *  include jo/jo.h, which doesn't include this file
 *  @bug No known bugs.
 */

#ifndef __JO_INTERNAL_FS_H__
# define __JO_INTERNAL_FS_H__

#ifdef JO_COMPILE_WITH_FS_SUPPORT

/** @brief Get the identifier of a file in the current directory (Internal usage)
 *  @param filename Filename (upper case, like "A.TXT")
 *  @return File identifier or -1 if the file doesn't exist
 */
int                                         __jo_fs_get_file_id(const char * const filename);

/** @brief Open a file (Internal usage)
 *  @param file_id File identifier (see __jo_fs_get_file_id())
 *  @return File handle or JO_NULL if the identifier is invalid or if too many files are opened
 *  @remarks JO_MAX_FS_BACKGROUND_JOBS files can be opened at the same time
 */
void                                        *__jo_fs_open(const int file_id);

/** @brief Close a file opened with __jo_fs_open() (Internal usage)
 *  @param handle File handle (JO_NULL is ignored)
 */
void                                        __jo_fs_close(void *handle);

/** @brief Get the size of a file in bytes (Internal usage)
 *  @param handle File handle
 *  @return File size in bytes
 */
int                                         __jo_fs_get_file_size(void *handle);

/** @brief Get the first sector of a file (Internal usage)
 *  @param handle File handle
 *  @return Frame address of the first sector
 */
unsigned int                                __jo_fs_get_fad(void *handle);

/** @brief Start a read of sectors for a stream, without waiting for them (Internal usage)
 *  @param fad First sector (frame address)
 *  @param count Sector count
 *  @return 0 if succeed, -1 if failed
 *  @remarks A read of a file (jo_fs_read_file()...) takes the drive back: see __jo_fs_stream_take()
 */
int                                         __jo_fs_stream_read(const unsigned int fad, const unsigned int count);

/** @brief Take the sectors of the stream read already in the buffer of the CD block, without waiting (Internal usage)
 *  @param buf Output buffer (aligned on 4 bytes)
 *  @param max_sectors Maximum sector count
 *  @return Sector count (0: none yet), -1 if the read is over or was cancelled by another read
 */
int                                         __jo_fs_stream_take(void *buf, const int max_sectors);

/** @brief The stream read started by __jo_fs_stream_read() still has sectors to come (Internal usage)
 *  @return true if the drive reads for the stream
 */
bool                                        __jo_fs_stream_is_reading(void);

/** @brief Stop the stream read (Internal usage) */
void                                        __jo_fs_stream_stop(void);

/** @brief Called by the reads of files while they wait for the drive (Internal usage) */
extern void                                 (*__jo_fs_stream_service)(void);

/** @brief Set the current position of a file (Internal usage)
 *  @param handle File handle
 *  @param sector Sector index from the beginning of the file
 *  @return false if the position is outside of the file
 */
bool                                        __jo_fs_seek_sector(void *handle, const int sector);

/** @brief Read sectors from the current position of a file (Internal usage)
 *  @param handle File handle
 *  @param sector_count Number of sectors to read
 *  @param buf Output buffer
 *  @param buf_size Size of the output buffer (bytes after it are not written)
 *  @return Number of bytes stored in buf, 0 at the end of the file, or -1 if the read failed
 *  @remarks The current position moves forward even if buf is too small
 */
int                                         __jo_fs_read_sectors(void *handle, int sector_count, void *buf, const int buf_size);

/** @brief Play CD audio tracks (Internal usage)
 *  @param from_track First track (the first audio track is usually 2)
 *  @param to_track Last track (played until its end)
 *  @param play_mode 0x00: no repeat, 0x01~0x0e: maximum repeats, 0x0f: endless repeat
 *  @remarks Any data read in progress is cancelled
 */
void                                        __jo_cd_play_tracks(const int from_track, const int to_track, const unsigned char play_mode);

/** @brief Play CD audio from a sector (Internal usage)
 *  @param fad First sector (frame address)
 *  @param from_track First track (played from its start if fad is not before the end of to_track)
 *  @param to_track Last track (played until its end)
 *  @param play_mode 0x00: no repeat, 0x01~0x0e: maximum repeats, 0x0f: endless repeat
 *  @remarks Used to resume the CD audio after a pause. Any data read in progress is cancelled
 */
void                                        __jo_cd_play_from_fad(const unsigned int fad, const int from_track, const int to_track,
                                                                  const unsigned char play_mode);

/** @brief Get the CD drive status (Internal usage)
 *  @param fad Current sector (frame address) of the pickup (output, can be JO_NULL)
 *  @return Drive status (JO_CD_DRIVE_PAUSE, JO_CD_DRIVE_PLAY, etc) or -1 if the CD block doesn't answer
 */
int                                         __jo_cd_get_status(unsigned int *fad);

/** @brief Pause the CD drive (Internal usage)
 *  @remarks Stops the CD audio, the pickup stays at the current position
 */
void                                        __jo_cd_pause(void);

#endif /* !JO_COMPILE_WITH_FS_SUPPORT */

#endif /* !__JO_INTERNAL_FS_H__ */

/*
** END OF FILE
*/
