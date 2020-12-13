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
#include "jo/tools.h"
#include "jo/malloc.h"
#include "jo/list.h"
#include "jo/backup.h"

# ifdef JO_COMPILE_WITH_BACKUP_SUPPORT

/*
** INTERNAL MACROS
*/

/** @brief Driver workspace size */
# define JO_BACKUP_LIB_SPACE_SIZE               (16384)
/** @brief User workspace size for the driver */
# define JO_BACKUP_WORK_SPACE_SIZE              (8192)
/** @brief Set write behaviour */
# define JO_BACKUP_OVERRIDE_FILE_IF_EXISTS      (0)
/** @brief backup driver base address */
# define JO_BACKUP_DRIVER_ADDR                  (*(volatile unsigned int *)(0x6000354))
/** @brief Helper to get backup driver functions addresses */
# define JO_BACKUP_FUNCTION_ADDR(INDEX)         (*(unsigned int *)(JO_BACKUP_DRIVER_ADDR + INDEX))

/** @brief Init backup driver
 *  @param LIB Driver address
 *  @param WORKSPACE User workspace for the driver
 *  @param CONFIG Driver configuration
 */
# define JO_BACKUP_DRIVER_INIT(LIB, WORKSPACE, CONFIG)       \
        ((void (*)(volatile unsigned int *, unsigned int *, jo_backup_config[3]))(*(volatile unsigned int *)(0x6000358)))(LIB, WORKSPACE, CONFIG)

/** @brief Format backup device
 *  @param DEVICE device id
 */
# define JO_BACKUP_DRIVER_FORMAT(DEVICE) \
        ((int (*)(unsigned int))JO_BACKUP_FUNCTION_ADDR(8))(DEVICE)

/** @brief Get stat on backup device
 *  @param DEVICE device id
 *  @param DATASIZE data size
 *  @param OUTPUT_STAT jo_backup_stat (output)
 */
# define JO_BACKUP_DRIVER_STAT(DEVICE, DATASIZE, OUTPUT_STAT)	\
        ((int (*)(unsigned int, unsigned int, jo_backup_stat *))JO_BACKUP_FUNCTION_ADDR(12))(DEVICE, DATASIZE, OUTPUT_STAT)

/** @brief Write on backup device
 *  @param DEVICE device id
 *  @param FILE jo_backup_file file struct pointer
 *  @param DATA file contents
 *  @param MODE Save mode (append, overwrite, etc)
 */
# define JO_BACKUP_DRIVER_WRITE(DEVICE, FILE, DATA, MODE)	\
        ((int (*)(unsigned int, jo_backup_file *, volatile unsigned char *, unsigned char))JO_BACKUP_FUNCTION_ADDR(16))(DEVICE, FILE, DATA, MODE)

/** @brief Read from backup device
 *  @param DEVICE device id
 *  @param FILENAME File name
 *  @param OUTPUT_DATA file contents (output)
 */
# define JO_BACKUP_DRIVER_READ(DEVICE, FILENAME, OUTPUT_DATA)	\
        ((int (*)(unsigned int, unsigned char *, volatile unsigned char *))JO_BACKUP_FUNCTION_ADDR(20))(DEVICE, FILENAME, OUTPUT_DATA)

/** @brief Delete file from backup device
 *  @param DEVICE device id
 *  @param FILENAME File name
 */
# define JO_BACKUP_DRIVER_DELETE(DEVICE, FILENAME)	\
        ((int (*)(unsigned int, unsigned char *))JO_BACKUP_FUNCTION_ADDR(24))(DEVICE, FILENAME)

/** @brief get file info from backup device
 *  @param DEVICE device id
 *  @param FILENAME File name
 *  @param FILESIZE File size (use JO_BACKUP_DRIVER_STAT before)
 *  @param OUTPUT_FILE jo_backup_file (output)
 */
# define JO_BACKUP_DRIVER_GET_FILE_INFO(DEVICE, FILENAME, FILESIZE, OUTPUT_FILE)   \
        ((int (*)(unsigned int, unsigned char *, unsigned short tbsize, jo_backup_file *))JO_BACKUP_FUNCTION_ADDR(28))(DEVICE, FILENAME, FILESIZE, OUTPUT_FILE)

/** @brief Check file checksum from backup device
 *  @param DEVICE device id
 *  @param FILENAME File name
 *  @param DATA file contents
 */
