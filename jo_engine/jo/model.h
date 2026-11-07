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
/** @file model.h
 *  @author Johannes Fetz
 *
 *  @brief 3D models (.JOM files): meshes made with a 3D software, converted by Compiler/COMMON/jo_model.py
 *  @remarks The converter reads Wavefront OBJ files (and their MTL materials, see jo_model.py for the options of the
 *           materials) and writes a JOM file. The data of a JOM file are in the memory format of the Saturn (big endian,
 *           16.16 fixed point): the points and the polygons are copied as they are, nothing is converted when it is loaded
 *
 *  JOM file (all the values are big endian, the chunks are aligned on 4 bytes):
 *  @code
 *  Header (24 bytes)
 *    char[4]  "JOM1"
 *    u16      version (1)
 *    u16      reserved (0)
 *    fixed[3] center of the bounding sphere (16.16)
 *    fixed    radius of the bounding sphere
 *  Chunks, until the "END " chunk. A chunk: char[4] tag, u32 size of the data (multiple of 4), data.
 *  The loader skips the unknown chunks (newer versions of the converter)
 *    "TIMG"  (optional, first chunk) u32 count, then count x (char[16] name, u16 width, u16 height, u16 format,
 *            u16 reserved, then the pixels, 4 bytes aligned): embedded textures. Format 0 (RGB 555): width x height u16
 *            pixels, 0 is transparent. Format 1 (16 colors, jo_lut in the MTL): 16 u16 colors (RGB 555, the color 0 is
 *            transparent), then 2 pixels per byte (the left pixel in the high bits), see jo_sprite_add_4bits_lut()
 *            (jo_model.py --embed-textures). jo_3d_load_model() reads them one at a time and copies them in the VRAM
 *            (a texture already loaded by another model is skipped), they don't stay in the work RAM
 *    "PNTS"  u32 count, then count x (fixed x, fixed y, fixed z)               Points (Saturn axes: Y down)
 *    "QUAD"  u32 count, then count x (fixed nx, ny, nz, u16 v0, v1, v2, v3)    Polygons (jo_3d_polygon): normal and
 *                                                                            4 point indexes (a triangle: v3 = v2)
 *    "QMAT"  u32 count, then count x u16 (+ u16 pad if count is odd)           Material of each polygon
 *    "MATL"  u32 count, then count x jo_3d_material (32 bytes)                 Materials
 *    "TEXT"  u32 count, then count x char[16]                                  Textures: names (the embedded textures, or
 *                                                                            the files on the CD: .BIN or .TGA)
 *    "NTAB"  u32 count, then count x (fixed x, fixed y, fixed z)               Normals (OBJ vn)
 *    "CNRM"  u32 count (4 x polygons), then count x u16 (+ pad)                Normal of each corner of each polygon
 *    "VCOL"  u32 count (points), then count x (fixed r, fixed g, fixed b)      Vertex colors (OBJ v x y z r g b), 1.0 = 0x10000
 *    "GOUR"  u32 count (4 x polygons), then count x u16 (+ pad)                Gouraud colors of the corners (RGB 555 of
 *                                                                            the vertex colors: 16 = texture unchanged)
 *    "GRUP"  u32 count, then count x jo_3d_model_group (24 bytes)              Groups (OBJ o and g): names, ranges of
 *                                                                            polygons and of the points used only by
 *                                                                            the group (for animations)
 *    "BONE"  u32 count, then count x jo_3d_bone (32 bytes)                     Rigid skeleton: each bone moves a group
 *                                                                            (its polygons use only its points), a
 *                                                                            parent is before its children
 *    "ANIM"  u32 count, then count x jo_3d_animation (28 bytes), then the keys Animations. The keys of an animation
 *                                                                            (at offset from the count): key_count x
 *                                                                            (fixed root x, y, z, then bones x
 *                                                                            (s16 angle x, y, z), + s16 pad if odd)
 *    "END "  0
 *  @endcode
 */

#ifndef __JO_MODEL_H__
# define __JO_MODEL_H__

#ifdef JO_COMPILE_WITH_3D_SUPPORT

/** @brief Version of the JOM files read by this engine */
# define JO_3D_MODEL_VERSION                    (1)

