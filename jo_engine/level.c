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

#include <stdbool.h>
#include "jo/conf.h"

#ifdef JO_COMPILE_WITH_LEVEL_SUPPORT

#include "jo/types.h"
#include "jo/sega_saturn.h"
#include "jo/smpc.h"
#include "jo/core.h"
#include "jo/tools.h"
#include "jo/malloc.h"
#include "jo/fs.h"
#include "jo/math.h"
#include "jo/level.h"

/** @brief Sections known by this engine and their versions: a required section must be known (else the level is refused) */
static const unsigned int           __jo_level_known[][2] =
{
    {JO_LEVEL_SECTION_INFO, JO_LEVEL_INFO_VERSION},
    {JO_LEVEL_SECTION_STRS, JO_LEVEL_STRS_VERSION},
    {JO_LEVEL_SECTION_TERR, JO_LEVEL_TERR_VERSION},
    {JO_LEVEL_SECTION_PLNT, JO_LEVEL_PLNT_VERSION},
    {JO_LEVEL_SECTION_MODL, JO_LEVEL_MODL_VERSION},
    {JO_LEVEL_SECTION_MBOX, JO_LEVEL_MBOX_VERSION},
    {JO_LEVEL_SECTION_INST, JO_LEVEL_INST_VERSION},
    {JO_LEVEL_SECTION_DECL, JO_LEVEL_DECL_VERSION},
    {JO_LEVEL_SECTION_ENTI, JO_LEVEL_ENTI_VERSION},
    {JO_LEVEL_SECTION_PROP, JO_LEVEL_PROP_VERSION},
    {JO_LEVEL_SECTION_LINK, JO_LEVEL_LINK_VERSION},
    {JO_LEVEL_SECTION_CELL, JO_LEVEL_CELL_VERSION},
    {JO_LEVEL_SECTION_PRTL, JO_LEVEL_PRTL_VERSION},
    {JO_LEVEL_SECTION_WEAP, JO_LEVEL_WEAP_VERSION},
};

/** @brief Refuse a level (the reason is printed with JO_DEBUG) */
static jo_level                     *jo_level_refuse(jo_level * const level, const char * const filename, const char * const reason,
                                                     const unsigned int id)
{
#ifdef JO_DEBUG
    char                            name[5];

    name[0] = (char)(id >> 24);
    name[1] = (char)(id >> 16);
    name[2] = (char)(id >> 8);
    name[3] = (char)id;
    name[4] = '\0';
    jo_core_error("%s: %s %s", filename, reason, id ? name : "");
#else
    JO_UNUSED_ARG(filename);
    JO_UNUSED_ARG(reason);
    JO_UNUSED_ARG(id);
#endif
    jo_level_free(level);
    return (JO_NULL);
}

jo_level                            *jo_level_load(const char * const filename)
{
    jo_level                        *level;
    const jo_level_file_header      *header;
    const jo_level_directory_entry  *e;
    unsigned int                    k;
    unsigned int                    j;
    int                             len;
    bool                            known;

    level = (jo_level *)jo_malloc(sizeof(*level));
    if (level == JO_NULL)
        return (JO_NULL);
    jo_memset(level, 0, sizeof(*level));
    level->data = (unsigned char *)jo_fs_read_file(filename, &len);
    if (level->data == JO_NULL)
        return (jo_level_refuse(level, filename, "file not found", 0));
    level->size = (unsigned int)len;
    header = (const jo_level_file_header *)level->data;
    if (level->size < sizeof(*header) || header->magic != JO_LEVEL_MAGIC)
        return (jo_level_refuse(level, filename, "not a level file", 0));
    if (header->version > JO_LEVEL_FORMAT_VERSION)
        return (jo_level_refuse(level, filename, "newer file format", 0));
    // (by divisions: a corrupted count must not wrap around 32 bits)
    if (header->size > level->size || header->section_count > (level->size - sizeof(*header)) / sizeof(*e))
        return (jo_level_refuse(level, filename, "file too short", 0));
    level->directory = (const jo_level_directory_entry *)(level->data + sizeof(*header));
    level->section_count = header->section_count;
    for (k = 0, e = level->directory; k < level->section_count; ++k, ++e)
    {
        // (records read in place: aligned on 4 bytes for the SH-2. Records of 0 bytes: any count passed, the copy of an
        // older version (count x the size of the engine) wrapped around 32 bits, a small block written past its end)
        if (((e->offset | e->header_size | e->record_size) & 3) != 0 || e->offset > level->size ||
            e->size > level->size - e->offset || e->header_size > e->size ||
            (e->record_count != 0 && (e->record_size == 0 || e->record_size > (e->size - e->header_size) / e->record_count)))
            return (jo_level_refuse(level, filename, "invalid section", e->id));
        // A required section: this engine must know it, in this version or an older one
        known = false;
        for (j = 0; j < (sizeof(__jo_level_known) / sizeof(__jo_level_known[0])); ++j)
            if (__jo_level_known[j][0] == e->id && e->version <= __jo_level_known[j][1])
                known = true;
        if ((e->flags & JO_LEVEL_FLAG_REQUIRED) && !known)
            return (jo_level_refuse(level, filename, "required section not supported:", e->id));
        // (the strings end with a 0: a damaged section without it, the last string was read past the end of the file)
        if (e->id == JO_LEVEL_SECTION_STRS && e->size != 0 && level->data[e->offset + e->size - 1] == '\0')
        {
            level->strings = (const char *)(level->data + e->offset);
            level->strings_size = e->size;
        }
    }
    return (level);
}

