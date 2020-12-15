/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2017, Johannes Fetz (johannesfetz@gmail.com)
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

jo_software_renderer_gfx                *gfx1;
jo_software_renderer_gfx                *gfx2;

/*
   Jo Engine Software Rendering XYZ axes

           Z-
          /
         /
        0------> X+ (width)
       /|
      / |
     Z+ V
        Y+ (height)
*/

void			                        my_soft_render_for_gfx1(void)
{
    jo_software_renderer_triangle       tri;
    jo_matrix                           transform_matrix;

    // It's not necessary to clear the buffer in our case because nothing moves.
    jo_software_renderer_clear(gfx1, JO_COLOR_Black);

/*
    Here we set each vertice (3) of our triangle

    A ___ C
     \   /
      \ /
       B
*/
    tri.a.color = JO_COLOR_Red;
    tri.a.pos.x = jo_int2fixed(1);
    tri.a.pos.y = jo_int2fixed(0);
    tri.a.pos.z = jo_int2fixed(0);
    tri.a.pos.w = jo_int2fixed(1);

    tri.b.color = JO_COLOR_Green;
    tri.b.pos.x = jo_int2fixed(5);
    tri.b.pos.y = jo_int2fixed(31);
    tri.b.pos.z = jo_int2fixed(0);
    tri.b.pos.w = jo_int2fixed(1);

    tri.c.color = JO_COLOR_Blue;
    tri.c.pos.x = jo_int2fixed(31);
    tri.c.pos.y = jo_int2fixed(0);
    tri.c.pos.z = jo_int2fixed(0);
    tri.c.pos.w = jo_int2fixed(1);

    // We render our triangle in wireframe
    // Note: It's the only way to draw a triangle today, but texture mapped triangle will arrive quickly


    jo_matrix_identity(&transform_matrix);

    //transform_matrix.m30 = jo_int2fixed(5);

    jo_software_renderer_draw_triangle(gfx1, &tri, &transform_matrix);


    //jo_software_renderer_draw_triangle_wireframe(gfx1, &tri);

    // This copy the rendering buffer into VDP1 VRAM using DMA of course
    // ▲ Note: You don't need to redraw everything at every frame but it's just a demo
    jo_software_renderer_flush(gfx1);
}

void			                        my_soft_render_for_gfx2(void)
{
    jo_software_renderer_clear(gfx2, JO_COLOR_Gray);

    // Check drawing functions in jo/software_renderer.h
    jo_software_renderer_draw_line3D(gfx2,
                                    jo_int2fixed(0), jo_int2fixed(16), jo_int2fixed(0),
                                    jo_int2fixed(319), jo_int2fixed(16), jo_int2fixed(0),
                                    JO_COLOR_DarkBlue, JO_COLOR_Cyan);


    // This copy the rendering buffer into VDP2 VRAM using DMA of course
    // ▲ Note: You don't need to redraw everything at every frame but it's just a demo
    jo_software_renderer_flush(gfx2);
}

void			my_draw(void)
{
    // Here you can display and use every sprite functions (gouraud, distortion, transparency, etc)
	jo_sprite_draw3D(gfx1->sprite_id, 0, 0, 500);
}

void			jo_main(void)
{
	jo_core_init(JO_COLOR_Black);
	jo_printf(4, 24, "*** Software Rendering Demo ***");

	// Here we initialize rendering like surface in SDL
	// JO_SPRITE_SCREEN will automatically create a sprite that you can use through gfx1->sprite_id
	gfx1 = jo_software_renderer_create(64, 64, JO_SPRITE_SCREEN);
	gfx2 = jo_software_renderer_create(320, 32, JO_NBG1_SCREEN);
    // For better performance we use the second processor for the rendering
	jo_core_add_slave_callback(my_soft_render_for_gfx1);
	jo_core_add_slave_callback(my_soft_render_for_gfx2);
	// But the other display logic stay in the main CPU (don't worry about synchronization, Jo Engine handle everything for you)
	jo_core_add_callback(my_draw);
	jo_core_run();
}

/*
** END OF FILE
*/
