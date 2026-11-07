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
/** @file internal/3d.h
 *  @author Johannes Fetz
 *
 *  @brief 3D: internal functions of the engine (see jo/3d.h)
 *  @warning Not part of the API: only for the source files of the engine (included after jo/3d.h). The games
 *  include jo/jo.h, which doesn't include this file
 *  @bug No known bugs.
 */

#ifndef __JO_INTERNAL_3D_H__
# define __JO_INTERNAL_3D_H__

#ifdef JO_COMPILE_WITH_3D_SUPPORT

/** @brief Draw polygons whose points are in parts, each one transformed by its own matrix: the bones of a skeleton
 *  (one draw for the whole model, see jo_3d_model_draw_pose()) (Internal usage)
 *  @param pdata Polygon data (vertices, polygons and attributes)
 *  @param parts Parts (the points not in a part are not transformed: the polygons must not use them)
 *  @param part_count Number of parts
 *  @return false if the VDP1 command table is full
 */
bool                                __jo_3d_draw_parts(const jo_3d_polygon_data * const pdata, const jo_3d_part * const parts,
                                                       const int part_count);

/** @brief Draw the parts of a mesh on the slave CPU (see __jo_3d_draw_parts(), internal usage) */
bool                                __jo_3d_slave_draw_parts(const jo_3d_polygon_data * const pdata, const jo_3d_part * const parts,
                                                       const int part_count);

/** @brief The matrix of a bone (Internal usage, see jo_3d_model_draw_pose()): the matrix of its parent turned around its
 *  joint (the same operations as jo_3d_translate_matrix_fixed() and __jo_3d_rotate() on the current matrix)
 *  @param out Matrix of the bone (output)
 *  @param parent Matrix of its parent
 *  @param pivot Joint
 *  @param angles Around X, Y and Z
 */
void                                __jo_3d_bone_matrix(jo_fixed (* const out)[3], const jo_fixed (* const parent)[3],
                                                        const jo_pos3D_fixed * const pivot, const jo_angle * const angles);

/** @brief The current matrix (Internal usage): 4 rows of 3 */
jo_fixed                            (*__jo_3d_current_matrix(void))[3];

/** @brief The current matrix of the slave CPU (Internal usage): 4 rows of 3 */
jo_fixed                            (*__jo_3d_slave_current_matrix(void))[3];

/** @brief Forget the tiles and the smaller copies of the textures freed from a sprite (Internal usage, see
 *  jo_3d_model_free_textures_from())
 *  @param sprite_id First sprite freed
 */
void                                __jo_3d_forget_sprites_from(const int sprite_id);

/** @brief Forget the tinted copies of the gouraud tables of a mesh: its polygons got other tables (Internal usage, see
 *  jo_3d_point_lights_add_mesh())
 *  @param pdata Polygons of the mesh
 */
void                                __jo_3d_forget_tint(const jo_3d_polygon_data * const pdata);

/** @brief Forget the environment map of a mesh freed (Internal usage, see jo_3d_free_mesh()) */
void                                __jo_3d_forget_environment(const jo_3d_polygon_data * const pdata);

/** @brief Sprites freed from a sprite (see jo_sprite_free_from()): the shared sprite of the gouraud tables is dropped
 *  if it is among them
 *  @param sprite_id First sprite freed
 */
void                                __jo_3d_gouraud_pool_freed(const int sprite_id);

#endif /* !JO_COMPILE_WITH_3D_SUPPORT */

#endif /* !__JO_INTERNAL_3D_H__ */

/*
** END OF FILE
*/
