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
/** @file 3d.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine 3D basics
 *  @bug No known bugs.
 */

#ifndef __JO_3D_H__
# define __JO_3D_H__

#ifdef JO_COMPILE_WITH_3D_SUPPORT

/** @brief Plane vertice table */
# define JO_3D_PLANE_VERTICES(SIZE) \
{ \
  { -SIZE, -SIZE, 0 }, \
  { SIZE, -SIZE, 0 }, \
  { SIZE, SIZE, 0 }, \
  { -SIZE, SIZE, 0 } \
}

/** @brief Triangle vertice table */
# define JO_3D_TRIANGLE_VERTICES(SIZE) \
{ \
  { -SIZE, -SIZE, 0 }, \
  { SIZE, -SIZE, 0 }, \
  { SIZE, SIZE, 0 } \
}

/** @brief Cube vertice table */
# define JO_3D_CUBE_VERTICES(SIZE) \
{ \
  { -SIZE, -SIZE, SIZE }, \
  { SIZE, -SIZE, SIZE }, \
  { SIZE, SIZE, SIZE }, \
  { -SIZE, SIZE, SIZE }, \
  { -SIZE, -SIZE, -SIZE }, \
  { -SIZE, -SIZE, SIZE }, \
  { -SIZE, SIZE, SIZE }, \
  { -SIZE, SIZE, -SIZE }, \
  { SIZE, -SIZE, -SIZE }, \
  { -SIZE, -SIZE, -SIZE }, \
  { -SIZE, SIZE, -SIZE }, \
  { SIZE, SIZE, -SIZE }, \
  { SIZE, -SIZE, SIZE }, \
  { SIZE, -SIZE, -SIZE }, \
  { SIZE, SIZE, -SIZE }, \
  { SIZE, SIZE, SIZE }, \
  { -SIZE, -SIZE, -SIZE }, \
  { SIZE, -SIZE, -SIZE }, \
  { SIZE, -SIZE, SIZE }, \
  { -SIZE, -SIZE, SIZE }, \
  { -SIZE, SIZE, SIZE }, \
  { SIZE, SIZE, SIZE }, \
  { SIZE, SIZE, -SIZE }, \
  { -SIZE, SIZE, -SIZE } \
}

/** @brief Vertice structure */
typedef jo_pos3D    jo_vertice;

/** @brief Default perspective angle in degree (see jo_3d_perspective_angle()) */
# define JO_3D_DEFAULT_PERSPECTIVE_ANGLE    (90)

/** @brief Default display level (see jo_3d_display_level()) */
# define JO_3D_DEFAULT_DISPLAY_LEVEL        (3)

/*
** 3D model data
*/

/** @brief Polygon sort: same depth as the previous polygon */
# define JO_3D_SORT_PREVIOUS                (0)
/** @brief Polygon sort: depth of the nearest vertex */
# define JO_3D_SORT_MIN                     (1)
/** @brief Polygon sort: depth of the farthest vertex */
# define JO_3D_SORT_MAX                     (2)
/** @brief Polygon sort: average depth of the 4 vertices */
# define JO_3D_SORT_CENTER                  (3)
/** @brief Polygon sort mask */
# define JO_3D_SORT_MASK                    (3)

/** @brief Polygon hidden when seen from the back */
# define JO_3D_SINGLE_PLANE                 (0)
/** @brief Polygon always displayed */
# define JO_3D_DUAL_PLANE                   (1)

/** @brief Polygon option: no option */
# define JO_3D_NO_OPTION                    (0)
/** @brief Polygon option: textured polygon */
# define JO_3D_USE_TEXTURE                  (JO_VDP1_OPTION_USE_TEXTURE)
/** @brief Polygon option: lighting (see jo_3d_light()) */
# define JO_3D_USE_LIGHT                    (1 << 3)
/** @brief Polygon option: the color is a palette (no lighting) */
# define JO_3D_USE_PALETTE                  (1 << 5)
/** @brief No texture */
# define JO_3D_NO_TEXTURE                   (0)

/** @brief Polygon (quadrilateral, a triangle repeats the last vertex) */
typedef struct
{
    jo_pos3D_fixed      normal;         // Normal vector (unit vector) used for lighting
    unsigned short      vertices[4];    // Vertex indexes
}                       jo_3d_polygon;

/** @brief Polygon attribute */
typedef struct
{
    unsigned char       plane;          // JO_3D_SINGLE_PLANE or JO_3D_DUAL_PLANE
    unsigned char       sort;           // JO_3D_SORT_xxx | JO_3D_USE_xxx options
    unsigned short      texture_id;     // Sprite id
    unsigned short      draw_mode;      // JO_VDP1_COLOR_MODE_xxx | JO_VDP1_COLOR_CALC_xxx | ...
    unsigned short      color;          // RGB color or palette
    unsigned short      gouraud_table;  // Gouraud table address / 8
    unsigned short      direction;      // JO_VDP1_FUNCTION_xxx | flip
}                       jo_3d_polygon_attribute;

/** @brief Polygon data (3D model) */
typedef struct
{
    jo_pos3D_fixed              *points;
    unsigned int                point_count;
    jo_3d_polygon               *polygons;
    unsigned int                polygon_count;
    jo_3d_polygon_attribute     *attributes;
}                               jo_3d_polygon_data;

/** @brief Range of polygons of a mesh (see jo_3d_mesh_draw_ranges()) */
typedef struct
{
    unsigned short              first;          // Index of the first polygon
    unsigned short              count;          // Number of polygons
}                               jo_3d_polygon_range;

/** @brief Polygon attribute initializer
 *  @param PLANE JO_3D_SINGLE_PLANE or JO_3D_DUAL_PLANE
 *  @param SORT JO_3D_SORT_xxx
 *  @param TEXTURE Sprite id (JO_3D_NO_TEXTURE)
 *  @param COLOR RGB color or palette
 *  @param GOURAUD Gouraud table address / 8 (JO_VDP1_NO_GOURAUD)
 *  @param DRAW_MODE JO_VDP1_COLOR_MODE_xxx | JO_VDP1_COLOR_CALC_xxx | ...
 *  @param DIRECTION JO_VDP1_SPRITE_NO_FLIP, JO_VDP1_POLYGON, etc
 *  @param OPTIONS JO_3D_USE_xxx (JO_3D_NO_OPTION)
 */
# define JO_3D_ATTRIBUTE(PLANE, SORT, TEXTURE, COLOR, GOURAUD, DRAW_MODE, DIRECTION, OPTIONS) \
    {(PLANE), (SORT) | JO_VDP1_DIRECTION_OPTIONS(DIRECTION) | (OPTIONS), (TEXTURE), (DRAW_MODE) | JO_VDP1_DIRECTION_DRAW_MODE(DIRECTION), \
     (COLOR), (GOURAUD), JO_VDP1_DIRECTION_COMMAND(DIRECTION)}

/** @brief Polygon normal initializer
 *  @param X Normal X (float)
 *  @param Y Normal Y (float)
 *  @param Z Normal Z (float)
 */
# define JO_3D_NORMAL(X, Y, Z)              {JO_FLOAT_TO_FIXED(X), JO_FLOAT_TO_FIXED(Y), JO_FLOAT_TO_FIXED(Z)}

/** @brief Polygon vertices initializer
 *  @param A First vertex index
 *  @param B Second vertex index
 *  @param C Third vertex index
 *  @param D Fourth vertex index (same as C for a triangle)
 */
# define JO_3D_VERTICES(A, B, C, D)         {(A), (B), (C), (D)}

/** @brief Camera structure */
typedef struct
{
    union
    {
        jo_fixed    viewpoint[3];
        jo_pos3D    viewpoint_pos;
    };
    union
    {
        jo_fixed    target[3];
        jo_pos3D    target_pos;
    };
    jo_angle        z_angle;
}                   jo_camera;

/** @brief Quadrilateral shape structure */
typedef struct
{
    jo_3d_polygon_data       data;
    jo_3d_polygon_attribute        attribute;
}               jo_3d_quad;

/** @brief Mesh structure */
typedef struct
{
    jo_3d_polygon_data       data;
}               jo_3d_mesh;

/** @brief Sprite quads (internal engine usage)
 *  @warning MC Hammer: don't touch this
 */
extern jo_3d_quad *__jo_sprite_quad[JO_MAX_SPRITE];
bool    jo_3d_create_sprite_quad(const int sprite_id);


/*
 ██████╗ █████╗ ███╗   ███╗███████╗██████╗  █████╗
██╔════╝██╔══██╗████╗ ████║██╔════╝██╔══██╗██╔══██╗
██║     ███████║██╔████╔██║█████╗  ██████╔╝███████║
██║     ██╔══██║██║╚██╔╝██║██╔══╝  ██╔══██╗██╔══██║
╚██████╗██║  ██║██║ ╚═╝ ██║███████╗██║  ██║██║  ██║
 ╚═════╝╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝

*/

/*
** Internal usage
*/
/** @brief Polygon count of the current frame (Internal usage) */
extern unsigned short               __jo_3d_polygon_count;
/** @brief Vertice count of the current frame (Internal usage) */
extern unsigned short               __jo_3d_vertice_count;
/** @brief Displayed polygon count of the current frame (Internal usage) */
extern unsigned short               __jo_3d_displayed_polygon_count;
/** @brief Multiply the current matrix by a camera matrix (Internal usage)
 *  @param camera Camera position (x, y, z)
 *  @param target Point looked at by the camera (x, y, z)
 *  @param angle Rotation around the view axis
 *  @remarks Y axis goes down, so the world "up" vector is -Y
 */
void                                __jo_3d_look_at(const jo_fixed * const camera, const jo_fixed * const target, const jo_angle angle);
/** @brief Translate the current matrix (Internal usage)
 *  @param x Horizontal translation
 *  @param y Vertical translation
 *  @param z Depth translation
 */
void                                __jo_3d_translate(const jo_fixed x, const jo_fixed y, const jo_fixed z);
/** @brief Scale the current matrix (Internal usage)
 *  @param x Horizontal scale (JO_FIXED_1 = no scale)
 *  @param y Vertical scale (JO_FIXED_1 = no scale)
 *  @param z Depth scale (JO_FIXED_1 = no scale)
 */
void                                __jo_3d_scale(const jo_fixed x, const jo_fixed y, const jo_fixed z);
/** @brief X axis for __jo_3d_rotate() */
# define JO_3D_AXIS_X                       (0)
/** @brief Y axis for __jo_3d_rotate() */
# define JO_3D_AXIS_Y                       (1)
/** @brief Z axis for __jo_3d_rotate() */
# define JO_3D_AXIS_Z                       (2)
/** @brief Rotate the current matrix around an axis (Internal usage)
 *  @param axis JO_3D_AXIS_X, JO_3D_AXIS_Y or JO_3D_AXIS_Z
 *  @param angle Rotation angle (0x10000 = 360 degrees)
 */
void                                __jo_3d_rotate(const int axis, const jo_angle angle);
/** @brief Set light direction (Internal usage)
 *  @param light Light direction (unit vector) in the current matrix space
 *  @remarks The vector is transformed by the current matrix, so call it after jo_3d_camera_look_at()
 */
void                                __jo_3d_light(const jo_pos3D_fixed * const light);
/** @brief Set perspective angle (Internal usage)
 *  @param angle Horizontal field of view (0x4000 = 90 degrees)
 *  @remarks Changes the projection distance used by 3D polygons, 3D sprites and the rotation scroll screen
 */
void                                __jo_3d_set_perspective(const jo_angle angle);
/** @brief The next 3D polygons (of the master CPU) are a front layer: drawn after (over) all the other polygons, sorted
 *  among themselves with the depth divided by 2^shift (a rear-view mirror: its content is far away, but in front of the
 *  view of the camera; draw a background first)
 *  @param shift The depth is divided by 2^shift (the sort keys of the layer stay small: 1 + z / 2^shift), -1: the normal
 *  sort again
 *  @remarks The sort keys of the other polygons must stay bigger: their depth bias (see jo_3d_set_depth_bias())
 */
