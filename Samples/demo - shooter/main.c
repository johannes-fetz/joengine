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
#include "ship.h"
#include "background.h"

static t_ship       ship;
static int          first_ship_sprite_id;
static int          blast_sprite_id;
static int          enemy_sprite_id;
static int          shield_sprite_id;
static jo_list      laser_blast_list;
static jo_list      enemies_list;
static int          level = 0;
static int          gameover = 0;
static char         having_shield = 1;

void                next_level(void)
{
    int             i;
    jo_list_data    enemy_spec;

    for (i = level; i >= 0; --i)
    {
        enemy_spec.coord.x = (jo_random(2) == 1 ? jo_random_using_multiple(141, 32) : -jo_random_using_multiple(141, 32));
        enemy_spec.coord.y = -JO_TV_HEIGHT_2 - jo_random_using_multiple(120, 40);
        jo_list_add(&enemies_list, enemy_spec);
    }
    ++level;
}

void         draw_ship_shield(void)
{
    ship.shield_pos_x = ship.x + jo_cos_mult(30, ship.shield_angle);
    ship.shield_pos_y = ship.y + jo_sin_mult(30, ship.shield_angle);
    jo_sprite_draw3D(shield_sprite_id, ship.shield_pos_x, ship.shield_pos_y, 520);
    ship.shield_angle += 4;
}

inline void         draw_ship(void)
{
    /* Instead of loading the same animation when we move the ship to the right, we just flip the sprite horizontally */
    if (ship.move == RIGHT)
        jo_sprite_enable_horizontal_flip();
    /* We reverse the animation when the ship doesn't move horizontally (see line 74) */
    jo_sprite_draw3D(ship.reverse_animation ? jo_get_anim_sprite_reverse(ship.anim_id) : jo_get_anim_sprite(ship.anim_id), ship.x, ship.y, 500);
    if (ship.move == RIGHT)
        jo_sprite_disable_horizontal_flip();

}

inline bool         check_if_laser_hit_enemy(jo_node *enemy, void *extra)
{
    jo_node         *blast;

    blast = (jo_node *)extra;
    if (!jo_hitbox_detection_custom_boundaries(blast_sprite_id, blast->data.coord.x, blast->data.coord.y, enemy->data.coord.x, enemy->data.coord.y, 20, 24))
        return false;
    jo_list_remove(&enemies_list, enemy);
    ++ship.score;
    return true;
}

inline void         draw_laser_blast(jo_node *node)
{
    jo_sprite_draw3D(blast_sprite_id, node->data.coord.x, node->data.coord.y, 520);
    node->data.coord.y -= 4;
    if (jo_list_any(&enemies_list, check_if_laser_hit_enemy, node))
        jo_list_remove(&laser_blast_list, node);
    else if (node->data.coord.y < -JO_TV_HEIGHT_2)
        jo_list_remove(&laser_blast_list, node);
}

inline void         draw_enemy(jo_node *node)
{
    jo_sprite_draw3D(enemy_sprite_id, node->data.coord.x, node->data.coord.y, 520);
    node->data.coord.y += 2;
    if (having_shield && jo_hitbox_detection(enemy_sprite_id, node->data.coord.x, node->data.coord.y, shield_sprite_id, ship.shield_pos_x, ship.shield_pos_y))
    {
        jo_list_remove(&enemies_list, node);
        having_shield = 0;
    }
    else if (jo_hitbox_detection_custom_boundaries(enemy_sprite_id, node->data.coord.x, node->data.coord.y, ship.x, ship.y, 28, 20))
    {
        gameover = 1;
        jo_list_remove(&enemies_list, node);
    }
    if (node->data.coord.y > JO_TV_HEIGHT_2 + 20)
        jo_list_remove(&enemies_list, node);
}

inline void         move_background(void)
{
    static int      background_vertical_scrolling = 0;

    jo_move_background(ship.x, background_vertical_scrolling);
    --background_vertical_scrolling;
}

void                my_draw(void)
{
    if (!gameover && enemies_list.count <= 0)
        next_level();

    jo_printf(1, 1, "Level: %d  ", level);
    jo_printf(28, 1, "Score: %d  ", ship.score);
#ifdef JO_DEBUG
    jo_printf(1, 2, jo_get_last_error());
#endif
    if (!gameover)
    {
        draw_ship();
        if (having_shield)
            draw_ship_shield();
    }
    else
    {
        jo_printf(15, 14, "Game over !");
        jo_printf(9, 16, "Press Start to restart");
    }
    jo_list_foreach(&laser_blast_list, draw_laser_blast);
    jo_list_foreach(&enemies_list, draw_enemy);
    move_background();
}