/*
** Material flags (jo_3d_material.flags, low 16 bits: the engine, high 16 bits: free for the program, see jo_model.py)
*/
/** @brief Material: drawn on both sides (JO_3D_DUAL_PLANE) */
# define JO_3D_MATERIAL_DUAL_PLANE              (1 << 0)
/** @brief Material: sorted with the nearest point (JO_3D_SORT_MIN) */
# define JO_3D_MATERIAL_SORT_MIN                (1 << 1)
/** @brief Material: sorted with the farthest point (JO_3D_SORT_MAX) */
# define JO_3D_MATERIAL_SORT_MAX                (1 << 2)
/** @brief Material: sorted with the depth of the previous polygon of the model (JO_3D_SORT_PREVIOUS): a decal drawn over the
 *  face before it (a grille, a sign), whatever their nearest points */
# define JO_3D_MATERIAL_SORT_PREVIOUS           (JO_3D_MATERIAL_SORT_MIN | JO_3D_MATERIAL_SORT_MAX)
/** @brief Material: gouraud shading with the vertex colors ("GOUR" chunk) */
# define JO_3D_MATERIAL_GOURAUD                 (1 << 3)
/** @brief Material: half transparent */
# define JO_3D_MATERIAL_HALF_TRANSPARENT        (1 << 4)
/** @brief Material: screen doors (mesh) */
# define JO_3D_MATERIAL_SCREEN_DOORS            (1 << 5)
/** @brief Material: lit by the light of the engine (jo_3d_light()) */
# define JO_3D_MATERIAL_LIGHT                   (1 << 6)
/** @brief Material: texture flipped horizontally */
# define JO_3D_MATERIAL_FLIP_H                  (1 << 7)
/** @brief Material: texture flipped vertically */
# define JO_3D_MATERIAL_FLIP_V                  (1 << 8)
/** @brief Material: the texture stops each line after its last visible pixel (jo_sprite_add_end_codes()) */
# define JO_3D_MATERIAL_END_CODES               (1 << 9)
/** @brief Material: smaller copies of the texture far away (jo_3d_set_texture_mipmaps()) */
# define JO_3D_MATERIAL_MIPMAPS                 (1 << 10)
/** @brief Material: the polygons near the camera are drawn in parts (jo_3d_set_texture_subdivision()) */
# define JO_3D_MATERIAL_SUBDIVISION             (1 << 11)
/** @brief Material: only the polygons that cross the near plane are drawn in parts (jo_3d_set_texture_subdivision_near_only(),
 *  MTL: jo_subdivision 2) */
# define JO_3D_MATERIAL_SUBDIVISION_NEAR        (1 << 12)
/** @brief First flag of the program (bits 16 to 31, MTL: jo_user) */
# define JO_3D_MATERIAL_USER(N)                 (1 << (16 + (N)))

/** @brief Material of a model (the same layout as the file: 32 bytes) */
typedef struct
{
    char                                    name[16];       // MTL newmtl (truncated, ended by a zero)
    unsigned char                           r;              // Color (MTL Kd): the polygons without texture
    unsigned char                           g;
    unsigned char                           b;
    unsigned char                           shine;          // MTL Ns (0 to 255, for the program)
    short                                   texture;        // Index in the textures of the model, -1: no texture
    unsigned short                          subdivision_error; // MTL jo_subdivision_error (pixels, see
                                                            // jo_3d_set_texture_subdivision_error()), 0: the default
    unsigned int                            flags;          // JO_3D_MATERIAL_xxx
    unsigned int                            user;           // MTL jo_value (for the program)
}                                           jo_3d_material;

/** @brief Group of a model: OBJ o or g (the same layout as the file: 24 bytes) */
typedef struct
{
    char                                    name[16];
    unsigned short                          first_point;
    unsigned short                          point_count;    // The points used only by the group (from first_point)
    unsigned short                          first_polygon;
    unsigned short                          polygon_count;
}                                           jo_3d_model_group;

/** @brief Bones of a model at most (jo_3d_pose) */
# define JO_3D_MAX_BONES                        (32)
/** @brief jo_3d_bone.group: the bone has no polygons */
# define JO_3D_BONE_NO_GROUP                    (0xFFFF)
/** @brief jo_3d_animation.flags: the last key goes back to the first one */
# define JO_3D_ANIMATION_LOOP                   (1 << 0)
/** @brief Ticks of the animations per second (the time of jo_3d_model_get_pose()) */
# define JO_3D_ANIMATION_TICKS_PER_SECOND       (60)

/** @brief Bone of a rigid skeleton (the same layout as the file: 32 bytes) */
typedef struct
{
    char                                    name[16];
    unsigned short                          group;          // Group moved by the bone (JO_3D_BONE_NO_GROUP: none)
    short                                   parent;         // Index of the parent bone, -1: root
    jo_pos3D_fixed                          pivot;          // Joint (model space): the bone turns around it
}                                           jo_3d_bone;

