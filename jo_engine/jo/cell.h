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
/** @file cell.h
 *  @author Johannes Fetz
 *
 *  @brief Cells and portals: the rooms of an inside level (sections CELL and PRTL, see Documentation/level_format.md)
 *  @remarks Module JO_COMPILE_WITH_LEVEL_MODULE = 1 in the makefile
 *
 *  - A cell is a room: its model (walls, floor, ceiling in world coordinates, the light baked in the colors of the
 *    vertices) and a box where the player can walk, with a flat floor or a slope (stairs)
 *  - The cells overlap at the doors: a position is walkable when a circle around it is in the box of a cell
 *  - Drawing: the cell of the camera, then the cells seen through its portals (the rectangle of a portal on the screen
 *    gets smaller at each portal: a cell behind 2 doors is drawn only if both doors are seen). Each polygon costs CPU
 *    time: the rooms behind the walls are not drawn
 */

#ifndef __JO_CELL_H__
# define __JO_CELL_H__

#if defined(JO_COMPILE_WITH_LEVEL_SUPPORT) && defined(JO_COMPILE_WITH_3D_SUPPORT)

/** @brief Cells at most */
# define JO_CELL_MAX                        (48)
/** @brief Portals at most */
# define JO_CELL_MAX_PORTALS                (96)
/** @brief Cells drawn in a frame at most */
# define JO_CELL_MAX_DRAWN                  (12)

/** @brief A cell */
typedef struct
{
    jo_3d_mesh                      *mesh;          // Model of the cell (world coordinates), JO_NULL: none
    jo_3d_mesh                      *reflection;    // Simpler model drawn in its mirrors (a polished floor), JO_NULL: its model
    jo_pos3D_fixed                  center;         // Bounding sphere of the model
    int                             radius;
    int                             x0;             // Box where the player can walk (world units)
    int                             z0;
    int                             x1;
    int                             z1;
    int                             floor0;         // Floor (Saturn Y): see JO_LEVEL_CELL_SLOPE_xxx
    int                             floor1;
    unsigned short                  slope;
    unsigned short                  first_portal;
    unsigned short                  portal_count;
    short                           yaw;            // The box is turned around its center (degrees)
    jo_fixed                        cos;            // Of the yaw (0 and 1: not turned)
    jo_fixed                        sin;
}                                   jo_cell;

/** @brief A portal: an opening of a cell toward another cell */
typedef struct
{
    jo_pos3D_fixed                  points[4];      // Corners (world space)
    unsigned short                  cell;           // Cell seen through it
    unsigned short                  kind;           // JO_LEVEL_PORTAL_xxx
}                                   jo_cell_portal;

/** @brief Create the cells of a level (sections CELL and PRTL): the previous ones are freed
 *  @param level Level (it can be freed after)
 *  @return Number of cells (0: the level has none, an outside level)
 */
int                                 jo_cells_load(jo_level * const level);

/** @brief Free the cells (and their models) */
void                                jo_cells_free(void);

/** @brief Number of cells */
int                                 jo_cells_count(void);

/** @brief Check if a cell has a door or a window to the outside (the level is not an inside level: there is a world
 *  around the cells)
 */
bool                                jo_cells_have_exits(void);

/** @brief A cell
 *  @param index Index (0 to jo_cells_count() - 1)
 */
const jo_cell                       *jo_cells_get(const int index);

/** @brief Rectangle of the screen where the outside is seen (after jo_cells_prepare()): the portals of the cells seen
 *  that go to the outside (PRTL.cell NO_INDEX: the door of a building on a terrain). Give it to jo_3d_set_view_rect():
 *  only the objects of the outside seen through the doors are drawn
 *  @param rect Rectangle (output, when the result is 1)
 *  @return -1: the camera is outside of the cells (the whole screen), 0: the outside is not seen, 1: seen in rect
 */
int                                 jo_cells_outside_view(jo_3d_rect * const rect);

/** @brief Floor of a cell at a position (its slope)
 *  @param cell Cell
 *  @param x Position (world)
 *  @param z Position (world)
 *  @return Floor (Saturn Y, world units)
 */
int                                 jo_cell_floor(const jo_cell * const cell, const int x, const int z);