# define JO_BACKUP_DRIVER_CHECKSUM(DEVICE, FILENAME, DATA) \
        ((int (*)(unsigned int, unsigned char *, volatile unsigned char *))JO_BACKUP_FUNCTION_ADDR(32))(DEVICE, FILENAME, DATA)

/** @brief Get backup device last edit date
 *  @param DEVICE device id
 *  @param OUTPUT_DATE jo_backup_date (output)
 */
# define JO_BACKUP_DRIVER_GET_DATE(DEVICE, OUTPUT_DATE)	\
        ((void (*)(unsigned int, jo_backup_date *))JO_BACKUP_FUNCTION_ADDR(36))(DEVICE, OUTPUT_DATE)

/** @brief Prepare date for backup devices
 *  @param DATE jo_backup_date
 */
# define JO_BACKUP_DRIVER_PREPARE_DATE(DATE)	\
        ((unsigned int (*)(jo_backup_date *))JO_BACKUP_FUNCTION_ADDR(40))(DATE)

/** @brief Internal backup config struct */
typedef struct
{
    unsigned short	unit_id;
    unsigned short	partition;
}                   jo_backup_config;

/** @brief Internal backup stat struct */
typedef	struct
{
    unsigned int	totalsize;
    unsigned int	totalblock;
    unsigned int	blocksize;
    unsigned int	freesize;
    unsigned int	freeblock;
    unsigned int	datanum;
}                   jo_backup_stat;

/** @brief Internal backup file struct */
typedef struct
{
    unsigned char	filename[JO_BACKUP_MAX_FILENAME_LENGTH];
    unsigned char	comment[JO_BACKUP_MAX_COMMENT_LENGTH + 1];
    unsigned char	language;
    unsigned int	date;
    unsigned int	datasize;
    unsigned short	blocksize;
}                   jo_backup_file;

/** @brief Internal backup date struct */
typedef struct
{
    unsigned char	year;
    unsigned char	month;
    unsigned char	day;
    unsigned char	time;
    unsigned char	min;
    unsigned char	week;
}                   jo_backup_date;

/** @brief Internal backup device struct */
typedef struct
{
    jo_backup_stat      sttb;
    int                 status;
    bool                is_mounted;
}                           __jo_backup_device;

/*
** GLOBALS
*/
static __jo_backup_device   __jo_backup_devices[3];
static bool                 __jo_backup_initialized = false;
static unsigned int         *__jo_backup_lib_space;
static unsigned int         *__jo_backup_work_space;

/*
** PRIVATE FUNCTIONS
*/

static void                     jo_backup_init(void)
{
    static jo_backup_config     cntb[3];

    if ((__jo_backup_lib_space = (unsigned int *)jo_malloc_with_behaviour(JO_BACKUP_LIB_SPACE_SIZE, JO_FAST_ALLOCATION)) == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Out of memory #1");
#endif
        return;
    }
    if ((__jo_backup_work_space = (unsigned int *)jo_malloc_with_behaviour(JO_BACKUP_WORK_SPACE_SIZE, JO_FAST_ALLOCATION)) == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Out of memory #2");
#endif
        return;
    }
    jo_core_disable_reset();
    JO_BACKUP_DRIVER_INIT(__jo_backup_lib_space, __jo_backup_work_space, cntb);
    jo_core_enable_reset();
    __jo_backup_devices[JoInternalMemoryBackup].is_mounted = false;
    __jo_backup_devices[JoCartridgeMemoryBackup].is_mounted = false;
    __jo_backup_devices[JoExternalDeviceBackup].is_mounted = false;
    __jo_backup_initialized = true;
}

/*
** PUBLIC FUNCTIONS
*/

int                         jo_backup_get_free_block_count(const jo_backup_device backup_device)
{
    if (!__jo_backup_devices[backup_device].is_mounted)
    {
#ifdef JO_DEBUG
        jo_core_error("Device not mounted");
#endif
        return (-1);
    }
    return (__jo_backup_devices[backup_device].sttb.freeblock);
}