/** @brief Animation of a skeleton (the same layout as the file: 28 bytes) */
typedef struct
{
    char                                    name[16];
    unsigned short                          key_count;
    unsigned short                          key_ticks;      // Time between 2 keys (1/60 s)
    unsigned short                          flags;          // JO_3D_ANIMATION_xxx
    unsigned short                          duplicate;      // 0 in the file. Set at the loading: 1 when an animation before
                                                            // has the same name (see jo_3d_model_is_animation())
    unsigned int                            offset;         // Keys: offset from the count of the "ANIM" chunk
}                                           jo_3d_animation;

/** @brief Pose of a skeleton: the angles of the bones (see jo_3d_model_get_pose() and jo_3d_model_draw_pose()) */
typedef struct
{
    jo_pos3D_fixed                          root;           // Offset of the whole model
    jo_angle                                angles[JO_3D_MAX_BONES][3];  // Around X, Y, Z (0x10000 = 360 degrees)
}                                           jo_3d_pose;

/** @brief Called by jo_3d_model_draw_pose() after each bone is drawn, with the matrix of the bone (the program draws a
 *         weapon in a hand, a muzzle flash...)
 *  @param bone Index of the bone
 *  @param param Parameter of jo_3d_model_draw_pose()
 *  @param matrix Matrix of the bone (4 rows of 3, see jo_3d_transform_point_by()); the current matrix is not the one of
 *         the bone
 */
typedef void                                (*jo_3d_bone_callback)(const int bone, void * const param, const jo_fixed (* const matrix)[3]);

/** @brief 3D model loaded from a JOM file */
typedef struct
{
    jo_3d_mesh                              *mesh;              // Drawn with jo_3d_mesh_draw()
    jo_pos3D_fixed                          center;             // Bounding sphere (see jo_3d_is_sphere_visible())
    jo_fixed                                radius;
    unsigned int                            material_count;
    jo_3d_material                          *materials;
    unsigned short                          *polygon_materials; // Material of each polygon
    unsigned int                            texture_count;
    int                                     *textures;          // Sprite ids
    unsigned int                            normal_count;       // 0: no normals
    jo_pos3D_fixed                          *normals;
    unsigned short                          *corner_normals;    // 4 per polygon: index in normals (JO_NULL: none)
    jo_pos3D_fixed                          *vertex_colors;     // r, g, b per point, 1.0 = JO_FIXED_1 (JO_NULL: none)
    unsigned short                          gouraud_tables;     // First gouraud table (polygon k: + k), 0: none
    unsigned short                          gouraud_table_count;
    unsigned int                            group_count;
    jo_3d_model_group                       *groups;
    unsigned int                            bone_count;         // 0: no skeleton
    jo_3d_bone                              *bones;
    signed char                             *bone_children;     // First child of each bone, then the next sibling of
                                                                // each bone (-1: none)
    unsigned int                            animation_count;
    jo_3d_animation                         *animations;
    const unsigned char                     *animation_data;    // The "ANIM" chunk from its count
    void                                    *file;              // Data of the file (jo_3d_load_model(): the points, the
                                                                // polygons, etc are in it), JO_NULL: copies
    bool                                    file_not_owned;     // The data of the file belong to the caller
                                                                // (jo_3d_create_model_in_place()): not freed with the model
    bool                                    single_block;       // The model, its mesh, the attributes and the texture ids
                                                                // in one memory block (jo_3d_create_model_in_place())
    void                                    *fast_block;        // Copy of the points, the polygons, the bones and the
                                                                // groups in the high work RAM (see
                                                                // jo_3d_model_use_fast_memory()), JO_NULL: none
}                                           jo_3d_model;

/** @brief Create a model from the content of a JOM file
 *  @param data Content of the file (4 bytes aligned)
 *  @param size Size in bytes
 *  @return The model, JO_NULL if the file is wrong or if out of memory
 *  @remarks The embedded textures ("TIMG" chunk) are copied to the VRAM, the other textures are loaded from the CD (see
 *           jo_3d_model_texture()). The data can be freed after the call
 */
jo_3d_model                                 *jo_3d_create_model_from_memory(const unsigned char * const data, const unsigned int size);

/** @brief Create a model that uses the content of a JOM file in place (like jo_3d_load_model(): the points, the polygons,
 *  the materials are not copied): much faster than jo_3d_create_model_from_memory() for many models (a level read in one
 *  block: 4 memory blocks per model instead of about 12), and smaller
 *  @param data Content of the file (4 bytes aligned), kept by the caller as long as the model exists
 *  @param size Size in bytes
 *  @return The model, JO_NULL if the file is wrong or if out of memory
 *  @remarks jo_3d_free_model() does not free the data
 */
