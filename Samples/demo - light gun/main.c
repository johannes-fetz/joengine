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
** Light gun demo: a shooting gallery for the Virtua Gun / Stunner (port 1 or port 2).
**
** - Pull the trigger to shoot the targets. Aimed at a dark area, the screen flashes white for a moment: the gun only
**   sees the bright pixels
** - START (on the gun): calibration, aim at the cross in the middle of the screen and pull the trigger
** - Without a gun: the pad of the port 1 moves a cursor (D-pad) and shoots (A)
**
** A real gun needs a CRT TV (it sees the beam of the CRT: no LCD TV, no upscaler).
** Mednafen emulates the gun with the mouse (left button: trigger, middle button: start, right button: a shot out of
** the screen) and draws its own red crosshair:
**      mednafen -ss.input.port1 gun game.cue
** (Mednafen keeps the setting: -ss.input.port1 gamepad to get the pad back)
*/

#include <jo/jo.h>

#define TARGET_COUNT        (4)
#define TARGET_RADIUS       (16)
#define TARGET_LIFETIME     (180)   /* frames before a target moves */
#define FLASH_FRAMES        (3)     /* frames of a pending shot: white screen in 2 of them (see read_gun()) */
#define HIT_MARKER_FRAMES   (30)

typedef struct
{
    int     x;      /* center, pixels from the top left corner of the screen */
    int     y;
    int     life;
}           target;

static target       targets[TARGET_COUNT];
static int          target_sprite;
static int          cross_sprite;
static int          hit_sprite;
static int          gun_port = -1;          /* -1: no gun, the pad of the port 1 is used */
static int          cursor_x = JO_TV_WIDTH_2;
static int          cursor_y = JO_TV_HEIGHT_2;
static bool         cursor_seen = false;    /* the gun saw the screen during the last frame */
static int          flash = 0;              /* white frames left (a shot of the gun is pending) */
static bool         calibrating = false;
static int          hit_x;
static int          hit_y;
static int          hit_marker = 0;
static int          score = 0;
static int          shots = 0;

/*
** Sprites drawn by code: a red and white target, a cross (the cursor, the hits)
*/
static jo_color     pixels[32 * 32];

static int          add_target_sprite(void)
{
    jo_img          img = {32, 32, pixels};
    int             x;
    int             y;
    int             d2;

    for (y = 0; y < 32; ++y)
        for (x = 0; x < 32; ++x)
        {
            d2 = (x - 16) * (x - 16) + (y - 16) * (y - 16);
            if (d2 >= 16 * 16)
                pixels[y * 32 + x] = JO_COLOR_Transparent;
            else
                pixels[y * 32 + x] = ((d2 / 30) & 1) || d2 < 16 ? JO_COLOR_Red : JO_COLOR_White;
        }
    return jo_sprite_add(&img);
}

static int          add_cross_sprite(const jo_color color)
{
    jo_img          img = {16, 16, pixels};
    int             i;

    for (i = 0; i < 16 * 16; ++i)
        pixels[i] = (i % 16 == 7 || i / 16 == 7) ? color : JO_COLOR_Transparent;
    return jo_sprite_add(&img);
}

static void         move_target(target *t)
{
    /* 16 pixels away from the edges: some TVs don't show them */
    t->x = 16 + TARGET_RADIUS + jo_random(JO_TV_WIDTH - 2 * (16 + TARGET_RADIUS));
    t->y = 32 + TARGET_RADIUS + jo_random(JO_TV_HEIGHT - 2 * (16 + TARGET_RADIUS) - 16);
    t->life = TARGET_LIFETIME + jo_random(60);
}

static void         shoot(const int x, const int y)
{
    int             i;

    hit_x = x;
    hit_y = y;
    hit_marker = HIT_MARKER_FRAMES;
    for (i = 0; i < TARGET_COUNT; ++i)
        if (JO_SQUARE(x - targets[i].x) + JO_SQUARE(y - targets[i].y) <= JO_SQUARE(TARGET_RADIUS))
        {
            ++score;
            move_target(&targets[i]);
            return;
        }
}

/*
** Game logic
*/
static void         find_gun(void)
{
    if (jo_get_input_type(0) == JoLightGun)
        gun_port = 0;
    else if (jo_get_input_type(JO_INPUT_PORT2_FIRST_DEVICE) == JoLightGun)
        gun_port = JO_INPUT_PORT2_FIRST_DEVICE;
    else
        gun_port = -1;
}