/** @brief Cell of a position: its box has the position, its floor is under it (the highest one)
 *  @param x Position (world, Q16.16)
 *  @param y Position: the feet (Saturn Y, Q16.16)
 *  @param z Position (world, Q16.16)
 *  @param step The floor can be this much over the feet (a step of the stairs, world units)
 *  @return Index of the cell, -1 if none
 */
int                                 jo_cells_find(const jo_fixed x, const jo_fixed y, const jo_fixed z, const int step);

/** @brief Floor under a position
 *  @param x Position (world, Q16.16)
 *  @param y Position: the feet (Saturn Y, Q16.16)
 *  @param z Position (world, Q16.16)
 *  @param step The floor can be this much over the feet (world units)
 *  @param floor Floor (output, Saturn Y, world units)
 *  @return false if there is no floor (outside of the cells)
 */
bool                                jo_cells_floor(const jo_fixed x, const jo_fixed y, const jo_fixed z, const int step, int * const floor);

/** @brief Check if the player can stand at a position: a circle around it is in the box of a cell, the floor of the cell
 *  is under the feet (or a step over them)
 *  @param x Position (world, Q16.16)
 *  @param y Position: the feet (Saturn Y, Q16.16)
 *  @param z Position (world, Q16.16)
 *  @param radius Radius of the player (world units)
 *  @param step The floor can be this much over the feet (world units)
 */
bool                                jo_cells_is_walkable(const jo_fixed x, const jo_fixed y, const jo_fixed z, const int radius, const int step);

/** @brief Draw the cells seen from the camera (the camera matrix is the current matrix, see jo_3d_set_depth_bias())
 *  @param camera Position of the camera (world)
 *  @return Number of cells drawn
 *  @remarks Outside of the cells (in a wall), all the cells in the view are drawn
 */
int                                 jo_cells_draw(const jo_pos3D_fixed * const camera);

/** @brief Find the cells seen from the camera (the camera matrix is the current matrix): jo_cells_draw() in 2 parts, to
 *  draw them on the slave CPU (the master CPU finds them, the slave CPU draws them while the master CPU draws the rest)
 *  @param camera Position of the camera (world)
 *  @return Number of cells seen
 */
int                                 jo_cells_prepare(const jo_pos3D_fixed * const camera);

/** @brief Draw the cells found by jo_cells_prepare()
 *  @param slave true: on the slave CPU (between jo_3d_slave_begin() and jo_3d_slave_end(), see jo_3d_slave_mesh_draw()),
 *  false: on the master CPU
 *  @remarks The depth bias of jo_cells_prepare() is used, plus 2048 for each portal between a cell and the cell of the
 *  camera (its walls hide the cells behind them)
 */
void                                jo_cells_draw_prepared(const bool slave);

/** @brief Draw a part of the cells found by jo_cells_prepare(): the 2 CPUs draw them together (the meshes split in 2 parts
 *  with about as many polygons, the same split on both CPUs)
 *  @param slave true: on the slave CPU (see jo_cells_draw_prepared())
 *  @param part 0 or 1 (-1: all)
 *  @param handicap Polygons counted in the part 1 before the cells (the other work of its CPU: the part 1 gets fewer)
 */
void                                jo_cells_draw_prepared_part(const bool slave, const int part, const int handicap);

/** @brief Draw a part of the cells found by jo_cells_prepare(): the 2 CPUs draw them together, the polygons split at a share
 *  (a mesh can be split: the CPUs can be balanced at each frame, see jo_core_slave_idle_time())
 *  @param slave true: on the slave CPU (see jo_cells_draw_prepared())
 *  @param part 0 (the polygons before the cut) or 1 (after it)
 *  @param share The cut: the share of the polygons in the part 0 (0 to 1024), the same on both CPUs
 *  @remarks A polygon sorted with the previous one (JO_3D_SORT_PREVIOUS: a decal) stays in the part of the previous one
 */
void                                jo_cells_draw_prepared_share(const bool slave, const int part, const int share);

/** @brief Share of the polygons of the cells drawn by the master CPU at the start (0 to 1024, see jo_cells_draw_prepared_on_both_cpus()) */
# define JO_CELLS_DEFAULT_SHARE              (400)
/** @brief Highest share of the master CPU by default (the slave CPU keeps a part: it waits for nothing else) */
# define JO_CELLS_DEFAULT_MAX_SHARE          (896)

