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

#include <jo/jo.h>

static jo_voxel         voxel;

void			        my_gamepad(void)
{
    bool change = false; // Avoid redraw the scene for nothing
    if (jo_is_input_key_pressed(0, JO_KEY_UP))
    {
        jo_voxel_move_forward(&voxel, JO_FIXED_3);
        change = true;
    }
    if (jo_is_input_key_pressed(0, JO_KEY_DOWN))
    {
        jo_voxel_move_backward(&voxel, JO_FIXED_3);
        change = true;
    }
    if (jo_is_input_key_pressed(0, JO_KEY_X))
    {
        jo_voxel_move_up(&voxel, JO_FIXED_2);
        change = true;
    }
    if (jo_is_input_key_pressed(0, JO_KEY_A))
    {
        jo_voxel_move_down(&voxel, JO_FIXED_2);
        change = true;
    }
    if (jo_is_input_key_pressed(0, JO_KEY_Y))
    {
        jo_voxel_horizon_up(&voxel, JO_FIXED_2);
        change = true;
    }
    if (jo_is_input_key_pressed(0, JO_KEY_B))
    {
        jo_voxel_horizon_down(&voxel, JO_FIXED_2);
        change = true;
    }
    if (jo_is_input_key_pressed(0, JO_KEY_Z))
    {
        jo_voxel_increase_view_distance(&voxel, JO_FIXED_2);
        change = true;
    }
    if (jo_is_input_key_pressed(0, JO_KEY_C))
    {
        jo_voxel_decrease_view_distance(&voxel, JO_FIXED_2);
        change = true;
    }
    if (jo_is_input_key_pressed(0, JO_KEY_LEFT))
    {
        jo_voxel_rotate_left(&voxel, jo_float2fixed(0.1));
        change = true;
    }
    if (jo_is_input_key_pressed(0, JO_KEY_RIGHT))
    {
        jo_voxel_rotate_right(&voxel, jo_float2fixed(0.1));
        change = true;
    }
    if (change)
        jo_voxel_redraw_and_flush(&voxel); // Redraw and flush the buffer
}

void			    my_draw(void)
{
    //In our example, the rendering is in a sprite (VDP1), but we can use VDP2
    // We multiply the size of the sprite by two (upscaling)
    jo_sprite_change_sprite_scale_xy_fixed(JO_FIXED_2, JO_FIXED_2);
    jo_sprite_draw3D(voxel.gfx->sprite_id, 0, 0, 500);
    // We restore sprite scaling (not necessary for this demo)
    jo_sprite_restore_sprite_scale();
}

void			    jo_main(void)
{
    jo_core_init(JO_COLOR_Black);

    // Camera setup
    voxel.camera.angle = jo_float2fixed(1.2); // RAD
    voxel.camera.scale = jo_int2fixed(100); // Changing this value allows you to have higher peaks
    voxel.camera.x = jo_int2fixed(128);
    voxel.camera.y = jo_int2fixed(128);
    voxel.camera.height = jo_int2fixed(20);
    voxel.camera.horizon = jo_int2fixed(10);
    voxel.camera.zfar = jo_int2fixed(80); // Has a big impact on performance with the size of the rendered surface

    // Z-buffer is only useful if you want to draw something else on the surface (gfx)
    voxel.use_zbuffer = false;
    // The Voxel module is based on the software rendering module which allows drawing on several types of surfaces (JO_SPRITE_SCREEN, JO_NBG1_SCREEN)
    // For our example, we draw the terrain on a surface twice as small as the screen which we will then upscale (for performance).
    voxel.gfx = jo_software_renderer_create_ex(JO_TV_WIDTH_2, JO_TV_HEIGHT_2, JO_SPRITE_SCREEN, voxel.use_zbuffer);

    // We load the texture of the terrain with the palette
    // Note: 8 bits TGA is mandatory because it will use to much memory otherwise
    voxel.terrain_img.data = JO_NULL;
    jo_create_palette(&voxel.terrain_pal);
    jo_set_tga_default_palette(&voxel.terrain_pal);
    jo_tga_8bits_loader(&voxel.terrain_img, JO_ROOT_DIR, "MAPCOL.TGA", JO_COLOR_Transparent);

    // We load the image that represent the relief (peaks in white)
    voxel.height_img.data = JO_NULL;
    jo_create_palette(&voxel.height_pal);
    jo_set_tga_default_palette(&voxel.height_pal);
    jo_tga_8bits_loader(&voxel.height_img, JO_ROOT_DIR, "MAPHEI.TGA", JO_COLOR_Transparent);

    // Initializes the base for the Voxel compute cache and checks a few things
    jo_voxel_run(&voxel);

    jo_printf(11, 1, "*** Voxel Demo ***");

    jo_core_add_callback(my_draw);
    jo_core_add_callback(my_gamepad);
    jo_core_run();
}

/*
** END OF FILE
*/
