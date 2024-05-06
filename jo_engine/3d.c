/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2024, Johannes Fetz (johannesfetz@gmail.com)
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
#include "jo/tools.h"
#include "jo/malloc.h"
#include "jo/colors.h"
#include "jo/sprites.h"
#include "jo/fs.h"
#include "jo/math.h"
#include "jo/list.h"
#include "jo/3d.h"

#ifdef JO_COMPILE_WITH_3D_SUPPORT

extern jo_picture_definition        __jo_sprite_pic[JO_MAX_SPRITE];
extern jo_sprite_attributes         __jo_sprite_attributes;
jo_3d_quad                          *__jo_sprite_quad[JO_MAX_SPRITE];

void                                jo_3d_init(void)
{
    register int		i;

    jo_3d_display_level(3);
    for (JO_ZERO(i); i < JO_MAX_SPRITE; ++i)
        __jo_sprite_quad[i] = JO_NULL;
#if JO_COMPILE_USING_SGL
    slDynamicFrame(ON); // Makes nearby objects close to the camera not disappear on real hardware. Credit : Ponut & TailsOfSaturn
#endif
}

static  __jo_force_inline void      __jo_to_sgl_fixed_vertices(jo_vertice * const vertices, int count)
{
    while (--count >= 0)
    {
        vertices[count].x <<= 16;
        vertices[count].y <<= 16;
        vertices[count].z <<= 16;
    }
}

void                        jo_3d_create_triangle(jo_3d_quad * const quad, jo_vertice * const vertices)
{
    static POLYGON          polygon[] = { NORMAL(JO_FIXED_0, JO_FIXED_1, JO_FIXED_0), VERTICES(0, 1, 2, 2) };

    __jo_to_sgl_fixed_vertices(vertices, 3);
    quad->data.nbPoint = 3;
    quad->data.pntbl = (POINT *)vertices;
    quad->data.nbPolygon = 1;
    quad->data.pltbl = polygon;
    quad->data.attbl = &quad->attribute;
    quad->data.attbl->flag = Dual_Plane;
    JO_ZERO(quad->data.attbl->gstb);
}

void                        jo_3d_create_plane(jo_3d_quad * const quad, jo_vertice * const vertices)
{
    static POLYGON          polygon[] = { NORMAL(JO_FIXED_0, JO_FIXED_1, JO_FIXED_0), VERTICES(0, 1, 2, 3) };

    __jo_to_sgl_fixed_vertices(vertices, 4);
    quad->data.nbPoint = 4;
    quad->data.pntbl = (POINT *)vertices;
    quad->data.nbPolygon = 1;
    quad->data.pltbl = polygon;
    quad->data.attbl = &quad->attribute;
    quad->data.attbl->flag = Dual_Plane;
    JO_ZERO(quad->data.attbl->gstb);
}

jo_3d_mesh                  *jo_3d_create_mesh_from_vertices_and_normals(const unsigned int quad_count, jo_vertice * const vertices, jo_vector * const normals)
{
    unsigned int            i;
    FIXED                   vertice_index;
    jo_3d_mesh              *mesh;

    mesh = (jo_3d_mesh *)jo_malloc_with_behaviour(sizeof(*mesh), JO_MALLOC_TRY_REUSE_SAME_BLOCK_SIZE);
    mesh->data.nbPoint = JO_MULT_BY_4(quad_count);
    mesh->data.pntbl = (POINT *)jo_malloc_with_behaviour(mesh->data.nbPoint * sizeof(*mesh->data.pntbl), JO_MALLOC_TRY_REUSE_SAME_BLOCK_SIZE);
    if (vertices != JO_NULL)
    {
        for (JO_ZERO(i); i < mesh->data.nbPoint; ++i)
        {
            mesh->data.pntbl[i][0] = jo_int2fixed(vertices[i].x);
            mesh->data.pntbl[i][1] = jo_int2fixed(vertices[i].y);
            mesh->data.pntbl[i][2] = jo_int2fixed(vertices[i].z);
        }
    }
    mesh->data.nbPolygon = quad_count;
    mesh->data.pltbl = (POLYGON *)jo_malloc_with_behaviour(quad_count * sizeof(*mesh->data.pltbl), JO_MALLOC_TRY_REUSE_SAME_BLOCK_SIZE);
    JO_ZERO(vertice_index);
    for (JO_ZERO(i); i < quad_count; ++i)
    {
        if (normals != JO_NULL)
        {
            mesh->data.pltbl[i].norm[0] = jo_int2fixed(normals[i].x);
            mesh->data.pltbl[i].norm[1] = jo_int2fixed(normals[i].y);
            mesh->data.pltbl[i].norm[2] = jo_int2fixed(normals[i].z);
        }
        else
        {
            mesh->data.pltbl[i].norm[0] = JO_FIXED_0;
            mesh->data.pltbl[i].norm[1] = JO_FIXED_1;
            mesh->data.pltbl[i].norm[2] = JO_FIXED_0;
        }
        mesh->data.pltbl[i].Vertices[0] = vertice_index;
        ++vertice_index;
        mesh->data.pltbl[i].Vertices[1] = vertice_index;
        ++vertice_index;
        mesh->data.pltbl[i].Vertices[2] = vertice_index;
        ++vertice_index;
        mesh->data.pltbl[i].Vertices[3] = vertice_index;
        ++vertice_index;
    }
    mesh->data.attbl = (ATTR *)jo_malloc_with_behaviour(quad_count * sizeof(*mesh->data.attbl), JO_MALLOC_TRY_REUSE_SAME_BLOCK_SIZE);
    for (JO_ZERO(i); i < quad_count; ++i)
    {
        mesh->data.attbl[i].flag = Dual_Plane;
        JO_ZERO(mesh->data.attbl[i].gstb);
    }
    return (mesh);
}

