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

#ifdef JO_COMPILE_WITH_3D_SUPPORT

#include "jo/types.h"
#include "jo/sega_saturn.h"
#include "jo/smpc.h"
#include "jo/core.h"
#include "jo/tools.h"
#include "jo/malloc.h"
#include "jo/colors.h"
#include "jo/vdp1_command_pipeline.h"
#include "jo/sprites.h"
#include "jo/fs.h"
#include "jo/math.h"
#include "jo/list.h"
#include "jo/image.h"
#include "jo/tga.h"
#include "jo/3d.h"
#include "jo/model.h"

/** @brief Size of the header of a JOM file */
# define JO_3D_MODEL_HEADER_SIZE                (24)
/** @brief Textures of the models loaded once (see jo_3d_model_texture()): a big model has parts of its texture
 *  (a mesh cut in a grid of textures). When full, the next textures were not kept: the
 *  models loaded later did not find them */
# define JO_3D_MODEL_MAX_TEXTURES               (JO_MAX_SPRITE)

/** @brief Textures of the models (the names as 2 hashes and the sprite ids): 10 bytes a texture instead of 20 (the names of
 *  16 characters: 8 KB with 400 sprites) */
typedef struct
{
    unsigned int                            hash[2];
}                                           jo_3d_model_texture_name;
static jo_3d_model_texture_name             __jo_3d_model_texture_names[JO_3D_MODEL_MAX_TEXTURES];
static short                                __jo_3d_model_texture_ids[JO_3D_MODEL_MAX_TEXTURES];
static int                                  __jo_3d_model_texture_count = 0;

static jo_3d_model                          *jo_3d_create_model(const unsigned char * const data, const unsigned int size, void * const file,
                                                                unsigned char * const storage);
static void                                 jo_3d_model_free(jo_3d_model * const model, const bool free_file);

/** @brief Compare 4 bytes with a chunk tag */
static  __jo_force_inline bool              jo_3d_model_is_tag(const unsigned char * const p, const char * const tag)
{
    return (p[0] == (unsigned char)tag[0] && p[1] == (unsigned char)tag[1] && p[2] == (unsigned char)tag[2] && p[3] == (unsigned char)tag[3]);
}

/** @brief Data of a chunk: in the file data kept by the model, or a copy
 *  @param model Model (model->file: the data of the file are kept)
 *  @param size Size in bytes
 *  @return The data, JO_NULL if out of memory
 */
static void                                 *jo_3d_model_copy(const jo_3d_model * const model, const unsigned char * const data, const unsigned int size)
{
    void                                    *p;

    if (model->file != JO_NULL)
        return ((void *)data);
    p = jo_malloc(size > 0 ? size : 4);
    if (p != JO_NULL)
        jo_memcpy(p, data, size);
    return (p);
}

/** @brief Case insensitive comparison of 2 file names (16 characters at most) */
static bool                                 jo_3d_model_same_name(const char * const a, const char * const b)
{
    register int                            i;
    char                                    ca;
    char                                    cb;

    for (JO_ZERO(i); i < 16; ++i)
    {
        ca = a[i];
        cb = b[i];
        // (the same character: no case conversion. The names of the animations are compared at each frame, see
        // jo_3d_model_is_animation())
        if (ca != cb)
        {
            ca = ca >= 'a' && ca <= 'z' ? ca - 'a' + 'A' : ca;
            cb = cb >= 'a' && cb <= 'z' ? cb - 'a' + 'A' : cb;
            if (ca != cb)
                return (false);
        }
        else if (ca == '\0')
            return (true);
    }
    return (true);
}

/** @brief Mark the animations whose name is the one of an animation before them (see jo_3d_animation.duplicate) */
static void                                 jo_3d_model_mark_duplicates(const jo_3d_model * const model)
{
    unsigned int                            k;
    unsigned int                            j;

    for (JO_ZERO(k); k < model->animation_count; ++k)
    {
        model->animations[k].duplicate = 0;
        for (JO_ZERO(j); j < k; ++j)
            if (jo_3d_model_same_name(model->animations[j].name, model->animations[k].name))
            {
                model->animations[k].duplicate = 1;
                break;
            }
    }
}

/** @brief The 2 hashes of a texture name (case insensitive, its first 15 characters: the length of the names kept before,
 *  FNV-1a and a second one with other constants: 2 different names have the same 64 bits once in billions)
 *  @return false if the name is longer than 15 characters (the names were kept with 15 characters at most: a longer name
 *  was never found)
 */
static bool                                 jo_3d_model_texture_hash(const char * const name, jo_3d_model_texture_name * const out)
{
    register int                            i;
    unsigned int                            c;

    out->hash[0] = 2166136261u;
    out->hash[1] = 0x9E3779B9u;
    for (JO_ZERO(i); i < 15 && name[i] != '\0'; ++i)
    {
        c = (unsigned char)name[i];
        if (c >= 'a' && c <= 'z')
            c -= 'a' - 'A';
        out->hash[0] = (out->hash[0] ^ c) * 16777619u;
        out->hash[1] = (out->hash[1] ^ (c + 0x5Bu)) * 0x01000193u + 0x7F4A7C15u;
    }
    return (name[i] == '\0');
}

/** @brief Find a texture of the models (see jo_3d_model_texture())
 *  @return Sprite id, -1 if not loaded
 */
static int                                  jo_3d_model_find_texture(const char * const name)
{
    register int                            i;
    jo_3d_model_texture_name                key;

    if (!jo_3d_model_texture_hash(name, &key))
        return (-1);
    for (JO_ZERO(i); i < __jo_3d_model_texture_count; ++i)
        if (__jo_3d_model_texture_names[i].hash[0] == key.hash[0] && __jo_3d_model_texture_names[i].hash[1] == key.hash[1])
            return (__jo_3d_model_texture_ids[i]);
    return (-1);
}

/** @brief Keep the name of a texture of the models (it is loaded once) */
static void                                 jo_3d_model_add_texture(const char * const name, const int id)
{
    if (id < 0 || __jo_3d_model_texture_count >= JO_3D_MODEL_MAX_TEXTURES)
        return;
    // (a name longer than 15 characters: kept with its first 15, as before)
    jo_3d_model_texture_hash(name, &__jo_3d_model_texture_names[__jo_3d_model_texture_count]);
    __jo_3d_model_texture_ids[__jo_3d_model_texture_count++] = (short)id;
}

