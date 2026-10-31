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
/** @file fs.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine File System function prototypes
 *  @bug No known bugs.
 */

#ifndef __JO_FS_H__
# define __JO_FS_H__

#ifdef JO_COMPILE_WITH_FS_SUPPORT

/*
** MACROS
*/
/** @brief Specify the root directory (on the CD) for jo_fs_read_file(), jo_map_load_from_file(), etc. */
# define JO_ROOT_DIR				((const char *)0)
/** @brief Specify the current directory (on the CD) for jo_fs_read_file(), jo_map_load_from_file(), etc. */
# define JO_CURRENT_DIR				((const char *)0)
/** @brief Specify the parent directory for jo_fs_read_file(), jo_map_load_from_file(), etc. */
# define JO_PARENT_DIR				("..")

/*
** TYPEDEFS
*/
/** @brief Function prototype for () */
typedef void                                (*jo_fs_async_read_callback)(char *contents, int length, int optional_token);

/** @brief Change the current directory (equivalent of Unix cd command)
 *  @param sub_dir Sub directory name (use JO_PARENT_DIR for parent directory)
 */
void                                        jo_fs_cd(const char *const sub_dir);

/** @brief Read a file on the CD
 *  @param filename Filename (upper case and shorter as possible like "A.TXT")
 *  @param sub_dir Sub directory name (use JO_ROOT_DIR if the file is on the root directory)
 *  @param len return the file length
 *  @return The stream
 */
char*                                       jo_fs_read_file_in_dir(const char *const filename, const char *const sub_dir, int *len);

/** @brief Read a file on the CD and put the contents to "buf"
 *  @param filename Filename (upper case and shorter as possible like "A.TXT")
 *  @param buf Output buffer
 *  @param len return the file length
 *  @return The stream
 */
char                                        *jo_fs_read_file_ptr(const char *const filename, void *buf, int *len);

/** @brief Statistics of the files read by jo_fs_read_file_ptr() (the time of a loading: the CD or the CPU)
 *  @param vblanks Time spent reading (VBLANK count), JO_NULL: not wanted
 *  @param bytes Bytes read
 *  @param files Files read
 *  @param reset The counts start again from 0
 */
void                                        jo_fs_get_read_stats(unsigned int * const vblanks, unsigned int * const bytes,
                                                                 unsigned int * const files, const bool reset);

/** @brief Read a file on the CD
 *  @param filename Filename (upper case and shorter as possible like "A.TXT")
 *  @param len return the file length
 *  @return The stream
 */
static  __jo_force_inline char*             jo_fs_read_file(const char *const filename, int *len)
{
    return (jo_fs_read_file_ptr(filename, 0, len));
}

/** @brief Read a file on the CD asynchronously and put the contents to "buf"
 *  @param filename Filename (upper case and shorter as possible like "A.TXT")
 *  @param callback Callback called when the file is loaded
 *  @param optional_token User value to identify the file
 *  @param buf Output buffer
 *  @return true if succeed
 */
bool                                        jo_fs_read_file_async_ptr(const char *const filename, jo_fs_async_read_callback callback, int optional_token, void *buf);

/** @brief Read a file on the CD asynchronously
 *  @param filename Filename (upper case and shorter as possible like "A.TXT")
 *  @param callback Callback called when the file is loaded
 *  @param optional_token User value to identify the file
 *  @return true if succeed
 */
static  __jo_force_inline bool              jo_fs_read_file_async(const char *const filename, jo_fs_async_read_callback callback, int optional_token)
{
    return (jo_fs_read_file_async_ptr(filename, callback, optional_token, 0));
}

/** @brief Open a file
 *  @param file Pointer to an allocated jo_file struct
 *  @param filename Filename (upper case and shorter as possible like "A.TXT")
 *  @return true if succeed
 */
bool                                        jo_fs_open(jo_file * const file, const char *const filename);

/** @brief Close a file
 *  @param file Pointer to an allocated and valid jo_file struct
 */
void                                        jo_fs_close(jo_file * const file);

/** @brief Set the size of the read buffer of the files opened by jo_fs_open()
 *  @param count Number of sectors (2048 bytes) read at once by jo_fs_read_next_bytes() (1 by default)
 *  @remarks Only the files opened after this call use the new size
 */
void                                        jo_fs_set_read_sector_count(const int count);

/** @brief Read bytes from a file
 *  @param file Pointer to an allocated and valid jo_file struct
 *  @param buffer Pointer to an allocated buffer (length >= nbytes)
 *  @param nbytes number of bytes to read
 *  @return Number of bytes read (<= 0 means EOF)
 */
int                                         jo_fs_read_next_bytes(jo_file * const file, char *buffer, unsigned int nbytes);

/** @brief Seek forward from current position of a file
 *  @param file Pointer to an allocated and valid jo_file struct
 *  @param nbytes number of bytes to skip
 *  @return true if succeed
 */
bool                                        jo_fs_seek_forward(jo_file * const file, unsigned int nbytes);

/*
** Internal usage
*/

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

/** @brief Get the size of a file in sectors (Internal usage)
 *  @param handle File handle
 *  @return File size in sectors (2048 bytes)
 */
int                                         __jo_fs_get_sector_count(void *handle);

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

/** @brief Drive status: the drive is paused */
# define JO_CD_DRIVE_PAUSE                          (0x01)
/** @brief Drive status: the drive is playing (audio or data) */
# define JO_CD_DRIVE_PLAY                           (0x03)

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

#endif /* !__JO_FS_H__ */

/*
** END OF FILE
*/