void                        jo_3d_free_mesh(const jo_3d_mesh * const mesh)
{
#ifdef JO_DEBUG
    if (mesh == JO_NULL)
    {
        jo_core_error("mesh is null");
        return ;
    }
#endif
    jo_free(mesh->data.pntbl);
    jo_free(mesh->data.pltbl);
    jo_free(mesh->data.attbl);
}

void                        jo_3d_create_cube(jo_3d_quad * const array, jo_vertice * const vertices)
{
    jo_3d_create_plane(array, vertices);
    jo_3d_create_plane(array + 1, vertices + 4);
    jo_3d_create_plane(array + 2, vertices + 8);
    jo_3d_create_plane(array + 3, vertices + 12);
    jo_3d_create_plane(array + 4, vertices + 16);
    jo_3d_create_plane(array + 5, vertices + 20);
}

void                        jo_3d_set_mesh_polygon_transparency(jo_3d_mesh * const mesh, const bool enabled, const unsigned int index)
{
#ifdef JO_DEBUG
    if (mesh == JO_NULL)
    {
        jo_core_error("mesh is null");
        return ;
    }
    if (index >= mesh->data.nbPolygon)
    {
        jo_core_error("index (%d) is too high (max=%d)", (int)index, (int)mesh->data.nbPolygon);
        return ;
    }
#endif
    if (enabled)
        JO_ADD_FLAG(mesh->data.attbl[index].atrb, CL_Trans);
    else
        JO_REMOVE_FLAG(mesh->data.attbl[index].atrb, CL_Trans);
}


void                        jo_3d_set_mesh_transparency(jo_3d_mesh * const mesh, const bool enabled)
{
    unsigned int            i;

#ifdef JO_DEBUG
    if (mesh == JO_NULL)
    {
        jo_core_error("mesh is null");
        return ;
    }
#endif
    for (JO_ZERO(i); i < mesh->data.nbPolygon; ++i)
        jo_3d_set_mesh_polygon_transparency(mesh, enabled, i);
}

void                        jo_3d_set_mesh_polygon_screen_doors(jo_3d_mesh * const mesh, const bool enabled, const unsigned int index)
{
#ifdef JO_DEBUG
    if (mesh == JO_NULL)
    {
        jo_core_error("mesh is null");
        return ;
    }
    if (index >= mesh->data.nbPolygon)
    {
        jo_core_error("index (%d) is too high (max=%d)", (int)index, (int)mesh->data.nbPolygon);
        return ;
    }
#endif
    if (enabled)
        JO_ADD_FLAG(mesh->data.attbl[index].atrb, MESHon);
    else
        JO_REMOVE_FLAG(mesh->data.attbl[index].atrb, MESHon);
}

void                        jo_3d_set_mesh_screen_doors(jo_3d_mesh * const mesh, const bool enabled)
{
    unsigned int            i;

#ifdef JO_DEBUG
    if (mesh == JO_NULL)
    {
        jo_core_error("mesh is null");
        return ;
    }
#endif
    for (JO_ZERO(i); i < mesh->data.nbPolygon; ++i)
        jo_3d_set_mesh_polygon_screen_doors(mesh, enabled, i);
}

