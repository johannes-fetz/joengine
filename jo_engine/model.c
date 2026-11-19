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
#include "internal/3d.h"
#include "internal/model.h"

/** @brief Size of the header of a JOM file */
# define JO_3D_MODEL_HEADER_SIZE                (24)
/** @brief Textures of the models loaded once (see jo_3d_model_texture()): a big model has parts of its texture
 *  (a mesh cut in a grid of textures). When full, the next textures were not kept: the
 *  models loaded later did not find them */
# define JO_3D_MODEL_MAX_TEXTURES               (JO_MAX_SPRITE)

/** @brief Textures of the models kept in the program (the next ones: in a bigger table allocated, see
 *  jo_3d_model_add_texture()): a quarter of the sprites, most of them are not textures of models (the textures of the
 *  program, the smaller copies, the tiles), 282 of 1600 in a 3D game */
#ifndef JO_3D_MODEL_KEPT_TEXTURES
# define JO_3D_MODEL_KEPT_TEXTURES              (JO_3D_MODEL_MAX_TEXTURES / 4 < 32 ? 32 : JO_3D_MODEL_MAX_TEXTURES / 4)
#endif

/** @brief Textures of the models (the names as 2 hashes and the sprite ids): 10 bytes a texture instead of 20 (the names of
 *  16 characters: 8 KB with 400 sprites) */
typedef struct
{
    unsigned int                            hash[2];
}                                           jo_3d_model_texture_name;
static jo_3d_model_texture_name             __jo_3d_model_texture_kept_names[JO_3D_MODEL_KEPT_TEXTURES];
static short                                __jo_3d_model_texture_kept_ids[JO_3D_MODEL_KEPT_TEXTURES];
/* (the tables used: the ones kept in the program, or a bigger one allocated: the names, then the ids) */
static jo_3d_model_texture_name             *__jo_3d_model_texture_names = __jo_3d_model_texture_kept_names;
static short                                *__jo_3d_model_texture_ids = __jo_3d_model_texture_kept_ids;
static int                                  __jo_3d_model_texture_capacity = JO_3D_MODEL_KEPT_TEXTURES;
static int                                  __jo_3d_model_texture_count = 0;

JO_LOW_WORK_RAM_CODE
static jo_3d_model                          *jo_3d_create_model(const unsigned char * const data, const unsigned int size, void * const file,
                                                                unsigned char * const storage);
static void                                 jo_3d_model_free(jo_3d_model * const model, const bool free_file);
static void                                 jo_3d_model_update_morph_draw(const jo_3d_model * const model);

/** @brief Compare 4 bytes with a chunk tag */
static  __jo_force_inline bool              jo_3d_model_is_tag(const unsigned char * const p, const char * const tag)
{
    return (p[0] == (unsigned char)tag[0] && p[1] == (unsigned char)tag[1] && p[2] == (unsigned char)tag[2] && p[3] == (unsigned char)tag[3]);
}

/** @brief A chunk not aligned or after the end of the data (a size of 0xFFFFFFF8 wrapped the pointer, the walk didn't move) */
# define JO_3D_MODEL_WRONG_CHUNK(CHUNK, P, END)     (((CHUNK) & 3) || (CHUNK) > (unsigned int)((END) - (P)) - 8)

/** @brief The count of the elements of a chunk fits in it
 *  @param p The chunk (tag, size, count, elements)
 *  @param chunk Size of the chunk (after its tag and its size)
 *  @param count Count of the elements
 *  @return false if the elements would be read after the chunk
 */
static bool                                 jo_3d_model_count_fits(const unsigned char * const p, const unsigned int chunk, const unsigned int count)
{
    unsigned int                            element;

    if (jo_3d_model_is_tag(p, "PNTS") || jo_3d_model_is_tag(p, "NTAB") || jo_3d_model_is_tag(p, "VCOL"))
        element = sizeof(jo_pos3D_fixed);
    else if (jo_3d_model_is_tag(p, "QUAD"))
        element = sizeof(jo_3d_polygon);
    else if (jo_3d_model_is_tag(p, "QMAT") || jo_3d_model_is_tag(p, "CNRM"))
        element = sizeof(unsigned short);
    else if (jo_3d_model_is_tag(p, "MATL"))
        element = sizeof(jo_3d_material);
    else if (jo_3d_model_is_tag(p, "TEXT"))
        element = 16;
    else if (jo_3d_model_is_tag(p, "GRUP"))
        element = sizeof(jo_3d_model_group);
    else if (jo_3d_model_is_tag(p, "BONE"))
        element = sizeof(jo_3d_bone);
    else if (jo_3d_model_is_tag(p, "ANIM"))
        element = sizeof(jo_3d_animation);
    else if (jo_3d_model_is_tag(p, "SKIN"))
        element = sizeof(jo_3d_skin_point);
    else if (jo_3d_model_is_tag(p, "MRPH"))
        // (the count, the shift and the flags, then the headers)
        return (chunk >= 8 && count <= (chunk - 8) / sizeof(jo_3d_morph));
    else
        return (true);
    return (chunk >= 4 && count <= (chunk - 4) / element);
}

/** @brief The keys of the animations of a model are in their chunk, at least 1 key and a time between 2 keys
 *  @param model Model (its bones and its animations loaded)
 *  @param size Size of the chunk of the animations (from its count)
 *  @return false if an animation is wrong
 */
static bool                                 jo_3d_model_animations_fit(const jo_3d_model * const model, const unsigned int size)
{
    const jo_3d_animation                   *a;
    unsigned int                            key_size;
    register unsigned int                   i;

    key_size = (12 + model->bone_count * 6 + 3) & ~3;
    for (JO_ZERO(i); i < model->animation_count; ++i)
    {
        a = &model->animations[i];
        // (the keys aligned on 4 bytes: their root is read as 32 bits values, an odd offset was an address error of the CPU)
        if (a->key_count == 0 || a->key_ticks == 0 || (a->offset & 3) || a->offset > size || a->key_count > (size - a->offset) / key_size)
        {
#ifdef JO_DEBUG
            jo_core_error("Wrong animation %d", (int)i);
#endif
            return (false);
        }
    }
    return (true);
}

/** @brief The frames of the vertex animations of a model are in their chunk, at least 1 frame and a time between 2 frames
 *  @param model Model (its points, its polygons and its vertex animations loaded)
 *  @param size Size of the chunk of the vertex animations (from its count)
 *  @return false if a vertex animation is wrong
 *  @remarks Optional: a chunk "MRPH" of another layout (the own data of a game, read with jo_3d_model_find_chunk()) is
 *           ignored, without error
 */