void                                        jo_3d_model_free_textures_from(const int sprite_id)
{
    register int                            i;
    register int                            n;

    // The names of the textures freed are forgotten (the others are packed: the names are in the order of the sprites)
    for (JO_ZERO(i), JO_ZERO(n); i < __jo_3d_model_texture_count; ++i)
    {
        if (__jo_3d_model_texture_ids[i] >= sprite_id)
            continue;
        if (n != i)
        {
            __jo_3d_model_texture_names[n] = __jo_3d_model_texture_names[i];
            __jo_3d_model_texture_ids[n] = __jo_3d_model_texture_ids[i];
        }
        ++n;
    }
    __jo_3d_model_texture_count = n;
    __jo_3d_forget_sprites_from(sprite_id);
    jo_sprite_free_from(sprite_id);
}

/** @brief Format of an embedded texture: RGB 555 */
#define JO_3D_MODEL_TEXTURE_RGB             (0)
/** @brief Format of an embedded texture: 16 colors (the 16 RGB 555 colors of its table, then 2 pixels per byte) */
#define JO_3D_MODEL_TEXTURE_16_COLORS       (1)

/** @brief Load a texture embedded in a JOM file ("TIMG" chunk), if it is not loaded yet
 *  @param entry Header of the texture: name, width, height, format
 *  @param pixels Pixels (RGB 555, 0 is transparent; 16 colors: the table, then the pixels)
 */
static void                                 jo_3d_model_add_embedded_texture(const unsigned char * const entry, const jo_color * const pixels)
{
    jo_img                                  img;

    if (jo_3d_model_find_texture((const char *)entry) >= 0)
        return;
    img.width = *(const unsigned short *)(entry + 16);
    img.height = *(const unsigned short *)(entry + 18);
    if (*(const unsigned short *)(entry + 20) == JO_3D_MODEL_TEXTURE_16_COLORS)
    {
        jo_3d_model_add_texture((const char *)entry, jo_sprite_add_4bits_lut((const unsigned char *)(pixels + 16), img.width, img.height, pixels));
        return;
    }
    img.data = (jo_color *)pixels;
    jo_3d_model_add_texture((const char *)entry, jo_sprite_add(&img));
}

/** @brief Size in bytes of the pixels of an embedded texture (4 bytes aligned) */
static  __jo_force_inline unsigned int      jo_3d_model_texture_size(const unsigned char * const entry)
{
    unsigned int                            pixels;

    pixels = (unsigned int)*(const unsigned short *)(entry + 16) * *(const unsigned short *)(entry + 18);
    if (*(const unsigned short *)(entry + 20) == JO_3D_MODEL_TEXTURE_16_COLORS)
        return ((16 * 2 + pixels / 2 + 3) & ~3);
    return ((pixels * 2 + 3) & ~3);
}

#ifdef JO_COMPILE_WITH_FS_SUPPORT

int                                         jo_3d_model_loaded_texture(const char * const name)
{
    return (jo_3d_model_find_texture(name));
}

int                                         jo_3d_model_texture(const char * const filename)
{
    int                                     length;
    int                                     id;

    if ((id = jo_3d_model_find_texture(filename)) >= 0)
        return (id);
    for (JO_ZERO(length); length < 15 && filename[length] != '\0'; ++length)
        ;
    id = -1;
    if (length > 4 && (filename[length - 3] == 'T' || filename[length - 3] == 't'))
    {
#ifdef JO_COMPILE_WITH_TGA_SUPPORT
        id = jo_sprite_add_tga(JO_NULL, filename, JO_COLOR_Transparent);
#endif
    }
    else
        id = jo_sprite_add_bin(JO_NULL, filename, JO_COLOR_Transparent);
#ifdef JO_DEBUG
    if (id < 0)
        jo_core_error("Texture %s not loaded", filename);
#endif
    jo_3d_model_add_texture(filename, id);
    return (id);
}

/** @brief Load the textures of a "TIMG" chunk from a file (one texture in memory at a time)
 *  @return false if the file is wrong or if out of memory
 */
static bool                                 jo_3d_model_read_textures(jo_file * const file)
{
    unsigned int                            words[6];
    unsigned char                           *entry;
    unsigned int                            count;
    unsigned int                            size;
    jo_color                                *pixels;

    // Aligned: the entry contains 16 and 32 bits values
    entry = (unsigned char *)words;
    if (jo_fs_read_next_bytes(file, (char *)entry, 4) != 4)
        return (false);
    for (count = *(const unsigned int *)entry; count > 0; --count)
    {
        if (jo_fs_read_next_bytes(file, (char *)entry, 24) != 24)
            return (false);
        size = jo_3d_model_texture_size(entry);
        // Already loaded (another model): skipped
        if (jo_3d_model_find_texture((const char *)entry) >= 0)
        {
            if (!jo_fs_seek_forward(file, size))
                return (false);
            continue;
        }
        pixels = (jo_color *)jo_malloc(size);
        if (pixels == JO_NULL)
            return (false);
        if (jo_fs_read_next_bytes(file, (char *)pixels, size) != (int)size)
        {
            jo_free(pixels);
            return (false);
        }
        jo_3d_model_add_embedded_texture(entry, pixels);
        jo_free(pixels);
    }
    return (true);
}

jo_3d_model                                 *jo_3d_load_model(const char * const filename)
{
    jo_file                                 file;
    unsigned char                           head[JO_3D_MODEL_HEADER_SIZE + 8];
    unsigned char                           *data;
    unsigned int                            size;
    jo_3d_model                             *model;
    bool                                    ok;
    bool                                    timg;

    if (!jo_fs_open(&file, filename))
        return (JO_NULL);
    model = JO_NULL;
    data = JO_NULL;
    ok = false;
    // Header and first chunk: the embedded textures are first (they go to the VRAM, the file data are smaller)
    if (jo_fs_read_next_bytes(&file, (char *)head, sizeof(head)) == sizeof(head))
    {
        // The rest of the file, after the header, the chunk header already read and the embedded textures
        size = (unsigned int)(file.size - file.read) + sizeof(head);
        timg = jo_3d_model_is_tag(head + JO_3D_MODEL_HEADER_SIZE, "TIMG");
        if (timg)
            size -= (((unsigned int)head[JO_3D_MODEL_HEADER_SIZE + 4] << 24) | ((unsigned int)head[JO_3D_MODEL_HEADER_SIZE + 5] << 16) |
                     ((unsigned int)head[JO_3D_MODEL_HEADER_SIZE + 6] << 8) | head[JO_3D_MODEL_HEADER_SIZE + 7]) + 8;
        // The data of the model are allocated first: the temporary buffers of the textures are freed above them (no hole
        // in the memory: the meshes loaded next stay in the fast memory)
        if (size > sizeof(head) && size <= (unsigned int)file.size && (data = (unsigned char *)jo_malloc(size)) != JO_NULL)
        {
            ok = !timg || (jo_3d_model_read_textures(&file) &&
                           jo_fs_read_next_bytes(&file, (char *)head + JO_3D_MODEL_HEADER_SIZE, 8) == 8);
            jo_memcpy(data, head, sizeof(head));
            ok = ok && jo_fs_read_next_bytes(&file, (char *)data + sizeof(head), size - sizeof(head)) == (int)(size - sizeof(head));
        }
    }
    // Closed before the model is created: the textures that are not embedded are read from the CD (one file is open at a time)
    jo_fs_close(&file);
    if (ok && data != JO_NULL)
        // The model uses the data of the file (the points and the polygons are not copied)
        model = jo_3d_create_model(data, size, data, JO_NULL);
#ifdef JO_DEBUG
    if (model == JO_NULL)
        jo_core_error("%s: wrong JOM file or out of memory", filename);
#endif
    if (model == JO_NULL && data != JO_NULL)
        jo_free(data);
    return (model);
}

