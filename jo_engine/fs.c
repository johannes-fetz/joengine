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

/*
** INCLUDES
*/
#include <stdbool.h>
#include "jo/sgl_prototypes.h"
#include "jo/conf.h"
#include "jo/types.h"
#include "jo/sega_saturn.h"
#include "jo/smpc.h"
#include "jo/core.h"
#include "jo/math.h"
#include "jo/fs.h"
#include "jo/tools.h"
#include "jo/malloc.h"

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

/** @brief Maximum sector fetched once (for jo_fs_read_file_async())
 */
# define JO_MAXIMUM_SECTOR_FETCHED_ONCE_ASYNC     (8)

/** @brief Size of each read jo(for jo_fs_do_background_jobs())
 */
# define JO_READ_SIZE_ASYNC                       (JO_MAXIMUM_SECTOR_FETCHED_ONCE_ASYNC * JO_SECTOR_SIZE)

#ifdef JO_COMPILE_WITH_FS_SUPPORT

typedef struct
{
    bool                        active;
    jo_fs_async_read_callback   callback;
    GfsHn                       gfs;
    int                         file_length;
    char                        *contents;
    char                        *ptr;
    int                         token;
}                               __jo_fs_background_job;

/*
** GLOBALS
*/
static char                     __jo_fs_work[GFS_WORK_SIZE(JO_OPEN_MAX)];
static GfsDirTbl				__jo_fs_dirtbl;
static GfsDirName				__jo_fs_dirname[JO_FS_MAX_FILES];
static __jo_fs_background_job   __jo_fs_background_jobs[JO_MAX_FS_BACKGROUND_JOBS];
unsigned int                    __jo_fs_background_job_count;
static int                      __jo_fs_read_buffer_size = JO_SECTOR_SIZE;

int								jo_fs_init()
{
    register int                i;

    JO_ZERO(__jo_fs_background_job_count);
    for (JO_ZERO(i); i < JO_MAX_FS_BACKGROUND_JOBS; ++i)
        __jo_fs_background_jobs[i].active = false;
    GFS_DIRTBL_TYPE(&__jo_fs_dirtbl) = GFS_DIR_NAME;
    GFS_DIRTBL_DIRNAME(&__jo_fs_dirtbl) = __jo_fs_dirname;
    GFS_DIRTBL_NDIR(&__jo_fs_dirtbl) = JO_FS_MAX_FILES;
    if (GFS_Init(JO_OPEN_MAX, __jo_fs_work, &__jo_fs_dirtbl) <= 2)
    {
#ifdef JO_DEBUG
        jo_core_error("GFS_Init() failed");
#endif
        return (0);
    }
    return (1);
}

void                    jo_fs_do_background_jobs(void)
{
    register int        i;
    Sint32              stat;
    Sint32              nbyte;

    for (JO_ZERO(i); i < JO_MAX_FS_BACKGROUND_JOBS; ++i)
    {
        if (!__jo_fs_background_jobs[i].active)
            continue;
        GFS_NwFread(__jo_fs_background_jobs[i].gfs, JO_MAXIMUM_SECTOR_FETCHED_ONCE_ASYNC, __jo_fs_background_jobs[i].ptr, JO_READ_SIZE_ASYNC);
        do
        {
            GFS_NwExecOne(__jo_fs_background_jobs[i].gfs);
            GFS_NwGetStat(__jo_fs_background_jobs[i].gfs, &stat, &nbyte);
        }
        while (nbyte < JO_READ_SIZE_ASYNC && stat != GFS_SVR_COMPLETED);
        if (stat == GFS_SVR_COMPLETED)
        {
            GFS_Close(__jo_fs_background_jobs[i].gfs);
            --__jo_fs_background_job_count;
            JO_ZERO(__jo_fs_background_jobs[i].contents[__jo_fs_background_jobs[i].file_length]);
            __jo_fs_background_jobs[i].active = false;
            __jo_fs_background_jobs[i].callback(__jo_fs_background_jobs[i].contents, __jo_fs_background_jobs[i].file_length, __jo_fs_background_jobs[i].token);
        }
        else
            __jo_fs_background_jobs[i].ptr += nbyte;
        return ; //TODO
    }
}

