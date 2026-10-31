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
/** @file light.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine point lights: light sources (a position, a color, a radius, a strength; all around like a bulb or in a
 *  cone like a torch) that light the static meshes (in world coordinates: the rooms of the cells, the buildings...)
 *  through their gouraud tables (no sprite: the colors of the corners of the polygons)
 *
 *  - The light of a source on a corner of a polygon: strength x facing x radius^2 / (radius^2 + distance^2) (facing: the
 *    face turned toward the light, both sides), up to 4 x radius. With shadows (jo_3d_point_lights_set_occlusion()), a
 *    ray from the light to the corner: a wall between them stops it (the light goes through the doors)
 *  - The light is computed once for each corner (when a light or a mesh is added, or a light moves); a change of the
 *    intensity, of the color or a light switched on or off only writes the gouraud tables of the polygons it lights:
 *    a light can flicker or pulse at each frame for little CPU time
 *  - The gouraud tables of the meshes keep their colors (the light baked in them): the lights are added to them
 *  - Only the changes are computed (the difference of the light of a light on its polygons), only on the meshes seen;
 *    the tables are written in a copy in the work RAM, copied into the VRAM by the DMA controller (the CPU waits a long
 *    time for the VRAM of the VDP1 while it draws)
 *  - Usage: jo_3d_point_lights_add_mesh() for each mesh (or jo_cells_add_point_light_meshes()), jo_3d_point_light_add()
 *    for each light, then jo_3d_point_lights_update() at each frame. jo_3d_point_lights_clear() before the meshes are freed
 *  - Option: jo_3d_point_lights_enable(false) gives the meshes their colors back (no point light)
 */

#ifndef __JO_LIGHT_H__
# define __JO_LIGHT_H__

#ifdef JO_COMPILE_WITH_3D_SUPPORT

/** @brief Point lights at most */
# define JO_MAX_POINT_LIGHTS                (8)
/** @brief Meshes lit by the point lights at most */
# define JO_MAX_POINT_LIGHT_MESHES          (64)
/** @brief Strength of a light that doubles the color of a polygon in front of it (gouraud: 16 is the color of the texture,
 *  a light of this strength adds 16) */
# define JO_POINT_LIGHT_STRENGTH_DEFAULT    (256)

/** @brief Add a point light
 *  @param position Position (world, Q16.16)
 *  @param r Red (0 to 255)
 *  @param g Green (0 to 255)
 *  @param b Blue (0 to 255)
 *  @param radius Distance where the light is half of its strength (world units); it stops at 4 x radius
 *  @param strength Light in front of it (JO_POINT_LIGHT_STRENGTH_DEFAULT: the color doubled)
 *  @return Id of the light, -1 if there are too many lights
 */
int                                 jo_3d_point_light_add(const jo_pos3D_fixed * const position, const unsigned char r, const unsigned char g,
                                                          const unsigned char b, const int radius, const int strength);

/** @brief Intensity of a light (a flicker, a pulse: cheap)
 *  @param id Id of the light
 *  @param intensity 0 (off) to 256 (its strength)
 */
void                                jo_3d_point_light_set_intensity(const int id, const int intensity);

/** @brief Color of a light (cheap)
 *  @param id Id of the light
 *  @param r Red (0 to 255)
 *  @param g Green (0 to 255)
 *  @param b Blue (0 to 255)
 */
void                                jo_3d_point_light_set_color(const int id, const unsigned char r, const unsigned char g, const unsigned char b);

/** @brief Move a light (its light is computed again on all the meshes: costly)
 *  @param id Id of the light
 *  @param position Position (world, Q16.16)
 */
void                                jo_3d_point_light_set_position(const int id, const jo_pos3D_fixed * const position);

/** @brief A spot light (a torch, a lamp with a shade): the light goes in a cone; or all around (a bulb, the default)
 *  @param id Id of the light
 *  @param direction Direction of the cone (world, unit vector Q16.16), JO_NULL: all around (a bulb)
 *  @param angle Angle of the cone (degrees, 1 to 179): full light in its inner 3/4, none outside of it
 *  @remarks The light is computed again on all the meshes (costly, like jo_3d_point_light_set_position())
 */