bool                        jo_backup_format_device(const jo_backup_device backup_device)
{
    if (!__jo_backup_devices[backup_device].is_mounted)
    {
#ifdef JO_DEBUG
        jo_core_error("Device not mounted");
#endif
        return (false);
    }
    jo_core_disable_reset();
    __jo_backup_devices[backup_device].status = JO_BACKUP_DRIVER_FORMAT(backup_device);
    jo_core_enable_reset();
    JO_BACKUP_DRIVER_STAT(backup_device, 10, &__jo_backup_devices[backup_device].sttb);
    return (__jo_backup_devices[backup_device].status == 0);
}

bool                        jo_backup_mount(const jo_backup_device backup_device)
{

    if (!__jo_backup_initialized)
        jo_backup_init();
    if ((__jo_backup_devices[backup_device].status = JO_BACKUP_DRIVER_STAT(backup_device, 10, &__jo_backup_devices[backup_device].sttb)) == 2)
        jo_backup_format_device(backup_device);
    __jo_backup_devices[backup_device].is_mounted = (__jo_backup_devices[backup_device].status == 0);
    if (!__jo_backup_devices[backup_device].is_mounted)
    {
#ifdef JO_DEBUG
        jo_core_error("Fail to mount device");
#endif
        return (false);
    }
    return (__jo_backup_devices[backup_device].is_mounted);
}

bool                    jo_backup_read_device(const jo_backup_device backup_device, jo_list * const filenames)
{
    register int        i;
    register int        j;
    jo_backup_file      *dir;
    char                *str;

    if (!__jo_backup_devices[backup_device].is_mounted)
    {
#ifdef JO_DEBUG
        jo_core_error("Device not mounted");
#endif
        return (false);
    }
    if ((dir = (jo_backup_file *)jo_malloc_with_behaviour(JO_BACKUP_MAX_FILE * sizeof(*dir), JO_FAST_ALLOCATION)) == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Out of memory #1");
#endif
        return (false);
    }
    for (JO_ZERO(i); i < JO_BACKUP_MAX_FILE; ++i)
        JO_ZERO(dir[i].filename[0]);
    JO_BACKUP_DRIVER_GET_FILE_INFO(backup_device, (unsigned char*)"", JO_BACKUP_MAX_FILE, dir);
    for (JO_ZERO(i); i < JO_BACKUP_MAX_FILE && dir[i].filename[0] != '\0'; ++i)
    {
        if ((str = (char *)jo_malloc_with_behaviour(JO_BACKUP_MAX_FILENAME_LENGTH * sizeof(*str), JO_MALLOC_TRY_REUSE_SAME_BLOCK_SIZE)) == JO_NULL)
        {
#ifdef JO_DEBUG
            jo_core_error("Out of memory #2");
#endif
            jo_free(dir);
            jo_list_free_and_clear(filenames);
            return (false);
        }
        for (JO_ZERO(j); dir[i].filename[j] != '\0'; ++j)
            str[j] = dir[i].filename[j];
        JO_ZERO(str[j]);
        jo_list_add_ptr(filenames, str);
    }
    jo_free(dir);
    return (true);
}

bool                jo_backup_file_exists(const jo_backup_device backup_device, const char * const fname)
{
    jo_backup_file  dir;

    if (!__jo_backup_devices[backup_device].is_mounted)
    {
#ifdef JO_DEBUG
        jo_core_error("Device not mounted");
#endif
        return (false);
    }
    __jo_backup_devices[backup_device].status = JO_BACKUP_DRIVER_GET_FILE_INFO(backup_device, (Uint8 *)fname, 1, &dir);
    return (__jo_backup_devices[backup_device].status == 1);
}