jo_3d_model                                 *jo_3d_create_model_in_place(const unsigned char * const data, const unsigned int size);

#ifdef JO_COMPILE_WITH_FS_SUPPORT

/** @brief Load a model (JOM file) from the current directory of the CD
 *  @param filename File name (example: "MODEL.JOM")
 *  @return The model, JO_NULL if the file is not found, wrong or if out of memory
 *  @remarks Free it with jo_3d_free_model()
 *  @remarks The model uses the data of the file (one memory block: the points and the polygons are not copied)
 *  @remarks Embedded textures (jo_model.py --embed-textures): read one at a time and copied to the VRAM, they don't stay in
 *           the work RAM
 */
jo_3d_model                                 *jo_3d_load_model(const char * const filename);

/** @brief Load a model with a gouraud table per polygon (Internal usage: the cells, lit by the point lights, see
 *  jo_3d_point_lights_add_mesh()). jo_3d_load_model() gives one table to the polygons with the same colors
 *  @param filename File name
 *  @return The model, JO_NULL if not loaded
 */
jo_3d_model                                 *__jo_3d_load_model_for_lights(const char * const filename);

/** @brief The next models created or loaded get a gouraud table per polygon, in the order of the polygons (default: the
 *  polygons with the same 4 colors share a table): for a model whose tables the game writes again polygon by polygon (the
 *  light on a car)
 *  @param unique true: a table per polygon
 *  @remarks Shared, the tables of a model are fewer than its polygons: a game that wrote a table per polygon from the
 *  first one wrote over the tables (and the color lookup tables of the textures) that follow them in the VRAM
 */
void                                        jo_3d_set_unique_gouraud_tables(const bool unique);

/** @brief Get the sprite id of a texture of the models (loaded the first time: a texture shared by several models,
 *         or by the program, is loaded once)
 *  @param filename File name on the CD (.BIN: jo_sprite_add_bin(), .TGA: jo_sprite_add_tga())
 *  @return Sprite id, -1 if not loaded
 *  @remarks The transparent color is JO_COLOR_Transparent
 */
int                                         jo_3d_model_texture(const char * const filename);

/** @brief Get the sprite id of a texture already loaded by the models (embedded in a model, or loaded by
 *         jo_3d_model_texture()), without loading it
 *  @param name Name of the texture (its file name, or its name in the model)
 *  @return Sprite id, -1 if no model has this texture
 */
int                                         jo_3d_model_loaded_texture(const char * const name);

#endif

/** @brief Free a model (the textures stay loaded)
 *  @param model Model
 */
void                                        jo_3d_free_model(jo_3d_model * const model);

/** @brief Free the sprites added since a sprite (the textures of the models loaded since, their tiles and smaller copies,
 *  their gouraud tables, the sprites added by the program): the VRAM of the VDP1 is given back
 *  @param sprite_id First sprite to free (jo_sprite_count() before the models were loaded)
 *  @remarks Call it after the models were freed (jo_3d_free_model()): a level loaded and freed again and again, with
 *  its models, would use up the VRAM otherwise (the gouraud tables of a model are new sprites at each load)
 */
void                                        jo_3d_model_free_textures_from(const int sprite_id);

/** @brief Find a group of a model
 *  @param model Model
 *  @param name Name of the group (OBJ o or g)
 *  @return The group, JO_NULL if not found
 */
const jo_3d_model_group                     *jo_3d_model_find_group(const jo_3d_model * const model, const char * const name);

/** @brief Find a bone of a model
 *  @param model Model
 *  @param name Name of the bone
 *  @return Index of the bone, -1 if not found
 */
int                                         jo_3d_model_find_bone(const jo_3d_model * const model, const char * const name);

/** @brief Find an animation of a model
 *  @param model Model
 *  @param name Name of the animation
 *  @return Index of the animation, -1 if not found
 */
int                                         jo_3d_model_find_animation(const jo_3d_model * const model, const char * const name);

/** @brief Check if an animation is the one jo_3d_model_find_animation() finds for a name (faster: the name of this one is
 *  compared, the ones before it only when it matches)
 *  @param model Model
 *  @param animation Index of the animation
 *  @param name Name (case insensitive)
 *  @return true if jo_3d_model_find_animation(model, name) == animation
 */
bool                                        jo_3d_model_is_animation(const jo_3d_model * const model, const int animation, const char * const name);