#endif /* JO_COMPILE_WITH_FS_SUPPORT */

/** @brief A gouraud table per polygon (the cells, lit by the point lights, see __jo_3d_load_model_for_lights()) */
static bool                                 __jo_3d_model_unique_gouraud_tables = false;

/** @brief A hash of the 4 colors of a gouraud table */
static  __jo_force_inline unsigned int      jo_3d_model_gouraud_hash(const unsigned short * const t)
{
    return ((((unsigned int)t[0] * 31u + t[1]) * 31u + t[2]) * 31u + t[3]) * 0x9E3779B1u;
}

/** @brief The gouraud tables of the polygons of a model in the VRAM: the polygons with the same 4 colors share one table
 *  (about 1 table in 7 is different)
 *  @param model Model (gouraud_table of an attribute: the index of the polygon + 1 when it uses its table, else 0)
 *  @param gouraud The table of each polygon (4 colors)
 *  @remarks Only the polygons that use their table: their attributes get the table in the VRAM. Out of memory for the
 *           search: a table per polygon, as before
 */
/** @brief The buffers of jo_3d_model_add_gouraud_tables() for the small models (entries of the hash, tables of 4 colors) */
#define JO_3D_MODEL_GOURAUD_HASH            (512)
#define JO_3D_MODEL_GOURAUD_COMPACT         (256)
static unsigned short                       __jo_3d_model_gouraud_hash[JO_3D_MODEL_GOURAUD_HASH];
static unsigned short                       __jo_3d_model_gouraud_compact[JO_3D_MODEL_GOURAUD_COMPACT * 4];

static void                                 jo_3d_model_add_gouraud_tables(jo_3d_model * const model, const unsigned short * const gouraud)
{
    jo_3d_polygon_attribute                 *attributes;
    unsigned short                          *hash;
    unsigned short                          *compact;
    const unsigned short                    *t;
    unsigned int                            n;
    unsigned int                            used;
    unsigned int                            size;
    unsigned int                            slot;
    unsigned int                            count;
    unsigned int                            first;
    unsigned int                            k;
    unsigned short                          base;

    attributes = model->mesh->data.attributes;
    n = model->mesh->data.polygon_count;
    for (JO_ZERO(k), JO_ZERO(used); k < n; ++k)
        if (attributes[k].gouraud_table != 0)
            ++used;
    if (used == 0)
        return;
    hash = JO_NULL;
    if (!__jo_3d_model_unique_gouraud_tables && n < 0xFFFF)
    {
        for (size = 16; size < used * 2; size *= 2)
            ;
        // (a small model: the buffers of the module, no memory block to find; the models are created one at a time)
        hash = size <= JO_3D_MODEL_GOURAUD_HASH ? __jo_3d_model_gouraud_hash :
               (unsigned short *)jo_malloc(size * sizeof(unsigned short));
    }
    if (hash == JO_NULL)
    {
        // A table per polygon (the tables of a room lit by the point lights, see __jo_3d_load_model_for_lights())
        base = jo_3d_add_gouraud_tables((const jo_color *)gouraud, n);
        model->gouraud_tables = base;
        for (JO_ZERO(k); k < n; ++k)
            if (attributes[k].gouraud_table != 0)
                attributes[k].gouraud_table = base != 0 ? (unsigned short)(base + k) : 0;
        if (base == 0)
            for (JO_ZERO(k); k < n; ++k)
                attributes[k].draw_mode &= ~JO_VDP1_COLOR_CALC_GOURAUD;
        return;
    }
    // The first polygon with the same colors: kept in the gouraud table of each polygon (index + 1)
    jo_memset(hash, 0xFF, size * sizeof(unsigned short));
    for (JO_ZERO(k), JO_ZERO(count); k < n; ++k)
    {
        if (attributes[k].gouraud_table == 0)
            continue;
        t = &gouraud[k * 4];
        for (slot = jo_3d_model_gouraud_hash(t) >> 16 & (size - 1);; slot = (slot + 1) & (size - 1))
        {
            if (hash[slot] == 0xFFFF)
            {
                hash[slot] = (unsigned short)k;
                ++count;
                break;
            }
            first = hash[slot];
            if (gouraud[first * 4] == t[0] && gouraud[first * 4 + 1] == t[1] && gouraud[first * 4 + 2] == t[2] &&
                gouraud[first * 4 + 3] == t[3])
                break;
        }
        attributes[k].gouraud_table = (unsigned short)(hash[slot] + 1);
    }
    if (hash != __jo_3d_model_gouraud_hash)
        jo_free(hash);
    compact = ((count + 1) & ~1u) <= JO_3D_MODEL_GOURAUD_COMPACT ? __jo_3d_model_gouraud_compact :
              (unsigned short *)jo_malloc(((count + 1) & ~1u) * 4 * sizeof(unsigned short));
    if (compact == JO_NULL)
    {
        // (the first polygon of each table: its own table)
        base = jo_3d_add_gouraud_tables((const jo_color *)gouraud, n);
        model->gouraud_tables = base;
        for (JO_ZERO(k); k < n; ++k)
            if (attributes[k].gouraud_table != 0)
                attributes[k].gouraud_table = base != 0 ? (unsigned short)(base + attributes[k].gouraud_table - 1) : 0;
        if (base == 0)
            for (JO_ZERO(k); k < n; ++k)
                attributes[k].draw_mode &= ~JO_VDP1_COLOR_CALC_GOURAUD;
        return;
    }
    // The different tables in the order of their first polygon: the index of the table of a polygon (the polygons before
    // it already have theirs, its first polygon too)
    for (JO_ZERO(k), JO_ZERO(count); k < n; ++k)
    {
        if (attributes[k].gouraud_table == 0)
            continue;
        first = attributes[k].gouraud_table - 1u;
        if (first == k)
        {
            jo_memcpy(&compact[count * 4], &gouraud[k * 4], 4 * sizeof(unsigned short));
            attributes[k].gouraud_table = (unsigned short)count++;
        }
        else
            attributes[k].gouraud_table = attributes[first].gouraud_table;
    }
    // (an odd count: the sprite of the tables has 2 tables per line)
    if (count & 1)
        jo_memset(&compact[count * 4], 0, 4 * sizeof(unsigned short));
    base = jo_3d_add_gouraud_tables((const jo_color *)compact, count);
    if (compact != __jo_3d_model_gouraud_compact)
        jo_free(compact);
    model->gouraud_tables = base;
    for (JO_ZERO(k); k < n; ++k)
    {
        if (!(attributes[k].draw_mode & JO_VDP1_COLOR_CALC_GOURAUD))
            continue;
        if (base == 0)
        {
            attributes[k].draw_mode &= ~JO_VDP1_COLOR_CALC_GOURAUD;
            attributes[k].gouraud_table = 0;
        }
        else
            attributes[k].gouraud_table = (unsigned short)(base + attributes[k].gouraud_table);
    }
}