bool                jo_backup_save_file_contents(const jo_backup_device backup_device, const char * const fname, const char * const comment, void *contents, unsigned int content_size)
{
    jo_backup_date  date;
    jo_backup_file  dir;
    jo_datetime     now;
    register int    len;
    register int    i;

    if (!__jo_backup_devices[backup_device].is_mounted)
    {
#ifdef JO_DEBUG
        jo_core_error("Device not mounted");
#endif
        return (false);
    }
    if (comment == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("comment is null");
#endif
        return (false);
    }
    if (contents == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("contents is null");
#endif
        return (false);
    }
    if (fname == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("fname is null");
#endif
        return (false);
    }
    len = jo_strlen(fname);
    if (len > 11)
    {
#ifdef JO_DEBUG
        jo_core_error("fname too long (%d) (max 11)", len);
#endif
        return (false);
    }
    for (JO_ZERO(i); i < len; ++i)
        dir.filename[i] = (Uint8)fname[i];
    JO_ZERO(dir.filename[i]);
    len = jo_strlen(comment);
    if (len > JO_BACKUP_MAX_COMMENT_LENGTH)
    {
#ifdef JO_DEBUG
        jo_core_error("comment too long (%d) (max %d)", len, JO_BACKUP_MAX_COMMENT_LENGTH);
#endif
        return (false);
    }
    for (JO_ZERO(i); i < len; ++i)
        dir.comment[i] = (Uint8)comment[i];
    JO_ZERO(dir.comment[i]);
    /* LANGUAGE */
    switch (jo_get_current_language())
    {
    case Espanol:
        dir.language = 4;
        break;
    case Japanese:
        dir.language = 0;
        break;
    case Italiano:
        dir.language = 5;
        break;
    case Deutsch:
        dir.language = 3;
        break;
    case French:
        dir.language = 2;
        break;
    case English:
    default:
        dir.language = 1;
        break;
    }
    /* DATE */
    jo_getdate(&now);
    date.year = now.year - 1980;
    date.month = now.month;
    date.week = now.week;
    date.day = now.day;
    date.time = now.hour;
    date.min = now.minute;
    dir.date = JO_BACKUP_DRIVER_PREPARE_DATE(&date);
    dir.datasize = content_size;
    jo_core_disable_reset();
    __jo_backup_devices[backup_device].status = JO_BACKUP_DRIVER_WRITE(backup_device, &dir, contents, JO_BACKUP_OVERRIDE_FILE_IF_EXISTS);
    if (__jo_backup_devices[backup_device].status == 0)
        __jo_backup_devices[backup_device].status = JO_BACKUP_DRIVER_CHECKSUM(backup_device, dir.filename, contents);
    jo_core_enable_reset();
    JO_BACKUP_DRIVER_STAT(backup_device, 10, &__jo_backup_devices[backup_device].sttb);
    return (__jo_backup_devices[backup_device].status == 0);
}

unsigned char           *jo_backup_load_file_comment(const jo_backup_device backup_device, const char * const fname)
{
    jo_backup_file      dir;
    unsigned char       *comment;
    register int        i;

    if (!__jo_backup_devices[backup_device].is_mounted)
    {
#ifdef JO_DEBUG
        jo_core_error("Device not mounted");
#endif
        return (JO_NULL);
    }
    if (JO_BACKUP_DRIVER_GET_FILE_INFO(backup_device, (Uint8 *)fname, 1, &dir) != 1)
        return (JO_NULL);
    if ((comment = (Uint8 *)jo_malloc_with_behaviour((JO_BACKUP_MAX_COMMENT_LENGTH + 1) * sizeof(*comment), JO_MALLOC_TRY_REUSE_BLOCK)) == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Out of memory");
#endif
        return (JO_NULL);
    }
    for (JO_ZERO(i); i < JO_BACKUP_MAX_COMMENT_LENGTH && dir.comment[i] != '\0'; ++i)
        comment[i] = dir.comment[i];
    JO_ZERO(comment[i]);
    return (comment);
}

void                    *jo_backup_load_file_contents(const jo_backup_device backup_device, const char * const fname, unsigned int *length)
{
    jo_backup_file      dir;
    unsigned char       *content;

    if (!__jo_backup_devices[backup_device].is_mounted)
    {
#ifdef JO_DEBUG
        jo_core_error("Device not mounted");
#endif
        return (JO_NULL);
    }
    if (JO_BACKUP_DRIVER_GET_FILE_INFO(backup_device, (Uint8 *)fname, 1, &dir) != 1)
        return (JO_NULL);
    if ((content = (Uint8 *)jo_malloc_with_behaviour(dir.datasize + 1, JO_MALLOC_TRY_REUSE_BLOCK)) == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Out of memory");
#endif
        return (JO_NULL);
    }
    jo_core_disable_reset();
    __jo_backup_devices[backup_device].status = JO_BACKUP_DRIVER_READ(backup_device, (Uint8 *)fname, content);
    if (__jo_backup_devices[backup_device].status == 0)
        __jo_backup_devices[backup_device].status = JO_BACKUP_DRIVER_CHECKSUM(backup_device, (Uint8 *)fname, content);
    jo_core_enable_reset();
    if (__jo_backup_devices[backup_device].status != 0)
    {
        jo_free(content);
        return (JO_NULL);
    }
    if (length != JO_NULL)
        *length = dir.datasize;
    JO_ZERO(content[dir.datasize]);
    return (content);
}