bool                    jo_fs_read_file_async_ptr(const char *const filename, jo_fs_async_read_callback callback, int optional_token, void *buf)
{
    register int        i;
    int			        fid;
    Sint32			    sctsize;
    Sint32              nsct;
    Sint32              lastsize;

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
        fid = GFS_NameToId((Sint8 *)filename);
        if (fid < 0)
        {
#ifdef JO_DEBUG
            jo_core_error("%s: File not found", filename);
#endif
            return (false);
        }
        if ((__jo_fs_background_jobs[i].gfs = GFS_Open(fid)) == JO_NULL)
        {
#ifdef JO_DEBUG
            jo_core_error("%s: GFS_Open() failed", filename);
#endif
            return (false);
        }
        GFS_GetFileSize(__jo_fs_background_jobs[i].gfs, &sctsize, &nsct, &lastsize);
        __jo_fs_background_jobs[i].file_length = (sctsize * (nsct - 1) + lastsize);
        if (buf != JO_NULL)
            __jo_fs_background_jobs[i].contents = buf;
        else if ((__jo_fs_background_jobs[i].contents = jo_malloc(sctsize * nsct + 1)) == JO_NULL)
        {
#ifdef JO_DEBUG
            jo_core_error("%s: Out of memory", filename);
#endif
            GFS_Close(__jo_fs_background_jobs[i].gfs);
            return (false);
        }
        __jo_fs_background_jobs[i].token = optional_token;
        __jo_fs_background_jobs[i].callback = callback;
        __jo_fs_background_jobs[i].ptr = __jo_fs_background_jobs[i].contents;
        GFS_SetReadPara(__jo_fs_background_jobs[i].gfs, JO_READ_SIZE_ASYNC);
        GFS_SetTransPara(__jo_fs_background_jobs[i].gfs, JO_MAXIMUM_SECTOR_FETCHED_ONCE_ASYNC);
        GFS_SetTmode(__jo_fs_background_jobs[i].gfs, GFS_TMODE_SCU);
        GFS_NwCdRead(__jo_fs_background_jobs[i].gfs, sctsize * nsct);
        __jo_fs_background_jobs[i].active = true;
        ++__jo_fs_background_job_count;
        return (true);
    }
#ifdef JO_DEBUG
    jo_core_error("%s: Too many background jobs", filename);
#endif
    return (false);
}

void			        jo_fs_cd(const char *const sub_dir)
{
    Sint32			    fid;

#ifdef JO_DEBUG
    if (sub_dir == JO_NULL)
    {
        jo_core_error("sub_dir is null");
        return ;
    }
#endif
    fid = GFS_NameToId((Sint8 *)sub_dir);
    GFS_DIRTBL_TYPE(&__jo_fs_dirtbl) = GFS_DIR_NAME;
    GFS_DIRTBL_DIRNAME(&__jo_fs_dirtbl) = __jo_fs_dirname;
    GFS_DIRTBL_NDIR(&__jo_fs_dirtbl) = JO_FS_MAX_FILES;
    GFS_LoadDir(fid, &__jo_fs_dirtbl);
    GFS_SetDir(&__jo_fs_dirtbl);
}

static int		        jo_fs_get_file_size(int file_id)
{
    GfsHn			    gfs;
    Sint32			    sctsize;
    Sint32              nsct;
    Sint32              lastsize;

    if ((gfs = GFS_Open(file_id)) == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("GFS_Open() failed");
#endif
        return (-1);
    }
    GFS_GetFileSize(gfs, &sctsize, &nsct, &lastsize);
    GFS_Close(gfs);
    return (sctsize * (nsct - 1) + lastsize);
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
    fid = GFS_NameToId((Sint8 *)filename);
    if (fid < 0)
    {
#ifdef JO_DEBUG
        jo_core_error("%s: File not found", filename);
#endif
        return (JO_NULL);
    }
    fsize = jo_fs_get_file_size(fid);
    if (fsize < 0)
        return (JO_NULL);
    if (buf != JO_NULL)
        stream = (char *)buf;
    else if ((stream = (char *)jo_malloc_with_behaviour((fsize + 1) * sizeof(*stream), JO_MALLOC_TRY_REUSE_BLOCK)) == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("%s: Out of memory", filename);
#endif
        return (JO_NULL);
    }
    for (retry = JO_READ_RETRY_COUNT; GFS_Load(fid, 0, stream, fsize) <= 0; --retry)
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

    if (sub_dir != JO_NULL)
        jo_fs_cd(sub_dir);
    stream = jo_fs_read_file(filename, len);
    if (sub_dir != JO_NULL)
        jo_fs_cd(JO_PARENT_DIR);
    return stream;
}