void                                        jo_3d_set_unique_gouraud_tables(const bool unique)
{
    __jo_3d_model_unique_gouraud_tables = unique;
}

#ifdef JO_COMPILE_WITH_FS_SUPPORT

jo_3d_model                                 *__jo_3d_load_model_for_lights(const char * const filename)
{
    jo_3d_model                             *model;

    __jo_3d_model_unique_gouraud_tables = true;
    model = jo_3d_load_model(filename);
    __jo_3d_model_unique_gouraud_tables = false;
    return (model);
}

#endif /* JO_COMPILE_WITH_FS_SUPPORT */

/** @brief Set the attributes of the polygons from their materials */
static void                                 jo_3d_model_set_attributes(jo_3d_model * const model, const unsigned short * const gouraud)
{
    register unsigned int                   k;
    const jo_3d_material                    *m;
    jo_3d_polygon_attribute                 *a;
    jo_3d_mesh                              *mesh;
    int                                     texture;

    mesh = model->mesh;
    for (JO_ZERO(k); k < mesh->data.polygon_count; ++k)
    {
        m = model->polygon_materials != JO_NULL && model->polygon_materials[k] < model->material_count ?
            &model->materials[model->polygon_materials[k]] : JO_NULL;
        texture = m != JO_NULL && m->texture >= 0 && (unsigned int)m->texture < model->texture_count ? model->textures[m->texture] : -1;
        if (m != JO_NULL && (m->flags & JO_3D_MATERIAL_LIGHT))
            jo_3d_set_mesh_polygon_light(mesh, true, k);
        if (texture >= 0)
            jo_3d_set_mesh_polygon_texture(mesh, texture, k);
        else
            jo_3d_set_mesh_polygon_color(mesh, m != JO_NULL ? JO_COLOR_RGB(m->r, m->g, m->b) : JO_COLOR_White, k);
        if (m == JO_NULL)
            continue;
        a = &mesh->data.attributes[k];
        a->plane = (m->flags & JO_3D_MATERIAL_DUAL_PLANE) ? JO_3D_DUAL_PLANE : JO_3D_SINGLE_PLANE;
        // (both: the depth of the previous polygon, a decal on the face drawn just before it)
        if ((m->flags & JO_3D_MATERIAL_SORT_PREVIOUS) == JO_3D_MATERIAL_SORT_PREVIOUS)
            a->sort = (a->sort & ~JO_3D_SORT_MASK) | JO_3D_SORT_PREVIOUS;
        else if (m->flags & JO_3D_MATERIAL_SORT_MIN)
            a->sort = (a->sort & ~JO_3D_SORT_MASK) | JO_3D_SORT_MIN;
        else if (m->flags & JO_3D_MATERIAL_SORT_MAX)
            a->sort = (a->sort & ~JO_3D_SORT_MASK) | JO_3D_SORT_MAX;
        if (m->flags & JO_3D_MATERIAL_HALF_TRANSPARENT)
            jo_3d_set_mesh_polygon_transparency(mesh, true, k);
        if (m->flags & JO_3D_MATERIAL_SCREEN_DOORS)
            jo_3d_set_mesh_polygon_screen_doors(mesh, true, k);
        if (texture >= 0 && (m->flags & JO_3D_MATERIAL_FLIP_H))
            a->direction |= JO_VDP1_H_FLIP;
        if (texture >= 0 && (m->flags & JO_3D_MATERIAL_FLIP_V))
            a->direction |= JO_VDP1_H_FLIP << 1;
        // (the table: see jo_3d_model_add_gouraud_tables(), the index of the polygon + 1 until then)
        if ((m->flags & JO_3D_MATERIAL_GOURAUD) && gouraud != JO_NULL)
        {
            a->draw_mode |= JO_VDP1_COLOR_CALC_GOURAUD;
            a->gouraud_table = (unsigned short)(k + 1);
        }
    }
    if (gouraud != JO_NULL)
        jo_3d_model_add_gouraud_tables(model, gouraud);
}

/** @brief Apply the texture options of the materials (once per texture) */
static void                                 jo_3d_model_set_texture_options(const jo_3d_model * const model)
{
    register unsigned int                   k;
    const jo_3d_material                    *m;
    int                                     texture;

    for (JO_ZERO(k), m = model->materials; k < model->material_count; ++k, ++m)
    {
        if (m->texture < 0 || (unsigned int)m->texture >= model->texture_count || (texture = model->textures[m->texture]) < 0)
            continue;
        // The end codes first: the smaller copies are made from the texture
        if (m->flags & JO_3D_MATERIAL_END_CODES)
            jo_sprite_add_end_codes(texture);
        if (m->flags & JO_3D_MATERIAL_SUBDIVISION_NEAR)
            jo_3d_set_texture_subdivision_near_only(texture);
        else if (m->flags & JO_3D_MATERIAL_SUBDIVISION)
            jo_3d_set_texture_subdivision(texture);
        if (m->subdivision_error != 0)
            jo_3d_set_texture_subdivision_error(texture, m->subdivision_error);
        if (m->flags & JO_3D_MATERIAL_MIPMAPS)
            jo_3d_set_texture_mipmaps(texture);
    }
}

