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
/** @file level.h
 *  @author Johannes Fetz
 *
 *  @brief Levels (.JOL files): the data of a level of a game (terrain, models, plants, decals...), see
 *         Documentation/level_format.md. The files are written by the Python tools of Compiler/COMMON/jo_level
 *  @remarks Module JO_COMPILE_WITH_LEVEL_MODULE = 1 in the makefile
 *  @remarks The file is read once and used as it is (big endian, aligned): a section is a pointer in the file. A header
 *           or records of an older version (smaller) are copied with the default values of the missing fields
 *           (jo_level_get_header(), jo_level_get_records())
 */

#ifndef __JO_LEVEL_H__
# define __JO_LEVEL_H__

#ifdef JO_COMPILE_WITH_LEVEL_SUPPORT

# include "level_format.h"

/** @brief Maximum number of normalized copies of headers and records of a level (older files, see jo_level_get_records()) */
# define JO_LEVEL_MAX_COPIES                (16)

/** @brief A loaded level */
typedef struct
{
    unsigned char                   *data;          // The file
    unsigned int                    size;
    const jo_level_directory_entry  *directory;
    unsigned int                    section_count;
    const char                      *strings;       // STRS section (JO_NULL: none)
    unsigned int                    strings_size;
    void                            *copies[JO_LEVEL_MAX_COPIES];
    unsigned int                    copy_count;
}                                   jo_level;

/** @brief Load a level file (in the current directory of the CD, see jo_fs_cd())
 *  @param filename File name (for instance "LEVEL.JOL")
 *  @return The level, JO_NULL if the file is missing or invalid (the reason is printed with JO_DEBUG)
 *  @remarks A section unknown by the engine is ignored, unless it is required (the level is refused)
 */
jo_level                            *jo_level_load(const char * const filename);

/** @brief Free a level (the pointers given by the functions below are not valid any more)
 *  @param level Level
 */
void                                jo_level_free(jo_level * const level);

/** @brief Find a section (also a section of the game: identifier with a lowercase letter)
 *  @param level Level
 *  @param id Identifier (JO_LEVEL_ID('T', 'E', 'R', 'R') or JO_LEVEL_SECTION_xxx)
 *  @return The entry of the directory (version, sizes, number of records), JO_NULL if the level has no such section
 */
const jo_level_directory_entry      *jo_level_find_section(const jo_level * const level, const unsigned int id);

/** @brief Start of a section in the file
 *  @param level Level
 *  @param id Identifier
 *  @return Pointer to the section (its header, then its records, then its arrays), JO_NULL if there is no such section
 */
const void                          *jo_level_section_data(const jo_level * const level, const unsigned int id);

/** @brief Header of a section, with the fields of this engine
 *  @param level Level
 *  @param id Identifier
 *  @param size Size of the header structure of the engine (sizeof(jo_level_xxx_header))
 *  @param defaults Default values (JO_LEVEL_xxx_HEADER_DEFAULTS): the fields missing in an older file
 *  @return The header (in the file when it has all the fields, else a copy), JO_NULL if there is no such section
 */
const void                          *jo_level_get_header(jo_level * const level, const unsigned int id, const unsigned int size,
                                                         const void * const defaults);

/** @brief Records of a section, with the fields of this engine
 *  @param level Level
 *  @param id Identifier
 *  @param size Size of the record structure of the engine (sizeof(jo_level_xxx_record))
 *  @param defaults Default values (JO_LEVEL_xxx_RECORD_DEFAULTS): the fields missing in an older file
 *  @param count Number of records (output, 0 if there is no such section)
 *  @return The records (in the file when they have all the fields, else a copy), JO_NULL if there are none
 */
const void                          *jo_level_get_records(jo_level * const level, const unsigned int id, const unsigned int size,
                                                          const void * const defaults, unsigned int * const count);

/** @brief Array of a section (an offset field of its header)
 *  @param level Level
 *  @param id Identifier
 *  @param offset Offset of the array from the start of the section (field of the header)
 *  @return Pointer to the array, JO_NULL if the offset is 0
 */
const void                          *jo_level_get_array(const jo_level * const level, const unsigned int id, const unsigned int offset);

/** @brief A string of the level (field of type string)
 *  @param level Level
 *  @param offset Offset in the STRS section
 *  @return The string, "" if there is no string
 */
const char                          *jo_level_string(const jo_level * const level, const unsigned int offset);

/** @brief Read a header of a section (typed): jo_level_xxx_header *h = JO_LEVEL_HEADER(level, TERR, terr) */
# define JO_LEVEL_HEADER(LEVEL, ID, NAME)   ({ static const jo_level_##NAME##_header __defaults = JO_LEVEL_##ID##_HEADER_DEFAULTS; \
                                               (const jo_level_##NAME##_header *)jo_level_get_header((LEVEL), JO_LEVEL_SECTION_##ID, \
                                               sizeof(jo_level_##NAME##_header), &__defaults); })

/** @brief Read the records of a section (typed): jo_level_xxx_record *r = JO_LEVEL_RECORDS(level, PLNT, plnt, &count) */
# define JO_LEVEL_RECORDS(LEVEL, ID, NAME, COUNT) ({ static const jo_level_##NAME##_record __defaults = JO_LEVEL_##ID##_RECORD_DEFAULTS; \
                                               (const jo_level_##NAME##_record *)jo_level_get_records((LEVEL), JO_LEVEL_SECTION_##ID, \
                                               sizeof(jo_level_##NAME##_record), &__defaults, (COUNT)); })

#endif /* !JO_COMPILE_WITH_LEVEL_SUPPORT */

#endif /* !__JO_LEVEL_H__ */

/*
** END OF FILE
*/
