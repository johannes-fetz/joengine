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
** The smallest program: JO_MINIMAL = 1 in the makefile (no file system, no jo_printf(), no second CPU... see
** Compiler/COMMON/jo_engine_makefile). A square moved with the pad over a frame and a sprite made in the program
** (no file to load)
*/

#include <jo/jo.h>

#define SPRITE_SIZE         (16)
#define SQUARE_SIZE         (24)

static jo_color             pixels[SPRITE_SIZE * SPRITE_SIZE];
static int                  sprite_id;
static int                  square_x = 148;
static int                  square_y = 100;

static void                 move_square(void)
{
    if (!jo_is_pad1_available())
        return;
    if (jo_is_pad1_key_pressed(JO_KEY_LEFT) && square_x > 8)
        --square_x;
    if (jo_is_pad1_key_pressed(JO_KEY_RIGHT) && square_x < JO_TV_WIDTH - 8 - SQUARE_SIZE)
        ++square_x;
    if (jo_is_pad1_key_pressed(JO_KEY_UP) && square_y > 8)
        --square_y;
    if (jo_is_pad1_key_pressed(JO_KEY_DOWN) && square_y < JO_TV_HEIGHT - 8 - SQUARE_SIZE)
        ++square_y;
}

static void                 draw(void)
{
    move_square();
    // A frame: a white rectangle and a dark blue one in it (farther: bigger depth)
    jo_sprite_draw_rectangle(4, 4, JO_TV_WIDTH - 8, JO_TV_HEIGHT - 8, JO_COLOR_White, 600);
    jo_sprite_draw_rectangle(8, 8, JO_TV_WIDTH - 16, JO_TV_HEIGHT - 16, JO_COLOR_DarkBlue, 600);
    // The sprite in the center of the screen, the square over it (nearer), half transparent
    jo_sprite_draw3D(sprite_id, 0, 0, 500);
    jo_sprite_enable_half_transparency();
    jo_sprite_draw_rectangle(square_x, square_y, SQUARE_SIZE, SQUARE_SIZE, JO_COLOR_Yellow, 400);
    jo_sprite_disable_half_transparency();
}

void                        jo_main(void)
{
    jo_img                  img;
    int                     x;
    int                     y;

    jo_core_init(JO_COLOR_Black);
    // A checkerboard sprite (the pixels in the program: no file system)
    for (y = 0; y < SPRITE_SIZE; ++y)
        for (x = 0; x < SPRITE_SIZE; ++x)
            pixels[y * SPRITE_SIZE + x] = ((x >> 2) ^ (y >> 2)) & 1 ? JO_COLOR_Red : JO_COLOR_Green;
    img.width = SPRITE_SIZE;
    img.height = SPRITE_SIZE;
    img.data = pixels;
    sprite_id = jo_sprite_add(&img);
    jo_core_add_callback(draw);
    jo_core_run();
}

/*
** END OF FILE
*/