inline void         start_ship_animation(t_ship_horiz_move move, char is_moving_horizontaly, char reverse_animation)
{
    jo_restart_sprite_anim(ship.anim_id);
    ship.is_moving_horizontaly = is_moving_horizontaly;
    ship.reverse_animation = reverse_animation;
    ship.move = move;
}

void                restart_game(void)
{
    jo_list_clear(&laser_blast_list);
    jo_list_clear(&enemies_list);
    level = 0;
    ship.score = 0;
    gameover = 0;
    having_shield = 1;
    jo_clear_screen();
}

inline void         shoot(void)
{
    jo_list_data    blast;

    blast.coord.x = ship.x;
    blast.coord.y = ship.y - 28;
    jo_list_add(&laser_blast_list, blast);
}

void			my_gamepad(void)
{
	if (!jo_is_pad1_available())
		return ;
    if (jo_is_pad1_key_down(JO_KEY_START))
        restart_game();
    if (gameover)
        return ;
    if (jo_is_pad1_key_down(JO_KEY_A))
        shoot();
	if (jo_is_pad1_key_pressed(JO_KEY_LEFT) && ship.x > -(JO_TV_WIDTH_2 - 16))
    {
        /* If the ship doesn't move or on the opposite side */
        if ((!ship.is_moving_horizontaly && jo_is_sprite_anim_stopped(ship.anim_id)) || (ship.is_moving_horizontaly && ship.move == RIGHT))
            start_ship_animation(LEFT, 1, 0);
        ship.x -= ship.speed;
    }
	else if (jo_is_pad1_key_pressed(JO_KEY_RIGHT) && ship.x < (JO_TV_WIDTH_2 - 16))
    {
        if ((!ship.is_moving_horizontaly && jo_is_sprite_anim_stopped(ship.anim_id)) || (ship.is_moving_horizontaly && ship.move == LEFT))
            start_ship_animation(RIGHT, 1, 0);
        ship.x += ship.speed;
    }
    else
    {
        /* We restore the initial state (the trackbed) of the ship */
        if ((ship.is_moving_horizontaly && jo_is_sprite_anim_stopped(ship.anim_id)))
            start_ship_animation(ship.move, 0, 1);
    }
	if (jo_is_pad1_key_pressed(JO_KEY_UP) && ship.y > -JO_TV_HEIGHT_2)
		ship.y -= ship.speed;
	if (jo_is_pad1_key_pressed(JO_KEY_DOWN) && ship.y < JO_TV_HEIGHT_2)
		ship.y += ship.speed;
}

void            init_game(void)
{
    jo_tile     ship_tileset[SHIP_TILE_COUNT] =
    {
        {200, 0, 40, 38},
        {160, 0, 40, 38},
        {120, 0, 40, 38},
        {80, 0, 40, 38},
        {40, 0, 40, 38},
        {0, 0, 40, 38},
    };

  	first_ship_sprite_id = jo_sprite_add_tga_tileset(JO_ROOT_DIR, "SHIP.TGA", JO_COLOR_Blue, ship_tileset, SHIP_TILE_COUNT);
    enemy_sprite_id = jo_sprite_add_tga(JO_ROOT_DIR, "EN.TGA", JO_COLOR_Blue);
    blast_sprite_id = jo_sprite_add_tga(JO_ROOT_DIR, "BLAST.TGA", JO_COLOR_Blue);
    shield_sprite_id = jo_sprite_add_tga(JO_ROOT_DIR, "SHIELD.TGA", JO_COLOR_Blue);
    ship.anim_id = jo_create_sprite_anim(first_ship_sprite_id, SHIP_TILE_COUNT, 3);
    ship.speed = 3;
    ship.score = 0;
    ship.shield_pos_x = 0;
    ship.shield_pos_y = 0;
    ship.shield_angle = 0;
    ship.move = NONE;
    jo_list_init(&laser_blast_list);
    jo_list_init(&enemies_list);
}

void			jo_main(void)
{
	jo_core_init(JO_COLOR_Black);
    jo_set_background_sprite(&SpriteBg, 0, 0);
    init_game();
	jo_core_add_callback(my_gamepad);
	jo_core_add_callback(my_draw);
	jo_core_run();
}

/*
** END OF FILE
*/