void                                jo_3d_set_front_layer(const int shift);

/** @brief Points of a mesh transformed by their own matrix (see __jo_3d_draw_parts()) */
typedef struct
{
    jo_fixed                        (*matrix)[3];   // 4 rows of 3 (the matrix of the camera and of the part)
    unsigned short                  first;          // First point
    unsigned short                  count;          // Number of points
}                                   jo_3d_part;

/** @brief Draw polygons using the current matrix (Internal usage)
 *  @param pdata Polygon data (vertices, polygons and attributes)
 *  @param point_count Number of points transformed (the first ones of pdata)
 *  @param ranges Ranges of polygons drawn (JO_NULL: all the polygons)
 *  @param range_count Number of ranges
 *  @return false if the VDP1 command table is full
 *  @remarks Hidden polygons (behind the camera, outside the window or seen from the back) are skipped
 */
bool                                __jo_3d_draw_ranges(const jo_3d_polygon_data * const pdata, const unsigned int point_count,
                                                        const jo_3d_polygon_range * const ranges, const int range_count);

/** @brief Draw polygons using the current matrix (Internal usage)
 *  @param pdata Polygon data (vertices, polygons and attributes)
 *  @return false if the VDP1 command table is full
 */
static  __jo_force_inline bool      __jo_3d_draw(const jo_3d_polygon_data * const pdata)
{
    return (__jo_3d_draw_ranges(pdata, pdata->point_count, JO_NULL, 0));
}

/** @brief Set the viewpoint of the camera (where the cameraman is)
 *  @param cam Pointer to a camera struct
 *  @param x X coord
 *  @param y Y coord
 *  @param z Z coord
 */
static  __jo_force_inline void      jo_3d_camera_set_viewpoint(jo_camera * const cam, const int x, const int y, const int z)
{
    cam->viewpoint_pos.x = JO_MULT_BY_65536(x);
    cam->viewpoint_pos.y = JO_MULT_BY_65536(y);
    cam->viewpoint_pos.z = JO_MULT_BY_65536(z);
}

/** @brief Set the target of the camera (where the scene is located)
 *  @param cam Pointer to a camera struct
 *  @param x X coord
 *  @param y Y coord
 *  @param z Z coord
 */
static  __jo_force_inline void      jo_3d_camera_set_target(jo_camera * const cam, const int x, const int y, const int z)
{
    cam->target_pos.x = JO_MULT_BY_65536(x);
    cam->target_pos.y = JO_MULT_BY_65536(y);
    cam->target_pos.z = JO_MULT_BY_65536(z);
}

/** @brief Set the angle of the camera
 *  @param cam Pointer to a camera struct
 *  @param angle Z-angle in degree
 */
static  __jo_force_inline void      jo_3d_camera_set_z_angle(jo_camera * const cam, const int angle)
{
    cam->z_angle = jo_DEGtoANG_int(angle);
}

/** @brief Initialize the camera with default values
 *  @param cam Pointer to a camera struct
 */
static  __jo_force_inline void      jo_3d_camera_init(jo_camera * const cam)
{
    jo_3d_camera_set_viewpoint(cam, 0, 0, -150);
    jo_3d_camera_set_target(cam, 0, 0, 0);
    jo_3d_camera_set_z_angle(cam, 0);
}

/** @brief look at the camera
 *  @param cam Pointer to a camera struct
 */
static  __jo_force_inline void      jo_3d_camera_look_at(jo_camera * const cam)
{
    __jo_3d_look_at(cam->viewpoint, cam->target, cam->z_angle);
}

/*
███████╗████████╗ █████╗ ████████╗██╗███████╗████████╗██╗ ██████╗███████╗
██╔════╝╚══██╔══╝██╔══██╗╚══██╔══╝██║██╔════╝╚══██╔══╝██║██╔════╝██╔════╝
███████╗   ██║   ███████║   ██║   ██║███████╗   ██║   ██║██║     ███████╗
╚════██║   ██║   ██╔══██║   ██║   ██║╚════██║   ██║   ██║██║     ╚════██║
███████║   ██║   ██║  ██║   ██║   ██║███████║   ██║   ██║╚██████╗███████║
╚══════╝   ╚═╝   ╚═╝  ╚═╝   ╚═╝   ╚═╝╚══════╝   ╚═╝   ╚═╝ ╚═════╝╚══════╝

*/

/** @brief Get polygon count (visible and not visible)
 *  @warning This function must be called after display polygons
 *  @return polygon count
 */
static  __jo_force_inline unsigned int       jo_3d_get_polygon_count(void)
{
    return (__jo_3d_polygon_count);
}

/** @brief Get vertice count
 *  @warning This function must be called after display polygons
 *  @return vertice count
 */
static  __jo_force_inline unsigned int       jo_3d_get_vertices_count(void)
{
    return (__jo_3d_vertice_count);
}

/** @brief Get displayed polygon count
 *  @warning This function must be called after display polygons
 *  @return polygon count
 */
static  __jo_force_inline unsigned int       jo_3d_get_displayed_polygon_count(void)
{
    return (__jo_3d_displayed_polygon_count);
}

/*
 ██████╗██████╗ ███████╗ █████╗ ████████╗███████╗    ███████╗██╗███╗   ███╗██████╗ ██╗     ███████╗
██╔════╝██╔══██╗██╔════╝██╔══██╗╚══██╔══╝██╔════╝    ██╔════╝██║████╗ ████║██╔══██╗██║     ██╔════╝
██║     ██████╔╝█████╗  ███████║   ██║   █████╗      ███████╗██║██╔████╔██║██████╔╝██║     █████╗
██║     ██╔══██╗██╔══╝  ██╔══██║   ██║   ██╔══╝      ╚════██║██║██║╚██╔╝██║██╔═══╝ ██║     ██╔══╝
╚██████╗██║  ██║███████╗██║  ██║   ██║   ███████╗    ███████║██║██║ ╚═╝ ██║██║     ███████╗███████╗
 ╚═════╝╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝   ╚═╝   ╚══════╝    ╚══════╝╚═╝╚═╝     ╚═╝╚═╝     ╚══════╝╚══════╝

 ██████╗ ██████╗      ██╗███████╗ ██████╗████████╗
██╔═══██╗██╔══██╗     ██║██╔════╝██╔════╝╚══██╔══╝
██║   ██║██████╔╝     ██║█████╗  ██║        ██║
██║   ██║██╔══██╗██   ██║██╔══╝  ██║        ██║
╚██████╔╝██████╔╝╚█████╔╝███████╗╚██████╗   ██║
 ╚═════╝ ╚═════╝  ╚════╝ ╚══════╝ ╚═════╝   ╚═╝

*/

/** @brief Create a mesh programmatically from vertices
 *  @param quad_count Quad count (Quad count == Polygon count)
 *  @param vertices An array of jo_vertice (4 * quad_count)
 *  @param normals An array of jo_vector (quad_count)
 *  @return The mesh
 *  @remarks The return type can be casted to (jo_3d_mesh *)
 *  @remarks Returns JO_NULL if out of memory. The polygons are white by default
 */
jo_3d_mesh       *jo_3d_create_mesh_from_vertices_and_normals(const unsigned int quad_count, jo_vertice * const vertices, jo_vector * const normals);

/** @brief Create a mesh programmatically from vertices
 *  @param quad_count Quad count (Quad count == Polygon count)
 *  @param vertices An array of jo_vertice (4 * quad_count)
 *  @return The mesh
 *  @remarks The return type can be casted to (jo_3d_mesh *)
 */
static  __jo_force_inline jo_3d_mesh       *jo_3d_create_mesh_from_vertices(const unsigned int quad_count, jo_vertice * const vertices)
{
    return (jo_3d_create_mesh_from_vertices_and_normals(quad_count, vertices, JO_NULL));
}

/** @brief Create a mesh programmatically from vertices
 *  @param quad_count Quad count (Quad count == Polygon count)
 *  @return The mesh
 *  @remarks The return type can be casted to (jo_3d_mesh *)
 */
static  __jo_force_inline jo_3d_mesh       *jo_3d_create_mesh(const unsigned int quad_count)
{
    return (jo_3d_create_mesh_from_vertices_and_normals(quad_count, JO_NULL, JO_NULL));
}

/** @brief Create a sphere (UV sphere)
 *  @param radius Radius
 *  @param slices Number of vertical slices (around the Y axis, 3 minimum)
 *  @param stacks Number of horizontal stacks (from the top to the bottom, 2 minimum)
 *  @return The mesh (slices * stacks polygons) or JO_NULL if out of memory
 *  @remarks The polygons are white (see jo_3d_set_mesh_color(), jo_3d_set_mesh_texture(), jo_3d_set_mesh_light(), etc)
 *  @remarks The normals are computed, so the lighting works (see jo_3d_set_mesh_light())
 *  @remarks The polygons near the poles are triangles. The back faces are not drawn (single plane polygons)
 *  @remarks Free the mesh with jo_3d_free_mesh()
 *  @warning The VDP1 draws polygons and not pixels, so keep slices * stacks low (for instance 12 * 8 = 96 polygons)
 */
jo_3d_mesh       *jo_3d_create_sphere(const int radius, const unsigned int slices, const unsigned int stacks);

/** @brief Free a mesh created with jo_3d_create_mesh()
 *  @param mesh The mesh to free
 *  @remarks The mesh itself is also freed, so don't use it anymore
 */
void                        jo_3d_free_mesh(const jo_3d_mesh * const mesh);

/** @brief Create a four vertices polygon (Quadrilateral shape)
 *  @param quad Address to a jo_3d_quad allocated struct
 *  @param vertices Vertices coords (table of 4 jo_vertice)
 */
void                                jo_3d_create_plane(jo_3d_quad * const quad, jo_vertice * const vertices);

/** @brief Create a cube
 *  @param array Address to a 6 jo_3d_quad allocated struct
 *  @param vertices Vertices coords (table of 24 (6x4) jo_vertice)
 */
void                                jo_3d_create_cube(jo_3d_quad * array, jo_vertice * const vertices);

/** @brief Create a triangle
 *  @param quad Address to a jo_3d_quad allocated struct
 *  @param vertices Vertices coords (table of 3 jo_vertice)
 */
void                                jo_3d_create_triangle(jo_3d_quad * const quad, jo_vertice * const vertices);

/** @brief Get sprite quad
 *  @param sprite_id Sprite Id returned by jo_sprite_add(), jo_sprite_add_tga() or jo_sprite_add_image_pack()
 *  @return Sprite quad
 */
static  __jo_force_inline jo_3d_quad    *jo_3d_get_sprite_quad(const int sprite_id)
{
    return (__jo_sprite_quad[sprite_id] == JO_NULL && !jo_3d_create_sprite_quad(sprite_id) ? JO_NULL : __jo_sprite_quad[sprite_id]);
}

/** @brief Delete sprite quad
 *  @param sprite_id Sprite Id returned by jo_sprite_add(), jo_sprite_add_tga() or jo_sprite_add_image_pack()
 */
void                                    jo_3d_free_sprite_quad(const int sprite_id);