void                                jo_level_free(jo_level * const level)
{
    unsigned int                    k;

    if (level == JO_NULL)
        return;
    for (k = 0; k < level->copy_count; ++k)
        jo_free(level->copies[k]);
    if (level->data != JO_NULL)
        jo_free(level->data);
    jo_free(level);
}

const jo_level_directory_entry      *jo_level_find_section(const jo_level * const level, const unsigned int id)
{
    unsigned int                    k;

    for (k = 0; k < level->section_count; ++k)
        if (level->directory[k].id == id)
            return (&level->directory[k]);
    return (JO_NULL);
}

const void                          *jo_level_section_data(const jo_level * const level, const unsigned int id)
{
    const jo_level_directory_entry  *e;

    e = jo_level_find_section(level, id);
    return (e != JO_NULL ? level->data + e->offset : JO_NULL);
}

/** @brief Copy items of an older version (smaller) with the default values of the missing fields
 *  @param level Level (the copy is freed with it)
 *  @param src Items in the file
 *  @param src_size Size of an item in the file
 *  @param size Size of an item for the engine
 *  @param defaults Default values (one item)
 *  @param count Number of items
 *  @return The copy, JO_NULL if the memory is full
 */
static void                         *jo_level_copy_items(jo_level * const level, const unsigned char *src, const unsigned int src_size,
                                                         const unsigned int size, const void * const defaults, const unsigned int count)
{
    unsigned char                   *copy;
    unsigned int                    k;

    if (level->copy_count >= JO_LEVEL_MAX_COPIES)
    {
#ifdef JO_DEBUG
        jo_core_error("Too many copies of older sections (max=%d)", JO_LEVEL_MAX_COPIES);
#endif
        return (JO_NULL);
    }
    // (a count of a damaged file must not wrap the size of the copy)
    if (size == 0 || count > 0xFFFFFFFFu / size)
        return (JO_NULL);
    copy = (unsigned char *)jo_malloc(size * JO_MAX(count, 1));
    if (copy == JO_NULL)
        return (JO_NULL);
    for (k = 0; k < count; ++k, src += src_size)
    {
        jo_memcpy(copy + k * size, defaults, size);
        jo_memcpy(copy + k * size, src, JO_MIN(src_size, size));
    }
    level->copies[level->copy_count++] = copy;
    return (copy);
}

const void                          *jo_level_get_header(jo_level * const level, const unsigned int id, const unsigned int size,
                                                         const void * const defaults)
{
    const jo_level_directory_entry  *e;

    e = jo_level_find_section(level, id);
    if (e == JO_NULL)
        return (JO_NULL);
    if (e->header_size >= size)
        return (level->data + e->offset);
    return (jo_level_copy_items(level, level->data + e->offset, e->header_size, size, defaults, 1));
}

const void                          *jo_level_get_records(jo_level * const level, const unsigned int id, const unsigned int size,
                                                          const void * const defaults, unsigned int * const count)
{
    const jo_level_directory_entry  *e;
    const unsigned char             *records;
    const void                      *result;

    *count = 0;
    e = jo_level_find_section(level, id);
    if (e == JO_NULL || e->record_count == 0)
        return (JO_NULL);
    records = level->data + e->offset + e->header_size;
    // The records of the file are used as they are when they have the same size (a newer, bigger record can't be used in
    // place: the structure of the engine is smaller than the step between the records)
    if (e->record_size == size)
        result = records;
    else if (e->record_size > size)
    {
        // Newer file: the fields of this engine (the first ones) are copied, the other fields are skipped
        result = jo_level_copy_items(level, records, e->record_size, size, records, e->record_count);
    }
    else
        result = jo_level_copy_items(level, records, e->record_size, size, defaults, e->record_count);
    // (the count only with the records: a copy can fail, out of memory or of copies)
    if (result != JO_NULL)
        *count = e->record_count;
    return (result);
}

const void                          *jo_level_get_array(const jo_level * const level, const unsigned int id, const unsigned int offset)
{
    const jo_level_directory_entry  *e;

    e = jo_level_find_section(level, id);
    if (e == JO_NULL || offset == 0 || offset >= e->size)
        return (JO_NULL);
    return (level->data + e->offset + offset);
}

const char                          *jo_level_string(const jo_level * const level, const unsigned int offset)
{
    if (level->strings == JO_NULL || offset >= level->strings_size)
        return ("");
    return (level->strings + offset);
}

#endif /* !JO_COMPILE_WITH_LEVEL_SUPPORT */

/*
** END OF FILE
*/