/** @brief Draw the cells found by jo_cells_prepare() on the 2 CPUs: the slave CPU draws its part (a job, see
 *  jo_core_exec_on_slave()) while the master CPU draws the other part. The share of each CPU follows the load of the last
 *  frames (see jo_cells_balance_share())
 *  @remarks Call it between jo_3d_slave_begin() and jo_3d_slave_end(), then jo_cells_balance_share() after
 *  jo_core_wait_for_slave(). The master CPU draws the first part: its commands come first in the table, 2 polygons at the same
 *  depth are drawn in the same order as with one CPU
 *  @remarks Without JO_COMPILE_WITH_DUAL_CPU_MODULE: the master CPU draws all the cells
 */
void                                jo_cells_draw_prepared_on_both_cpus(void);

/** @brief Balance the share of the CPUs for the next frame (see jo_cells_draw_prepared_on_both_cpus()): more polygons
 *  to the slave CPU when it waited in the last frame, fewer when the master CPU waited for it (or when its command table
 *  is nearly full: its last polygons would be lost)
 *  @remarks Call it after jo_core_wait_for_slave(), once per frame
 */
void                                jo_cells_balance_share(void);

/** @brief Limits of the share of the master CPU (see jo_cells_draw_prepared_on_both_cpus())
 *  @param min Lowest share of the master CPU (0 to 1024, 0: the slave CPU can draw all the cells)
 *  @param max Highest share (0 to 1024, JO_CELLS_DEFAULT_MAX_SHARE by default)
 */
void                                jo_cells_set_share_limits(const int min, const int max);

/** @brief Share of the polygons of the cells drawn by the master CPU in the next frame (0 to 1024) */
int                                 jo_cells_get_share(void);

/** @brief Draw the mirror seen by the camera (in the cell of the camera or in a cell seen through a door), when
 *  jo_cells_prepare() found one (a portal MIRROR): its cell is
 *  drawn again with the camera reflected by the plane of the mirror, in a far window (JO_3D_FAR_WINDOW, then
 *  JO_3D_FAR_WINDOW_2 for a second mirror: a polished floor and a mirror on a wall seen together) clipped by the VDP1 to the
 *  rectangle of the mirror on the screen. On the master CPU, the camera matrix is the current matrix
 *  @param draw_more The game draws the other things seen in the mirror (the entities, the model of the player...) with
 *  the reflected matrix, JO_NULL: nothing
 *  @return true if a mirror is drawn
 *  @remarks The wall has a hole for the mirror (the reflected room is behind it). The windows 1 and 2 of the VDP1 are
 *  used (a rear-view mirror can't be drawn in the same frame). The cost: the polygons of the cell once more per mirror
 */
bool                                jo_cells_draw_mirror(void (*draw_more)(void));

/** @brief The mirror drawn now (in draw_more of jo_cells_draw_mirror()) is a polished floor (a horizontal portal MIRROR under a half transparent floor), not a mirror
 *  on a wall (in draw_more of jo_cells_draw_mirror(): the player does not see his own body under him)
 *  @return true for a floor
 */
bool                                jo_cells_mirror_is_floor(void);

/** @brief Distance from the outside under which a cell is seen through its windows (portals JO_LEVEL_PORTAL_WINDOW to the
 *  outside): farther, the cell is not drawn through them (a window is small on the screen, its cell would cost a lot of
 *  CPU time for a few pixels: the model outside can show something dark in the window instead)
 *  @param distance Horizontal distance between the camera and the center of the window (units), 0: never (default)
 */
void                                jo_cells_set_window_distance(const int distance);

/** @brief The rectangle of the screen where a point is seen (the cell of the point seen through the doors, see
 *  jo_cells_prepare(): after it). For an effect drawn over the 3D view that the walls don't hide (a glow, see
 *  jo_3d_glows_set_clip()): it is seen only in this rectangle
 *  @param position Point (world)
 *  @param rect Rectangle (output: the whole screen if the point and the camera are in the same cell, or both outside)
 *  @return false if the cell of the point is not drawn (or the outside is not seen)
 */
bool                                jo_cells_view_rect_at(const jo_pos3D_fixed * const position, jo_3d_rect * const rect);