/*
███╗   ███╗███████╗███████╗██╗  ██╗    ████████╗███████╗██╗  ██╗████████╗██╗   ██╗██████╗ ███████╗
████╗ ████║██╔════╝██╔════╝██║  ██║    ╚══██╔══╝██╔════╝╚██╗██╔╝╚══██╔══╝██║   ██║██╔══██╗██╔════╝
██╔████╔██║█████╗  ███████╗███████║       ██║   █████╗   ╚███╔╝    ██║   ██║   ██║██████╔╝█████╗
██║╚██╔╝██║██╔══╝  ╚════██║██╔══██║       ██║   ██╔══╝   ██╔██╗    ██║   ██║   ██║██╔══██╗██╔══╝
██║ ╚═╝ ██║███████╗███████║██║  ██║       ██║   ███████╗██╔╝ ██╗   ██║   ╚██████╔╝██║  ██║███████╗
╚═╝     ╚═╝╚══════╝╚══════╝╚═╝  ╚═╝       ╚═╝   ╚══════╝╚═╝  ╚═╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝╚══════╝

*/

/** @brief Set a texture on one polygon on the mesh
 *  @param mesh Address to a jo_3d_mesh allocated struct
 *  @param sprite_id Sprite ID
 *  @param index polygon index on the mesh
 */
void                                jo_3d_set_mesh_polygon_texture(jo_3d_mesh * const mesh, const int sprite_id, const unsigned int index);

/** @brief Set a texture on the mesh
 *  @param mesh Address to a jo_3d_mesh allocated struct
 *  @param sprite_id Sprite ID
 */
void                                jo_3d_set_mesh_texture(jo_3d_mesh * const mesh, const int sprite_id);

/** @brief Set a texture on the quadrilateral
 *  @param quad Address to a jo_3d_quad allocated struct
 *  @param sprite_id Sprite ID
 */
static  __jo_force_inline void      jo_3d_set_texture(jo_3d_quad * const quad, const int sprite_id)
{
    jo_3d_set_mesh_polygon_texture((jo_3d_mesh *)quad, sprite_id, 0);
}

/** @brief Set wireframe mode on the mesh
 *  @param mesh Address to a jo_3d_mesh allocated struct
 *  @param index polygon index on the mesh
 *  @param wireframe True to enable wireframe mode
 */
void                        jo_3d_set_mesh_polygon_wireframe(jo_3d_mesh * const mesh, const unsigned int index, bool wireframe);

/*
   8 BITS PALETTE HANDLING
*/

/** @brief Set a palette on the mesh
 *  @param mesh Address to a jo_3d_mesh allocated struct
 *  @param index polygon index on the mesh
 *  @param palette_id Palette id from jo_create_palette()
 */
static  __jo_force_inline void      jo_3d_set_mesh_polygon_palette(jo_3d_mesh * const mesh, const int palette_id, const unsigned int index)
{
    mesh->data.attributes[index].color = JO_MULT_BY_256(palette_id);
}

/** @brief Set a palette on one polygon on the mesh
 *  @param mesh Address to a jo_3d_mesh allocated struct
 *  @param palette_id Palette id from jo_create_palette()
 */
static  __jo_force_inline void      jo_3d_set_mesh_palette(jo_3d_mesh * const mesh, const int palette_id)
{
    for (unsigned int i = 0; i < mesh->data.polygon_count; ++i)
        jo_3d_set_mesh_polygon_palette(mesh, palette_id, i);
}

/** @brief Set a palette on the quadrilateral
 *  @param quad Address to a jo_3d_quad allocated struct
 *  @param palette_id Palette id from jo_create_palette()
 */
static  __jo_force_inline void      jo_3d_set_palette(jo_3d_quad * const quad, const int palette_id)
{
    quad->attribute.color = JO_MULT_BY_256(palette_id);
}

/** @brief Set a palette on a sprite (3D)
 *  @param sprite_id Sprite Id returned by jo_sprite_add(), jo_sprite_add_tga() or jo_sprite_add_image_pack()
 *  @param palette_id Palette id from jo_create_palette()
 *  @return true if succeeded otherwise false
 */
static  __jo_force_inline bool      jo_3d_set_sprite_palette(const int sprite_id, const int palette_id)
{
    if (__jo_sprite_quad[sprite_id] == JO_NULL && !jo_3d_create_sprite_quad(sprite_id)) return (false);
    jo_3d_set_palette(__jo_sprite_quad[sprite_id], palette_id);
    return (true);
}

/** @brief Enable/Disable High Speed Shrink on one polygon on the mesh
 *  @param mesh Address to a jo_3d_mesh allocated struct
 *  @param enabled true to enable High Speed Shrink
 *  @param index polygon index on the mesh
 */
void                                jo_3d_set_mesh_polygon_high_speed_shrink(jo_3d_mesh * const mesh, const bool enabled, const unsigned int index);

/** @brief Enable/Disable High Speed Shrink on the mesh
 *  @param mesh Address to a jo_3d_mesh allocated struct
 *  @param enabled true to enable High Speed Shrink
 */
void                                jo_3d_set_mesh_high_speed_shrink(jo_3d_mesh * const mesh, const bool enabled);

/*
███╗   ███╗███████╗███████╗██╗  ██╗    ██╗   ██╗███████╗██████╗ ████████╗██╗ ██████╗███████╗       ██╗       ███╗   ██╗ ██████╗ ██████╗ ███╗   ███╗ █████╗ ██╗
████╗ ████║██╔════╝██╔════╝██║  ██║    ██║   ██║██╔════╝██╔══██╗╚══██╔══╝██║██╔════╝██╔════╝       ██║       ████╗  ██║██╔═══██╗██╔══██╗████╗ ████║██╔══██╗██║
██╔████╔██║█████╗  ███████╗███████║    ██║   ██║█████╗  ██████╔╝   ██║   ██║██║     █████╗      ████████╗    ██╔██╗ ██║██║   ██║██████╔╝██╔████╔██║███████║██║
██║╚██╔╝██║██╔══╝  ╚════██║██╔══██║    ╚██╗ ██╔╝██╔══╝  ██╔══██╗   ██║   ██║██║     ██╔══╝      ██╔═██╔═╝    ██║╚██╗██║██║   ██║██╔══██╗██║╚██╔╝██║██╔══██║██║
██║ ╚═╝ ██║███████╗███████║██║  ██║     ╚████╔╝ ███████╗██║  ██║   ██║   ██║╚██████╗███████╗    ██████║      ██║ ╚████║╚██████╔╝██║  ██║██║ ╚═╝ ██║██║  ██║███████╗
╚═╝     ╚═╝╚══════╝╚══════╝╚═╝  ╚═╝      ╚═══╝  ╚══════╝╚═╝  ╚═╝   ╚═╝   ╚═╝ ╚═════╝╚══════╝    ╚═════╝      ╚═╝  ╚═══╝ ╚═════╝ ╚═╝  ╚═╝╚═╝     ╚═╝╚═╝  ╚═╝╚══════╝
*/

/** @brief Set vertice position in mesh
 *  @param mesh Address to a jo_3d_mesh allocated struct
 *  @param x X position
 *  @param y Y position
 *  @param z Z position
 *  @param index vertice index on the mesh (4 vertices for each quad)
 */
static  __jo_force_inline void      jo_3d_set_mesh_vertice(jo_3d_mesh * const mesh, const jo_fixed x, const jo_fixed y, const jo_fixed z, const unsigned int index)
{
    mesh->data.points[index].x = x;
    mesh->data.points[index].y = y;
    mesh->data.points[index].z = z;
}

/** @brief Set quad normal in mesh
 *  @param mesh Address to a jo_3d_mesh allocated struct
 *  @param x X position
 *  @param y Y position
 *  @param z Z position
 *  @param index quad index on the mesh
 */
static  __jo_force_inline void      jo_3d_set_mesh_normal(jo_3d_mesh * const mesh, const jo_fixed x, const jo_fixed y, const jo_fixed z, const unsigned int index)
{
    mesh->data.polygons[index].normal.x = x;
    mesh->data.polygons[index].normal.y = y;
    mesh->data.polygons[index].normal.z = z;
}

/*
███╗   ███╗███████╗███████╗██╗  ██╗    ██╗     ██╗ ██████╗ ██╗  ██╗████████╗
████╗ ████║██╔════╝██╔════╝██║  ██║    ██║     ██║██╔════╝ ██║  ██║╚══██╔══╝
██╔████╔██║█████╗  ███████╗███████║    ██║     ██║██║  ███╗███████║   ██║
██║╚██╔╝██║██╔══╝  ╚════██║██╔══██║    ██║     ██║██║   ██║██╔══██║   ██║
██║ ╚═╝ ██║███████╗███████║██║  ██║    ███████╗██║╚██████╔╝██║  ██║   ██║
╚═╝     ╚═╝╚══════╝╚══════╝╚═╝  ╚═╝    ╚══════╝╚═╝ ╚═════╝ ╚═╝  ╚═╝   ╚═╝

*/

/** @brief Use a light source
 *  @param x X position of the light source
 *  @param y Y position of the light source
 *  @param z Z position of the light source
 */
static  __jo_force_inline void      jo_3d_light(const int x, const int y, const int z)
{
    jo_pos3D_fixed  p;

    p.x = JO_MULT_BY_65536(x);
    p.y = JO_MULT_BY_65536(y);
    p.z = JO_MULT_BY_65536(z);
    __jo_3d_light(&p);
}

/** @brief Use a light source: its direction as a unit vector
 *  @param x X of the direction (Q16.16)
 *  @param y Y of the direction (Q16.16)
 *  @param z Z of the direction (Q16.16)
 *  @remarks The vector is transformed by the current matrix, so call it after jo_3d_camera_look_at()
 */
static  __jo_force_inline void      jo_3d_light_fixed(const jo_fixed x, const jo_fixed y, const jo_fixed z)
{
    jo_pos3D_fixed  p;

    p.x = x;
    p.y = y;
    p.z = z;
    __jo_3d_light(&p);
}

/** @brief Palette lighting (the light of jo_3d_light() without the RGB colors: the only lighting of the 640 and 704 pixels
 *  modes): a lit polygon of a palette color (or a texture of 16 colors with a color bank) takes the shade of its light,
 *  its color code + shade * stride (shade 0 = the darkest)
 *  @param levels Shades of each color (2 to 32; 0 = off, the default)
 *  @param stride Color codes between 2 shades of a color (the count of jo_palette_set_light_shades(): the same for all the
 *  lit palette polygons)
 *  @remarks In the 640 and 704 pixels modes, the polygons of the RGB textures are not lit (their gouraud shading needs an
 *  RGB frame buffer)
 */
void                                jo_3d_set_palette_light(const int levels, const int stride);

/** @brief Enable/Disable light on one polygon on the mesh
 *  @param mesh Address to a jo_3d_mesh allocated struct
 *  @param enabled true to enable light support
 *  @param index polygon index on the mesh
 */
void                                jo_3d_set_mesh_polygon_light(jo_3d_mesh * const mesh, const bool enabled, const unsigned int index);

/** @brief Enable/Disable light on the mesh
 *  @param mesh Address to a jo_3d_mesh allocated struct
 *  @param enabled true to enable light support
 */
void                                jo_3d_set_mesh_light(jo_3d_mesh * const mesh, const bool enabled);

/** @brief Enable/Disable light on the quad
 *  @param quad Address to a jo_3d_quad allocated struct
 *  @param enabled true to enable light support
 */
static  __jo_force_inline void      jo_3d_set_light(jo_3d_quad *const quad, const bool enabled)
{
    jo_3d_set_mesh_polygon_light((jo_3d_mesh *)quad, enabled, 0);
}

/*
████████╗██████╗  █████╗ ███╗   ██╗███████╗██████╗  █████╗ ██████╗ ███████╗███╗   ██╗ ██████╗██╗   ██╗
╚══██╔══╝██╔══██╗██╔══██╗████╗  ██║██╔════╝██╔══██╗██╔══██╗██╔══██╗██╔════╝████╗  ██║██╔════╝╚██╗ ██╔╝
   ██║   ██████╔╝███████║██╔██╗ ██║███████╗██████╔╝███████║██████╔╝█████╗  ██╔██╗ ██║██║      ╚████╔╝
   ██║   ██╔══██╗██╔══██║██║╚██╗██║╚════██║██╔═══╝ ██╔══██║██╔══██╗██╔══╝  ██║╚██╗██║██║       ╚██╔╝
   ██║   ██║  ██║██║  ██║██║ ╚████║███████║██║     ██║  ██║██║  ██║███████╗██║ ╚████║╚██████╗   ██║
   ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝╚══════╝╚═╝     ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚═╝  ╚═══╝ ╚═════╝   ╚═╝

*/