static void         read_gun(void)
{
    int             x;
    int             y;

    cursor_seen = jo_get_input_gun_position(gun_port, &x, &y);
    if (cursor_seen)
    {
        cursor_x = x;
        cursor_y = y;
    }
    if (jo_is_input_key_down(gun_port, JO_GUN_START_BUTTON))
        calibrating = !calibrating;
    if (jo_is_input_key_down(gun_port, JO_GUN_TRIGGER))
    {
        flash = FLASH_FRAMES;
        ++shots;
    }
    if (flash == 0)
        return;
    /* The position seen by the gun (at once if it points at a bright pixel). Otherwise the white screen drawn by this
       frame is shown at the next VBLANK, the gun sees it during the next field and the position is read by the frame after
       it (with JO_FRAMERATE = 1) */
    --flash;
    if (!cursor_seen)
        return;
    flash = 0;
    if (calibrating)
    {
        jo_calibrate_input_gun(gun_port, JO_TV_WIDTH_2, JO_TV_HEIGHT_2);
        calibrating = false;
    }
    else
        shoot(cursor_x, cursor_y);
}

static void         read_pad(void)
{
    cursor_seen = true;
    if (jo_is_pad1_key_pressed(JO_KEY_LEFT) && cursor_x > 0)
        cursor_x -= 4;
    if (jo_is_pad1_key_pressed(JO_KEY_RIGHT) && cursor_x < JO_TV_WIDTH - 4)
        cursor_x += 4;
    if (jo_is_pad1_key_pressed(JO_KEY_UP) && cursor_y > 0)
        cursor_y -= 4;
    if (jo_is_pad1_key_pressed(JO_KEY_DOWN) && cursor_y < JO_TV_HEIGHT - 4)
        cursor_y += 4;
    if (jo_is_pad1_key_down(JO_KEY_A))
    {
        ++shots;
        shoot(cursor_x, cursor_y);
    }
}

static void         my_logic(void)
{
    int             i;

    find_gun();
    if (gun_port >= 0)
        read_gun();
    else
        read_pad();
    if (!calibrating)
        for (i = 0; i < TARGET_COUNT; ++i)
            if (--targets[i].life <= 0)
                move_target(&targets[i]);
    if (hit_marker > 0)
        --hit_marker;
}

/*
** Display (the sprites are centered on 0,0: the positions from the top left corner minus the half of the screen)
*/
static void         my_draw(void)
{
    int             i;

    /* Shot of the gun: a white screen (the gun sees the beam of the CRT only where the screen is bright) */
    jo_set_default_background_color(flash > 0 ? JO_COLOR_White : JO_COLOR_Black);
    jo_printf(1, 1, "Light gun demo   Score: %d / %d   ", score, shots);
    if (gun_port < 0)
        jo_printf(1, 2, "No gun: D-pad and A on the pad 1  ");
    else
        jo_printf(1, 2, "Gun on port %d: START = calibration", gun_port == 0 ? 1 : 2);
    if (cursor_seen)
        jo_printf(1, 3, "Aim: %3d %3d  ", cursor_x, cursor_y);
    else
        jo_printf(1, 3, "Aim: no light ");
    if (calibrating)
    {
        jo_printf(1, 4, "Aim at the cross and pull the trigger");
        jo_sprite_draw3D(cross_sprite, 0, 0, 500);
        return;
    }
    jo_printf(1, 4, "                                     ");
    for (i = 0; i < TARGET_COUNT; ++i)
        jo_sprite_draw3D(target_sprite, targets[i].x - JO_TV_WIDTH_2, targets[i].y - JO_TV_HEIGHT_2, 500);
    if (cursor_seen)
        jo_sprite_draw3D(cross_sprite, cursor_x - JO_TV_WIDTH_2, cursor_y - JO_TV_HEIGHT_2, 400);
    if (hit_marker > 0)
        jo_sprite_draw3D(hit_sprite, hit_x - JO_TV_WIDTH_2, hit_y - JO_TV_HEIGHT_2, 300);
}

void                jo_main(void)
{
    int             i;

    jo_core_init(JO_COLOR_Black);
    target_sprite = add_target_sprite();
    cross_sprite = add_cross_sprite(JO_COLOR_Green);
    hit_sprite = add_cross_sprite(JO_COLOR_Yellow);
    for (i = 0; i < TARGET_COUNT; ++i)
        move_target(&targets[i]);
    jo_core_add_callback(my_logic);
    jo_core_add_callback(my_draw);
    jo_core_run();
}

/*
** END OF FILE
*/
