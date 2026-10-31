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
** HUD: the screen is clean. The health and the ammunition: at the bottom left the health (its icon, its value), at the bottom right
** the ammunition (its icon, the magazine / the reserve, RELOAD while the weapon is reloaded). The red crosshair only
** in the aiming mode (L + R). Opaque sprites with their own dark outline (no panel: half transparency costs VDP1 time)
*/

#include "demo.h"

/** @brief Depth of the sprites of the HUD (in front of the weapon: WEAPON_DEPTH_BIAS) */
#define HUD_Z                       (10)
/** @brief Bottom of the screen: the middle of the health and the ammo, the space between them and the edges of the screen */
#define PANEL_Y                     (JO_TV_HEIGHT_2 - 14)
#define PANEL_MARGIN                (6)

static int                          tex_digit[10];
static int                          tex_small_digit[10];
static int                          tex_slash;
static int                          tex_reload;
static int                          tex_health;
static int                          tex_ammo;
static int                          tex_aim;

static int                          load_hud_sprite(const char * const filename)
{
    return (jo_sprite_add_bin(JO_ROOT_DIR, filename, JO_COLOR_Transparent));
}

void                                hud_load(void)
{
    char                            name[16];
    int                             k;

    for (k = 0; k < 10; ++k)
    {
        jo_memcpy(name, "HUDD0.BIN", 10);
        name[4] = (char)('0' + k);
        tex_digit[k] = load_hud_sprite(name);
        name[3] = 'S';
        tex_small_digit[k] = load_hud_sprite(name);
    }
    tex_slash = load_hud_sprite("HUDSSL.BIN");
    tex_reload = load_hud_sprite("HUDRLD.BIN");
    tex_health = load_hud_sprite("HUDHEAL.BIN");
    tex_ammo = load_hud_sprite("HUDAMMO.BIN");
    tex_aim = jo_sprite_add_bin_4bits_lut(JO_ROOT_DIR, "AIM.BIN", JO_COLOR_Transparent);
}

/** @brief Number with the digit sprites (the last digit at x)
 *  @return x of the first digit
 */
static int                          draw_number(int value, int x, const int y, const int *digits, const int spacing)
{
    if (value < 0)
        value = 0;
    do
    {
        jo_sprite_draw3D(digits[value % 10], x, y, HUD_Z);
        value /= 10;
        x -= spacing;
    }
    while (value > 0);
    return (x + spacing);
}

void                                hud_draw(void)
{
    int                             x;

    // (the sprites are sorted with the depth bias of the world otherwise: behind the weapon)
    jo_3d_set_depth_bias(0);
    // The crosshair of the aiming mode
    if (aiming && tex_aim >= 0)
        jo_sprite_draw3D(tex_aim, aim_x, aim_y, HUD_Z);
    // Health: icon, value
    x = -JO_TV_WIDTH_2 + PANEL_MARGIN;
    jo_sprite_draw3D(tex_health, x + 12, PANEL_Y, HUD_Z);
    draw_number(JO_MAX(player_health, 0), x + 44, PANEL_Y, tex_digit, 8);
    // Ammo: icon, magazine / reserve (RELOAD while the weapon is reloaded)
    x = JO_TV_WIDTH_2 - PANEL_MARGIN - 80;
    jo_sprite_draw3D(tex_ammo, x + 10, PANEL_Y, HUD_Z);
    if (jo_weapons_reload_progress() > 0)
    {
        if ((frame_counter & 8) == 0)
            jo_sprite_draw3D(tex_reload, x + 44, PANEL_Y, HUD_Z);
    }
    else
    {
        draw_number(jo_weapons_ammo(), x + 32, PANEL_Y, tex_digit, 8);
        jo_sprite_draw3D(tex_slash, x + 42, PANEL_Y + 2, HUD_Z);
        draw_number(jo_weapons_reserve(), x + 64, PANEL_Y + 2, tex_small_digit, 6);
    }
    jo_3d_set_depth_bias(WORLD_DEPTH_BIAS);
}

/*
** END OF FILE
*/