/** @brief Enable/Disable transparency on one polygon on the mesh
 *  @param mesh Address to a jo_3d_mesh allocated struct
 *  @param enabled true to enable transparency
 *  @param index polygon index on the mesh
 */
void                                jo_3d_set_mesh_polygon_transparency(jo_3d_mesh * const mesh, const bool enabled, const unsigned int index);

/** @brief Enable/Disable transparency on the mesh
 *  @param mesh Address to a jo_3d_mesh allocated struct
 *  @param enabled true to enable transparency
 */
void                                jo_3d_set_mesh_transparency(jo_3d_mesh * const mesh, const bool enabled);

/** @brief Enable/Disable transparency on the quadrilateral
 *  @param quad Address to a jo_3d_quad allocated struct
 *  @param enabled true to enable transparency
 */
static  __jo_force_inline void      jo_3d_set_transparency(jo_3d_quad *const quad, const bool enabled)
{
    jo_3d_set_mesh_polygon_transparency((jo_3d_mesh *)quad, enabled, 0);
}

/******************************************************
*              SCREEN DOORS TRANSPARENCY
*******************************************************/

/** @brief Enable/Disable screen doors transparency on one polygon on the mesh
 *  @param mesh Address to a jo_3d_mesh allocated struct
 *  @param enabled true to enable screen doors transparency
 *  @param index polygon index on the mesh
 */
void                                jo_3d_set_mesh_polygon_screen_doors(jo_3d_mesh * const mesh, const bool enabled, const unsigned int index);

/** @brief Enable/Disable screen doors transparency on the mesh
 *  @param mesh Address to a jo_3d_mesh allocated struct
 *  @param enabled true to enable screen doors transparency
 */
void                                jo_3d_set_mesh_screen_doors(jo_3d_mesh * const mesh, const bool enabled);

/** @brief Enable/Disable screen doors transparency on the quad
 *  @param quad Address to a jo_3d_quad allocated struct
 *  @param enabled true to enable screen doors transparency
 */
static  __jo_force_inline void      jo_3d_set_screen_doors(jo_3d_quad *const quad, const bool enabled)
{
    jo_3d_set_mesh_polygon_screen_doors((jo_3d_mesh *)quad, enabled, 0);
}

/*
 ██████╗ ██████╗ ██╗      ██████╗ ██████╗ ███████╗
██╔════╝██╔═══██╗██║     ██╔═══██╗██╔══██╗██╔════╝
██║     ██║   ██║██║     ██║   ██║██████╔╝███████╗
██║     ██║   ██║██║     ██║   ██║██╔══██╗╚════██║
╚██████╗╚██████╔╝███████╗╚██████╔╝██║  ██║███████║
 ╚═════╝ ╚═════╝ ╚══════╝ ╚═════╝ ╚═╝  ╚═╝╚══════╝

*/

/** @brief Set the color on one polygon on the mesh with wireframe option
 *  @param mesh Address to a jo_3d_mesh allocated struct
 *  @param color Color
 *  @param index polygon index on the mesh
 *  @param wireframe True to display only the edge of polygons
 */
void                                jo_3d_set_mesh_polygon_color_ex(jo_3d_mesh * const mesh, const jo_color color, const unsigned int index, bool wireframe);

/** @brief Set the color on one polygon on the mesh
 *  @param mesh Address to a jo_3d_mesh allocated struct
 *  @param color Color
 *  @param index polygon index on the mesh
 */
static  __jo_force_inline void      jo_3d_set_mesh_polygon_color(jo_3d_mesh * const mesh, const jo_color color, const unsigned int index)
{
    jo_3d_set_mesh_polygon_color_ex(mesh, color, index, false);
}

/** @brief Set the color on the mesh
 *  @param mesh Address to a jo_3d_mesh allocated struct
 *  @param color Color
 */
void                                jo_3d_set_mesh_color(jo_3d_mesh * const mesh, const jo_color color);

/** @brief Set the color on the mesh
 *  @param mesh Address to a jo_3d_mesh allocated struct
 *  @param color Color
 *  @param wireframe True to enable wireframe mode
 */
void                                jo_3d_set_mesh_color_ex(jo_3d_mesh * const mesh, const jo_color color, bool wireframe);

/** @brief Set the color on the quadrilateral
 *  @param quad Address to a jo_3d_quad allocated struct
 *  @param color Color
 */
static  __jo_force_inline void      jo_3d_set_color(jo_3d_quad * const quad, const jo_color color)
{
    jo_3d_set_mesh_polygon_color((jo_3d_mesh *)quad, color, 0);
}

/*
███╗   ███╗ █████╗ ████████╗██████╗ ██╗██╗  ██╗
████╗ ████║██╔══██╗╚══██╔══╝██╔══██╗██║╚██╗██╔╝
██╔████╔██║███████║   ██║   ██████╔╝██║ ╚███╔╝
██║╚██╔╝██║██╔══██║   ██║   ██╔══██╗██║ ██╔██╗
██║ ╚═╝ ██║██║  ██║   ██║   ██║  ██║██║██╔╝ ██╗
╚═╝     ╚═╝╚═╝  ╚═╝   ╚═╝   ╚═╝  ╚═╝╚═╝╚═╝  ╚═╝
*/

/** @brief Push 3D matrix
 */
void                                jo_3d_push_matrix(void);

/** @brief Pop 3D matrix
 */
void                                jo_3d_pop_matrix(void);

/*
███╗   ███╗ █████╗ ████████╗██████╗ ██╗██╗  ██╗    ██████╗  ██████╗ ████████╗ █████╗ ████████╗██╗ ██████╗ ███╗   ██╗
████╗ ████║██╔══██╗╚══██╔══╝██╔══██╗██║╚██╗██╔╝    ██╔══██╗██╔═══██╗╚══██╔══╝██╔══██╗╚══██╔══╝██║██╔═══██╗████╗  ██║
██╔████╔██║███████║   ██║   ██████╔╝██║ ╚███╔╝     ██████╔╝██║   ██║   ██║   ███████║   ██║   ██║██║   ██║██╔██╗ ██║
██║╚██╔╝██║██╔══██║   ██║   ██╔══██╗██║ ██╔██╗     ██╔══██╗██║   ██║   ██║   ██╔══██║   ██║   ██║██║   ██║██║╚██╗██║
██║ ╚═╝ ██║██║  ██║   ██║   ██║  ██║██║██╔╝ ██╗    ██║  ██║╚██████╔╝   ██║   ██║  ██║   ██║   ██║╚██████╔╝██║ ╚████║
╚═╝     ╚═╝╚═╝  ╚═╝   ╚═╝   ╚═╝  ╚═╝╚═╝╚═╝  ╚═╝    ╚═╝  ╚═╝ ╚═════╝    ╚═╝   ╚═╝  ╚═╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝

*/

/** @brief Rotate 3D matrix using degree
 *  @param x X Angle in degree
 *  @param y Y Angle in degree
 *  @param z Z Angle in degree
 */
static  __jo_force_inline void      jo_3d_rotate_matrix(short x, short y, short z)
{
    __jo_3d_rotate(JO_3D_AXIS_X, jo_DEGtoANG_int(x));
    __jo_3d_rotate(JO_3D_AXIS_Y, jo_DEGtoANG_int(y));
    __jo_3d_rotate(JO_3D_AXIS_Z, jo_DEGtoANG_int(z));
}

/** @brief Rotate 3D matrix using degree (X axis)
 *  @param x X Angle in degree
 */
static  __jo_force_inline void      jo_3d_rotate_matrix_x(short x)
{
    __jo_3d_rotate(JO_3D_AXIS_X, jo_DEGtoANG_int(x));
}

/** @brief Rotate 3D matrix using degree (Y axis)
 *  @param y Y Angle in degree
 */
static  __jo_force_inline void      jo_3d_rotate_matrix_y(short y)
{
    __jo_3d_rotate(JO_3D_AXIS_Y, jo_DEGtoANG_int(y));
}

/** @brief Rotate 3D matrix using degree (Z axis)
 *  @param z Z Angle in degree
 */
static  __jo_force_inline void      jo_3d_rotate_matrix_z(short z)
{
    __jo_3d_rotate(JO_3D_AXIS_Z, jo_DEGtoANG_int(z));
}

/** @brief Rotate 3D matrix using radian
 *  @param x X Angle in radian
 *  @param y Y Angle in radian
 *  @param z Z Angle in radian
 */
static  __jo_force_inline void      jo_3d_rotate_matrix_rad(float x, float y, float z)
{
    __jo_3d_rotate(JO_3D_AXIS_X, JO_RAD_TO_ANGLE(x));
    __jo_3d_rotate(JO_3D_AXIS_Y, JO_RAD_TO_ANGLE(y));
    __jo_3d_rotate(JO_3D_AXIS_Z, JO_RAD_TO_ANGLE(z));
}

/** @brief Rotate 3D matrix using radian (X axis)
 *  @param x X Angle in radian
 */
static  __jo_force_inline void      jo_3d_rotate_matrix_rad_x(float x)
{
    __jo_3d_rotate(JO_3D_AXIS_X, JO_RAD_TO_ANGLE(x));
}

/** @brief Rotate 3D matrix using radian (Y axis)
 *  @param y Y Angle in radian
 */
static  __jo_force_inline void      jo_3d_rotate_matrix_rad_y(float y)
{
    __jo_3d_rotate(JO_3D_AXIS_Y, JO_RAD_TO_ANGLE(y));
}

/** @brief Rotate 3D matrix using radian (Z axis)
 *  @param z Z Angle in radian
 */
static  __jo_force_inline void      jo_3d_rotate_matrix_rad_z(float z)
{
    __jo_3d_rotate(JO_3D_AXIS_Z, JO_RAD_TO_ANGLE(z));
}

/*
███╗   ███╗ █████╗ ████████╗██████╗ ██╗██╗  ██╗    ████████╗██████╗  █████╗ ███╗   ██╗███████╗██╗      █████╗ ████████╗██╗ ██████╗ ███╗   ██╗
████╗ ████║██╔══██╗╚══██╔══╝██╔══██╗██║╚██╗██╔╝    ╚══██╔══╝██╔══██╗██╔══██╗████╗  ██║██╔════╝██║     ██╔══██╗╚══██╔══╝██║██╔═══██╗████╗  ██║
██╔████╔██║███████║   ██║   ██████╔╝██║ ╚███╔╝        ██║   ██████╔╝███████║██╔██╗ ██║███████╗██║     ███████║   ██║   ██║██║   ██║██╔██╗ ██║
██║╚██╔╝██║██╔══██║   ██║   ██╔══██╗██║ ██╔██╗        ██║   ██╔══██╗██╔══██║██║╚██╗██║╚════██║██║     ██╔══██║   ██║   ██║██║   ██║██║╚██╗██║
██║ ╚═╝ ██║██║  ██║   ██║   ██║  ██║██║██╔╝ ██╗       ██║   ██║  ██║██║  ██║██║ ╚████║███████║███████╗██║  ██║   ██║   ██║╚██████╔╝██║ ╚████║
╚═╝     ╚═╝╚═╝  ╚═╝   ╚═╝   ╚═╝  ╚═╝╚═╝╚═╝  ╚═╝       ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝╚══════╝╚══════╝╚═╝  ╚═╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝

*/