void                                jo_3d_point_light_set_spot(const int id, const jo_pos3D_fixed * const direction, const int angle);

/** @brief Shadows of a light (on by default when jo_3d_point_lights_set_occlusion() is set): off for a light that moves
 *  at each frame (a muzzle flash, an explosion): its light is computed again faster (no ray)
 *  @param id Id of the light
 *  @param shadows true: the walls stop its light
 */
void                                jo_3d_point_light_set_shadows(const int id, const bool shadows);

/** @brief Light of a light seen by jo_3d_point_lights_sample() (the objects, the weapon of the player): on by default; off
 *  for a light that is only a sheen on the walls (a torch on the camera would light the weapon in front of it)
 *  @param id Id of the light
 *  @param sampled true: jo_3d_point_lights_sample() adds its light
 */
void                                jo_3d_point_light_set_sampled(const int id, const bool sampled);

/** @brief Switch a light on or off (cheap)
 *  @param id Id of the light
 *  @param enabled true: on
 */
void                                jo_3d_point_light_enable(const int id, const bool enabled);

/** @brief The point lights on or off (off: the meshes get their colors back)
 *  @param enabled true: on (the default)
 */
void                                jo_3d_point_lights_enable(const bool enabled);

/** @brief Add a mesh lit by the point lights: its points in world coordinates (a mesh drawn without a matrix of its own),
 *  its polygons with gouraud tables (JO_VDP1_COLOR_CALC_GOURAUD). Its gouraud tables are read now: their colors are kept
 *  @param mesh Mesh
 *  @return true if added
 *  @remarks Each polygon needs its own table (the light of each polygon is written in it): the polygons of a mesh that share
 *           a table (a model loaded by jo_3d_load_model(): the same colors, one table) get a table each here, in a new block
 *           of the VRAM (the models of the cells: loaded with a table per polygon)
 */
bool                                jo_3d_point_lights_add_mesh(jo_3d_mesh * const mesh);

/** @brief A mesh is seen or not in this frame (for instance the cells drawn, see jo_cells_prepare()): the light changes of
 *  the meshes not seen are applied when they are seen again (a light that pulses costs nothing in the rooms not seen)
 *  @param mesh Mesh (added by jo_3d_point_lights_add_mesh())
 *  @param seen true: seen (the default)
 */
void                                jo_3d_point_lights_set_mesh_seen(const jo_3d_mesh * const mesh, const bool seen);

/** @brief Shadows: a function that tells if a ray hits a wall (for instance the walls of the cells, see
 *  jo_cells_add_point_light_meshes()), JO_NULL: no shadow
 *  @param hit The ray from origin along dir (unit vector) hits something before max_t (Q16.16)
 */
void                                jo_3d_point_lights_set_occlusion(bool (*hit)(const jo_pos3D_fixed * const origin, const jo_pos3D_fixed * const dir,
                                                                                 const jo_fixed max_t));

/** @brief Write the gouraud tables changed by the lights (computes the light of the lights added or moved first): once
 *  per frame, before the meshes are drawn */
void                                jo_3d_point_lights_update(void);

/** @brief The light of the lights at a point (for an object that moves: a character, a weapon, see jo_3d_set_tint())
 *  @param position Point (world)
 *  @param r Red added (gouraud units: 16 doubles a color)
 *  @param g Green added
 *  @param b Blue added
 *  @remarks The walls between a light with shadows and the point stop its light (a ray for each light in reach)
 */
void                                jo_3d_point_lights_sample(const jo_pos3D_fixed * const position, int * const r, int * const g, int * const b);

/** @brief Remove the lights and the meshes (the gouraud tables get their colors back): before the meshes are freed */
void                                jo_3d_point_lights_clear(void);

#endif /* !JO_COMPILE_WITH_3D_SUPPORT */

#endif /* !__JO_LIGHT_H__ */

/*
** END OF FILE
*/