/** @brief Create a model from the data of a JOM file
 *  @param file The data are kept by the model (freed with it), JO_NULL: the data are copied
 */
/*
** Skeleton
*/

/** @brief Check the skeleton (the groups of the bones in the model, a parent before its children), the children lists */
static bool                                 jo_3d_model_create_skeleton(jo_3d_model * const model)
{
    register unsigned int                   k;
    register int                            j;
    const jo_3d_model_group                 *g;
    signed char                             *last;

    if (model->groups == JO_NULL || model->bones == JO_NULL)
        return (false);
    model->bone_children = (signed char *)jo_malloc(model->bone_count * 3);
    if (model->bone_children == JO_NULL)
        return (false);
    jo_memset(model->bone_children, -1, model->bone_count * 3);
    last = model->bone_children + model->bone_count * 2;
    for (JO_ZERO(k); k < model->bone_count; ++k)
    {
        j = model->bones[k].parent;
        if (j >= (int)k)
            return (false);
        if (j >= 0)
        {
            // Children in their order: after the last child of the parent
            if (last[j] < 0)
                model->bone_children[j] = (signed char)k;
            else
                model->bone_children[model->bone_count + last[j]] = (signed char)k;
            last[j] = (signed char)k;
        }
        if (model->bones[k].group == JO_3D_BONE_NO_GROUP)
            continue;
        if (model->bones[k].group >= model->group_count)
            return (false);
        g = &model->groups[model->bones[k].group];
        if (g->first_point + g->point_count > model->mesh->data.point_count ||
            g->first_polygon + g->polygon_count > model->mesh->data.polygon_count)
            return (false);
    }
    return (true);
}

int                                         jo_3d_model_find_bone(const jo_3d_model * const model, const char * const name)
{
    register unsigned int                   k;

    for (JO_ZERO(k); k < model->bone_count; ++k)
        if (jo_3d_model_same_name(model->bones[k].name, name))
            return ((int)k);
    return (-1);
}

int                                         jo_3d_model_find_animation(const jo_3d_model * const model, const char * const name)
{
    register unsigned int                   k;

    for (JO_ZERO(k); k < model->animation_count; ++k)
        if (jo_3d_model_same_name(model->animations[k].name, name))
            return ((int)k);
    return (-1);
}

bool                                        jo_3d_model_is_animation(const jo_3d_model * const model, const int animation, const char * const name)
{
    // (the first one with this name: an animation after it with the same name is marked at the loading, its name was compared
    // with all the names before it at each call: several calls per character and per frame)
    return (animation >= 0 && animation < (int)model->animation_count && !model->animations[animation].duplicate &&
            jo_3d_model_same_name(model->animations[animation].name, name));
}

int                                         jo_3d_model_animation_duration(const jo_3d_model * const model, const int animation)
{
    const jo_3d_animation                   *a;

    if (animation < 0 || animation >= (int)model->animation_count)
        return (0);
    a = &model->animations[animation];
    return ((a->flags & JO_3D_ANIMATION_LOOP ? a->key_count : a->key_count - 1) * a->key_ticks);
}

void                                        jo_3d_model_get_pose(const jo_3d_model * const model, const int animation, int time,
                                                                 jo_3d_pose * const pose)
{
    const jo_3d_animation                   *a;
    const unsigned char                     *k0;
    const unsigned char                     *k1;
    const jo_angle                          *a0;
    const jo_angle                          *a1;
    unsigned int                            key_size;
    int                                     key;
    int                                     next;
    int                                     frac;
    register unsigned int                   b;
    register int                            j;

    if (animation < 0 || animation >= (int)model->animation_count)
    {
        jo_memset(pose, 0, sizeof(*pose));
        return;
    }
    a = &model->animations[animation];
    // A key: the root (3 fixed), the angles of the bones (3 x s16 each), aligned on 4 bytes
    key_size = (12 + model->bone_count * 6 + 3) & ~3;
    if (time < 0)
        time = 0;
    if (a->flags & JO_3D_ANIMATION_LOOP)
    {
        // (the divisions by the division unit of the CPU, see jo_int_div(): several poses per character and per frame)
        time -= jo_int_div(time, a->key_count * a->key_ticks) * (a->key_count * a->key_ticks);
        // (one division: (time x 256) / ticks = key x 256 + frac, 0 <= frac < 256. Positive: a logical shift, not the
        // routine of the library of the compiler)
        frac = jo_int_div(time << 8, a->key_ticks);
        key = (int)((unsigned int)frac >> 8);
        frac &= 0xFF;
        next = key + 1 < a->key_count ? key + 1 : 0;
    }
    else if (time >= (a->key_count - 1) * a->key_ticks)
    {
        key = next = a->key_count - 1;
        frac = 0;
    }
    else
    {
        frac = jo_int_div(time << 8, a->key_ticks);
        key = (int)((unsigned int)frac >> 8);
        frac &= 0xFF;
        next = key + 1;
    }
    k0 = model->animation_data + a->offset + key * key_size;
    k1 = model->animation_data + a->offset + next * key_size;
    // (the shifts of signed values without the routine of the library, see jo_shift_right8())
    pose->root.x = ((const jo_fixed *)k0)[0] + jo_shift_right8(((const jo_fixed *)k1)[0] - ((const jo_fixed *)k0)[0]) * frac;
    pose->root.y = ((const jo_fixed *)k0)[1] + jo_shift_right8(((const jo_fixed *)k1)[1] - ((const jo_fixed *)k0)[1]) * frac;
    pose->root.z = ((const jo_fixed *)k0)[2] + jo_shift_right8(((const jo_fixed *)k1)[2] - ((const jo_fixed *)k0)[2]) * frac;
    a0 = (const jo_angle *)(k0 + 12);
    a1 = (const jo_angle *)(k1 + 12);
    for (JO_ZERO(b); b < model->bone_count; ++b, a0 += 3, a1 += 3)
        for (JO_ZERO(j); j < 3; ++j)
            // The shortest way (the difference of 2 angles is an angle; x frac (0 to 255): under 2^23)
            pose->angles[b][j] = (jo_angle)(a0[j] + jo_shift_right8_small(((jo_angle)(a1[j] - a0[j])) * frac));
}