/** @brief Translate 3D matrix (using floating numbers)
 *  @param x X Position
 *  @param y Y Position
 *  @param z Z Position
 */
static  __jo_force_inline void      jo_3d_translate_matrixf(float x, float y, float z)
{
    __jo_3d_translate(JO_FLOAT_TO_FIXED(x), JO_FLOAT_TO_FIXED(y), JO_FLOAT_TO_FIXED(z));
}

/** @brief Translate 3D matrix
 *  @param x X Position
 *  @param y Y Position
 *  @param z Z Position
 */
static  __jo_force_inline void      jo_3d_translate_matrix_fixed(jo_fixed x, jo_fixed y, jo_fixed z)
{
    __jo_3d_translate(x, y, z);
}

/** @brief Translate 3D matrix
 *  @param x X Position
 *  @param y Y Position
 *  @param z Z Position
 */
static  __jo_force_inline void      jo_3d_translate_matrix(int x, int y, int z)
{
    __jo_3d_translate(jo_int2fixed(x), jo_int2fixed(y), jo_int2fixed(z));
}

/** @brief Translate 3D matrix (X axis)
 *  @param x X Position
 */
static  __jo_force_inline void      jo_3d_translate_matrix_x(int x)
{
    __jo_3d_translate(jo_int2fixed(x), 0, 0);
}

/** @brief Translate 3D matrix (Y axis)
 *  @param y Y Position
 */
static  __jo_force_inline void      jo_3d_translate_matrix_y(int y)
{
    __jo_3d_translate(0, jo_int2fixed(y), 0);
}

/** @brief Translate 3D matrix (Z axis)
 *  @param z Z Position
 */
static  __jo_force_inline void      jo_3d_translate_matrix_z(int z)
{
    __jo_3d_translate(0, 0, jo_int2fixed(z));
}

/*
██████╗ ██████╗  █████╗ ██╗    ██╗██╗███╗   ██╗ ██████╗
██╔══██╗██╔══██╗██╔══██╗██║    ██║██║████╗  ██║██╔════╝
██║  ██║██████╔╝███████║██║ █╗ ██║██║██╔██╗ ██║██║  ███╗
██║  ██║██╔══██╗██╔══██║██║███╗██║██║██║╚██╗██║██║   ██║
██████╔╝██║  ██║██║  ██║╚███╔███╔╝██║██║ ╚████║╚██████╔╝
╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝ ╚══╝╚══╝ ╚═╝╚═╝  ╚═══╝ ╚═════╝

*/

/** @brief Draw a quadrilateral
 *  @param quad Address to a jo_3d_quad allocated struct
 */
static  __jo_force_inline void      jo_3d_draw(jo_3d_quad *const quad)
{
    __jo_3d_draw(&quad->data);
}

/** @brief Draw a mesh
 *  @param mesh Address to a jo_3d_mesh allocated struct
 */
static  __jo_force_inline void      jo_3d_mesh_draw(jo_3d_mesh *const mesh)
{
    __jo_3d_draw(&mesh->data);
}

/** @brief Draw some ranges of polygons of a mesh, without copying them (the points are transformed once)
 *  @param mesh Mesh
 *  @param point_count Number of points used by these polygons (the first points of the mesh are transformed). With all the
 *         points of the mesh, the points used by a few polygons are searched (only these ones are transformed); with fewer
 *         points, they are all transformed (the caller knows the points of its ranges)
 *  @param ranges Ranges of polygons
 *  @param range_count Number of ranges
 *  @return false if the VDP1 command table is full or a polygon uses a point after point_count
 */
static  __jo_force_inline bool      jo_3d_mesh_draw_ranges(const jo_3d_mesh * const mesh, const unsigned int point_count,
                                                           const jo_3d_polygon_range * const ranges, const int range_count)
{
    return (__jo_3d_draw_ranges(&mesh->data, point_count, ranges, range_count));
}

/** @brief Get polygon count on the mesh
 *  @param mesh Address to a jo_3d_mesh allocated struct
 *  @return polygon count
 */
static  __jo_force_inline unsigned int       jo_3d_get_mesh_polygon_count(jo_3d_mesh * const mesh)
{
    return mesh->data.polygon_count;
}

/** @brief Internal usage for polygon display
 *  @warning Don't use it
 */
static __jo_force_inline void       __jo_poylgon_draw_iterator(jo_node *node)
{
    jo_3d_draw((jo_3d_quad * const)node->data.ptr);
}

/** @brief Draw a list of quadrilateral
 *  @param list List of quadrilateral
 */
static  __jo_force_inline void      jo_3d_draw_list(jo_list *const list)
{
    jo_list_foreach(list, __jo_poylgon_draw_iterator);
}

/** @brief Draw an array of quadrilateral
 *  @param array Array of quadrilateral
 *  @param array_size Size of the array
 */
static  __jo_force_inline void      jo_3d_draw_array(jo_3d_quad * const array, int array_size)
{
    while (--array_size >= 0) jo_3d_draw(array + array_size);
}

/** @brief Draw a sprite
 *  @param sprite_id Sprite Id returned by jo_sprite_add(), jo_sprite_add_tga() or jo_sprite_add_image_pack()
 *  @return true if succeeded otherwise false
 */
static  __jo_force_inline bool      jo_3d_draw_sprite(const int sprite_id)
{
    if (__jo_sprite_quad[sprite_id] == JO_NULL && !jo_3d_create_sprite_quad(sprite_id)) return (false);
    jo_3d_draw(__jo_sprite_quad[sprite_id]);
    return (true);
}

/** @brief Draw a sprite
 *  @param sprite_id Sprite Id returned by jo_sprite_add(), jo_sprite_add_tga() or jo_sprite_add_image_pack()
 *  @param x X coordinate
 *  @param y Y coordinate
 *  @param z Z coordinate
 */
static  __jo_force_inline void      jo_3d_draw_sprite_at(const int sprite_id, const int x, const int y, const int z)
{
    jo_3d_push_matrix();
    jo_3d_translate_matrix(x, y, z);
    jo_3d_draw_sprite(sprite_id);
    jo_3d_pop_matrix();
}

/** @brief Draw billboard
 *  @param sprite_id Sprite Id returned by jo_sprite_add(), jo_sprite_add_tga() or jo_sprite_add_image_pack()
 *  @param x X coordinate
 *  @param y Y coordinate
 *  @param z Z coordinate
 */
static  __jo_force_inline void      jo_3d_draw_billboard(const int sprite_id, const int x, const int y, const int z)
{
    extern jo_pos3D __jo_sprite_pos;
    __internal_jo_sprite_set_position3D(x, y, z);
    jo_sprite_draw(sprite_id, &__jo_sprite_pos, true, true);
}

/** @brief Draw billboard with a specific scale
 *  @param sprite_id Sprite Id returned by jo_sprite_add(), jo_sprite_add_tga() or jo_sprite_add_image_pack()
 *  @param x X coordinate
 *  @param y Y coordinate
 *  @param z Z coordinate
 *  @param scale Scale
 */
static  __jo_force_inline void      jo_3d_draw_scaled_billboard(const int sprite_id, const int x, const int y, const int z, const int scale)
{
    if (scale == 1)
        jo_3d_draw_billboard(sprite_id, x, y, z);
    else
    {
        extern jo_pos3D __jo_sprite_pos;
        __internal_jo_sprite_set_position3D(x, y, z);
        jo_fixed previous_scale_x = __jo_sprite_attributes.fixed_scale_x;
        jo_fixed previous_scale_y = __jo_sprite_attributes.fixed_scale_y;
        __jo_sprite_attributes.fixed_scale_x = jo_int2fixed(scale);
        __jo_sprite_attributes.fixed_scale_y = jo_int2fixed(scale);
        jo_sprite_draw(sprite_id, &__jo_sprite_pos, true, true);
        __jo_sprite_attributes.fixed_scale_x = previous_scale_x;
        __jo_sprite_attributes.fixed_scale_y = previous_scale_y;
    }
}

/*
███████╗ ██████╗ █████╗ ██╗     ███████╗
██╔════╝██╔════╝██╔══██╗██║     ██╔════╝
███████╗██║     ███████║██║     █████╗
╚════██║██║     ██╔══██║██║     ██╔══╝
███████║╚██████╗██║  ██║███████╗███████╗
╚══════╝ ╚═════╝╚═╝  ╚═╝╚══════╝╚══════╝

*/

/** @brief Change scale using floating number
 *  @param x X Scale
 *  @param y Y Scale
 *  @param z Z Scale
 */
static  __jo_force_inline void      jo_3d_set_scalef(const float x, const float y, const float z)
{
    __jo_3d_scale(JO_FLOAT_TO_FIXED(x), JO_FLOAT_TO_FIXED(y), JO_FLOAT_TO_FIXED(z));
}

/** @brief Change scale
 *  @param x X Scale
 *  @param y Y Scale
 *  @param z Z Scale
 */
static  __jo_force_inline void      jo_3d_set_scale(const int x, const int y, const int z)
{
    __jo_3d_scale(JO_MULT_BY_65536(x), JO_MULT_BY_65536(y), JO_MULT_BY_65536(z));
}

/** @brief Change scale using fixed number
 *  @param x X Scale
 *  @param y Y Scale
 *  @param z Z Scale
 */
static  __jo_force_inline void      jo_3d_set_scale_fixed(const jo_fixed x, const jo_fixed y, const jo_fixed z)
{
    __jo_3d_scale(x, y, z);
}

/** @brief Scale of 1 (nothing changes): the scale of jo_3d_set_scale() is in the current matrix, jo_3d_pop_matrix()
 *  restores the matrix before it (jo_3d_push_matrix(), jo_3d_set_scale(), draw, jo_3d_pop_matrix())
 */
static  __jo_force_inline void	jo_3d_restore_scale(void)
{
    __jo_3d_scale(JO_NO_ZOOM, JO_NO_ZOOM, JO_NO_ZOOM);
}

/*
██╗    ██╗██╗███╗   ██╗██████╗  ██████╗ ██╗    ██╗       ██╗       ██████╗ ██████╗  ██████╗      ██╗███████╗ ██████╗████████╗██╗ ██████╗ ███╗   ██╗
██║    ██║██║████╗  ██║██╔══██╗██╔═══██╗██║    ██║       ██║       ██╔══██╗██╔══██╗██╔═══██╗     ██║██╔════╝██╔════╝╚══██╔══╝██║██╔═══██╗████╗  ██║
██║ █╗ ██║██║██╔██╗ ██║██║  ██║██║   ██║██║ █╗ ██║    ████████╗    ██████╔╝██████╔╝██║   ██║     ██║█████╗  ██║        ██║   ██║██║   ██║██╔██╗ ██║
██║███╗██║██║██║╚██╗██║██║  ██║██║   ██║██║███╗██║    ██╔═██╔═╝    ██╔═══╝ ██╔══██╗██║   ██║██   ██║██╔══╝  ██║        ██║   ██║██║   ██║██║╚██╗██║
╚███╔███╔╝██║██║ ╚████║██████╔╝╚██████╔╝╚███╔███╔╝    ██████║      ██║     ██║  ██║╚██████╔╝╚█████╔╝███████╗╚██████╗   ██║   ██║╚██████╔╝██║ ╚████║
 ╚══╝╚══╝ ╚═╝╚═╝  ╚═══╝╚═════╝  ╚═════╝  ╚══╝╚══╝     ╚═════╝      ╚═╝     ╚═╝  ╚═╝ ╚═════╝  ╚════╝ ╚══════╝ ╚═════╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝

*/

/** @brief Sets up a window region on the projection surface
 *  @param left
 *  @param top
 *  @param right
 *  @param bottom
 *  @param z_limit
 *  @param center_x
 *  @param center_y
 *  @return false if Failed
 */