bool                    jo_backup_delete_file(const jo_backup_device backup_device, const char * const fname)
{
    if (!__jo_backup_devices[backup_device].is_mounted)
    {
#ifdef JO_DEBUG
        jo_core_error("Device not mounted");
#endif
        return (false);
    }
    jo_core_disable_reset();
    __jo_backup_devices[backup_device].status = JO_BACKUP_DRIVER_DELETE(backup_device, (Uint8 *)fname);
    jo_core_enable_reset();
    JO_BACKUP_DRIVER_STAT(backup_device, 10, &__jo_backup_devices[backup_device].sttb);
    return (__jo_backup_devices[backup_device].status == 0);
}

bool                        jo_backup_unmount(const jo_backup_device backup_device)
{
    if (!__jo_backup_devices[backup_device].is_mounted)
    {
#ifdef JO_DEBUG
        jo_core_error("Device not mounted");
#endif
        return (false);
    }
    __jo_backup_devices[backup_device].is_mounted = false;
    if (!__jo_backup_devices[JoInternalMemoryBackup].is_mounted && !__jo_backup_devices[JoCartridgeMemoryBackup].is_mounted && !__jo_backup_devices[JoExternalDeviceBackup].is_mounted)
    {
        jo_free(__jo_backup_lib_space);
        jo_free(__jo_backup_work_space);
        __jo_backup_initialized = false;
    }
    return (true);
}

bool                jo_backup_get_file_last_modified_date(const jo_backup_device backup_device, const char * const fname, jo_datetime *datetime)
{
    jo_backup_file  dir;
    jo_backup_date  bdate;

    if (datetime == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("datetime is null");
#endif
        return (false);
    }
    if (!__jo_backup_devices[backup_device].is_mounted)
    {
#ifdef JO_DEBUG
        jo_core_error("Device not mounted");
#endif
        return (false);
    }
    if (JO_BACKUP_DRIVER_GET_FILE_INFO(backup_device, (Uint8 *)fname, 1, &dir) == 1)
    {
        JO_BACKUP_DRIVER_GET_DATE(dir.date, &bdate);
        datetime->year = bdate.year + 1980;
        datetime->month = bdate.month;
        datetime->week = bdate.week;
        datetime->day = bdate.day;
        datetime->hour = bdate.time;
        datetime->minute = bdate.min;
        return (true);
    }
    return (false);
}

bool                jo_backup_get_file_size(const jo_backup_device backup_device, const char * const fname, unsigned int* const num_bytes, unsigned int* const num_blocks)
{
    jo_backup_file  dir;

    if (num_bytes == JO_NULL && num_blocks == NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("num_bytes and num_blocks can't both be null");
#endif
        return (false);
    }
    if (!__jo_backup_devices[backup_device].is_mounted)
    {
#ifdef JO_DEBUG
        jo_core_error("Device not mounted");
#endif
        return (false);
    }
    if (JO_BACKUP_DRIVER_GET_FILE_INFO(backup_device, (Uint8 *)fname, 1, &dir) == 1)
    {
        if (num_bytes != JO_NULL)
            *num_bytes = dir.datasize;
        if (num_blocks != JO_NULL)
            *num_blocks = dir.blocksize;
        return (true);
    }
    return (false);
}

jo_backup_status    jo_backup_get_last_status(const jo_backup_device backup_device)
{
    if (!__jo_backup_devices[backup_device].is_mounted)
    {
#ifdef JO_DEBUG
        jo_core_error("Device not mounted");
#endif
        return (JoBackupStatusUnknown);
    }
    switch (__jo_backup_devices[backup_device].status)
    {
    case 0:
    case 1:
        return (JoBackupStatusOk);
    case 3:
        return (JoBackupStatusWriteProtect);
    case 4:
        return (JoBackupStatusOutOfMemory);
    case 5:
        return (JoBackupStatusNotFound);
    case 7:
        return (JoBackupStatusNoMatch);
    case 8:
        return (JoBackupStatusBroken);
    default:
        return (JoBackupStatusUnknown);
    }
}

#endif /* !JO_COMPILE_WITH_BACKUP_SUPPORT */

/*
** END OF FILE
*/