void                        jo_3d_set_mesh_polygon_light(jo_3d_mesh * const mesh, const bool enabled, const unsigned int index)
{
#ifdef JO_DEBUG
    if (mesh == JO_NULL)
    {
        jo_core_error("mesh is null");
        return ;
    }
    if (index >= mesh->data.nbPolygon)
    {
        jo_core_error("index (%d) is too high (max=%d)", (int)index, (int)mesh->data.nbPolygon);
        return ;
    }
#endif
    if (enabled)
        JO_ADD_FLAG(mesh->data.attbl[index].sort, UseLight);
    else
        JO_REMOVE_FLAG(mesh->data.attbl[index].sort, UseLight);
}

void                        jo_3d_set_mesh_light(jo_3d_mesh * const mesh, const bool enabled)
{
    unsigned int            i;

#ifdef JO_DEBUG
    if (mesh == JO_NULL)
    {
        jo_core_error("mesh is null");
        return ;
    }
#endif
    for (JO_ZERO(i); i < mesh->data.nbPolygon; ++i)
        jo_3d_set_mesh_polygon_light(mesh, enabled, i);
}

void                        jo_3d_set_mesh_polygon_texture(jo_3d_mesh * const mesh, const int sprite_id, const unsigned int index)
{
    bool                    use_light;
    bool                    use_screen_doors;

#ifdef JO_DEBUG
    if (mesh == JO_NULL)
    {
        jo_core_error("mesh is null");
        return ;
    }
    if (index >= mesh->data.nbPolygon)
    {
        jo_core_error("index (%d) is too high (max=%d)", (int)index, (int)mesh->data.nbPolygon);
        return ;
    }
#endif
    use_light = (mesh->data.attbl[index].sort & UseLight) != 0;
    use_screen_doors = (mesh->data.attbl[index].atrb & MESHon) != 0;
    mesh->data.attbl[index].sort = (SORT_CEN) | (((sprNoflip) >> 16) & 0x1c) | (No_Option);
    if (use_light)
        JO_ADD_FLAG(mesh->data.attbl[index].sort, UseLight);
    mesh->data.attbl[index].texno = sprite_id;
    if (__jo_sprite_pic[sprite_id].color_mode == COL_256)
    {
        mesh->data.attbl[index].colno = (unsigned short)__jo_sprite_attributes.color_table_index;
        mesh->data.attbl[index].atrb = (CL256Bnk | No_Gouraud) | (((sprNoflip) >> 24) & 0xc0);
        JO_ADD_FLAG(mesh->data.attbl[index].sort, UsePalette);
    }
    else
    {
        mesh->data.attbl[index].colno = No_Palet;
        mesh->data.attbl[index].atrb = (CL32KRGB | No_Gouraud) | (((sprNoflip) >> 24) & 0xc0);
    }
    if (use_screen_doors)
        JO_ADD_FLAG(mesh->data.attbl[index].atrb, MESHon);
    mesh->data.attbl[index].dir = (sprNoflip) & 0x3f;
}

void                        jo_3d_set_mesh_texture(jo_3d_mesh * const mesh, const int sprite_id)
{
    unsigned int            i;

#ifdef JO_DEBUG
    if (mesh == JO_NULL)
    {
        jo_core_error("mesh is null");
        return ;
    }
#endif
    for (JO_ZERO(i); i < mesh->data.nbPolygon; ++i)
        jo_3d_set_mesh_polygon_texture(mesh, sprite_id, i);
}

void                        jo_3d_set_mesh_polygon_wireframe(jo_3d_mesh * const mesh, const unsigned int index, bool wireframe)
{
#ifdef JO_DEBUG
    if (mesh == JO_NULL)
    {
        jo_core_error("mesh is null");
        return ;
    }
#endif
    mesh->data.attbl[index].sort = (SORT_CEN) | (((wireframe ? sprPolyLine : sprPolygon) >> 16) & 0x1c) | (No_Option);
    mesh->data.attbl[index].atrb = (CL32KRGB | No_Gouraud) | (((wireframe ? sprPolyLine : sprPolygon) >> 24) & 0xc0);
    mesh->data.attbl[index].dir = (wireframe ? sprPolyLine : sprPolygon) & 0x3f;
}