static  __jo_force_inline bool      jo_3d_window(const int left, const int top, const int right, const int bottom, const int z_limit, const int center_x, const int center_y)
{
    __jo_vdp1_set_window(left, top, right, bottom, z_limit, center_x, center_y);
    return (true);
}

/** @brief The 3D polygons drawn in the current window are clipped by the VDP1 to its rectangle (user clipping: a mirror,
 *  a rear-view mirror, a view of a split screen), or not (default: the whole screen)
 *  @param clip true: clipped to the rectangle of jo_3d_window()
 *  @remarks With windows apart (jo_3d_set_windows_apart(), set before), a clipped window is a screen of its own: the
 *  polygons are culled, cut and drawn in parts against its rectangle, not against the whole screen (its commands are
 *  relative to its top left corner: the local coordinates of the VDP1, written at the end of the frame: the window must
 *  not change during the frame). Else the polygons are only clipped by the VDP1 to the rectangle of the window at the end
 *  of the frame (a mirror)
 */
static  __jo_force_inline void      jo_3d_window_clip(const bool clip)
{
    __jo_vdp1_set_window_clip(clip);
}

/** @brief Near window (default) for jo_3d_select_window() */
# define JO_3D_NEAR_WINDOW                  (0)

/** @brief Far window for jo_3d_select_window() */
# define JO_3D_FAR_WINDOW                   (1)

/** @brief Second far window for jo_3d_select_window() (a second mirror) */
# define JO_3D_FAR_WINDOW_2                 (2)

/** @brief Select the window used by jo_3d_window() and by the next drawings
 *  @param window JO_3D_NEAR_WINDOW, JO_3D_FAR_WINDOW or JO_3D_FAR_WINDOW_2
 */
static  __jo_force_inline void      jo_3d_select_window(const int window)
{
    __jo_vdp1_select_window(window);
}

/** @brief The windows do not overlap (a split screen: a view per player in its window): the commands of each window are
 *  drawn together, window after window (window 0 first), sorted by depth in their window; else (default) the commands of
 *  all the windows are sorted together by depth (a mirror in the view: its window over the other one)
 *  @param apart true: the windows drawn one after the other
 *  @remarks From the next frame (applied when the commands of a frame are sorted). Mixed by depth, the commands of 2 windows
 *  side by side change the window hundreds of times per frame: each change costs a header of 3 commands in the VRAM
 */
static  __jo_force_inline void      jo_3d_set_windows_apart(const bool apart)
{
    __jo_vdp1_windows_apart = apart;
}

/** @brief Set the perspective angle
 *  @param angle Angle in degree (from 10 to 160)
 */
static  __jo_force_inline void      jo_3d_perspective_angle(const int angle)
{
#ifdef JO_DEBUG
    if (angle < 10 || angle > 160)
        jo_core_error("Valid angle range is 10 to 160");
#endif
    __jo_3d_set_perspective(jo_DEGtoANG_int(angle));
}

/** @brief Set the display level of the viewing volume
 *  @param level Distance from the viewpoint and the projection surface
 */
static  __jo_force_inline void      jo_3d_display_level(const unsigned short level)
{
    __jo_vdp1_display_level = level;
    __jo_vdp1_update_near_clip();
}

/** @brief Transform the vertices with the SCU DSP (the third processor of the Saturn)
 *  @param slave_cpu false: for the objects drawn by the master CPU (jo_3d_mesh_draw(), etc), true: for the objects drawn by
 *         the slave CPU (jo_3d_slave_mesh_draw())
 *  @remarks The DSP computes the rotation of the vertices of a batch while the CPU projects the previous batch: the CPU time of
 *           the vertices is about 2 times smaller (the results are the same)
 *  @remarks Only for the vertices in the high work RAM (the DSP can't read the low work RAM) and the objects of 12 vertices or more
 *  @remarks The DSP is used by one CPU only. The program of the DSP is loaded by this function: call it again after using the DSP
 *           for something else (see dsp.h)
 */
void                                jo_3d_enable_dsp(const bool slave_cpu);

/** @brief Stop using the SCU DSP to transform the vertices (see jo_3d_enable_dsp()) */
void                                jo_3d_disable_dsp(void);

/** @brief Biggest guard band around the screen (pixels): the coordinates of the VDP1 are limited */
# define JO_3D_MAX_GUARD_BAND               (512)
/** @brief Default guard band around the screen (pixels, see jo_3d_set_guard_band()): only the polygons that go very far outside
 *  of the screen are clipped by the CPU */
# define JO_3D_DEFAULT_GUARD_BAND           (JO_3D_MAX_GUARD_BAND)

/** @brief Guard band (internal engine usage, see jo_3d_set_guard_band()) */
extern int                          __jo_3d_guard_band;

/** @brief Set the guard band around the screen used by the clipping of the polygons drawn after this call
 *  @param pixels Distance outside of the screen (0 to JO_3D_MAX_GUARD_BAND pixels). A polygon that goes further is clipped by the CPU
 *  (at the edges of the screen), a polygon that stays inside is drawn by the VDP1 as it is
 *  @remarks The VDP1 also spends time on the pixels outside of the screen: a small guard band (64 pixels for instance) saves VDP1 time,
 *  a big one saves CPU time. A big guard band is useful for big polygons near the camera that always go a little outside of the screen
 *  (the weapon of a first person game for instance)
 *  @warning The VDP1 maps the whole texture on each part of a clipped polygon: the texture of a clipped polygon is distorted.
 *  A small guard band is fine for detail textures (ground, grass) but not for a texture with lines (a grid for instance)
 *  @remarks The default is JO_3D_DEFAULT_GUARD_BAND (only the polygons too big for the VDP1 are clipped).
 *  The guard band isn't reset at the end of the frame
 */
static  __jo_force_inline void      jo_3d_set_guard_band(const int pixels)
{
    __jo_3d_guard_band = pixels < 0 ? 0 : (pixels > JO_3D_MAX_GUARD_BAND ? JO_3D_MAX_GUARD_BAND : pixels);
}

/** @brief Wireframe mode: the polygons and the standing sprites (distorted sprites) are drawn as their outline, in one
 *  color; the normal and scaled sprites (a HUD for instance) are drawn as they are
 *  @param enabled true: wireframe, false: normal drawing
 *  @remarks From the next frame (applied when the commands of a frame are copied into the VRAM)
 */
static  __jo_force_inline void      jo_3d_set_wireframe(const bool enabled)
{
    __jo_vdp1_wireframe = enabled;
}

/** @brief Check the wireframe mode (see jo_3d_set_wireframe())
 *  @return true: wireframe
 */
static  __jo_force_inline bool      jo_3d_is_wireframe(void)
{
    return (__jo_vdp1_wireframe);
}

/** @brief Set a depth added to the polygons and sprites drawn after this call, to sort them (the farthest are drawn first)
 *  @param bias Depth (units). For instance, in a first person game the world is drawn with a positive bias and
 *  the weapon without bias: the weapon is always over the world, even when a wall is nearer than the weapon
 *  @remarks The depth used to sort is limited to 0 to 65535: the depth plus the bias must stay in this range
 *  @remarks The bias isn't reset at the end of the frame
 */
static  __jo_force_inline void      jo_3d_set_depth_bias(const int bias)
{
    __jo_vdp1_depth_bias = bias;
}

/** @brief Get the depth added to the polygons and sprites (see jo_3d_set_depth_bias())
 *  @return Depth (units)
 */
static  __jo_force_inline int       jo_3d_get_depth_bias(void)
{
    return (__jo_vdp1_depth_bias);
}


/** @brief Transform a point by the current matrix (for instance a world position into the view space, after
 *  jo_3d_camera_look_at() or the rotations and translations of the camera)
 *  @param in Point
 *  @param out Transformed point (output)
 */
void                                jo_3d_transform_point(const jo_pos3D_fixed * const in, jo_pos3D_fixed *out);

/** @brief Transform a point by a matrix (see jo_3d_transform_point(): the same result with the current matrix)
 *  @param m Matrix: 4 rows of 3 (the 3 axes, then the translation), for instance the matrix of a bone (see jo_3d_bone_callback)
 *  @param in Point
 *  @param out Transformed point (output)
 */
void                                jo_3d_transform_point_by(const jo_fixed (* const m)[3], const jo_pos3D_fixed * const in,
                                                             jo_pos3D_fixed * const out);

/** @brief Select the window of the next drawings of the slave CPU (call it on the slave CPU; jo_3d_slave_begin() selects
 *  the window of the master CPU, see jo_3d_select_window())
 *  @param window JO_3D_NEAR_WINDOW, JO_3D_FAR_WINDOW or JO_3D_FAR_WINDOW_2
 */
void                                jo_3d_slave_select_window(const int window);

/** @brief Copy the current matrix
 *  @param m Output: 12 values, 4 rows of 3 values. Rows 0 to 2: the rotation (m[2], m[5], m[8] is the direction of the
 *  view in the world when the matrix only has the rotations of the camera), row 3: the translation
 */
void                                jo_3d_get_matrix(jo_fixed *m);

/** @brief Multiply the current matrix by a reflection through a plane (a mirror: the camera matrix of the reflected view)
 *  @param normal Normal of the plane (unit vector, world space)
 *  @param distance The plane is n.p = distance (world units, Q16.16)
 *  @remarks The winding of the polygons is reversed in the reflected view: see jo_3d_set_mirror()
 */
void                                jo_3d_reflect_matrix(const jo_pos3D_fixed * const normal, const jo_fixed distance);

/** @brief The next polygons of the master CPU are seen in a mirror: their front faces are counterclockwise on the screen
 *  @param mirror true in a reflected view (see jo_3d_reflect_matrix()), false after it
 */
void                                jo_3d_set_mirror(const bool mirror);

/** @brief Get the projection distance (distance of the projection plane, in pixels)
 *  @return Q16.16 distance: a point of the view space (x, y, z) is drawn at (x * distance / z, y * distance / z)
 *  @remarks See jo_3d_perspective_angle()
 */
static  __jo_force_inline jo_fixed  jo_3d_get_projection_distance(void)
{
    return (__jo_vdp1_projection_distance);
}

/** @brief Check if a sphere (in the view space) can be seen (view frustum of the screen)
 *  @param center Center of the sphere in the view space (see jo_3d_transform_point())
 *  @param radius Radius (Q16.16)
 *  @param max_distance Farthest distance (Q16.16)
 *  @return true if a part of the sphere may be in the view (the test is conservative)
 */
bool                                jo_3d_is_sphere_visible(const jo_pos3D_fixed * const center, const jo_fixed radius, const jo_fixed max_distance);

/** @brief jo_3d_is_sphere_visible() for the slave CPU: with the rectangle of jo_3d_set_view_rect() as it was at
 *  jo_3d_slave_begin() (the master CPU can change it while the slave CPU draws)
 *  @param center Center of the sphere in the view space (see jo_3d_slave_transform_point())
 *  @param radius Radius (Q16.16)
 *  @param max_distance Farthest distance (Q16.16)
 *  @return true if a part of the sphere may be in the view (the test is conservative)
 */
bool                                jo_3d_slave_is_sphere_visible(const jo_pos3D_fixed * const center, const jo_fixed radius, const jo_fixed max_distance);

/** @brief A rectangle of the screen (pixels, the edges are in it) */
typedef struct
{
    short                           left;
    short                           top;
    short                           right;
    short                           bottom;
}                                   jo_3d_rect;

/** @brief The objects are seen only in a rectangle of the screen: jo_3d_is_sphere_visible() is false for the spheres
 *  outside of it (the camera is in a building: the outside is seen only through its door, see jo_cells_outside_view())
 *  @param rect Rectangle (left > right: nothing is seen, but the spheres around the camera), JO_NULL: the whole screen
 *  @remarks Used by both CPUs: set it before jo_core_exec_on_slave()
 */
void                                jo_3d_set_view_rect(const jo_3d_rect * const rect);