bool                    jo_fs_open(jo_file * const file, const char *const filename)
{
    Sint32			    sctsize;
    Sint32              nsct;
    Sint32              lastsize;

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
    file->id = GFS_NameToId((Sint8 *)filename);
    if (file->id < 0)
    {
#ifdef JO_DEBUG
        jo_core_error("%s: File not found", filename);
#endif
        return (false);
    }
    if ((file->handle = GFS_Open(file->id)) == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("GFS_Open() failed");
#endif
        return (false);
    }
    GFS_GetFileSize((GfsHn)file->handle, &sctsize, &nsct, &lastsize);
    file->size = sctsize * (nsct - 1) + lastsize;
    file->read_index = __jo_fs_read_buffer_size;
    JO_ZERO(file->read);
    if ((file->read_buffer = (char *)jo_malloc_with_behaviour(__jo_fs_read_buffer_size * sizeof(*file->read_buffer),
                             JO_MALLOC_TRY_REUSE_BLOCK)) == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("%s: Out of memory", filename);
#endif
        return (false);
    }
    return (true);
}

bool                    jo_fs_seek_forward(jo_file * const file, unsigned int nbytes)
{
    int                 buffer_remaining;
    int                 sectors;
    int                 retry;

    file->read += nbytes;
    buffer_remaining = (__jo_fs_read_buffer_size - 1) - file->read_index;
    if (buffer_remaining >= (int)nbytes)
    {
        file->read_index += nbytes;
        return (true);
    }
    nbytes -= buffer_remaining;
    sectors = nbytes / __jo_fs_read_buffer_size;
    for (retry = JO_READ_RETRY_COUNT; GFS_Seek((GfsHn)file->handle, sectors, GFS_SEEK_CUR) < 0; --retry)
    {
        if (!retry)
        {
#ifdef JO_DEBUG
            jo_core_error("Seek failed");
#endif
            return (false);
        }
        jo_spin_wait(JO_READ_SPIN_WAIT);
    }
    nbytes = nbytes % __jo_fs_read_buffer_size;
    if (!nbytes)
        return (true);
    for (retry = JO_READ_RETRY_COUNT; GFS_Fread((GfsHn)file->handle, 1, file->read_buffer, __jo_fs_read_buffer_size) < 0; --retry)
    {
        if (!retry)
        {
#ifdef JO_DEBUG
            jo_core_error("Read failed");
#endif
            return (false);
        }
        jo_spin_wait(JO_READ_SPIN_WAIT);
    }
    file->read_index = nbytes - 1;
    return (true);
}

int                     jo_fs_read_next_bytes(jo_file * const file, char *buffer, unsigned int nbytes)
{
    int                 readed;
    int                 len;
    int                 retry;

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
    JO_ZERO(readed);
fs_read_from_buffer:
    while (file->read_index < __jo_fs_read_buffer_size && nbytes > 0)
    {
        *buffer++ = file->read_buffer[file->read_index++];
        --nbytes;
        ++readed;
    }
    if (nbytes <= 0 || file->read >= file->size)
        return (readed);
    JO_ZERO(file->read_index);
    for (retry = JO_READ_RETRY_COUNT; (len = GFS_Fread((GfsHn)file->handle, 1, file->read_buffer, __jo_fs_read_buffer_size)) < 0; --retry)
    {
        if (!retry)
            return (readed);
        jo_spin_wait(JO_READ_SPIN_WAIT);
    }
    if (len <= 0)
        return (readed);
    file->read += len;
    goto fs_read_from_buffer;
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
    jo_free(file->read_buffer);
    GFS_Close((GfsHn)file->handle);
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