void                                        jo_3d_pose_blend(const jo_3d_model * const model, jo_3d_pose * const pose,
                                                             const jo_3d_pose * const other, const int weight)
{
    register unsigned int                   b;
    register int                            j;

    // (the shifts of signed values without the routine of the library, see jo_shift_right8(): weight is any value)
    pose->root.x += jo_shift_right8(other->root.x - pose->root.x) * weight;
    pose->root.y += jo_shift_right8(other->root.y - pose->root.y) * weight;
    pose->root.z += jo_shift_right8(other->root.z - pose->root.z) * weight;
    for (JO_ZERO(b); b < model->bone_count; ++b)
        for (JO_ZERO(j); j < 3; ++j)
            pose->angles[b][j] = (jo_angle)(pose->angles[b][j] + jo_shift_right8(((jo_angle)(other->angles[b][j] - pose->angles[b][j])) * weight));
}

/** @brief A matrix: 4 rows of 3 (see jo_3d_get_matrix()) */
typedef jo_fixed                            jo_3d_matrix_rows[4][3];

/** @brief The matrices of the bones of a pose (see jo_3d_model_draw_pose()) */
typedef struct
{
    jo_3d_matrix_rows                       matrices[JO_3D_MAX_BONES];
    jo_3d_part                              parts[JO_3D_MAX_BONES];
    int                                     part_count;
}                                           jo_3d_model_pose_matrices;

/** @brief The bones not drawn by jo_3d_model_draw_pose() (bits, see jo_3d_model_set_hidden_bones()) */
static unsigned int                         __jo_3d_hidden_bones = 0;

void                                        jo_3d_model_set_hidden_bones(const unsigned int mask)
{
    __jo_3d_hidden_bones = mask;
}

/** @brief The matrix of a bone and of its children
 *  @param parent The matrix of its parent (computed here into the matrix of the bone, or a copy on the stack: not with the
 *  stack of the matrices, the copies and the calls cost more than the products of a joint)
 */
static void                                 jo_3d_model_pose_bone(const jo_3d_model * const model, const int bone,
                                                                  const jo_3d_pose * const pose, const jo_3d_bone_callback callback,
                                                                  void * const param, jo_3d_model_pose_matrices * const m,
                                                                  const jo_fixed (* const parent)[3])
{
    static const jo_angle                   no_angles[3] = {0, 0, 0};
    const jo_3d_bone                        *b;
    const jo_3d_model_group                 *g;
    jo_3d_part                              *part;
    jo_fixed                                (*matrix)[3];
    jo_3d_matrix_rows                       local;
    bool                                    hidden;
    int                                     child;

    b = &model->bones[bone];
    // (the mask first: usually 0. The shift by a variable count is a routine of the library of the compiler)
    hidden = __jo_3d_hidden_bones != 0 && b->group != JO_3D_BONE_NO_GROUP && bone < 32 && (__jo_3d_hidden_bones & (1u << bone));
    // (a bone with its points, not hidden: its matrix is the one of its part)
    matrix = b->group != JO_3D_BONE_NO_GROUP && !hidden ? m->matrices[bone] : local;
    __jo_3d_bone_matrix(matrix, parent, &b->pivot, pose != JO_NULL ? pose->angles[bone] : no_angles);
    if (b->group != JO_3D_BONE_NO_GROUP)
    {
        g = &model->groups[b->group];
        if (hidden)
        {
            // (hidden: all its points behind the camera (the view space), its polygons are not drawn)
            jo_memset(m->matrices[bone], 0, sizeof(m->matrices[bone]));
            m->matrices[bone][3][2] = jo_int2fixed(-1000);
        }
        part = &m->parts[m->part_count++];
        part->matrix = m->matrices[bone];
        part->first = g->first_point;
        part->count = g->point_count;
    }
    if (callback != JO_NULL)
        callback(bone, param, (const jo_fixed (*)[3])matrix);
    for (child = model->bone_children[bone]; child >= 0; child = model->bone_children[model->bone_count + child])
        jo_3d_model_pose_bone(model, child, pose, callback, param, m, (const jo_fixed (*)[3])matrix);
}

void                                        jo_3d_model_draw_pose(const jo_3d_model * const model, const jo_3d_pose * const pose,
                                                                  const jo_3d_bone_callback callback, void * const param)
{
    jo_3d_model_pose_matrices               m;
    register unsigned int                   k;

    // The matrices of the bones, then one draw: the points of each bone are transformed by its matrix (a draw per bone
    // would cost about 0.1 ms each)
    m.part_count = 0;
    jo_3d_push_matrix();
    {
        if (pose != JO_NULL && (pose->root.x | pose->root.y | pose->root.z) != 0)
            jo_3d_translate_matrix_fixed(pose->root.x, pose->root.y, pose->root.z);
        for (JO_ZERO(k); k < model->bone_count; ++k)
            if (model->bones[k].parent < 0)
                jo_3d_model_pose_bone(model, (int)k, pose, callback, param, &m, (const jo_fixed (*)[3])__jo_3d_current_matrix());
    }
    jo_3d_pop_matrix();
    if (m.part_count > 0)
        __jo_3d_draw_parts(&model->mesh->data, m.parts, m.part_count);
}

/*
** Loading
*/

/** @brief The parts of the memory block of a model created in place (aligned on 4 bytes): the model, its mesh, the
 *  attributes of its polygons, the ids of its textures */
#define JO_3D_MODEL_BLOCK_MESH              ((sizeof(jo_3d_model) + 3) & ~3u)
#define JO_3D_MODEL_BLOCK_ATTRIBUTES        (JO_3D_MODEL_BLOCK_MESH + ((sizeof(jo_3d_mesh) + 3) & ~3u))
#define JO_3D_MODEL_BLOCK_TEXTURES(polygons) (JO_3D_MODEL_BLOCK_ATTRIBUTES + (((polygons) * sizeof(jo_3d_polygon_attribute) + 3) & ~3u))

/** @brief Create a model
 *  @param file The data of the file used in place (JO_NULL: copies)
 *  @param storage One memory block for the model, its mesh, the attributes and the texture ids (see
 *         jo_3d_create_model_in_place()), JO_NULL: a block each
 */
