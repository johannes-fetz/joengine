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

#include "jo/jo.h"

#ifdef JO_COMPILE_WITH_3D_SUPPORT

/*
** The sun: see jo/sun.h. Its direction in the view of the camera (it turns by its heading, then looks down by the angle
** of tangent look_down), projected; more than 50 degrees aside: out of the view. The halo strips then the core over its
** middle: the same sort key (depth z + bias), drawn in this order. The lens flare: stronger when the sun is near the
** middle of the screen (JO_SUN_FLARE_FULL at the middle, a step less every FLARE_FALLOFF pixels away), a step a frame.
*/

/** @brief The lens flare fades 1 step for this many pixels between the sun and the middle of the screen */
# define FLARE_FALLOFF      (24)
/** @brief The lens flare is drawn above this level */
# define FLARE_MIN_LEVEL    (2)

/** @brief The default lens flare (place, size, piece: 0 ring, 1 disc, 2 hexagon) */
static const jo_sun_flare   default_flares[] = {{150, 7, 1}, {70, 5, 0}, {-30, 3, 2}, {-90, 7, 0}, {-150, 4, 1},
                                                {-220, 10, 2}, {-300, 6, 0}};

void                        jo_sun_init(jo_sun * const sun, const jo_angle azimuth, const jo_angle elevation)
{
    int                     k;

    jo_memset(sun, 0, sizeof(*sun));
    sun->azimuth = azimuth;
    sun->elevation = elevation;
    for (JO_ZERO(k); k < JO_SUN_MAX_HALO_STRIPS; ++k)
        sun->halo[k] = -1;
    sun->halo_half = 64;
    sun->strip_height = 32;
    sun->core = -1;
    sun->core_half = 16;
    for (JO_ZERO(k); k < JO_SUN_FLARE_PIECES; ++k)
    {
        sun->flare[k] = -1;
        sun->flare_rgb[k] = -1;
    }
    sun->flares = default_flares;
    sun->flare_count = (int)(sizeof(default_flares) / sizeof(*default_flares));
    sun->z = 1000;
    sun->bias = 40000;
    sun->flare_z = 200;
    sun->flare_bias = -30000;
}

/** @brief The sun on the screen (sun->seen, x, y, horizon_y) */
static void                 place_sun(jo_sun * const sun, const jo_angle heading, const jo_fixed look_down)
{
    jo_angle                delta;
    jo_fixed                cp;
    jo_fixed                sp;
    jo_fixed                ce;
    jo_fixed                se;
    jo_fixed                cd;
    jo_fixed                sd;
    jo_fixed                depth;
    jo_fixed                pd;

    delta = (jo_angle)(sun->azimuth - heading);
    cd = jo_cos_angle(delta);
    sd = jo_sin_angle(delta);
    sun->seen = false;
    // (more than 50 degrees aside: out of the view)
    if (cd < JO_FIXED_1 * 64 / 100)
        return;
    cp = jo_fixed_div(JO_FIXED_1, jo_fixed_sqrt(JO_FIXED_1 + jo_fixed_mult(look_down, look_down)));
    sp = jo_fixed_mult(look_down, cp);
    ce = jo_cos_angle(sun->elevation);
    se = jo_sin_angle(sun->elevation);
    // (the direction (sin d cos e, -sin e, cos d cos e) in the view (x right, y down, z ahead) turned by the look down)
    depth = jo_fixed_mult(jo_fixed_mult(cd, ce), cp) - jo_fixed_mult(se, sp);
    if (depth <= 0)
        return;
    pd = jo_3d_get_projection_distance();
    sun->horizon_y = jo_core_get_screen_height() / 2 - jo_fixed2int(jo_fixed_mult(pd, look_down));
    sun->x = jo_core_get_screen_width() / 2 + jo_fixed2int(jo_fixed_div(jo_fixed_mult(pd, jo_fixed_mult(sd, ce)), depth));
    sun->y = jo_core_get_screen_height() / 2 -
             jo_fixed2int(jo_fixed_div(jo_fixed_mult(pd, jo_fixed_mult(se, cp) + jo_fixed_mult(jo_fixed_mult(cd, ce), sp)), depth));
    sun->seen = sun->x > -sun->halo_half && sun->x < jo_core_get_screen_width() + sun->halo_half &&
                sun->y > -sun->halo_half;
}

void                        jo_sun_update(jo_sun * const sun, const jo_angle heading, const jo_fixed look_down,
                                          bool (*hidden)(void))
{
    int                     want;
    int                     half_width;

    place_sun(sun, heading, look_down);
    JO_ZERO(want);
    half_width = jo_core_get_screen_width() / 2;
    if (sun->seen && JO_ABS(sun->x - half_width) < half_width - 10 && sun->y > 10 && (hidden == JO_NULL || !hidden()))
        want = JO_MAX(0, JO_SUN_FLARE_FULL - (JO_ABS(sun->x - half_width) + JO_ABS(sun->y - jo_core_get_screen_height() / 2)) /
                         FLARE_FALLOFF);
    sun->flare_level += sun->flare_level < want ? 1 : sun->flare_level > want ? -1 : 0;
}

void                        jo_sun_draw(const jo_sun * const sun)
{
    int                     k;

    if (!sun->seen)
        return;
    jo_3d_set_depth_bias(sun->bias);
    // (the halo in strips (their colors made for the sky behind them), then the core over its middle: the same key, drawn
    // in this order)
    for (JO_ZERO(k); k < sun->halo_strips; ++k)
        jo_sprite_draw3D2(sun->halo[k], sun->x - sun->halo_half, sun->y - sun->halo_half + k * sun->strip_height, sun->z);
    if (sun->core >= 0)
        jo_sprite_draw3D2(sun->core, sun->x - sun->core_half, sun->y - sun->core_half, sun->z);
    jo_3d_set_depth_bias(0);
}

void                        jo_sun_draw_flare(const jo_sun * const sun)
{
    const jo_sun_flare      *f;
    jo_fixed                s;
    int                     dx;
    int                     dy;
    int                     x;
    int                     y;
    int                     k;

    if (sun->flare_level <= FLARE_MIN_LEVEL)
        return;
    dx = sun->x - jo_core_get_screen_width() / 2;
    dy = sun->y - jo_core_get_screen_height() / 2;
    jo_3d_set_depth_bias(sun->flare_bias);
    for (JO_ZERO(k); k < sun->flare_count; ++k)
    {
        f = &sun->flares[k];
        s = jo_int2fixed(f->size) * sun->flare_level / 128;
        if (s < JO_FIXED_1 / 4 || sun->flare[f->piece] < 0)
            continue;
        x = dx * f->place / 256;
        y = dy * f->place / 256;
        jo_sprite_change_sprite_scale_xy_fixed(s, s);
        // (over the sky: mixed with it by the VDP2; under the horizon: half transparent, by the VDP1)
        if (y + jo_core_get_screen_height() / 2 < sun->horizon_y || sun->flare_rgb[f->piece] < 0)
            jo_sprite_draw3D(sun->flare[f->piece], x, y, sun->flare_z);
        else
        {
            jo_sprite_enable_half_transparency();
            jo_sprite_draw3D(sun->flare_rgb[f->piece], x, y, sun->flare_z);
            jo_sprite_disable_half_transparency();
        }
    }
    jo_sprite_restore_sprite_scale();
    jo_3d_set_depth_bias(0);
}

#endif /* JO_COMPILE_WITH_3D_SUPPORT */

/*
** END OF FILE
*/