/** @brief The rectangle of jo_3d_set_view_rect()
 *  @param rect The rectangle (output)
 *  @return false if there is none (the whole screen)
 */
bool                                jo_3d_get_view_rect(jo_3d_rect * const rect);

/** @brief Bounding sphere of the points of a mesh (the sphere around their bounding box)
 *  @param mesh Mesh
 *  @param center Center of the sphere (output, in the space of the mesh, whole units)
 *  @return Radius (whole units: 2 more than the half diagonal of the box, the points are rounded to whole units)
 *  @remarks Computed once (when the mesh is loaded), then used with jo_3d_is_sphere_visible() to skip the meshes outside
 *  of the view
 */
int                                 jo_3d_mesh_get_bounding_sphere(const jo_3d_mesh * const mesh, jo_pos3D_fixed * const center);

/** @brief Plane of a polygon for the rays (see jo_3d_mesh_create_ray_planes()): the normal and its distance, without the
 *  vertices (the models are in the slow work RAM) */
typedef struct
{
    short               nx;             // Normal (Q14)
    short               ny;
    short               nz;
    short               unused;
    int                 d;              // n.a (a: the first vertex, whole units)
    short               box[6];         // Box of the polygon (whole units, a margin of 1): x, y, z min, then max
}                       jo_3d_ray_plane;

/** @brief First polygon of a mesh hit by a ray (the bullets: the walls of a building, the rooms of a level...)
 *  @param mesh Mesh
 *  @param origin Origin (in the space of the mesh)
 *  @param dir Direction (unit vector, in the space of the mesh)
 *  @param best Distance of the nearest hit (Q16.16): give the length of the ray, updated when a nearer polygon is hit
 *  @param normal Normal of the polygon hit (in the space of the mesh): updated
 *  @return true if a nearer polygon was hit
 *  @remarks Only the front faces are hit. 32 bits integers (whole units, polygons up to 4000 units): the 64 bits
 *  operations are slow on the SH-2
 */
bool                                jo_3d_mesh_ray_hit(const jo_3d_mesh * const mesh, const jo_pos3D_fixed * const origin,
                                                       const jo_pos3D_fixed * const dir, jo_fixed * const best,
                                                       jo_pos3D_fixed * const normal);

/** @brief The planes of the polygons of a mesh for jo_3d_mesh_ray_hit_with_planes() (the rays are faster: the most polygons are
 *  rejected by their plane)
 *  @param mesh Mesh (its polygons don't change)
 *  @return The planes (jo_free() them before the mesh), JO_NULL if the memory is full
 */
jo_3d_ray_plane                     *jo_3d_mesh_create_ray_planes(const jo_3d_mesh * const mesh);

/** @brief jo_3d_mesh_ray_hit() with the planes of the polygons of the mesh (the same result, faster)
 *  @param mesh Mesh
 *  @param planes The planes of the mesh (see jo_3d_mesh_create_ray_planes()), JO_NULL: without them
 *  @param origin Origin (in the space of the mesh)
 *  @param dir Direction (unit vector, in the space of the mesh)
 *  @param best Distance of the nearest hit (Q16.16): give the length of the ray, updated when a nearer polygon is hit
 *  @param normal Normal of the polygon hit (in the space of the mesh): updated
 *  @return true if a nearer polygon was hit
 */
bool                                jo_3d_mesh_ray_hit_with_planes(const jo_3d_mesh * const mesh, const jo_3d_ray_plane * const planes,
                                                                   const jo_pos3D_fixed * const origin, const jo_pos3D_fixed * const dir,
                                                                   jo_fixed * const best, jo_pos3D_fixed * const normal);

/** @brief Draw a sprite standing on the ground and facing the camera (trees, grass, characters far away...)
 *  @param sprite_id Sprite Id returned by jo_sprite_add(), jo_sprite_add_tga(), jo_sprite_add_bin(), etc
 *  @param foot Position of the bottom center of the sprite (world space, Q16.16)
 *  @param width Width (world units, Q16.16)
 *  @param height Height (world units, Q16.16)
 *  @param sway Horizontal offset of the top of the sprite (world units, Q16.16): wind
 *  @param flip Horizontal flip
 *  @return true if it is drawn (false: behind the camera, outside of the screen or too near)
 *  @remarks The sprite is sorted with the depth of its foot (see jo_3d_set_depth_bias())
 */
bool                                jo_3d_draw_standing_sprite(const int sprite_id, const jo_pos3D_fixed * const foot, const jo_fixed width,
                                                               const jo_fixed height, const jo_fixed sway, const bool flip);

/** @brief Create an empty mesh made of shared points and polygons (indexes of the points)
 *  @param point_count Point count
 *  @param polygon_count Polygon count
 *  @return The mesh (points, polygons and attributes cleared) or JO_NULL if out of memory
 *  @remarks Set the points, the vertices of the polygons and their attributes (see jo_3d_set_mesh_polygon_texture()).
 *  Free the mesh with jo_3d_free_mesh()
 */
jo_3d_mesh                          *jo_3d_create_indexed_mesh(const unsigned int point_count, const unsigned int polygon_count);

/** @brief Draw the polygons that use a texture in parts when they are near the camera (2 x 2 or 4 x 4 parts, each part
 *  with its tile of the texture)
 *  @param sprite_id Texture (RGB sprite, width multiple of 16: 2 x 2 tiles, of 32: 4 x 4 tiles too)
 *  @return true if the tiles were created
 *  @remarks The VDP1 maps a texture without perspective: a big polygon near the camera (a wall, the ground) is distorted,
 *  and the parts of a polygon clipped by the near plane get the whole texture. In parts, each part keeps its piece of the
 *  texture, the distortion stays small and the parts are sorted with their own depth (fewer sorting errors near the camera).
 *  A polygon is drawn in parts when its farthest vertex is 1.5 times (2 x 2) or 3 times (4 x 4) farther than its nearest
 *  vertex, or when it crosses the near plane
 *  @remarks The tiles are new sprites (copies of the texture: about twice the VRAM of the texture). Call it once, after the
 *  texture was added (jo_sprite_add(), jo_sprite_add_bin(), etc)
 *  @remarks A texture magnified near the camera (a few texels on a big quad): see jo_3d_set_texture_subdivision_fine()
 */
bool                                jo_3d_set_texture_subdivision(const int sprite_id);

/** @brief Add finer tiles to the tiles of a texture (see jo_3d_set_texture_subdivision()): 8 x 8 and 16 x 16 tiles, for the parts
 *  of its polygons cut in 4 x 4 parts that cross the near plane, go very far outside of the screen, are big and partly out
 *  of the screen, or still distorted by the perspective. The last tile of a part through the near plane, or with its lines
 *  pulled in to the screen (VDP1 time), is drawn whole on what is seen of it (the VDP1 can't start a texture at any column):
 *  its texture is squeezed by up to a tile, 8 texels of a 32 x 32 texture with the 4 x 4 tiles, 2 with the 16 x 16 tiles.
 *  For a texture magnified near the camera (texels of more than about 10 pixels: a texture of a few texels on a big wall,
 *  a big floor)
 *  @param sprite_id Texture (RGB or 16 colors sprite, width and height multiples of 16)
 *  @return true if the tiles were created (with the 2 x 2 and 4 x 4 tiles if they were not)
 *  @remarks One more sprite, the tiles one under the other, 1/8 of the width of the texture and at least 8 pixels wide (16 for
 *  16 colors: a narrower tile has its texels repeated): 3 times the VRAM of the texture (64 pixels wide and more, RGB; 128,
 *  16 colors), 6 times for a 32 x 32 RGB texture. More parts near the camera (CPU time, VDP1 commands)
 */
bool                                jo_3d_set_texture_subdivision_fine(const int sprite_id);

/** @brief Like jo_3d_set_texture_subdivision(), but only the polygons that cross the near plane (or go very far outside of
 *  the screen) are drawn in parts: a model already cut in small quads when it was made keeps its quads as they are,
 *  the parts of a quad next to a whole quad overlapped it
 *  @param sprite_id Texture (see jo_3d_set_texture_subdivision())
 *  @return true if the tiles were created
 */
bool                                jo_3d_set_texture_subdivision_near_only(const int sprite_id);

/** @brief Draw in parts (see jo_3d_set_texture_subdivision()) only the polygons of a texture whose nearest vertex is nearer
 *  than a depth: the ground next to the camera, where its texture slides the most (farther, the parts cost CPU time for
 *  little)
 *  @param sprite_id Texture (see jo_3d_set_texture_subdivision())
 *  @param depth Depth in the view space (fixed), 0: all the polygons (default)
 */
void                                jo_3d_set_texture_subdivision_depth(const int sprite_id, const jo_fixed depth);

/** @brief Distortion of a texture over which its polygons are drawn in parts (see jo_3d_set_texture_subdivision()): a
 *  pattern with straight lines (a tiled floor) shows the distortion more, the lines bend where a part meets a whole polygon
 *  @param sprite_id Texture (see jo_3d_set_texture_subdivision())
 *  @param pixels Distortion (pixels on the screen, 1 to 255), 0: the default (8). 4: the lines of a tiled floor stay
 *  straight; about 1 ms more for each CPU in a room of a level (more polygons drawn in parts)
 */
void                                jo_3d_set_texture_subdivision_error(const int sprite_id, const int pixels);

/** @brief Create smaller copies of a texture (half and quarter size, "mipmaps") used when the texture is drawn small
 *  @param sprite_id Texture (RGB sprite, width multiple of 16: half size, of 32: quarter size too)
 *  @return true if the copies were created
 *  @remarks The VDP1 reads all the texels of a texture, even when it is drawn small: a polygon far away costs as much as
 *  its texture. The smallest copy that has at least as many texels as pixels on the screen is used: the polygons and the
 *  standing sprites (jo_3d_draw_standing_sprite()) far away cost less VDP1 time, and their texture flickers less
 *  @remarks The copies are new sprites (a third of the VRAM of the texture). Call it once, after the texture was added
 *  @remarks A polygon with the texture seen as a thin line of the screen (the ground at a grazing angle, 64 pixels long at
 *  least and 4 pixels wide at most) is drawn as a polygon of the mean color of the texture, as thin as it: the VDP1 drew
 *  its lines along the whole polygon (about the square of its length in pixels, 10 to 20 % of a frame on a hilly ground)
 */
bool                                jo_3d_set_texture_mipmaps(const int sprite_id);

#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT

/*
** 3D on the slave CPU: the slave CPU draws objects at the same time as the master CPU (for instance the ground on the
** slave CPU, the characters and the buildings on the master CPU). Sequence of a frame:
**   master CPU: camera matrix, jo_3d_slave_begin(), jo_core_exec_on_slave(my_slave_function), other drawings,
**               jo_core_wait_for_slave(), jo_3d_slave_end()
**   slave CPU (my_slave_function): jo_3d_slave_mesh_draw() for each object (jo_3d_slave_transform_point() to cull them)
** The slave CPU starts with the matrix of the camera given to jo_3d_slave_begin(), it has its own (small) matrix stack
*/

/** @brief Prepare the 3D drawing of the slave CPU (call it on the master CPU, before jo_core_exec_on_slave())
 *  @remarks The slave CPU uses a copy of the current matrix, the current depth bias and the current window
 *  @remarks A second drawing of the slave CPU in a frame (a split screen): after jo_core_wait_for_slave() of the first one
 *  (this function replaces the matrix and the counters that the slave CPU uses while it draws)
 */
void                                jo_3d_slave_begin(void);

/** @brief Change the depth bias of the objects drawn by the slave CPU (call it on the slave CPU)
 *  @param bias Depth (see jo_3d_set_depth_bias())
 */
void                                jo_3d_slave_set_depth_bias(const int bias);