/** @brief Duration of an animation
 *  @param model Model
 *  @param animation Index of the animation
 *  @return Ticks (1/60 s): a loop goes back to its first key at this time, the other animations stay on their last key
 */
int                                         jo_3d_model_animation_duration(const jo_3d_model * const model, const int animation);

/** @brief Pose of a skeleton at a time of an animation (the keys are interpolated)
 *  @param model Model
 *  @param animation Index of the animation, -1: the bones don't turn (the pose of the OBJ)
 *  @param time Ticks (1/60 s) from the start of the animation
 *  @param pose Pose
 */
void                                        jo_3d_model_get_pose(const jo_3d_model * const model, const int animation, int time,
                                                                 jo_3d_pose * const pose);

/** @brief Blend 2 poses (a smooth change of animation)
 *  @param model Model
 *  @param pose First pose, the result
 *  @param other Second pose
 *  @param weight Weight of the second pose (0 to 256)
 */
void                                        jo_3d_pose_blend(const jo_3d_model * const model, jo_3d_pose * const pose,
                                                             const jo_3d_pose * const other, const int weight);

/** @brief Draw a model with a skeleton with the current matrix
 *  @param model Model
 *  @param pose Pose (JO_NULL: the pose of the OBJ)
 *  @param callback Called after each bone is drawn, with the matrix of the bone (JO_NULL: none)
 *  @param param Parameter of the callback
 */
void                                        jo_3d_model_draw_pose(const jo_3d_model * const model, const jo_3d_pose * const pose,
                                                                  const jo_3d_bone_callback callback, void * const param);

/** @brief Draw a model with a skeleton in a pose on the slave CPU (call it on the slave CPU, see jo_3d_model_draw_pose())
 *  @param model Model
 *  @param pose Pose (JO_NULL: the pose of the OBJ)
 *  @param callback Called after each bone (JO_NULL: none; on the slave CPU: what it writes is read by the master CPU
 *         without its cache)
 *  @param param Parameter of the callback
 *  @param hidden_bones Bones not drawn (bits, see jo_3d_model_set_hidden_bones())
 *  @remarks The tint of the slave CPU (see jo_3d_slave_set_tint())
 */
void                                        jo_3d_slave_model_draw_pose(const jo_3d_model * const model, const jo_3d_pose * const pose,
                                                                        const jo_3d_bone_callback callback, void * const param,
                                                                        const unsigned int hidden_bones);

/** @brief Bones not drawn by the next jo_3d_model_draw_pose() (a hat shot off the head of a character: its bone hidden, the hat
 *  drawn alone where it falls: the other bones hidden)
 *  @param mask Bit k: the polygons of the bone k hidden (its children are drawn), 0: all drawn
 */
void                                        jo_3d_model_set_hidden_bones(const unsigned int mask);

/** @brief Move what the drawing of a model reads for each vertex, polygon and bone (its points, its polygons, their
 *  attributes, its bones and their groups) to the high work RAM, when it is in a slower memory (the low work RAM, a RAM
 *  cartridge)
 *  @param model Model loaded by jo_3d_load_model() (its data in the file: the copies replace them, the file stays)
 *  @return true if the model reads them in the high work RAM now (moved, or already there), false: not enough free high
 *          work RAM (the model is unchanged)
 *  @remarks For the models drawn in many copies at each frame (the characters of a crowd): the high work RAM is full after
 *           the rooms of a level, the characters loaded after them are read in the low work RAM. Measured (FPS demo, 9
 *           scientists in front of the camera): the slave CPU drew them in 23.8 ms, 20.7 ms with their models in the high
 *           work RAM
 *  @remarks jo_malloc() takes the high work RAM first: it is often full after the loading of a level. Keep some for this
 *           (jo_malloc() a block before the loading, jo_free() it just before this call)
 *  @remarks The animations stay in the file (only 2 keys are read per pose)
 */
bool                                        jo_3d_model_use_fast_memory(jo_3d_model * const model);

/** @brief Draw a model with the current matrix
 *  @param model Model
 *  @remarks A model with a skeleton is drawn in the pose of the OBJ (see jo_3d_model_draw_pose())
 */
static  __jo_force_inline void              jo_3d_model_draw(const jo_3d_model * const model)
{
    if (model->bone_count > 0)
        jo_3d_model_draw_pose(model, JO_NULL, JO_NULL, JO_NULL);
    else
        jo_3d_mesh_draw(model->mesh);
}

#endif /* !JO_COMPILE_WITH_3D_SUPPORT */

#endif /* !__JO_MODEL_H__ */

/*
** END OF FILE
*/