JO_LOW_WORK_RAM_CODE
static bool                                 jo_3d_model_morphs_fit(const jo_3d_model * const model, const unsigned int size)
{
    const jo_3d_morph                       *m;
    register unsigned int                   i;

    // (a value of a frame x 2^shift: at most 2^16 (a s16 x 2^16 is 2^31: a whole 16.16 fixed))
    if (model->morph_shift > 16 || model->morph_frame_size == 0)
        return (false);
    for (JO_ZERO(i); i < model->morph_count; ++i)
    {
        m = &model->morphs[i];
        if (m->frame_count == 0 || m->frame_ticks == 0 || (m->offset & 3) || m->offset > size ||
            m->frame_count > (size - m->offset) / model->morph_frame_size)
            return (false);
    }
    return (true);
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
    for (JO_ZERO(k); k < model->morph_count; ++k)
    {
        model->morphs[k].duplicate = 0;
        for (JO_ZERO(j); j < k; ++j)
            if (jo_3d_model_same_name(model->morphs[j].name, model->morphs[k].name))
            {
                model->morphs[k].duplicate = 1;
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

/** @brief Use the tables of the textures of the models of a capacity (the names and the ids copied)
 *  @param capacity JO_3D_MODEL_KEPT_TEXTURES: the tables kept in the program, more: a table allocated
 *  @return false if out of memory (the tables used stay)
 */
JO_LOW_WORK_RAM_CODE
static bool                                 jo_3d_model_texture_tables(const int capacity)
{
    jo_3d_model_texture_name                *names;
    short                                   *ids;

    if (capacity == JO_3D_MODEL_KEPT_TEXTURES)
    {
        names = __jo_3d_model_texture_kept_names;
        ids = __jo_3d_model_texture_kept_ids;
    }
    else
    {
        // (one block: the names, then the ids)
        if ((names = (jo_3d_model_texture_name *)jo_malloc((unsigned int)capacity * (sizeof(*names) + sizeof(*ids)))) == JO_NULL)
            return (false);
        ids = (short *)(names + capacity);
    }
    jo_memcpy(names, __jo_3d_model_texture_names, (unsigned int)__jo_3d_model_texture_count * sizeof(*names));
    jo_memcpy(ids, __jo_3d_model_texture_ids, (unsigned int)__jo_3d_model_texture_count * sizeof(*ids));
    if (__jo_3d_model_texture_names != __jo_3d_model_texture_kept_names)
        jo_free(__jo_3d_model_texture_names);
    __jo_3d_model_texture_names = names;
    __jo_3d_model_texture_ids = ids;
    __jo_3d_model_texture_capacity = capacity;
    return (true);
}

/** @brief Keep the name of a texture of the models (it is loaded once) */
JO_LOW_WORK_RAM_CODE
static void                                 jo_3d_model_add_texture(const char * const name, const int id)
{
    if (id < 0 || __jo_3d_model_texture_count >= JO_3D_MODEL_MAX_TEXTURES)
        return;
    // (more textures of models than the tables kept in the program: a table twice as big allocated)
    if (__jo_3d_model_texture_count >= __jo_3d_model_texture_capacity &&
        !jo_3d_model_texture_tables(JO_MIN(JO_MULT_BY_2(__jo_3d_model_texture_capacity), JO_3D_MODEL_MAX_TEXTURES)))
        return;
    // (a name longer than 15 characters: kept with its first 15, as before)
    jo_3d_model_texture_hash(name, &__jo_3d_model_texture_names[__jo_3d_model_texture_count]);
    __jo_3d_model_texture_ids[__jo_3d_model_texture_count++] = (short)id;
}

JO_LOW_WORK_RAM_CODE
void                                        __jo_3d_model_forget_textures_from(const int sprite_id)
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
    // (back in the tables kept in the program when they are enough: the table allocated freed)
    if (__jo_3d_model_texture_capacity != JO_3D_MODEL_KEPT_TEXTURES && n <= JO_3D_MODEL_KEPT_TEXTURES)
        jo_3d_model_texture_tables(JO_3D_MODEL_KEPT_TEXTURES);
}

JO_LOW_WORK_RAM_CODE
void                                        jo_3d_model_free_textures_from(const int sprite_id)
{
    __jo_3d_model_forget_textures_from(sprite_id);
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
    // (a damaged size must not wrap: the callers then refuse a size bigger than their data)
    if (pixels > 0x7FFFFFF0u)
        return (0xFFFFFFFCu);
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

JO_LOW_WORK_RAM_CODE
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
    // (set below when the header is read: GCC -Os saw it maybe uninitialized)
    JO_ZERO(size);
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
/** @brief The polygons drawn without gouraud: 1 those whose gouraud table leaves the texels as they are (see
 *  jo_3d_set_neutral_gouraud_skipped()), 2 those whose 4 colors are the same too (see jo_3d_set_uniform_gouraud_skipped(),
 *  the default) */
static unsigned char                        __jo_3d_model_skip_gouraud = 2;
/** @brief The copies of the color lookup tables with a light (see jo_3d_set_uniform_gouraud_skipped()): the table, the
 *  light (a gouraud color), the copy (VRAM / 8, 0: free entry) */
#define JO_3D_MODEL_LIT_LUTS                (256)
/** @brief The hash of the copies: 2 x JO_3D_MODEL_LIT_LUTS entries (bits of the index) */
#define JO_3D_MODEL_LIT_HASH_BITS           (9)
typedef struct
{
    unsigned short                          lut;
    unsigned short                          light;
    unsigned short                          copy;
}                                           jo_3d_model_lit_lut;
/** @brief The copies, a hash of them (entry + 1, 0: free), the copies to make for a model (2.5 KB): forgotten by
 *  jo_3d_set_uniform_gouraud_skipped(), when sprites are freed (their copies may be among them) and when half of them are
 *  used at the start of a model (the next ones make their copies again) */
typedef struct
{
    jo_3d_model_lit_lut                     entries[JO_3D_MODEL_LIT_LUTS];
    unsigned short                          hash[JO_3D_MODEL_LIT_LUTS * 2];
    unsigned short                          pending[JO_3D_MODEL_LIT_LUTS];
}                                           jo_3d_model_lit_lut_set;
static jo_3d_model_lit_lut_set              __jo_3d_model_lit_set;
static unsigned int                         __jo_3d_model_lit_lut_count = 0;
#define __jo_3d_model_lit_luts              (__jo_3d_model_lit_set.entries)
#define __jo_3d_model_lit_lut_hash          (__jo_3d_model_lit_set.hash)
#define __jo_3d_model_lit_pending           (__jo_3d_model_lit_set.pending)
/** @brief A gouraud color that leaves the texel as it is (16, 16, 16) */
#define JO_3D_MODEL_NEUTRAL_GOURAUD         (0xC210)

/** @brief A hash of the 4 colors of a gouraud table */
static  __jo_force_inline unsigned int      jo_3d_model_gouraud_hash(const unsigned short * const t)
{
    return ((((unsigned int)t[0] * 31u + t[1]) * 31u + t[2]) * 31u + t[3]) * 0x9E3779B1u;
}

/** @brief A component of a color + a gouraud one: what the VDP1 draws (saturated between 0 and 31, as Mednafen)
 *  @param sum The component + the gouraud one
 *  @return The component
 */
static  __jo_force_inline unsigned int      jo_3d_model_lit_component(const int sum)
{
    return ((unsigned int)JO_MIN(31, JO_MAX(0, sum - 16)));
}

/** @brief An RGB color with a gouraud color: what the VDP1 draws (each component + the gouraud one - 16, between 0 and 31)
 *  @param color RGB color (a palette color, without its MSB: as it is, the VDP1 does not shade it)
 *  @param light Gouraud color
 *  @return The color
 */
static  __jo_force_inline unsigned short    jo_3d_model_lit_color(const unsigned short color, const unsigned short light)
{
    if (!(color & JO_COLOR_SATURN_MSB))
        return (color);
    return ((unsigned short)(JO_COLOR_SATURN_MSB |
                             (jo_3d_model_lit_component((int)JO_COLOR_SATURN_GET_B(color) + (int)JO_COLOR_SATURN_GET_B(light)) << 10) |
                             (jo_3d_model_lit_component((int)JO_COLOR_SATURN_GET_G(color) + (int)JO_COLOR_SATURN_GET_G(light)) << 5) |
                             jo_3d_model_lit_component((int)JO_COLOR_SATURN_GET_R(color) + (int)JO_COLOR_SATURN_GET_R(light))));
}

/** @brief The copy of a color lookup table with a light (see jo_3d_set_uniform_gouraud_skipped())
 *  @param lut Color lookup table (VRAM / 8)
 *  @param light Gouraud color
 *  @return Its entry (copy: 0 if not made yet), JO_NULL if the list is full
 */
JO_LOW_WORK_RAM_CODE
static jo_3d_model_lit_lut                  *jo_3d_model_find_lit_lut(const unsigned short lut, const unsigned short light)
{
    jo_3d_model_lit_lut                     *entry;
    unsigned int                            slot;

    // (a hash of the entries + 1, 0: free; thousands of polygons in a track, the list in one loop took 0.4 s)
    if (__jo_3d_model_lit_lut_count == 0)
        jo_memset(__jo_3d_model_lit_lut_hash, 0, sizeof(__jo_3d_model_lit_lut_hash));
    for (slot = ((unsigned int)lut * 31u + light) * 0x9E3779B1u >> (32 - JO_3D_MODEL_LIT_HASH_BITS);;
         slot = (slot + 1) & (JO_3D_MODEL_LIT_LUTS * 2 - 1))
    {
        if (__jo_3d_model_lit_lut_hash[slot] == 0)
            break;
        entry = &__jo_3d_model_lit_luts[__jo_3d_model_lit_lut_hash[slot] - 1];
        if (entry->lut == lut && entry->light == light)
            return (entry);
    }
    if (__jo_3d_model_lit_lut_count >= JO_3D_MODEL_LIT_LUTS)
        return (JO_NULL);
    entry = &__jo_3d_model_lit_luts[__jo_3d_model_lit_lut_count++];
    __jo_3d_model_lit_lut_hash[slot] = (unsigned short)__jo_3d_model_lit_lut_count;
    entry->lut = lut;
    entry->light = light;
    entry->copy = 0;
    return (entry);
}

/** @brief The polygons whose 4 gouraud colors are the same: without gouraud, the light in their color (see
 *  jo_3d_set_uniform_gouraud_skipped())
 *  @param model Model (gouraud_table of an attribute: the index of the polygon + 1 when it uses its table)
 *  @param gouraud The table of each polygon (4 colors)
 *  @param pending The copies of color lookup tables to make for this model (output: indexes in __jo_3d_model_lit_luts)
 *  @param pending_count Their count (output)
 *  @remarks The polygons of a texture whose copy is to make keep their color lookup table and their gouraud flag, without
 *           table (0), until the copy is made: see jo_3d_model_set_lit_luts(). Not a mark in their color: a table in the
 *           upper half of the VRAM (address / 8 >= 0x8000) looked like one, it got the copy of another texture
 */
JO_LOW_WORK_RAM_CODE
static void                                 jo_3d_model_skip_uniform_gouraud(jo_3d_model * const model, const unsigned short * const gouraud,
                                                                             unsigned short * const pending, unsigned int * const pending_count)
{
    jo_3d_polygon_attribute                 *a;
    jo_3d_model_lit_lut                     *entry;
    const unsigned short                    *t;
    unsigned int                            n;
    unsigned int                            k;
    unsigned int                            i;
    unsigned int                            j;
    unsigned short                          light;

    n = model->mesh->data.polygon_count;
    for (JO_ZERO(k); k < n; ++k)
    {
        a = &model->mesh->data.attributes[k];
        t = &gouraud[k * 4];
        if (a->gouraud_table == 0 || t[0] != t[1] || t[0] != t[2] || t[0] != t[3] || (a->sort & JO_3D_USE_LIGHT))
            continue;
        light = (unsigned short)(t[0] | JO_COLOR_SATURN_MSB);
        if (light == JO_3D_MODEL_NEUTRAL_GOURAUD)
            ;
        else if (!(a->sort & JO_3D_USE_TEXTURE))
        {
            // (a palette color: the VDP1 does not shade it, as it is)
            a->color = jo_3d_model_lit_color(a->color, light);
        }
        else if ((a->draw_mode & JO_VDP1_COLOR_MODE_MASK) == JO_VDP1_COLOR_MODE_16_LOOKUP && a->color != 0 &&
                 (entry = jo_3d_model_find_lit_lut(a->color, light)) != JO_NULL)
        {
            if (entry->copy != 0)
                a->color = entry->copy;
            else
            {
                // (a copy to make: once per model; the polygon waits for it, gouraud flag kept, no table)
                i = (unsigned int)(entry - __jo_3d_model_lit_luts);
                for (JO_ZERO(j); j < *pending_count && pending[j] != i; ++j)
                    ;
                if (j >= *pending_count)
                    pending[(*pending_count)++] = (unsigned short)i;
                a->gouraud_table = 0;
                continue;
            }
        }
        else
            continue;
        a->gouraud_table = 0;
        a->draw_mode &= ~JO_VDP1_COLOR_CALC_GOURAUD;
    }
}

/** @brief The copies of the color lookup tables with their light (see jo_3d_model_skip_uniform_gouraud())
 *  @param compact Where to write them (16 colors each: the VRAM)
 *  @param pending Their entries
 *  @param count Their count
 */
JO_LOW_WORK_RAM_CODE
static void                                 jo_3d_model_write_lit_luts(volatile unsigned short * const compact, const unsigned short * const pending,
                                                                       const unsigned int count)
{
    const volatile unsigned short           *src;
    const jo_3d_model_lit_lut               *entry;
    unsigned int                            k;
    unsigned int                            i;

    for (JO_ZERO(k); k < count; ++k)
    {
        entry = &__jo_3d_model_lit_luts[pending[k]];
        src = (const volatile unsigned short *)(JO_VDP1_VRAM + JO_MULT_BY_8(entry->lut));
        for (JO_ZERO(i); i < 16; ++i)
            compact[k * 16 + i] = jo_3d_model_lit_color(src[i], entry->light);
    }
}

/** @brief The polygons that wait for a copy of their color lookup table (gouraud flag without table, see
 *  jo_3d_model_skip_uniform_gouraud()): the copy, or their table without the light (base 0: out of VRAM)
 *  @param model Model
 *  @param gouraud The table of each polygon (4 colors): the light of a polygon, to find its copy
 *  @param pending The entries of the copies made
 *  @param count Their count
 *  @param base The first copy (VRAM / 8, a multiple of 4, the next ones 4 tables after), 0: not made
 */
JO_LOW_WORK_RAM_CODE
static void                                 jo_3d_model_set_lit_luts(jo_3d_model * const model, const unsigned short * const gouraud,
                                                                     const unsigned short * const pending, const unsigned int count,
                                                                     const unsigned short base)
{
    jo_3d_polygon_attribute                 *a;
    jo_3d_model_lit_lut                     *entry;
    unsigned int                            k;
    unsigned int                            i;

    for (JO_ZERO(i); i < count; ++i)
        __jo_3d_model_lit_luts[pending[i]].copy = base != 0 ? (unsigned short)(base + i * 4) : 0;
    for (JO_ZERO(k); k < model->mesh->data.polygon_count; ++k)
    {
        a = &model->mesh->data.attributes[k];
        if (a->gouraud_table != 0 || !(a->draw_mode & JO_VDP1_COLOR_CALC_GOURAUD))
            continue;
        // (no copy: the table of the texture, without the light; the entry forgotten for the next models)
        if (base != 0 && (a->sort & JO_3D_USE_TEXTURE) &&
            (a->draw_mode & JO_VDP1_COLOR_MODE_MASK) == JO_VDP1_COLOR_MODE_16_LOOKUP &&
            (entry = jo_3d_model_find_lit_lut(a->color, (unsigned short)(gouraud[k * 4] | JO_COLOR_SATURN_MSB))) != JO_NULL &&
            entry->copy != 0)
            a->color = entry->copy;
        a->draw_mode &= ~JO_VDP1_COLOR_CALC_GOURAUD;
    }
    if (base == 0)
        __jo_3d_model_lit_lut_count = 0;
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

JO_LOW_WORK_RAM_CODE
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
    unsigned int                            luts;
    unsigned int                            reserved;
    unsigned short                          base;

    attributes = model->mesh->data.attributes;
    n = model->mesh->data.polygon_count;
    JO_ZERO(luts);
    // (the polygons whose 4 colors leave the texels as they are: without gouraud, the same picture; Mednafen emulates a
    // pixel of gouraud slower: the gouraud of all the polygons of a split screen cost 6 % of its time)
    // (half of the copies of the color lookup tables known: forgotten, room for those of this model)
    if (__jo_3d_model_lit_lut_count >= JO_3D_MODEL_LIT_LUTS / 2)
        __jo_3d_model_lit_lut_count = 0;
    if (__jo_3d_model_skip_gouraud == 2 && !__jo_3d_model_unique_gouraud_tables)
        jo_3d_model_skip_uniform_gouraud(model, gouraud, __jo_3d_model_lit_pending, &luts);
    else if (__jo_3d_model_skip_gouraud && !__jo_3d_model_unique_gouraud_tables)
        for (JO_ZERO(k); k < n; ++k)
            if (attributes[k].gouraud_table != 0 &&
                (gouraud[k * 4] | 0x8000) == JO_3D_MODEL_NEUTRAL_GOURAUD &&
                (gouraud[k * 4 + 1] | 0x8000) == JO_3D_MODEL_NEUTRAL_GOURAUD &&
                (gouraud[k * 4 + 2] | 0x8000) == JO_3D_MODEL_NEUTRAL_GOURAUD &&
                (gouraud[k * 4 + 3] | 0x8000) == JO_3D_MODEL_NEUTRAL_GOURAUD)
            {
                attributes[k].gouraud_table = 0;
                attributes[k].draw_mode &= ~JO_VDP1_COLOR_CALC_GOURAUD;
            }
    for (JO_ZERO(k), JO_ZERO(used); k < n; ++k)
        if (attributes[k].gouraud_table != 0)
            ++used;
    if (used == 0 && luts == 0)
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
        // (the copies of the color lookup tables not made: the tables of the textures, without the light)
        if (luts != 0)
            jo_3d_model_set_lit_luts(model, gouraud, __jo_3d_model_lit_pending, luts, 0);
        if (used == 0)
            return;
        // A table per polygon (the tables of a room lit by the point lights, see __jo_3d_load_model_for_lights())
        base = jo_3d_add_gouraud_tables((const jo_color *)gouraud, n);
        model->gouraud_tables = base;
        model->gouraud_table_count = base != 0 ? (unsigned short)n : 0;
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
    // (the copies of the color lookup tables first: 4 tables each, at an address multiple of 32 bytes (the VDP1 reads a
    // table there: the 2 low bits of the color ignored), then the gouraud tables)
    reserved = luts != 0 ? luts * 4 + 4 : 0;
    compact = ((reserved + count + 1) & ~1u) <= JO_3D_MODEL_GOURAUD_COMPACT ? __jo_3d_model_gouraud_compact :
              (unsigned short *)jo_malloc(((reserved + count + 1) & ~1u) * 4 * sizeof(unsigned short));
    if (compact == JO_NULL)
    {
        if (luts != 0)
            jo_3d_model_set_lit_luts(model, gouraud, __jo_3d_model_lit_pending, luts, 0);
        // (the first polygon of each table: its own table)
        base = jo_3d_add_gouraud_tables((const jo_color *)gouraud, n);
        model->gouraud_tables = base;
        model->gouraud_table_count = base != 0 ? (unsigned short)n : 0;
        for (JO_ZERO(k); k < n; ++k)
            if (attributes[k].gouraud_table != 0)
                attributes[k].gouraud_table = base != 0 ? (unsigned short)(base + attributes[k].gouraud_table - 1) : 0;
        if (base == 0)
            for (JO_ZERO(k); k < n; ++k)
                attributes[k].draw_mode &= ~JO_VDP1_COLOR_CALC_GOURAUD;
        return;
    }
    jo_memset(compact, 0, reserved * 4 * sizeof(unsigned short));
    // The different tables in the order of their first polygon: the index of the table of a polygon (the polygons before
    // it already have theirs, its first polygon too)
    for (JO_ZERO(k), count = reserved; k < n; ++k)
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
    if (base != 0 && luts != 0)
    {
        first = (base + 3u) & ~3u;
        jo_3d_model_write_lit_luts((unsigned short *)(JO_VDP1_VRAM + JO_MULT_BY_8(first)), __jo_3d_model_lit_pending, luts);
        jo_3d_model_set_lit_luts(model, gouraud, __jo_3d_model_lit_pending, luts, (unsigned short)first);
    }
    else if (luts != 0)
        jo_3d_model_set_lit_luts(model, gouraud, __jo_3d_model_lit_pending, luts, 0);
    model->gouraud_tables = base;
    model->gouraud_table_count = base != 0 ? (unsigned short)count : 0;
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

void                                        __jo_3d_model_forget_lit_luts(void)
{
    // (the copies stay with the models that made them: not shared with the next ones)
    __jo_3d_model_lit_lut_count = 0;
}

void                                        jo_3d_set_neutral_gouraud_skipped(const bool skipped)
{
    __jo_3d_model_forget_lit_luts();
    __jo_3d_model_skip_gouraud = skipped ? 1 : 0;
}

void                                        jo_3d_set_uniform_gouraud_skipped(const bool skipped)
{
    __jo_3d_model_forget_lit_luts();
    __jo_3d_model_skip_gouraud = skipped ? 2 : 0;
}

#ifdef JO_COMPILE_WITH_FS_SUPPORT

JO_LOW_WORK_RAM_CODE
jo_3d_model                                 *__jo_3d_load_model_for_lights(const char * const filename)
{
    jo_3d_model                             *model;

    __jo_3d_model_unique_gouraud_tables = true;
    model = jo_3d_load_model(filename);
    __jo_3d_model_unique_gouraud_tables = false;
    return (model);
}

JO_LOW_WORK_RAM_CODE
jo_3d_model                                 *__jo_3d_load_model_for_tint(const char * const filename)
{
    jo_3d_model                             *model;
    unsigned char                           skip;

    // (all the polygons keep their gouraud table: the tint is a copy of the tables, see jo_3d_set_tint())
    skip = __jo_3d_model_skip_gouraud;
    __jo_3d_model_skip_gouraud = 0;
    model = jo_3d_load_model(filename);
    __jo_3d_model_skip_gouraud = skip;
    return (model);
}

#endif /* JO_COMPILE_WITH_FS_SUPPORT */

/** @brief Set the attributes of the polygons from their materials */
JO_LOW_WORK_RAM_CODE
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
        if (texture >= 0 && (m->flags & JO_3D_MATERIAL_HIGH_SPEED_SHRINK))
            a->draw_mode |= JO_VDP1_HIGH_SPEED_SHRINK;
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
JO_LOW_WORK_RAM_CODE
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

/** @brief The key of a time of an animation and the fraction to the next key
 *  @param time Time (0 or more)
 *  @param ticks Time between 2 keys (1 to 65535)
 *  @param frac The fraction (0 to 255)
 *  @return The key
 */
static  __jo_force_inline int               jo_3d_model_key_of(const int time, const int ticks, int * const frac)
{
    int                                     key;

    // (one division: (time x 256) / ticks = key x 256 + frac, 0 <= frac < 256. Positive: a logical shift, not the routine of
    // the library of the compiler)
    if (time < 0x800000)
    {
        *frac = jo_int_div(time << 8, ticks);
        key = (int)((unsigned int)*frac >> 8);
        *frac &= 0xFF;
        return (key);
    }
    // (from 2^23 ticks, time x 256 overflows: the key, then the fraction of the rest, under 2^24)
    key = jo_int_div(time, ticks);
    *frac = jo_int_div((time - key * ticks) << 8, ticks);
    return (key);
}

void                                        jo_3d_model_get_pose(const jo_3d_model * const model, const int animation, int time,
                                                                 jo_3d_pose * const pose)
{
    unsigned int                            total;
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
    // (no key or no time between 2 keys: no pose, see jo_3d_model_animations_fit())
    if (a->key_count == 0 || a->key_ticks == 0)
    {
        jo_memset(pose, 0, sizeof(*pose));
        return;
    }
    // A key: the root (3 fixed), the angles of the bones (3 x s16 each), aligned on 4 bytes
    key_size = (12 + model->bone_count * 6 + 3) & ~3;
    if (time < 0)
        time = 0;
    if (a->flags & JO_3D_ANIMATION_LOOP)
    {
        // (the divisions by the division unit of the CPU, see jo_int_div(): several poses per character and per frame. The
        // duration unsigned: 65535 keys of 65535 ticks overflow an int; from 2^31, any time is under it)
        total = (unsigned int)a->key_count * a->key_ticks;
        if (total <= 0x7FFFFFFFu)
            time -= jo_int_div(time, (int)total) * (int)total;
        key = jo_3d_model_key_of(time, a->key_ticks, &frac);
        next = key + 1 < a->key_count ? key + 1 : 0;
    }
    else if ((unsigned int)time >= (unsigned int)(a->key_count - 1) * a->key_ticks)
    {
        key = next = a->key_count - 1;
        frac = 0;
    }
    else
    {
        key = jo_3d_model_key_of(time, a->key_ticks, &frac);
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
    unsigned int                            hidden_bones;   // (one per CPU: the slave CPU replaced the mask of the master)
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
    hidden = m->hidden_bones != 0 && b->group != JO_3D_BONE_NO_GROUP && bone < 32 && (m->hidden_bones & (1u << bone));
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

/** @brief The matrices of a bone and of its children in the space of the model (without the camera and the root offset
 *  of the pose: the bones of a skin, see jo_3d_model_skin())
 *  @param out The matrices of all the bones
 */
static void                                 jo_3d_model_bone_space(const jo_3d_model * const model, const int bone,
                                                                   const jo_3d_pose * const pose, jo_3d_matrix_rows * const out,
                                                                   const jo_fixed (* const parent)[3])
{
    static const jo_angle                   no_angles[3] = {0, 0, 0};
    int                                     child;

    __jo_3d_bone_matrix(out[bone], parent, &model->bones[bone].pivot, pose != JO_NULL ? pose->angles[bone] : no_angles);
    for (child = model->bone_children[bone]; child >= 0; child = model->bone_children[model->bone_count + child])
        jo_3d_model_bone_space(model, child, pose, out, (const jo_fixed (*)[3])out[bone]);
}

/** @brief The points of the skin of a pose (see jo_3d_skin_point) in the buffer of the model for this CPU: a point drawn
 *  by the matrix of its bone, moved toward where the other bone puts it: p + weight x (B^-1 x O x p - p), B and O the
 *  matrices of the 2 bones in the space of the model (rotations and translations: B^-1 is its transpose)
 *  @return The polygon data to draw (the buffer, or the mesh of the model without a buffer)
 */
static const jo_3d_polygon_data            *jo_3d_model_skin(const jo_3d_model * const model, const jo_3d_pose * const pose,
                                                              jo_3d_polygon_data * const d)
{
    static const jo_fixed                   identity[4][3] = {{JO_FIXED_1, 0, 0}, {0, JO_FIXED_1, 0}, {0, 0, JO_FIXED_1}, {0, 0, 0}};
    jo_3d_matrix_rows                       *bones;
    const jo_3d_skin_point                  *s;
    const jo_pos3D_fixed                    *p;
    jo_pos3D_fixed                          *out;
    jo_pos3D_fixed                          q;
    jo_fixed                                weight;
    jo_fixed                                (*b)[3];
    register unsigned int                   k;

    if (model->skin_count == 0 || d == JO_NULL)
        return (&model->mesh->data);
    // (the matrices at the end of the block of this CPU, see jo_3d_model_create_morph_draw())
    bones = (jo_3d_matrix_rows *)((unsigned char *)d + ((sizeof(jo_3d_polygon_data) + 3) & ~3u) +
                                  d->point_count * sizeof(jo_pos3D_fixed) +
                                  (d->polygons != model->mesh->data.polygons ? d->polygon_count * sizeof(jo_3d_polygon) : 0));
    for (JO_ZERO(k); k < model->bone_count; ++k)
        if (model->bones[k].parent < 0)
            jo_3d_model_bone_space(model, (int)k, pose, bones, identity);
    // (the other points: the ones of the model (a vertex animation of the same model wrote over them))
    if (model->morph_count > 0)
        jo_memcpy(d->points, model->mesh->data.points, d->point_count * sizeof(jo_pos3D_fixed));
    for (JO_ZERO(k); k < model->skin_count; ++k)
    {
        s = &model->skin[k];
        p = &model->mesh->data.points[s->point];
        out = &d->points[s->point];
        // Where the other bone puts the point (in the space of the model), back in the space of its own bone
        jo_3d_transform_point_by((const jo_fixed (*)[3])bones[s->other], p, &q);
        b = bones[s->bone];
        q.x -= b[3][0];
        q.y -= b[3][1];
        q.z -= b[3][2];
        // (JO_3D_SKIN_ONE is 1.0 / 2 in 16.16)
        weight = s->weight * (JO_FIXED_1 / JO_3D_SKIN_ONE);
        out->x = p->x + jo_fixed_mult(jo_fixed_dot((const jo_fixed *)&q, b[0]) - p->x, weight);
        out->y = p->y + jo_fixed_mult(jo_fixed_dot((const jo_fixed *)&q, b[1]) - p->y, weight);
        out->z = p->z + jo_fixed_mult(jo_fixed_dot((const jo_fixed *)&q, b[2]) - p->z, weight);
    }
    return (d);
}

#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
/** @brief The matrices of the bones of the pose drawn by the slave CPU (not on its stack: it is small) */
static jo_3d_model_pose_matrices            __jo_3d_slave_pose_matrices;
#endif

void                                        jo_3d_slave_model_draw_pose(const jo_3d_model * const model, const jo_3d_pose * const pose,
                                                                        const jo_3d_bone_callback callback, void * const param,
                                                                        const unsigned int hidden_bones)
{
#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
    jo_3d_model_pose_matrices               *m;
    register unsigned int                   k;
#else
    unsigned int                            hidden;
#endif

#ifdef JO_DEBUG
    if (model == JO_NULL)
    {
        jo_core_error("model is null");
        return;
    }
#endif
#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
    // (the bones hidden: in the matrices of the slave CPU, the mask of the master CPU is not changed while it draws)
    m = &__jo_3d_slave_pose_matrices;
    m->part_count = 0;
    m->hidden_bones = hidden_bones;
    if (jo_3d_slave_push_matrix())
    {
        if (pose != JO_NULL && (pose->root.x | pose->root.y | pose->root.z) != 0)
            jo_3d_slave_translate_matrix_fixed(pose->root.x, pose->root.y, pose->root.z);
        for (JO_ZERO(k); k < model->bone_count; ++k)
            if (model->bones[k].parent < 0)
                jo_3d_model_pose_bone(model, (int)k, pose, callback, param, m, (const jo_fixed (*)[3])__jo_3d_slave_current_matrix());
        jo_3d_slave_pop_matrix();
    }
    if (m->part_count > 0)
        __jo_3d_slave_draw_parts(jo_3d_model_skin(model, pose, model->morph_draw[1]), m->parts, m->part_count);
#else
    // (without the dual CPU module: drawn by the master CPU, like a job of jo_core_exec_on_slave())
    hidden = __jo_3d_hidden_bones;
    __jo_3d_hidden_bones = hidden_bones;
    jo_3d_model_draw_pose(model, pose, callback, param);
    __jo_3d_hidden_bones = hidden;
#endif
}

void                                        jo_3d_model_draw_pose(const jo_3d_model * const model, const jo_3d_pose * const pose,
                                                                  const jo_3d_bone_callback callback, void * const param)
{
    jo_3d_model_pose_matrices               m;
    register unsigned int                   k;

#ifdef JO_DEBUG
    if (model == JO_NULL)
    {
        jo_core_error("model is null");
        return;
    }
#endif
    // The matrices of the bones, then one draw: the points of each bone are transformed by its matrix (a draw per bone
    // would cost about 0.1 ms each)
    m.part_count = 0;
    m.hidden_bones = __jo_3d_hidden_bones;
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
        __jo_3d_draw_parts(jo_3d_model_skin(model, pose, model->morph_draw[0]), m.parts, m.part_count);
}

/** @brief An address in the high work RAM (cached or not) */
static  __jo_force_inline bool              jo_3d_model_is_fast_memory(const void * const p)
{
    return (((unsigned int)p & 0x0FF00000) == 0x06000000);
}

bool                                        jo_3d_model_use_fast_memory(jo_3d_model * const model)
{
    jo_3d_polygon_data                      *pdata;
    jo_3d_polygon_attribute                 *attributes;
    unsigned char                           *block;
    unsigned int                            points;
    unsigned int                            polygons;
    unsigned int                            bones;
    unsigned int                            groups;

    if (model == JO_NULL || model->mesh == JO_NULL)
        return (false);
    pdata = &model->mesh->data;
    // (already in the high work RAM: nothing to do. Only the data of a file loaded by jo_3d_load_model(): the other models
    // own their arrays, the ones of jo_3d_create_model_in_place() belong to the caller)
    if (model->fast_block != JO_NULL || (jo_3d_model_is_fast_memory(pdata->points) && jo_3d_model_is_fast_memory(pdata->polygons)))
        return (true);
    if (model->file == JO_NULL || model->single_block || model->file_not_owned)
        return (false);
    // One block: the points, the polygons, the bones, the groups (sizes multiple of 4)
    points = pdata->point_count * sizeof(jo_pos3D_fixed);
    polygons = pdata->polygon_count * sizeof(jo_3d_polygon);
    bones = model->bones != JO_NULL ? model->bone_count * sizeof(jo_3d_bone) : 0;
    groups = model->groups != JO_NULL ? model->group_count * sizeof(jo_3d_model_group) : 0;
    groups = (groups + 3) & ~3u;
    block = (unsigned char *)jo_malloc(points + polygons + bones + groups);
    if (block == JO_NULL)
        return (false);
    attributes = (jo_3d_polygon_attribute *)jo_malloc(pdata->polygon_count * sizeof(jo_3d_polygon_attribute));
    // (not in the high work RAM: the slower memory is kept as it is)
    if (!jo_3d_model_is_fast_memory(block) || attributes == JO_NULL || !jo_3d_model_is_fast_memory(attributes))
    {
        if (attributes != JO_NULL)
            jo_free(attributes);
        jo_free(block);
        return (false);
    }
    jo_memcpy(block, pdata->points, points);
    pdata->points = (jo_pos3D_fixed *)block;
    jo_memcpy(block + points, pdata->polygons, polygons);
    pdata->polygons = (jo_3d_polygon *)(block + points);
    if (bones > 0)
    {
        jo_memcpy(block + points + polygons, model->bones, bones);
        model->bones = (jo_3d_bone *)(block + points + polygons);
    }
    if (groups > 0)
    {
        jo_memcpy(block + points + polygons + bones, model->groups, model->group_count * sizeof(jo_3d_model_group));
        model->groups = (jo_3d_model_group *)(block + points + polygons + bones);
    }
    // (the attributes: their own block, freed with the model as before (see jo_3d_model_free()))
    jo_memcpy(attributes, pdata->attributes, pdata->polygon_count * sizeof(jo_3d_polygon_attribute));
    jo_free(pdata->attributes);
    pdata->attributes = attributes;
    jo_3d_model_update_morph_draw(model);
    model->fast_block = block;
    return (true);
}

/*
** Loading
*/

/** @brief The parts of the memory block of a model created in place (aligned on 4 bytes): the model, its mesh, the
 *  attributes of its polygons, the ids of its textures */
#define JO_3D_MODEL_BLOCK_MESH              ((sizeof(jo_3d_model) + 3) & ~3u)
#define JO_3D_MODEL_BLOCK_ATTRIBUTES        (JO_3D_MODEL_BLOCK_MESH + ((sizeof(jo_3d_mesh) + 3) & ~3u))
#define JO_3D_MODEL_BLOCK_TEXTURES(polygons) (JO_3D_MODEL_BLOCK_ATTRIBUTES + (((polygons) * sizeof(jo_3d_polygon_attribute) + 3) & ~3u))

/*
** Vertex animations (morph targets)
*/

/** @brief jo_3d_morph_normal(): the golden angle (360 x (1 - 1 / golden ratio) degrees) in jo_angle */
#define JO_3D_MORPH_GOLDEN_ANGLE                (25033)

void                                        jo_3d_morph_normal(const int index, jo_pos3D_fixed * const normal)
{
    jo_fixed                                y;
    jo_fixed                                r;
    jo_fixed                                s;
    jo_fixed                                c;
    int                                     i;

    // The directions: a golden angle spiral on the sphere, y from 1 to -1 (Compiler/COMMON/jo_model.py
    // morph_normal_table(): the same directions), computed (a table cost 768 bytes)
    i = index & (JO_3D_MORPH_NORMALS - 1);
    y = JO_FIXED_1 - (2 * i + 1) * (JO_FIXED_1 / JO_3D_MORPH_NORMALS);
    r = jo_fixed_sqrt(JO_FIXED_1 - jo_fixed_mult(y, y));
    jo_sincos_angle((jo_angle)(i * JO_3D_MORPH_GOLDEN_ANGLE), &s, &c);
    normal->x = jo_fixed_mult(c, r);
    normal->y = y;
    normal->z = jo_fixed_mult(s, r);
}

/** @brief The polygons of a model are lit (jo_light: the normals of the frames are used) */
static bool                                 jo_3d_model_has_lit_polygons(const jo_3d_model * const model)
{
    register unsigned int                   k;

    for (JO_ZERO(k); k < model->mesh->data.polygon_count; ++k)
        if (model->mesh->data.attributes[k].sort & JO_3D_USE_LIGHT)
            return (true);
    return (false);
}

/** @brief The polygon data drawn by jo_3d_model_draw_morph() point to the polygons and the attributes of the model (after
 *  they moved, see jo_3d_model_use_fast_memory()) */
JO_LOW_WORK_RAM_CODE
static void                                 jo_3d_model_update_morph_draw(const jo_3d_model * const model)
{
    jo_3d_polygon_data                      *d;
    register unsigned int                   k;

    for (JO_ZERO(k); k < 2; ++k)
    {
        d = model->morph_draw[k];
        if (d == JO_NULL)
            continue;
        d->attributes = model->mesh->data.attributes;
        // (the polygons of the model, or their copy with the normals of the frames: its vertices are the same)
        if (!(model->morph_normals && jo_3d_model_has_lit_polygons(model)))
            d->polygons = model->mesh->data.polygons;
    }
}

/** @brief Bytes of the matrices of the bones of the skin of a model (see jo_3d_model_skin()) */
static unsigned int                         jo_3d_model_skin_matrices_size(const jo_3d_model * const model)
{
    return (model->skin_count > 0 ? model->bone_count * sizeof(jo_3d_matrix_rows) : 0);
}

/** @brief The polygon data drawn by jo_3d_model_draw_morph() (one block for each CPU: the polygon data, the points, the
 *  polygons with the normals of the frames if some are lit)
 *  @return false if out of memory
 */
JO_LOW_WORK_RAM_CODE
static bool                                 jo_3d_model_create_morph_draw(jo_3d_model * const model)
{
    const jo_3d_polygon_data                *src;
    jo_3d_polygon_data                      *d;
    unsigned char                           *block;
    unsigned int                            head;
    unsigned int                            points;
    unsigned int                            polygons;
    register unsigned int                   k;

    src = &model->mesh->data;
    head = (sizeof(jo_3d_polygon_data) + 3) & ~3u;
    points = src->point_count * sizeof(jo_pos3D_fixed);
    polygons = model->morph_normals && jo_3d_model_has_lit_polygons(model) ? src->polygon_count * sizeof(jo_3d_polygon) : 0;
    for (JO_ZERO(k); k < 2; ++k)
    {
        // (then the matrices of the bones of a skin: not on the stack, the one of the slave CPU is small)
        block = (unsigned char *)jo_malloc(head + points + polygons + jo_3d_model_skin_matrices_size(model));
        if (block == JO_NULL)
            return (false);
        d = (jo_3d_polygon_data *)block;
        *d = *src;
        d->points = (jo_pos3D_fixed *)(block + head);
        jo_memcpy(d->points, src->points, points);
        if (polygons > 0)
        {
            d->polygons = (jo_3d_polygon *)(block + head + points);
            jo_memcpy(d->polygons, src->polygons, polygons);
        }
        model->morph_draw[k] = d;
    }
    return (true);
}

/** @brief Free the polygon data of jo_3d_model_draw_morph() (and the copies of their tints) */
JO_LOW_WORK_RAM_CODE
static void                                 jo_3d_model_free_morph_draw(jo_3d_model * const model)
{
    register unsigned int                   k;

    for (JO_ZERO(k); k < 2; ++k)
        if (model->morph_draw[k] != JO_NULL)
        {
            __jo_3d_forget_tint(model->morph_draw[k]);
            jo_free(model->morph_draw[k]);
            model->morph_draw[k] = JO_NULL;
        }
}

int                                         jo_3d_model_find_morph(const jo_3d_model * const model, const char * const name)
{
    register unsigned int                   k;

    for (JO_ZERO(k); k < model->morph_count; ++k)
        if (jo_3d_model_same_name(model->morphs[k].name, name))
            return ((int)k);
    return (-1);
}

int                                         jo_3d_model_morph_duration(const jo_3d_model * const model, const int morph)
{
    const jo_3d_morph                       *m;

    if (morph < 0 || morph >= (int)model->morph_count)
        return (0);
    m = &model->morphs[morph];
    return ((m->flags & JO_3D_MORPH_LOOP ? m->frame_count : m->frame_count - 1) * m->frame_ticks);
}

/** @brief The points (and the normals) of a time of a vertex animation in polygon data of the model
 *  @param model Model
 *  @param morph Index of the vertex animation
 *  @param time Ticks (1/60 s)
 *  @param d The polygon data written (model->morph_draw[k])
 *  @return false if there is no such vertex animation
 */
static bool                                 jo_3d_model_morph_frame(const jo_3d_model * const model, const int morph, int time,
                                                                    jo_3d_polygon_data * const d)
{
    const jo_3d_morph                       *m;
    const short                             *f0;
    const short                             *f1;
    const unsigned char                     *n;
    jo_fixed                                *out;
    unsigned int                            total;
    int                                     frame;
    int                                     next;
    int                                     frac;
    int                                     scale;
    int                                     v;
    register unsigned int                   k;

    if (morph < 0 || morph >= (int)model->morph_count || d == JO_NULL)
        return (false);
    m = &model->morphs[morph];
    if (time < 0)
        time = 0;
    // (the frame and the fraction to the next one, like the keys of a skeleton: see jo_3d_model_get_pose())
    if (m->flags & JO_3D_MORPH_LOOP)
    {
        total = (unsigned int)m->frame_count * m->frame_ticks;
        if (total <= 0x7FFFFFFFu)
            time -= jo_int_div(time, (int)total) * (int)total;
        frame = jo_3d_model_key_of(time, m->frame_ticks, &frac);
        next = frame + 1 < m->frame_count ? frame + 1 : 0;
    }
    else if ((unsigned int)time >= (unsigned int)(m->frame_count - 1) * m->frame_ticks)
    {
        frame = next = m->frame_count - 1;
        JO_ZERO(frac);
    }
    else
    {
        frame = jo_3d_model_key_of(time, m->frame_ticks, &frac);
        next = frame + 1;
    }
    f0 = (const short *)(model->morph_data + m->offset + frame * model->morph_frame_size);
    f1 = (const short *)(model->morph_data + m->offset + next * model->morph_frame_size);
    out = (jo_fixed *)d->points;
    // A value: a + (b - a) x frac / 256 (x 256: under 2^23), then x 2^shift: x 2^(shift - 8), or x 2^shift then / 256 (a
    // shift of 8 and multiplications: the shifts by a variable count are routines of the library of the compiler on the SH-2)
    scale = model->morph_shift >= 8 ? 1 << (model->morph_shift - 8) : 1 << model->morph_shift;
    for (JO_ZERO(k); k < d->point_count * 3; ++k)
    {
        v = (f0[k] * 256 + (f1[k] - f0[k]) * frac) * scale;
        out[k] = model->morph_shift >= 8 ? v : jo_shift_right8(v);
    }
    // The normals of the polygons: the ones of the nearest frame (the polygons of the model when none is lit)
    if (model->morph_normals && d->polygons != model->mesh->data.polygons)
    {
        n = (const unsigned char *)((frac < 128 ? f0 : f1) + d->point_count * 3);
        for (JO_ZERO(k); k < d->polygon_count; ++k)
            jo_3d_morph_normal(n[k], &d->polygons[k].normal);
    }
    return (true);
}

void                                        jo_3d_model_draw_morph(const jo_3d_model * const model, const int morph, const int time)
{
#ifdef JO_DEBUG
    if (model == JO_NULL)
    {
        jo_core_error("model is null");
        return;
    }
#endif
    if (jo_3d_model_morph_frame(model, morph, time, model->morph_draw[0]))
        __jo_3d_draw(model->morph_draw[0]);
    else
        __jo_3d_draw(&model->mesh->data);
}

bool                                        jo_3d_slave_model_draw_morph(const jo_3d_model * const model, const int morph, const int time)
{
#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
    // (the mesh of the slave CPU: jo_3d_slave_mesh_draw() reads the polygon data of a mesh)
    if (!jo_3d_model_morph_frame(model, morph, time, model->morph_draw[1]))
        return (jo_3d_slave_mesh_draw(model->mesh));
    return (jo_3d_slave_mesh_draw((const jo_3d_mesh *)model->morph_draw[1]));
#else
    // (without the dual CPU module: drawn by the master CPU, like jo_3d_slave_model_draw_pose())
    jo_3d_model_draw_morph(model, morph, time);
    return (true);
#endif
}

/** @brief Create a model
 *  @param file The data of the file used in place (JO_NULL: copies)
 *  @param storage One memory block for the model, its mesh, the attributes and the texture ids (see
 *         jo_3d_create_model_in_place()), JO_NULL: a block each
 */
/** @brief Check if a chunk of a model was seen before (each chunk once in a JOM file)
 *  @param p The chunk
 *  @param seen The chunks seen (bits, updated)
 *  @return true if it was seen (a wrong file)
 */
static bool                                 jo_3d_model_chunk_seen(const unsigned char * const p, unsigned int * const seen)
{
    static const char                       tags[] = "PNTSQUADQMATMATLTIMGTEXTNTABCNRMVCOLGOURGRUPBONEANIMMRPHSKIN";
    unsigned int                            k;

    for (JO_ZERO(k); k < sizeof(tags) / 4; ++k)
        if (jo_3d_model_is_tag(p, &tags[k * 4]))
        {
            if (*seen & (1u << k))
                return (true);
            *seen |= 1u << k;
            return (false);
        }
    return (false);
}

/** @brief The counts of the polygons and of the textures of a model in one block (see jo_3d_create_model_in_place())
 *  @param polygons Count of the polygons ("QUAD" chunk), 0 if none
 *  @param textures Count of the textures ("TEXT" chunk), 0 if none
 *  @remarks The same counts for the size of the block and for the places in it. Only a count that fits in its chunk: the
 *  count of the polygons of a damaged file wrapped the size of the block around 32 bits, the model was written after a
 *  block of a few bytes (jo_3d_create_model() refuses such a chunk)
 */
static void                                 jo_3d_model_block_counts(const unsigned char * const data, const unsigned int size,
                                                                     unsigned int * const polygons, unsigned int * const textures)
{
    const unsigned char                     *p;
    unsigned int                            chunk;
    unsigned int                            count;

    JO_ZERO(*polygons);
    JO_ZERO(*textures);
    for (p = data + JO_3D_MODEL_HEADER_SIZE; p + 12 <= data + size && !jo_3d_model_is_tag(p, "END "); p += 8 + chunk)
    {
        chunk = *(const unsigned int *)(p + 4);
        if (JO_3D_MODEL_WRONG_CHUNK(chunk, p, data + size))
            break;
        count = *(const unsigned int *)(p + 8);
        if (!jo_3d_model_count_fits(p, chunk, count))
            continue;
        if (jo_3d_model_is_tag(p, "QUAD"))
            *polygons = count;
        else if (jo_3d_model_is_tag(p, "TEXT"))
            *textures = count;
    }
}

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
    unsigned int                            storage_textures;
    unsigned int                            qmat_count;
    unsigned int                            cnrm_count;
    unsigned int                            vcol_count;
    unsigned int                            gouraud_size;
    unsigned int                            anim_size;
    unsigned int                            morph_size;
    unsigned int                            seen;
    bool                                    wrong;
    register unsigned int                   k;

    wrong = false;
    JO_ZERO(qmat_count);
    JO_ZERO(cnrm_count);
    JO_ZERO(vcol_count);
    JO_ZERO(gouraud_size);
    JO_ZERO(anim_size);
    JO_ZERO(morph_size);
    if (data == JO_NULL || size < JO_3D_MODEL_HEADER_SIZE || !jo_3d_model_is_tag(data, "JOM1") ||
        (data[4] << 8 | data[5]) > JO_3D_MODEL_VERSION)
    {
#ifdef JO_DEBUG
        jo_core_error("Not a JOM file (version %d at most)", JO_3D_MODEL_VERSION);
#endif
        return (JO_NULL);
    }
    // (the polygons of the block: the texture ids are after their attributes. The texture ids the block holds: the count
    // of the last "TEXT" chunk, as jo_3d_create_model_in_place() counted them)
    JO_ZERO(storage_polygons);
    JO_ZERO(storage_textures);
    if (storage != JO_NULL)
        jo_3d_model_block_counts(data, size, &storage_polygons, &storage_textures);
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
    JO_ZERO(seen);
    end = data + size;
    for (p = data + JO_3D_MODEL_HEADER_SIZE; p + 8 <= end && !jo_3d_model_is_tag(p, "END "); p += 8 + chunk)
    {
        chunk = *(const unsigned int *)(p + 4);
        if (JO_3D_MODEL_WRONG_CHUNK(chunk, p, end))
            break;
        count = chunk >= 4 ? *(const unsigned int *)(p + 8) : 0;
        // (the elements of a chunk in the chunk: a wrong count read after it. The vertex animations are optional: a chunk
        // "MRPH" of another layout is skipped, see jo_3d_model_morphs_fit())
        if (!jo_3d_model_count_fits(p, chunk, count))
        {
            if (jo_3d_model_is_tag(p, "MRPH"))
                continue;
            wrong = true;
            break;
        }
        // (a chunk twice: the first copy was lost, and in one block (jo_3d_create_model_in_place()) a second "TEXT" bigger
        // than the last one was written after the block)
        if (jo_3d_model_chunk_seen(p, &seen))
        {
            wrong = true;
            break;
        }
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
        {
            qmat_count = count;
            model->polygon_materials = (unsigned short *)jo_3d_model_copy(model, p + 12, count * sizeof(unsigned short));
        }
        else if (jo_3d_model_is_tag(p, "MATL"))
        {
            model->material_count = count;
            model->materials = (jo_3d_material *)jo_3d_model_copy(model, p + 12, count * sizeof(jo_3d_material));
        }
        else if (jo_3d_model_is_tag(p, "TIMG"))
        {
            const unsigned char             *entry;

            // (the pixels of each texture in the chunk too: a wrong size read them after the data of the file)
            for (JO_ZERO(k), entry = p + 12; k < count && entry + 24 <= p + 8 + chunk &&
                 jo_3d_model_texture_size(entry) <= (unsigned int)(p + 8 + chunk - (entry + 24)); ++k)
            {
                jo_3d_model_add_embedded_texture(entry, (const jo_color *)(entry + 24));
                entry += 24 + jo_3d_model_texture_size(entry);
            }
        }
        else if (jo_3d_model_is_tag(p, "TEXT"))
        {
            // (in one block: more ids than the block holds is a wrong file. A first "TEXT" bigger than a second one wrote
            // its ids after the block, before the second one was found)
            if (storage != JO_NULL && count > storage_textures)
            {
                wrong = true;
                break;
            }
            model->texture_count = count;
            model->textures = storage != JO_NULL ? (int *)(storage + JO_3D_MODEL_BLOCK_TEXTURES(storage_polygons)) :
                              (int *)jo_malloc((count > 0 ? count : 1) * sizeof(int));
            for (JO_ZERO(k); model->textures != JO_NULL && k < count; ++k)
            {
#ifdef JO_COMPILE_WITH_FS_SUPPORT
                // (a damaged name of 16 characters without its end: its file name was read after the entry)
                model->textures[k] = p[12 + k * 16 + 15] != '\0' ? -1 : jo_3d_model_texture((const char *)(p + 12 + k * 16));
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
        {
            cnrm_count = count;
            model->corner_normals = (unsigned short *)jo_3d_model_copy(model, p + 12, count * sizeof(unsigned short));
        }
        else if (jo_3d_model_is_tag(p, "VCOL"))
        {
            vcol_count = count;
            model->vertex_colors = (jo_pos3D_fixed *)jo_3d_model_copy(model, p + 12, count * sizeof(jo_pos3D_fixed));
        }
        else if (jo_3d_model_is_tag(p, "GOUR"))
        {
            gouraud = (const unsigned short *)(p + 12);
            gouraud_size = chunk >= 4 ? chunk - 4 : 0;
        }
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
            anim_size = chunk;
            if (model->animation_data != JO_NULL)
            {
                model->animation_count = count;
                model->animations = (jo_3d_animation *)(model->animation_data + 4);
            }
        }
        else if (jo_3d_model_is_tag(p, "SKIN"))
        {
            model->skin = (const jo_3d_skin_point *)jo_3d_model_copy(model, p + 12, count * sizeof(jo_3d_skin_point));
            if (model->skin != JO_NULL)
                model->skin_count = count;
        }
        else if (jo_3d_model_is_tag(p, "MRPH"))
        {
            // (the frames are after the headers: the whole chunk is kept)
            model->morph_data = (const unsigned char *)jo_3d_model_copy(model, p + 8, chunk);
            morph_size = chunk;
            if (model->morph_data != JO_NULL)
            {
                model->morph_count = count;
                model->morph_shift = *(const unsigned short *)(model->morph_data + 4);
                model->morph_normals = (*(const unsigned short *)(model->morph_data + 6) & 1) != 0;
                model->morphs = (jo_3d_morph *)(model->morph_data + 8);
            }
        }
    }
    // The vertices of the polygons are points of the model, a material and 4 gouraud colors per polygon, the keys of the
    // animations in their chunk
    for (JO_ZERO(k); !wrong && polygons != JO_NULL && k < polygon_count; ++k)
    {
        const jo_3d_polygon                 *polygon;

        polygon = (const jo_3d_polygon *)polygons + k;
        if (polygon->vertices[0] >= point_count || polygon->vertices[1] >= point_count ||
            polygon->vertices[2] >= point_count || polygon->vertices[3] >= point_count)
            wrong = true;
    }
    // (a "QMAT" chunk of 0 materials: its table was read for each polygon, after its end)
    if ((model->polygon_materials != JO_NULL && qmat_count < polygon_count) ||
        (gouraud != JO_NULL && gouraud_size < polygon_count * 4 * sizeof(unsigned short)))
        wrong = true;
    // (the tables given to the game shorter than the points or the polygons, a normal out of its table: read after their end)
    if ((model->corner_normals != JO_NULL && cnrm_count < polygon_count * 4) ||
        (model->vertex_colors != JO_NULL && vcol_count < point_count))
        wrong = true;
    for (JO_ZERO(k); !wrong && model->corner_normals != JO_NULL && k < polygon_count * 4; ++k)
        if (model->corner_normals[k] >= model->normal_count)
            wrong = true;
    // (the groups out of the points or of the polygons: jo_3d_model_find_group() gave them to the game, drawn after the mesh)
    for (JO_ZERO(k); !wrong && model->groups != JO_NULL && k < model->group_count; ++k)
        if ((unsigned int)model->groups[k].first_point + model->groups[k].point_count > point_count ||
            (unsigned int)model->groups[k].first_polygon + model->groups[k].polygon_count > polygon_count)
            wrong = true;
    if (!wrong && !jo_3d_model_animations_fit(model, anim_size))
        JO_ZERO(model->animation_count);
    // (a frame: the points (3 x s16), the normals of the polygons (u8), 4 bytes aligned)
    model->morph_frame_size = (point_count * 6 + (model->morph_normals ? polygon_count : 0) + 3) & ~3u;
    if (!wrong && model->morph_count > 0 && !jo_3d_model_morphs_fit(model, morph_size))
        JO_ZERO(model->morph_count);
    // (after the checks: the data of a chunk of another layout, kept in the file, are not written)
    if (!wrong)
        jo_3d_model_mark_duplicates(model);
    // (a point of the skin out of the points, a bone out of the skeleton: the skeleton is rigid)
    for (JO_ZERO(k); k < model->skin_count; ++k)
        if (model->skin[k].point >= point_count || model->skin[k].bone >= model->bone_count ||
            model->skin[k].other >= model->bone_count || model->skin[k].weight > JO_3D_SKIN_ONE)
        {
#ifdef JO_DEBUG
            jo_core_error("Wrong skin point %d", (int)k);
#endif
            JO_ZERO(model->skin_count);
        }
    if (wrong || points == JO_NULL || polygons == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error(wrong ? "Wrong JOM file" : "JOM file without points or polygons");
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
    if ((model->morph_count > 0 || model->skin_count > 0) && !jo_3d_model_create_morph_draw(model))
    {
#ifdef JO_DEBUG
        jo_core_error("Out of memory");
#endif
        jo_3d_model_free(model, false);
        return (JO_NULL);
    }
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

JO_LOW_WORK_RAM_CODE
jo_3d_model                                 *jo_3d_create_model_from_memory(const unsigned char * const data, const unsigned int size)
{
    return (jo_3d_create_model(data, size, JO_NULL, JO_NULL));
}

JO_LOW_WORK_RAM_CODE
jo_3d_model                                 *jo_3d_create_model_in_place(const unsigned char * const data, const unsigned int size)
{
    jo_3d_model                             *model;
    unsigned int                            polygon_count;
    unsigned int                            texture_count;
    unsigned char                           *storage;

    // The counts of the chunks first: the model, its mesh, the attributes of its polygons and the ids of its textures in
    // one memory block (a level of 1000 models: 4000 blocks less, each one found after a search among the others)
    // (a wrong file: jo_3d_create_model() says so, before the block is allocated)
    if (data == JO_NULL || size < JO_3D_MODEL_HEADER_SIZE || !jo_3d_model_is_tag(data, "JOM1") ||
        (data[4] << 8 | data[5]) > JO_3D_MODEL_VERSION)
        return (jo_3d_create_model(data, size, (void *)data, JO_NULL));
    jo_3d_model_block_counts(data, size, &polygon_count, &texture_count);
    storage = (unsigned char *)jo_malloc(JO_3D_MODEL_BLOCK_TEXTURES(polygon_count) +
                                         (texture_count > 0 ? texture_count : 1) * sizeof(int));
    if (storage == JO_NULL)
        return (JO_NULL);
    model = jo_3d_create_model(data, size, (void *)data, storage);
    if (model != JO_NULL)
        model->file_not_owned = true;
    return (model);
}

JO_LOW_WORK_RAM_CODE
void                                        jo_3d_free_model(jo_3d_model * const model)
{
    jo_3d_model_free(model, true);
}

/** @brief Free a model
 *  @param free_file Free the data of the file too (else the caller frees them)
 */
JO_LOW_WORK_RAM_CODE
static void                                 jo_3d_model_free(jo_3d_model * const model, const bool free_file)
{
    if (model == JO_NULL)
        return;
    // (the copies of its tints and its environment map, then its gouraud tables: the VRAM of the VDP1 given back when
    // they are the last sprites, a level loaded and freed again and again used it up)
    if (model->mesh != JO_NULL)
    {
        __jo_3d_forget_tint(&model->mesh->data);
        __jo_3d_forget_environment(&model->mesh->data);
    }
    jo_3d_model_free_morph_draw(model);
    jo_3d_free_gouraud_tables(model->gouraud_tables, model->gouraud_table_count);
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
        // The data are in the file (see jo_3d_load_model()), or their copies in the high work RAM
        if (model->fast_block != JO_NULL)
            jo_free(model->fast_block);
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
    if (model->morph_data != JO_NULL)
        jo_free((void *)model->morph_data);
    if (model->skin != JO_NULL)
        jo_free((void *)model->skin);
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

/** @brief Most chunks walked by jo_3d_model_find_chunk() (the size of the data is not known: a wrong file stops there) */
# define JO_3D_MODEL_FIND_CHUNK_MAX             (64)

const unsigned char                         *jo_3d_model_find_chunk(const void * const data, const char * const tag, unsigned int * const size)
{
    const unsigned char                     *p;
    unsigned int                            chunk;
    register unsigned int                   k;

    if (data == JO_NULL)
        return (JO_NULL);
    p = (const unsigned char *)data + JO_3D_MODEL_HEADER_SIZE;
    for (JO_ZERO(k); k < JO_3D_MODEL_FIND_CHUNK_MAX && !jo_3d_model_is_tag(p, "END "); ++k)
    {
        chunk = *(const unsigned int *)(p + 4);
        if (jo_3d_model_is_tag(p, tag))
        {
            if (size != JO_NULL)
                *size = chunk;
            return (p + 8);
        }
        // (the sizes are multiples of 4: a wrong one stops the walk, the next chunk would not be aligned)
        if (chunk & 3)
            break;
        p += 8 + chunk;
    }
    return (JO_NULL);
}

const unsigned short                        *jo_3d_model_file_gouraud_colors(const void * const data, const unsigned int polygon_count)
{
    const unsigned char                     *p;

    // (the count of the colors: 4 per polygon)
    p = jo_3d_model_find_chunk(data, "GOUR", JO_NULL);
    if (p == JO_NULL || *(const unsigned int *)p != JO_MULT_BY_4(polygon_count))
        return (JO_NULL);
    return ((const unsigned short *)(p + 4));
}

bool                                        jo_3d_model_set_environment(jo_3d_model * const model, const int sprite_id, const int tiles,
                                                                        const unsigned int material_flags)
{
    jo_3d_mesh                              *mesh;
    register unsigned int                   k;

    if (model == JO_NULL || sprite_id < 0)
        return (false);
    mesh = model->mesh;
    if (!jo_3d_set_mesh_environment(mesh, sprite_id, tiles))
        return (false);
    if (model->materials == JO_NULL || model->polygon_materials == JO_NULL)
        return (true);
    for (JO_ZERO(k); k < mesh->data.polygon_count; ++k)
        // (a material out of the table: none, as jo_3d_model_set_attributes())
        if (model->polygon_materials[k] < model->material_count && (model->materials[model->polygon_materials[k]].flags & material_flags))
            jo_3d_set_mesh_polygon_environment(mesh, k);
    return (true);
}

#ifdef JO_COMPILE_WITH_FS_SUPPORT

JO_LOW_WORK_RAM_CODE
void                                        jo_3d_model_pool_init(jo_3d_model_pool * const pool, const unsigned int block_size,
                                                                  const unsigned int bytes)
{
    pool->block = JO_NULL;
    JO_ZERO(pool->left);
    pool->bytes_left = bytes;
    pool->block_size = block_size;
    JO_ZERO(pool->block_count);
}

JO_LOW_WORK_RAM_CODE
jo_3d_model                                 *jo_3d_model_pool_read(jo_3d_model_pool * const pool, jo_file * const file, const unsigned int size)
{
    unsigned int                            size4;
    unsigned int                            used;
    unsigned int                            chunk;
    unsigned char                           *data;
    unsigned char                           *p;
    unsigned char                           *cut;
    jo_3d_model                             *model;

    if (size == 0)
        return (JO_NULL);
    size4 = (size + 3) & ~3u;
    if (size4 > pool->left)
    {
        // A new block (the rest of the current one is lost; the memory left in pieces: a block of the size of the model)
        pool->left = JO_MAX(pool->bytes_left > 0 ? JO_MIN(pool->block_size, pool->bytes_left) : pool->block_size, size4);
        pool->block = (unsigned char *)jo_malloc(pool->left);
        if (pool->block == JO_NULL)
        {
            pool->left = size4;
            pool->block = (unsigned char *)jo_malloc(pool->left);
        }
        if (pool->block == JO_NULL || pool->block_count >= JO_3D_MODEL_POOL_MAX_BLOCKS)
        {
            if (pool->block != JO_NULL)
                jo_free(pool->block);
            pool->block = JO_NULL;
            JO_ZERO(pool->left);
            return (JO_NULL);
        }
        pool->blocks[pool->block_count++] = pool->block;
    }
    data = pool->block;
    if (!jo_fs_read_exact(file, data, size4))
        return (JO_NULL);
    pool->bytes_left -= JO_MIN(pool->bytes_left, size4);
    model = jo_3d_create_model_in_place(data, size);
    if (model == JO_NULL)
        return (JO_NULL);
    // The chunks used only while the model was created, last in the file: the embedded textures, the names of the
    // textures, the gouraud colors are in the VRAM now. The next model is read over them, an end chunk in their place
    // (about 150 KB less for a level of a 3D game)
    used = size4;
    cut = JO_NULL;
    for (p = data + JO_3D_MODEL_HEADER_SIZE; p + 8 <= data + size; p += 8 + chunk)
    {
        chunk = *(const unsigned int *)(p + 4);
        if (jo_3d_model_is_tag(p, "END "))
        {
            if (cut != JO_NULL)
            {
                jo_memcpy(cut, "END \0\0\0\0", 8);
                used = (unsigned int)(cut + 8 - data);
            }
            break;
        }
        // (a wrong size: nothing given back)
        if (chunk & 3)
            break;
        if (jo_3d_model_is_tag(p, "TIMG") || jo_3d_model_is_tag(p, "TEXT") || jo_3d_model_is_tag(p, "GOUR"))
        {
            if (cut == JO_NULL)
                cut = p;
        }
        else
            cut = JO_NULL;
    }
    pool->block += used;
    pool->left -= used;
    return (model);
}

JO_LOW_WORK_RAM_CODE
void                                        jo_3d_model_pool_free(jo_3d_model_pool * const pool)
{
    register int                            k;

    for (JO_ZERO(k); k < pool->block_count; ++k)
        jo_free(pool->blocks[k]);
    JO_ZERO(pool->block_count);
    pool->block = JO_NULL;
    JO_ZERO(pool->left);
}

#endif /* JO_COMPILE_WITH_FS_SUPPORT */

#endif /* JO_COMPILE_WITH_3D_SUPPORT */

/*
** END OF FILE
*/