void                        jo_3d_set_mesh_polygon_color_ex(jo_3d_mesh * const mesh, const jo_color color, const unsigned int index, bool wireframe)
{
    bool                    use_light;
    bool                    use_screen_doors;

#ifdef JO_DEBUG
    if (mesh == JO_NULL)
    {
        jo_core_error("mesh is null");
        return ;
    }
    if (index >= mesh->data.nbPolygon)
    {
        jo_core_error("index (%d) is too high (max=%d)", (int)index, (int)mesh->data.nbPolygon);
        return ;
    }
#endif
    use_light = (mesh->data.attbl[index].sort & UseLight) != 0;
    use_screen_doors = (mesh->data.attbl[index].atrb & MESHon) != 0;
    mesh->data.attbl[index].sort = (SORT_CEN) | (((wireframe ? sprPolyLine : sprPolygon) >> 16) & 0x1c) | (No_Option);
    if (use_light)
        JO_ADD_FLAG(mesh->data.attbl[index].sort, UseLight);
    mesh->data.attbl[index].texno = No_Texture;
    mesh->data.attbl[index].atrb = (CL32KRGB | No_Gouraud) | (((wireframe ? sprPolyLine : sprPolygon) >> 24) & 0xc0);
    if (use_screen_doors)
        JO_ADD_FLAG(mesh->data.attbl[index].atrb, MESHon);
    mesh->data.attbl[index].colno = color;
    mesh->data.attbl[index].dir = (wireframe ? sprPolyLine : sprPolygon) & 0x3f;
}

void                        jo_3d_set_mesh_color(jo_3d_mesh * const mesh, const jo_color color)
{
    unsigned int            i;

#ifdef JO_DEBUG
    if (mesh == JO_NULL)
    {
        jo_core_error("mesh is null");
        return ;
    }
#endif
    for (JO_ZERO(i); i < mesh->data.nbPolygon; ++i)
        jo_3d_set_mesh_polygon_color(mesh, color, i);
}

void                jo_3d_free_sprite_quad(int sprite_id)
{
    jo_free(__jo_sprite_quad[sprite_id]->data.pntbl);
    jo_free(__jo_sprite_quad[sprite_id]);
    __jo_sprite_quad[sprite_id] = JO_NULL;
}

bool                jo_3d_create_sprite_quad(const int sprite_id)
{
    int             img_width;
    int             img_height;
    jo_vertice      *vertices;

    if ((__jo_sprite_quad[sprite_id] = (jo_3d_quad *)jo_malloc_with_behaviour(sizeof(*__jo_sprite_quad[sprite_id]), JO_FAST_ALLOCATION)) == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Out of memory #1");
#endif
        return (false);
    }
    if ((vertices = (jo_vertice *)jo_malloc_with_behaviour(4 * sizeof(*vertices), JO_FAST_ALLOCATION)) == JO_NULL)
    {
        jo_free(__jo_sprite_quad[sprite_id]);
        __jo_sprite_quad[sprite_id] = JO_NULL;
#ifdef JO_DEBUG
        jo_core_error("Out of memory #2");
#endif
        return (false);
    }
    jo_3d_create_plane(__jo_sprite_quad[sprite_id], vertices);
    img_width = JO_DIV_BY_2(jo_sprite_get_width(sprite_id));
    img_height = JO_DIV_BY_2(jo_sprite_get_height(sprite_id));
    vertices[0].x = JO_MULT_BY_65536(-img_width);
    vertices[1].x = JO_MULT_BY_65536(img_width);
    vertices[2].x = JO_MULT_BY_65536(img_width);
    vertices[3].x = JO_MULT_BY_65536(-img_width);
    vertices[0].y = JO_MULT_BY_65536(-img_height);
    vertices[1].y = JO_MULT_BY_65536(-img_height);
    vertices[2].y = JO_MULT_BY_65536(img_height);
    vertices[3].y = JO_MULT_BY_65536(img_height);
    JO_ZERO(vertices[0].z);
    JO_ZERO(vertices[1].z);
    JO_ZERO(vertices[2].z);
    JO_ZERO(vertices[3].z);
    jo_3d_set_texture(__jo_sprite_quad[sprite_id], sprite_id);
    jo_3d_set_screen_doors(__jo_sprite_quad[sprite_id], false);
    jo_3d_set_light(__jo_sprite_quad[sprite_id], false);
    return (true);
}

#endif /* !JO_COMPILE_WITH_3D_SUPPORT */

/*
** END OF FILE
*/