static jo_3d_model                          *jo_3d_create_model(const unsigned char * const data, const unsigned int size, void * const file,
                                                                unsigned char * const storage)
{
    jo_3d_model                             *model;
    const unsigned char                     *p;
    const unsigned char                     *end;
    const unsigned char                     *points;
    const unsigned char                     *polygons;
    const unsigned short                    *gouraud;
    unsigned int                            point_count;
    unsigned int                            polygon_count;
    unsigned int                            chunk;
    unsigned int                            count;
    unsigned int                            storage_polygons;
    register unsigned int                   k;

    if (data == JO_NULL || size < JO_3D_MODEL_HEADER_SIZE || !jo_3d_model_is_tag(data, "JOM1") ||
        (data[4] << 8 | data[5]) > JO_3D_MODEL_VERSION)
    {
#ifdef JO_DEBUG
        jo_core_error("Not a JOM file (version %d at most)", JO_3D_MODEL_VERSION);
#endif
        return (JO_NULL);
    }
    // (the polygons of the block: the texture ids are after their attributes)
    JO_ZERO(storage_polygons);
    if (storage != JO_NULL)
        for (p = data + JO_3D_MODEL_HEADER_SIZE; p + 12 <= data + size && !jo_3d_model_is_tag(p, "END "); p += 8 + chunk)
        {
            chunk = *(const unsigned int *)(p + 4);
            if (jo_3d_model_is_tag(p, "QUAD"))
                storage_polygons = *(const unsigned int *)(p + 8);
        }
    model = storage != JO_NULL ? (jo_3d_model *)storage : (jo_3d_model *)jo_malloc(sizeof(*model));
    if (model == JO_NULL)
        return (JO_NULL);
    jo_memset(model, 0, sizeof(*model));
    model->file = file;
    model->single_block = storage != JO_NULL;
    // The data are aligned: the values are read directly (big endian, like the SH-2)
    model->center.x = *(const jo_fixed *)(data + 8);
    model->center.y = *(const jo_fixed *)(data + 12);
    model->center.z = *(const jo_fixed *)(data + 16);
    model->radius = *(const jo_fixed *)(data + 20);
    points = JO_NULL;
    polygons = JO_NULL;
    gouraud = JO_NULL;
    JO_ZERO(point_count);
    JO_ZERO(polygon_count);
    end = data + size;
    for (p = data + JO_3D_MODEL_HEADER_SIZE; p + 8 <= end && !jo_3d_model_is_tag(p, "END "); p += 8 + chunk)
    {
        chunk = *(const unsigned int *)(p + 4);
        if (p + 8 + chunk > end)
            break;
        count = chunk >= 4 ? *(const unsigned int *)(p + 8) : 0;
        if (jo_3d_model_is_tag(p, "PNTS"))
        {
            point_count = count;
            points = p + 12;
        }
        else if (jo_3d_model_is_tag(p, "QUAD"))
        {
            polygon_count = count;
            polygons = p + 12;
        }
        else if (jo_3d_model_is_tag(p, "QMAT"))
            model->polygon_materials = (unsigned short *)jo_3d_model_copy(model, p + 12, count * sizeof(unsigned short));
        else if (jo_3d_model_is_tag(p, "MATL"))
        {
            model->material_count = count;
            model->materials = (jo_3d_material *)jo_3d_model_copy(model, p + 12, count * sizeof(jo_3d_material));
        }
        else if (jo_3d_model_is_tag(p, "TIMG"))
        {
            const unsigned char             *entry;

            for (JO_ZERO(k), entry = p + 12; k < count && entry + 24 <= p + 8 + chunk; ++k)
            {
                jo_3d_model_add_embedded_texture(entry, (const jo_color *)(entry + 24));
                entry += 24 + jo_3d_model_texture_size(entry);
            }
        }
        else if (jo_3d_model_is_tag(p, "TEXT"))
        {
            model->texture_count = count;
            model->textures = storage != JO_NULL ? (int *)(storage + JO_3D_MODEL_BLOCK_TEXTURES(storage_polygons)) :
                              (int *)jo_malloc((count > 0 ? count : 1) * sizeof(int));
            for (JO_ZERO(k); model->textures != JO_NULL && k < count; ++k)
            {
#ifdef JO_COMPILE_WITH_FS_SUPPORT
                model->textures[k] = jo_3d_model_texture((const char *)(p + 12 + k * 16));
#else
                // Only the embedded textures ("TIMG" chunk)
                model->textures[k] = jo_3d_model_find_texture((const char *)(p + 12 + k * 16));
#endif
            }
        }
        else if (jo_3d_model_is_tag(p, "NTAB"))
        {
            model->normal_count = count;
            model->normals = (jo_pos3D_fixed *)jo_3d_model_copy(model, p + 12, count * sizeof(jo_pos3D_fixed));
        }
        else if (jo_3d_model_is_tag(p, "CNRM"))
            model->corner_normals = (unsigned short *)jo_3d_model_copy(model, p + 12, count * sizeof(unsigned short));
        else if (jo_3d_model_is_tag(p, "VCOL"))
            model->vertex_colors = (jo_pos3D_fixed *)jo_3d_model_copy(model, p + 12, count * sizeof(jo_pos3D_fixed));
        else if (jo_3d_model_is_tag(p, "GOUR"))
            gouraud = (const unsigned short *)(p + 12);
        else if (jo_3d_model_is_tag(p, "GRUP"))
        {
            model->group_count = count;
            model->groups = (jo_3d_model_group *)jo_3d_model_copy(model, p + 12, count * sizeof(jo_3d_model_group));
        }
        else if (jo_3d_model_is_tag(p, "BONE") && count <= JO_3D_MAX_BONES)
        {
            model->bone_count = count;
            model->bones = (jo_3d_bone *)jo_3d_model_copy(model, p + 12, count * sizeof(jo_3d_bone));
        }
        else if (jo_3d_model_is_tag(p, "ANIM"))
        {
            // (the keys are after the headers: the whole chunk is kept)
            model->animation_data = (const unsigned char *)jo_3d_model_copy(model, p + 8, chunk);
            if (model->animation_data != JO_NULL)
            {
                model->animation_count = count;
                model->animations = (jo_3d_animation *)(model->animation_data + 4);
                jo_3d_model_mark_duplicates(model);
            }
        }
    }
    if (points == JO_NULL || polygons == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("JOM file without points or polygons");
#endif
        jo_3d_model_free(model, false);
        return (JO_NULL);
    }
    if (file != JO_NULL)
    {
        // The points and the polygons in the data of the file (the same layout as jo_pos3D_fixed and jo_3d_polygon)
        model->mesh = storage != JO_NULL ? (jo_3d_mesh *)(storage + JO_3D_MODEL_BLOCK_MESH) : (jo_3d_mesh *)jo_malloc(sizeof(jo_3d_mesh));
        if (model->mesh != JO_NULL)
        {
            jo_memset(model->mesh, 0, sizeof(jo_3d_mesh));
            model->mesh->data.point_count = point_count;
            model->mesh->data.points = (jo_pos3D_fixed *)points;
            model->mesh->data.polygon_count = polygon_count;
            model->mesh->data.polygons = (jo_3d_polygon *)polygons;
            model->mesh->data.attributes = storage != JO_NULL ? (jo_3d_polygon_attribute *)(storage + JO_3D_MODEL_BLOCK_ATTRIBUTES) :
                                           (jo_3d_polygon_attribute *)jo_malloc(polygon_count * sizeof(jo_3d_polygon_attribute));
            if (model->mesh->data.attributes != JO_NULL)
                jo_memset(model->mesh->data.attributes, 0, polygon_count * sizeof(jo_3d_polygon_attribute));
        }
    }
    else if ((model->mesh = jo_3d_create_indexed_mesh(point_count, polygon_count)) != JO_NULL)
    {
        jo_memcpy(model->mesh->data.points, points, point_count * sizeof(jo_pos3D_fixed));
        jo_memcpy(model->mesh->data.polygons, polygons, polygon_count * sizeof(jo_3d_polygon));
    }
    if (model->mesh == JO_NULL || model->mesh->data.attributes == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Out of memory");
#endif
        jo_3d_model_free(model, false);
        return (JO_NULL);
    }
    jo_3d_model_set_texture_options(model);
    jo_3d_model_set_attributes(model, gouraud);
    if (model->bone_count > 0 && !jo_3d_model_create_skeleton(model))
    {
#ifdef JO_DEBUG
        jo_core_error("Wrong skeleton or out of memory");
#endif
        jo_3d_model_free(model, false);
        return (JO_NULL);
    }
    return (model);
}