/** @brief The rectangle of the screen where a cell is seen from the outside (through its doors and its near windows, see
 *  jo_cells_set_window_distance()), after jo_cells_prepare(): the program can draw what is in the cell in a window
 *  clipped to this rectangle (jo_3d_window(), jo_3d_window_clip()): the polygons outside of it are not drawn
 *  @param index Cell
 *  @param rect Rectangle (output)
 *  @return false if the camera is in a cell, or the cell is not seen
 */
bool                                jo_cells_seen_through(const int index, jo_3d_rect * const rect);

/** @brief Check if a cell is seen from the outside through one of its windows in this frame (see
 *  jo_cells_set_window_distance(); or from its doorway: the whole screen)
 *  @param index Cell
 *  @return false if it is seen through its doors only, or not seen
 */
bool                                jo_cells_seen_through_window(const int index);

/** @brief Check if a cell was drawn in the last call of jo_cells_draw() or jo_cells_prepare() (the objects in the other
 *  cells are hidden). The camera outside of the cells: a cell with doors to the outside is drawn when one of them is seen
 *  @param index Cell (-1: true)
 */
bool                                jo_cells_is_drawn(const int index);

/** @brief Close the portals of a door (JO_LEVEL_PORTAL_DOOR) whose middle is near a point: the cells behind them are not
 *  drawn through them (a closed door: its panel hides the room behind it), or open them again
 *  @param x Point (world space)
 *  @param z Point
 *  @param radius Distance on X and on Z (world units)
 *  @param closed true: closed
 *  @return Number of portals changed (the portals of both sides)
 */
int                                 jo_cells_close_portals(const jo_fixed x, const jo_fixed z, const int radius, const bool closed);

/** @brief Depth bias of an object in the cells (see jo_cells_draw()): the cells seen through more portals are drawn behind,
 *  the objects in them too (else they are seen through the walls of the nearer cells)
 *  @param x Position of the object
 *  @param y Position of the object (its feet)
 *  @param z Position of the object
 *  @return Bias to add to the depth of the object (0: outside of the cells), -1: in a cell that is not drawn (hidden)
 */
int                                 jo_cells_depth_bias_at(const jo_fixed x, const jo_fixed y, const jo_fixed z);

/** @brief Cell of an object (see jo_cells_depth_bias_at())
 *  @param x Position of the object
 *  @param y Position of the object (its feet)
 *  @param z Position of the object
 *  @return Index of the cell, -1: outside of the cells (or no cells)
 *  @remarks The cell of a point doesn't change until the cells change (see jo_cells_generation()): an object that doesn't
 *           move can keep it
 */
int                                 jo_cells_object_cell(const jo_fixed x, const jo_fixed y, const jo_fixed z);

/** @brief Depth bias of an object in a cell (see jo_cells_depth_bias_at())
 *  @param cell Cell of the object (jo_cells_object_cell()), -1: outside of the cells
 *  @return Bias to add to the depth of the object (0: outside of the cells), -1: in a cell that is not drawn (hidden)
 */
int                                 jo_cells_depth_bias_of(const int cell);

/** @brief Generation of the cells: it changes when cells are loaded or freed (the cells found before are then stale)
 *  @return Generation (never 0)
 */
unsigned short                      jo_cells_generation(void);

/** @brief First wall (polygon of a cell) hit by a ray (the bullets)
 *  @param origin Origin (world)
 *  @param dir Direction (unit vector, Q16.16)
 *  @param max_t Length of the ray (world units, Q16.16)
 *  @param t Distance of the hit (output, Q16.16)
 *  @param normal Normal of the polygon hit (output, unit vector)
 *  @return true if a polygon is hit
 */
bool                                jo_cells_ray_hit(const jo_pos3D_fixed * const origin, const jo_pos3D_fixed * const dir, const jo_fixed max_t,
                                                     jo_fixed * const t, jo_pos3D_fixed * const normal);

/** @brief The models of the cells are lit by the point lights (see jo/light.h): their meshes are added, and with shadows
 *  the walls of the cells stop the light (it goes through the doors)
 *  @param shadows true: the walls stop the light (a ray for each corner near a light, at the loading)
 *  @return Meshes added
 */
int                                 jo_cells_add_point_light_meshes(const bool shadows);

#endif /* !JO_COMPILE_WITH_LEVEL_SUPPORT && JO_COMPILE_WITH_3D_SUPPORT */

#endif /* !__JO_CELL_H__ */

/*
** END OF FILE
*/
