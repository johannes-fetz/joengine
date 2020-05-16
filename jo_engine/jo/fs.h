/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2020, Johannes Fetz (johannesfetz@gmail.com)
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

#endif /* !JO_COMPILE_WITH_FS_SUPPORT */

#endif /* !__JO_FS_H__ */

/*
** END OF FILE
*/