/** @brief Change the guard band of the objects drawn by the slave CPU (call it on the slave CPU, see jo_3d_set_guard_band())
 *  @param pixels Distance outside of the screen (0 to JO_3D_MAX_GUARD_BAND)
 *  @remarks jo_3d_slave_begin() copies the guard band of the master CPU: the change lasts until the next frame
 */
void                                jo_3d_slave_set_guard_band(const int pixels);

/** @brief Transform a point by the matrix of the slave CPU (call it on the slave CPU, see jo_3d_transform_point())
 *  @param in Point
 *  @param out Transformed point (output)
 */
void                                jo_3d_slave_transform_point(const jo_pos3D_fixed * const in, jo_pos3D_fixed *out);

/** @brief Push the matrix of the slave CPU (call it on the slave CPU, see jo_3d_push_matrix())
 *  @return false if the stack of the slave CPU is full (4 matrices)
 */
bool                                jo_3d_slave_push_matrix(void);

/** @brief Pop the matrix of the slave CPU (call it on the slave CPU, see jo_3d_pop_matrix()) */
void                                jo_3d_slave_pop_matrix(void);

/** @brief Translate the matrix of the slave CPU (call it on the slave CPU, see jo_3d_translate_matrix_fixed())
 *  @param x X (Q16.16)
 *  @param y Y (Q16.16)
 *  @param z Z (Q16.16)
 */
void                                jo_3d_slave_translate_matrix_fixed(const jo_fixed x, const jo_fixed y, const jo_fixed z);

/** @brief Rotate the matrix of the slave CPU (call it on the slave CPU, see jo_3d_rotate_matrix())
 *  @param x X angle (degrees)
 *  @param y Y angle (degrees)
 *  @param z Z angle (degrees)
 */
void                                jo_3d_slave_rotate_matrix(const int x, const int y, const int z);

/** @brief Rotate the matrix of the slave CPU around an axis by a precise angle (call it on the slave CPU): a slow turn
 *  (a big wheel, a fan) moves by less than a degree per frame
 *  @param axis JO_3D_AXIS_X, JO_3D_AXIS_Y or JO_3D_AXIS_Z
 *  @param angle Angle (jo_angle: 65536 = 360 degrees)
 */
void                                jo_3d_slave_rotate(const int axis, const jo_angle angle);

/** @brief Scale the matrix of the slave CPU (call it on the slave CPU, see jo_3d_set_scale_fixed())
 *  @param x X scale (Q16.16)
 *  @param y Y scale (Q16.16)
 *  @param z Z scale (Q16.16)
 */
void                                jo_3d_slave_set_scale_fixed(const jo_fixed x, const jo_fixed y, const jo_fixed z);

/** @brief Draw some ranges of polygons of a mesh on the slave CPU, without copying them (see jo_3d_mesh_draw_ranges())
 *  @param mesh Mesh
 *  @param point_count Number of points used by these polygons (1024 at most)
 *  @param ranges Ranges of polygons
 *  @param range_count Number of ranges
 *  @return false if the command table of the slave CPU is full (960 commands) or a polygon uses a point after point_count
 */
bool                                jo_3d_slave_mesh_draw_ranges(const jo_3d_mesh * const mesh, const unsigned int point_count,
                                                                 const jo_3d_polygon_range * const ranges, const int range_count);

/** @brief Draw a mesh on the slave CPU (call it on the slave CPU, between jo_3d_slave_begin() and jo_3d_slave_end())
 *  @param mesh Mesh (1024 points at most, JO_3D_SLAVE_MAX_VERTICES)
 *  @return false if the command table of the slave CPU is full (960 commands)
 */
static  __jo_force_inline bool      jo_3d_slave_mesh_draw(const jo_3d_mesh * const mesh)
{
    return (jo_3d_slave_mesh_draw_ranges(mesh, mesh->data.point_count, JO_NULL, 0));
}

/** @brief VDP1 commands used by the slave CPU in the last frame (see jo_3d_slave_end())
 *  @return Command count (polygons and their parts)
 *  @remarks With jo_3d_slave_get_max_commands(): the room left for more objects on the slave CPU
 */
int                                 jo_3d_slave_get_command_count(void);

/** @brief Draw a sprite standing on the ground and facing the camera on the slave CPU (see jo_3d_draw_standing_sprite())
 *  @param sprite_id Sprite Id
 *  @param foot Position of the bottom center of the sprite (world space, Q16.16)
 *  @param width Width (world units, Q16.16)
 *  @param height Height (world units, Q16.16)
 *  @param sway Horizontal offset of the top of the sprite (world units, Q16.16): wind
 *  @param flip Horizontal flip
 *  @return true if it is drawn (false: behind the camera, outside of the screen, too near, or the commands of the slave CPU
 *  are used up)
 *  @remarks The sprite attributes are the ones of the master CPU at jo_3d_slave_begin(), the depth bias the one of the
 *  slave CPU (jo_3d_slave_set_depth_bias())
 */
bool                                jo_3d_slave_draw_standing_sprite(const int sprite_id, const jo_pos3D_fixed * const foot,
                                                                     const jo_fixed width, const jo_fixed height,
                                                                     const jo_fixed sway, const bool flip);

/** @brief Commands drawn by the slave CPU since jo_3d_slave_begin() (0 after jo_3d_slave_end())
 *  @return Command count
 *  @remarks Called by the master CPU while the slave CPU draws: the count at this moment
 */
int                                 jo_3d_slave_get_current_command_count(void);

/** @brief Commands the slave CPU can draw in a frame (the others are dropped, jo_3d_slave_mesh_draw() returns false)
 *  @return Command count
 */
int                                 jo_3d_slave_get_max_commands(void);

/** @brief Points of the biggest object the slave CPU can draw (a bigger object is not drawn)
 *  @return Point count
 */
int                                 jo_3d_slave_get_max_vertices(void);

/** @brief Add the polygons drawn by the slave CPU to the frame (call it on the master CPU, after jo_core_wait_for_slave())
 *  @remarks The commands kept for what is drawn after it are not used (see jo_3d_slave_set_kept_commands()): the last polygons
 *  of the slave CPU are dropped when there is no room left
 */
void                                jo_3d_slave_end(void);

/** @brief VDP1 commands kept for what is drawn after jo_3d_slave_end() (the weapon, the HUD): the polygons of the slave CPU
 *  don't take them. Without it, a frame full of polygons left no room for the HUD (and JO_DEBUG stopped the game: "Too many
 *  VDP1 commands in a frame")
 *  @param count Commands (0 by default)
 */
void                                jo_3d_slave_set_kept_commands(const int count);

#endif /* !JO_COMPILE_WITH_DUAL_CPU_SUPPORT */

/** @brief Add gouraud tables in the VDP1 VRAM (4 colors per table, one per vertex of a polygon)
 *  @param tables Colors (4 per table), or JO_NULL to only reserve the memory (black)
 *  @param count Table count
 *  @return Address / 8 of the first table (gouraud_table of a polygon attribute, the next table is + 1), 0 if out of memory
 *  @remarks The tables use the memory of the sprites (2 tables per line of a sprite of 8 pixels wide).
 *  A color of a table is added to the color of the polygon: 16 for each component (JO_COLOR_SATURN_RGB(16, 16, 16)) doesn't change it
 */
unsigned short                      jo_3d_add_gouraud_tables(const jo_color * const tables, const unsigned int count);

/** @brief Give back gouraud tables of jo_3d_add_gouraud_tables() (jo_3d_free_model() gives back the tables of a model)
 *  @param adr Address / 8 returned by jo_3d_add_gouraud_tables()
 *  @param count Table count given to jo_3d_add_gouraud_tables()
 *  @remarks The memory is given back for the last set of the shared sprite of the small sets, or for a set in its own
 *  sprite when it is the last sprite: free the sets in the reverse order of their adding. Else it stays used
 *  (jo_sprite_free_from() of an earlier sprite gives it back)
 */
void                                jo_3d_free_gouraud_tables(const unsigned short adr, const unsigned int count);

/** @brief Tint the next meshes drawn by the master CPU: added to the colors of their gouraud tables (the light of a lamp on
 *  a character, see jo_3d_point_lights_sample()), jo_3d_set_tint(0, 0, 0) to stop
 *  @param r Red added (gouraud units, -31 to 31, steps of 2)
 *  @param g Green added
 *  @param b Blue added
 *  @remarks Only the polygons with a gouraud table. Each mesh keeps a few tinted copies of its tables in the VDP1 VRAM
 *  (a copy is written when a new tint is needed; a copy used in the frame is kept for it: the slave CPU draws with the
 *  copies, see jo_3d_slave_set_tint(). All the copies used in the frame: the nearest one, within 3 units, else no tint)
 */
void                                jo_3d_set_tint(const int r, const int g, const int b);

/** @brief The tint of the master CPU, packed (see jo_3d_set_tint()): 5 bits per component (signed, steps of 2), 0: none
 *  @remarks For jo_3d_set_packed_tint() and jo_3d_slave_set_tint(): the tint of an object sampled by the master CPU
 *  (jo_3d_point_lights_sample()), drawn by the slave CPU
 */
unsigned short                      jo_3d_get_tint(void);

/** @brief Tint the next meshes drawn by the master CPU (see jo_3d_set_tint())
 *  @param tint Packed tint (see jo_3d_get_tint()), 0: none
 */
void                                jo_3d_set_packed_tint(const unsigned short tint);

/** @brief Tint the next meshes drawn by the slave CPU (call it on the slave CPU)
 *  @param tint Packed tint (see jo_3d_get_tint()), 0: none
 *  @remarks The slave CPU writes no copy of the tables: it uses the nearest copy made by the master CPU (within 3 units),
 *  else no tint. The master CPU makes the copy of a tint before the job of the slave CPU with jo_3d_prepare_tint()
 */
void                                jo_3d_slave_set_tint(const unsigned short tint);

/** @brief Make the tinted copy of the gouraud tables of a mesh for the current tint of the master CPU (see
 *  jo_3d_set_tint()), for the slave CPU to draw it with this tint (jo_3d_slave_set_tint())
 *  @param mesh Mesh
 */
void                                jo_3d_prepare_tint(const jo_3d_mesh * const mesh);

/** @brief Environment map of a mesh (a reflection, like the paint of a car): its polygons set by
 *  jo_3d_set_mesh_polygon_environment() show a tile of a picture of the surroundings, the tile of the direction of their
 *  normal in the view. The picture is cut in tiles x tiles tiles: the tile (i, j) is for the normals that point to the left
 *  (i = 0) or to the right (i = tiles - 1), up (j = 0) or down (j = tiles - 1): a sphere seen from the front (a "matcap").
 *  The reflection moves on the object when the camera or the object turns
 *  @param mesh Mesh (the normals of its polygons are computed)
 *  @param sprite_id The picture (RGB, its width / tiles a multiple of 8 pixels)
 *  @param tiles Tiles on a side (4 to 8: more tiles, a smoother reflection on round objects)
 *  @return false if the picture can't be cut, or too many meshes have a reflection (8)
 *  @remarks The polygons of the mesh are drawn by the C loop of the master CPU (the matrix of the object: not the bones of
 *  a pose)
 */
bool                                jo_3d_set_mesh_environment(jo_3d_mesh * const mesh, const int sprite_id, const int tiles);

/** @brief A polygon shows the environment map of its mesh (see jo_3d_set_mesh_environment()): its gouraud table tints the
 *  reflection (16: the picture as it is)
 *  @param mesh Mesh
 *  @param index Polygon
 *  @return false if the mesh has no environment map
 */
bool                                jo_3d_set_mesh_polygon_environment(jo_3d_mesh * const mesh, const unsigned int index);

#endif /* !JO_COMPILE_WITH_3D_SUPPORT */

#endif /* !__JO_3D_H__ */

/*
** END OF FILE
*/
