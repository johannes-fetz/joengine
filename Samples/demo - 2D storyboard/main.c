/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2018, Johannes Fetz (johannesfetz@gmail.com)
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

// A custom structure to show that you can use customs as well as jo_2d_object_attributes, jo_pos2D, etc.
typedef struct
{
    /* Must be the first field in our custom structure in order to let the storyboard retrieve where the location of our object is */
    jo_2d_object_attributes     attributes;
    int                         sprite_id;
    // Add other attributes here or not ;)
}                               circular_saw_t;

static circular_saw_t           saws[2];
static jo_pos2D                 drone;
static jo_pos2D                 sentry;

static int                      sentry_sprite_id;
static int                      drone_sprite_id;

static jo_storyboard            *drone_storyboard;

void			                my_draw(void)
{
    jo_printf(15, 1, "*Storyboard*");
    jo_printf(3, 2, "Press A to pause drone animation");

    // The storyboard automatically compute the next position and angle of all objects in all storyboard. You just have to display objects
    jo_sprite_draw3D_and_rotate(saws[0].sprite_id, saws[0].attributes.x, saws[0].attributes.y, saws[0].attributes.z, saws[0].attributes.rz);
    jo_sprite_draw3D_and_rotate(saws[1].sprite_id, saws[1].attributes.x, saws[1].attributes.y, saws[1].attributes.z, saws[1].attributes.rz);
    // Sure you can make a loop :)
    jo_sprite_draw3D(drone_sprite_id, drone.x, drone.y, 550);
    jo_sprite_draw3D(sentry_sprite_id, sentry.x, sentry.y, 400);
    // You can also animate 3D object by switching axis
}

void			                my_gamepad(void)
{
    // If you can control the storyboard easily
    if (jo_is_pad1_key_down(JO_KEY_A))
        jo_storyboard_toggle(drone_storyboard);
}

void                            animate_circular_saws(void)
{
    jo_storyboard               *storyboard;
    jo_animation                *animation;

    // We create a new storyboard for all saw (that means that you can use the same storyboard with multiple object)
    storyboard = jo_storyboard_create(true, false);
    // Now we add a new custom animation (smooth translation + rotation) (that means that you can add multiple animation)
    animation = jo_storyboard_create_animation(storyboard, 0, 0);
    animation->sin_radius = 80;
    animation->translation_speed_y = 2;
    animation->rotation_speed = 10;
    // Now we can animate any object
    jo_storyboard_add_object(storyboard, &saws[0]);
    jo_storyboard_add_object(storyboard, &saws[1]);
    // Next step in my_draw()
}

void                            animate_drone(void)
{
    //jo_storyboard_create_for_object() = jo_storyboard_create() + jo_storyboard_add_object()
    drone_storyboard = jo_storyboard_create_for_object(true, true, &drone);
    jo_storyboard_create_translation_animation_using_direction(drone_storyboard, RIGHT, 1, 160);
    jo_storyboard_create_translation_animation_using_direction(drone_storyboard, DOWN, 1, 160);
    jo_storyboard_create_translation_animation_using_direction(drone_storyboard, LEFT, 1, 160);
    // You can also translate in any direction (angle)
    jo_storyboard_create_translation_animation(drone_storyboard, 270, 1, 160);
    // You can create complex animation :)
}

void                            animate_sentry(void)
{
    // There is many powerful helpers in jo/storyboard.h
    // jo_storyboard_move_object_in_circle() = jo_storyboard_create() + jo_storyboard_add_object() + jo_storyboard_create_circle_animation()
    jo_storyboard_move_object_in_circle(&sentry, 90, 1, JO_STORYBOARD_INFINITE_DURATION);
}

void                            init_circular_saw_sprite_and_position(void)
{
    // First saw
    saws[0].attributes.z = 500;
    saws[0].attributes.x = -50;
    saws[0].sprite_id = jo_sprite_add_tga(JO_ROOT_DIR, "SAW.TGA", JO_COLOR_Green);
    // Second saw
    saws[1].attributes.x = 50;
    saws[1].attributes.z = saws[0].attributes.z; // Same z position
    saws[1].sprite_id = saws[0].sprite_id; // Same sprite
}

void                            init_drone_sprite_and_position(void)
{
    drone_sprite_id = jo_sprite_add_tga(JO_ROOT_DIR, "EMY.TGA", JO_COLOR_Green);
    drone.x = -80;
    drone.y = -80;
}

void                            init_sentry_sprite_and_position(void)
{
    sentry_sprite_id = jo_sprite_add_tga(JO_ROOT_DIR, "CAM.TGA", JO_COLOR_Green);
    sentry.x = 0;
    sentry.y = 0;
}

void			jo_main(void)
{
	jo_core_init(JO_COLOR_Black);

	init_drone_sprite_and_position();
	init_sentry_sprite_and_position();
	init_circular_saw_sprite_and_position();

	animate_circular_saws();
	animate_drone();
	animate_sentry();

	jo_core_add_callback(my_gamepad);
	jo_core_add_callback(my_draw);
	jo_core_run();
}

/*
** END OF FILE
*/
