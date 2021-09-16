/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2021, Johannes Fetz (johannesfetz@gmail.com)
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

#include <jo/jo.h>

jo_camera               cam;
jo_3d_mesh              *mesh;

void			    my_draw(void)
{
    static short    rx, ry, rz;

    jo_printf(12, 1, "*Custom 3D demo*");

    jo_3d_camera_look_at(&cam);
    jo_3d_push_matrix();
    {
        jo_3d_rotate_matrix(rx, ry, rz);
        rx += 3;
        ry += 2;
        rz += 1;
        jo_3d_mesh_draw(mesh);
    }
    jo_3d_pop_matrix();

    jo_printf(0, 28, "Polygon count: %d  ", jo_3d_get_polygon_count());
}

void			jo_main(void)
{
	jo_core_init(JO_COLOR_Black);
    jo_3d_camera_init(&cam);

    // We create programmatically a mesh composed of 2 quads
    mesh = jo_3d_create_mesh(2);
    // Note : There is also jo_3d_create_mesh_from_vertices() and jo_3d_create_mesh_from_vertices_and_normals()
    // We fill in the first 4 vertices of the first quad (index 0 to 3)
    jo_3d_set_mesh_vertice(mesh, jo_int2fixed(-50), jo_int2fixed(-50), jo_int2fixed(0), 0);
    jo_3d_set_mesh_vertice(mesh, jo_int2fixed(50), jo_int2fixed(-50), jo_int2fixed(0), 1);
    jo_3d_set_mesh_vertice(mesh, jo_int2fixed(50), jo_int2fixed(50), jo_int2fixed(0), 2);
    jo_3d_set_mesh_vertice(mesh, jo_int2fixed(-50), jo_int2fixed(50), jo_int2fixed(0), 3);
    // We fill in the first 4 vertices of the second quad (index 4 to 7)
    jo_3d_set_mesh_vertice(mesh, jo_int2fixed(-50), jo_int2fixed(-50), jo_int2fixed(-50), 4);
    jo_3d_set_mesh_vertice(mesh, jo_int2fixed(-50), jo_int2fixed(50), jo_int2fixed(-50), 5);
    jo_3d_set_mesh_vertice(mesh, jo_int2fixed(-50), jo_int2fixed(50), jo_int2fixed(50), 6);
    jo_3d_set_mesh_vertice(mesh, jo_int2fixed(-50), jo_int2fixed(-50), jo_int2fixed(50), 7);
    // Note: If the coordinates of the two last vertices in a quad are identical, this will produce a triangle
    // We set the color of the first quad (index 0)
    jo_3d_set_mesh_polygon_color(mesh, JO_COLOR_Yellow, 0);
    // We set the color of the second quad (index 1)
    jo_3d_set_mesh_polygon_color(mesh, JO_COLOR_Cyan, 1);
    // Note: of course, you can also use other methods like jo_3d_set_mesh_polygon_transparency(), jo_3d_set_mesh_polygon_texture(), etc
    // Warning: you can experience some glitch. this is because on have to set other attributes in mesh like sorting and normals

	jo_core_add_callback(my_draw);
	jo_core_run();
}

/*
** END OF FILE
*/