jo_3d_model                                 *jo_3d_create_model_from_memory(const unsigned char * const data, const unsigned int size)
{
    return (jo_3d_create_model(data, size, JO_NULL, JO_NULL));
}

jo_3d_model                                 *jo_3d_create_model_in_place(const unsigned char * const data, const unsigned int size)
{
    jo_3d_model                             *model;
    const unsigned char                     *p;
    unsigned int                            chunk;
    unsigned int                            polygon_count;
    unsigned int                            texture_count;
    unsigned char                           *storage;

    // The counts of the chunks first: the model, its mesh, the attributes of its polygons and the ids of its textures in
    // one memory block (a level of 1000 models: 4000 blocks less, each one found after a search among the others)
    JO_ZERO(polygon_count);
    JO_ZERO(texture_count);
    // (a wrong file: jo_3d_create_model() says so, before the block is allocated)
    if (data == JO_NULL || size < JO_3D_MODEL_HEADER_SIZE || !jo_3d_model_is_tag(data, "JOM1") ||
        (data[4] << 8 | data[5]) > JO_3D_MODEL_VERSION)
        return (jo_3d_create_model(data, size, (void *)data, JO_NULL));
    for (p = data + JO_3D_MODEL_HEADER_SIZE; p + 12 <= data + size && !jo_3d_model_is_tag(p, "END "); p += 8 + chunk)
    {
        chunk = *(const unsigned int *)(p + 4);
        if (jo_3d_model_is_tag(p, "QUAD"))
            polygon_count = *(const unsigned int *)(p + 8);
        else if (jo_3d_model_is_tag(p, "TEXT"))
            texture_count = *(const unsigned int *)(p + 8);
    }
    storage = (unsigned char *)jo_malloc(JO_3D_MODEL_BLOCK_TEXTURES(polygon_count) +
                                         (texture_count > 0 ? texture_count : 1) * sizeof(int));
    if (storage == JO_NULL)
        return (JO_NULL);
    model = jo_3d_create_model(data, size, (void *)data, storage);
    if (model != JO_NULL)
        model->file_not_owned = true;
    return (model);
}

void                                        jo_3d_free_model(jo_3d_model * const model)
{
    jo_3d_model_free(model, true);
}

/** @brief Free a model
 *  @param free_file Free the data of the file too (else the caller frees them)
 */
static void                                 jo_3d_model_free(jo_3d_model * const model, const bool free_file)
{
    if (model == JO_NULL)
        return;
    if (model->single_block)
    {
        // The model, its mesh, the attributes, the texture ids: one block (see jo_3d_create_model_in_place())
        if (model->bone_children != JO_NULL)
            jo_free(model->bone_children);
        if (free_file && model->file != JO_NULL && !model->file_not_owned)
            jo_free(model->file);
        jo_free(model);
        return;
    }
    if (model->file != JO_NULL)
    {
        // The data are in the file (see jo_3d_load_model())
        if (model->mesh != JO_NULL)
        {
            if (model->mesh->data.attributes != JO_NULL)
                jo_free(model->mesh->data.attributes);
            jo_free(model->mesh);
        }
        if (model->textures != JO_NULL)
            jo_free(model->textures);
        if (model->bone_children != JO_NULL)
            jo_free(model->bone_children);
        if (free_file && !model->file_not_owned)
            jo_free(model->file);
        jo_free(model);
        return;
    }
    if (model->mesh != JO_NULL)
        jo_3d_free_mesh(model->mesh);
    if (model->materials != JO_NULL)
        jo_free(model->materials);
    if (model->polygon_materials != JO_NULL)
        jo_free(model->polygon_materials);
    if (model->textures != JO_NULL)
        jo_free(model->textures);
    if (model->normals != JO_NULL)
        jo_free(model->normals);
    if (model->corner_normals != JO_NULL)
        jo_free(model->corner_normals);
    if (model->vertex_colors != JO_NULL)
        jo_free(model->vertex_colors);
    if (model->groups != JO_NULL)
        jo_free(model->groups);
    if (model->bones != JO_NULL)
        jo_free(model->bones);
    if (model->bone_children != JO_NULL)
        jo_free(model->bone_children);
    if (model->animation_data != JO_NULL)
        jo_free((void *)model->animation_data);
    jo_free(model);
}

const jo_3d_model_group                     *jo_3d_model_find_group(const jo_3d_model * const model, const char * const name)
{
    register unsigned int                   k;

    for (JO_ZERO(k); k < model->group_count; ++k)
        if (jo_3d_model_same_name(model->groups[k].name, name))
            return (&model->groups[k]);
    return (JO_NULL);
}

#endif /* JO_COMPILE_WITH_3D_SUPPORT */

/*
** END OF FILE
*/
