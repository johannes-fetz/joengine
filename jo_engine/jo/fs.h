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
 *  @return true if the directory is the current one, false if not found or not read (the current directory is kept)
 */
bool                                        jo_fs_cd(const char *const sub_dir);

/** @brief Check if a file is in the current directory of the CD (no error printed when it is not: an optional file)
 *  @param filename File name (example: "LEVEL.BIN")
 *  @return true if the file exists
 */
bool                                        jo_fs_file_exists(const char *const filename);

/** @brief Read a file on the CD
 *  @param filename Filename (upper case and shorter as possible like "A.TXT")
 *  @param sub_dir Sub directory name (use JO_ROOT_DIR if the file is on the root directory)
 *  @param len return the file length
 *  @return The stream
 */
char*                                       jo_fs_read_file_in_dir(const char *const filename, const char *const sub_dir, int *len);

/** @brief Read a file on the CD and put the contents to "buf"
 *  @param filename Filename (upper case and shorter as possible like "A.TXT")
 *  @param buf Output buffer: the file length + 1 byte at least (a null character is written after the contents)
 *  @param len return the file length
 *  @return The stream
 *  @remarks A compressed file ("JOLZ" first: Compiler/COMMON/jo_lz.py -f) is read decompressed: len is the size of its
 *  data, buf must hold it (+ 1). Fewer sectors read: a file of 40 % of its size loads about 2.5 times faster (the CPU
 *  decompresses about 1 MB per second, while the drive reads the next sectors)
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
 *  @param buf Output buffer: the file length + 1 byte at least (a null character is written after the contents)
 *  @return true if succeed
 *  @remarks A compressed file (see jo_fs_read_file_ptr()) is read as it is on the CD (not decompressed)
 *  @remarks The frames do not wait for the drive: each one takes the sectors already read (8 at most). The music
 *  (jo_audio_play_music()) goes on: it has the drive first when its buffer needs it. The files are read one after the other
 */
bool                                        jo_fs_read_file_async_ptr(const char *const filename, jo_fs_async_read_callback callback, int optional_token, void *buf);

/** @brief Wait for the end of the reads of jo_fs_read_file_async() (their callbacks called, the files they start too)
 *  @remarks The music (jo_audio_play_music()) goes on meanwhile
 */
void                                        jo_fs_wait_background_jobs(void);

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
 *  @remarks A compressed file ("JOLZ" first, see jo_fs_read_file_ptr()) is read decompressed by jo_fs_read_next_bytes()
 *  and jo_fs_seek_forward() (file->size: the size of its data; 4 KB more of memory while it is open)
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

/** @brief Drive status: the drive is paused */
# define JO_CD_DRIVE_PAUSE                          (0x01)
/** @brief Drive status: the drive is playing (audio or data) */
# define JO_CD_DRIVE_PLAY                           (0x03)

#endif /* !JO_COMPILE_WITH_FS_SUPPORT */

#endif /* !__JO_FS_H__ */

/*
** END OF FILE
*/
