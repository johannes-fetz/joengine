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

#define HEIGHT_BOUNDARY (JO_TV_HEIGHT_2 + 10)
#define WIDTH_BOUNDARY  (JO_TV_WIDTH_2 + 10)

static jo_list          bullet_list;
static int              bullet_sprite_id;
static jo_fixed         bullet_angle_rad = 0;

typedef struct
{
    jo_fixed            x;
    jo_fixed            y;
    jo_fixed            angle;
    jo_fixed            speed;
}                       t_bullet;

static void             draw_bullet(jo_node *node)
{
    t_bullet            *bullet;

    bullet = (t_bullet *)node->data.ptr;
    jo_sprite_draw3D_and_rotate(bullet_sprite_id, jo_fixed2int(bullet->x), jo_fixed2int(bullet->y), 500, jo_fixed2int(jo_fixed_rad2deg(bullet->angle)) + 90);

    // We move the bullet according to the angle
    bullet->x += jo_fixed_mult(bullet->speed, jo_fixed_cos(bullet->angle));
    bullet->y += jo_fixed_mult(bullet->speed, jo_fixed_sin(bullet->angle));

    // When the bullet is off the screen with a certain margin, because it is the center of the image that counts, we remove it from the list
    if (JO_ABS(jo_fixed2int(bullet->y)) > HEIGHT_BOUNDARY || JO_ABS(jo_fixed2int(bullet->x)) > WIDTH_BOUNDARY)
        jo_list_free_and_remove(&bullet_list, node);
}

static void             my_draw(void)
{
    //jo_list_foreach() calls the function draw_bullet() for each element of the list
    jo_list_foreach(&bullet_list, draw_bullet);
}

static void             my_input(void)
{
    t_bullet            *bullet;

    if (jo_is_pad1_key_pressed(JO_KEY_A))
    {
        //We allocate memory for the bullet while trying not to fragment the memory too much (JO_MALLOC_TRY_REUSE_BLOCK)
        bullet = (t_bullet *)jo_malloc_with_behaviour(sizeof(*bullet), JO_MALLOC_TRY_REUSE_BLOCK);

        bullet->angle = bullet_angle_rad;
        bullet->x = jo_fixed_mult(jo_float2fixed(12), jo_fixed_cos(bullet->angle));
        bullet->y = jo_fixed_mult(jo_float2fixed(12), jo_fixed_sin(bullet->angle));
        bullet->speed = jo_float2fixed(1.6);
        //We add the bullet to our list
        jo_list_add_ptr(&bullet_list, bullet);
        // We increase the angle for the rotation effect
        bullet_angle_rad += jo_float2fixed(0.2);
    }
}

void                    jo_main(void)
{
	jo_core_init(JO_COLOR_Black);
    jo_printf(14, 1, "*Bullet demo*");
    jo_printf(6, 2, "Press [A] to shoot bullets");

    bullet_sprite_id = jo_sprite_add_tga("TEX", "BLAST.TGA", JO_COLOR_Blue);
    //▲ Do not forget to initialize the list
    jo_list_init(&bullet_list);
	jo_core_add_callback(my_input);
	jo_core_add_callback(my_draw);
	jo_core_run();
}

/*
** END OF FILE
*/
