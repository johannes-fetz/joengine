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
** The agent: the controls and the movement
**
** - D-pad: forward / backward, turn (the turn speeds up over a few frames: a smooth start like an analog stick)
** - L / R: step to the side. L + R held: the aiming mode (the agent stands still, the D-pad moves the crosshair; near the
**   edges of the screen the view turns)
** - C: crouch / stand up (a toggle)
** - B: open the door in front of the agent
** - The walls are the boxes of the cells (jo_cells_is_walkable()), the closed doors and the solid entities block the agent;
**   he slides along the walls (the move is tried along X then along Z)
** - Gravity: the agent falls when the floor is below his feet (the drop from the duct)
*/

#include "demo.h"

/** @brief Speed of the turn now (degrees per frame x 256) and the fraction of a degree of the yaw (x 256: at 60 frames
 *  per second a frame turns about one degree, the fraction matters) */
static int                      turn_speed = 0;
static int                      yaw_fraction = 0;
/** @brief Phase of the walk (degrees) and the amount of the walk (0 to 65536), smoothed */
static int                      walk_phase = 0;
static jo_fixed                 walk_amount = 0;
/** @brief Vertical speed (units per second, down positive) */
static int                      fall_speed = 0;
/** @brief In the air (the drop from the duct): no move on the ground, he falls straight down through the hole */
static bool                     in_air = false;
static bool                     c_was_down = false;
static bool                     was_in_duct = false;
static bool                     b_was_down = false;
static int                      turn_delta = 0;

void                            player_init(void)
{
    turn_speed = 0;
    walk_phase = 0;
    walk_amount = 0;
    fall_speed = 0;
}

jo_fixed                        player_bob(void)
{
    return (jo_sin(walk_phase));
}

jo_fixed                        player_walk_amount(void)
{
    return (walk_amount);
}

int                             player_turn_delta(void)
{
    return (turn_delta);
}

/** @brief Check if the agent is in a duct: a cell narrower than DUCT_WIDTH, over the floor of the rooms (he crawls in it) */
bool                            player_in_duct(void)
{
    const jo_cell               *cell;
    int                         index;

    index = jo_cells_find(player_x, player_y, player_z, PLAYER_STEP);
    if (index < 0)
        return (false);
    cell = jo_cells_get(index);
    // (over the floor of the rooms: the end of the duct over its hole is a narrow cell too, the agent falls from it)
    return (cell->floor0 < 0 && (cell->x1 - cell->x0 <= DUCT_WIDTH || cell->z1 - cell->z0 <= DUCT_WIDTH));
}

/** @brief Check if the agent can stand at a position */
static bool                     is_free(const jo_fixed x, const jo_fixed z)
{
    if (!jo_cells_is_walkable(x, player_y, z, PLAYER_RADIUS, PLAYER_STEP))
        return (false);
    // (a door that starts to close on him in the doorway: he can leave it, the door opens again, see doors.c)
    if ((doors_block(x, z, PLAYER_RADIUS) && !doors_block(player_x, player_z, PLAYER_RADIUS)) ||
        doors_panel_block(x, z, PLAYER_RADIUS, player_x, player_z))
        return (false);
    if (jo_entities_is_blocked_at(x, player_y, z, PLAYER_RADIUS, crouched ? EYE_HEIGHT_CROUCHED : EYE_HEIGHT, JO_NULL))
        return (false);
    return (true);
}

/** @brief Move the agent on the ground, sliding along the walls */
static void                     move(const jo_fixed dx, const jo_fixed dz)
{
    jo_fixed                    part_x;
    jo_fixed                    part_z;

    if (dx == 0 && dz == 0)
        return;
    // (in the air: no control, he falls straight; walking on, he went out of the hole into the ceiling)
    if (in_air)
        return;
    // Already inside a solid (dropped on something): the agent is never stuck, he walks out, but not through the walls (landed
    // from the duct against the edge of its hole, he walked back under the duct, out of the level)
    if (!is_free(player_x, player_z))
    {
        if (jo_cells_is_walkable(player_x + dx, player_y, player_z + dz, PLAYER_RADIUS, PLAYER_STEP))
        {
            player_x += dx;
            player_z += dz;
        }
        return;
    }
    if (is_free(player_x + dx, player_z + dz))
    {
        player_x += dx;
        player_z += dz;
        return;
    }
    // Shorter moves: the edge of a cell (the top of stairs: the slope there is a little higher than the step from the
    // flat floor at the edge of the next cell, the agent goes to the edge first)
    part_x = dx;
    part_z = dz;
    for (int halving = 0; halving < MOVE_HALVINGS; ++halving)
    {
        part_x /= 2;
        part_z /= 2;
        if (is_free(player_x + part_x, player_z + part_z))
        {
            player_x += part_x;
            player_z += part_z;
            return;
        }
    }
    // Along a wall
    if (is_free(player_x + dx, player_z))
        player_x += dx;
    if (is_free(player_x, player_z + dz))
        player_z += dz;
}

/** @brief Gravity: the floor of the cell under the feet (the agent drops from the duct into the washroom) */
static void                     fall(void)
{
    int                         floor;
    jo_fixed                    floor_y;
    bool                        in_air_before;

    in_air_before = in_air;
    in_air = false;
    if (!jo_cells_floor(player_x, player_y, player_z, PLAYER_STEP, &floor))
    {
        fall_speed = 0;
        return;
    }
    floor_y = jo_int2fixed(floor);
    // Walking down (stairs, a slope, a step): on the ground in the last frame and the floor at most a step lower, he
    // follows it (falling, he dropped by a few units then landed on each step: the descent jumped)
    if (fall_speed == 0 && !in_air_before && floor_y > player_y && floor_y <= player_y + jo_int2fixed(PLAYER_STEP))
        player_y = floor_y;
    if (floor_y > player_y + JO_FIXED_1)
    {
        in_air = true;
        // In the air: falling
        fall_speed = JO_MIN(fall_speed + GRAVITY / GAME_FPS, FALL_MAX_SPEED);
        player_y += jo_int2fixed(fall_speed) / GAME_FPS;
        if (player_y >= floor_y)
        {
            player_y = floor_y;
            fall_speed = 0;
        }
    }
    else
    {
        // On the floor (or a step up)
        player_y = floor_y;
        fall_speed = 0;
    }
}

void                            player_update(void)
{
    bool                        up;
    bool                        down;
    bool                        left;
    bool                        right;
    bool                        l;
    bool                        r;
    bool                        c;
    bool                        b;
    int                         forward;
    int                         side;
    int                         speed;
    int                         turn;
    jo_fixed                    dx;
    jo_fixed                    dz;
    jo_fixed                    target_amount;
    int                         previous_yaw;

    up = jo_is_pad1_key_pressed(JO_KEY_UP);
    down = jo_is_pad1_key_pressed(JO_KEY_DOWN);
    left = jo_is_pad1_key_pressed(JO_KEY_LEFT);
    right = jo_is_pad1_key_pressed(JO_KEY_RIGHT);
    l = jo_is_pad1_key_pressed(JO_KEY_L);
    r = jo_is_pad1_key_pressed(JO_KEY_R);
    c = jo_is_pad1_key_pressed(JO_KEY_C);
    b = jo_is_pad1_key_pressed(JO_KEY_B);
    previous_yaw = yaw;
    // Crouch (a toggle); in a duct (a narrow cell), the agent crawls
    if (c && !c_was_down)
        crouched = !crouched;
    c_was_down = c;
    if (player_in_duct())
    {
        crouched = true;
        was_in_duct = true;
    }
    else if (was_in_duct)
    {
        // Out of the duct (the drop into the washroom): the agent stands up
        crouched = false;
        was_in_duct = false;
    }
    // B pressed: the door in front of the agent; with no door there, the reload of the weapon
    if (b && !b_was_down && !doors_use())
        weapon_request_reload();
    b_was_down = b;
    // The aiming mode: L + R held
    aiming = l && r;
    forward = 0;
    side = 0;
    turn = 0;
    if (aiming)
    {
        // The D-pad moves the crosshair; at the edges of the screen, the view turns
        if (left)
            aim_x -= AIM_CURSOR_SPEED / GAME_FPS;
        if (right)
            aim_x += AIM_CURSOR_SPEED / GAME_FPS;
        if (up)
            aim_y -= AIM_CURSOR_SPEED / GAME_FPS;
        if (down)
            aim_y += AIM_CURSOR_SPEED / GAME_FPS;
        if (aim_x < -AIM_CURSOR_EDGE_X)
        {
            yaw -= (-AIM_CURSOR_EDGE_X - aim_x) / 2;
            aim_x = -AIM_CURSOR_EDGE_X;
        }
        else if (aim_x > AIM_CURSOR_EDGE_X)
        {
            yaw += (aim_x - AIM_CURSOR_EDGE_X) / 2;
            aim_x = AIM_CURSOR_EDGE_X;
        }
        if (aim_y < -AIM_CURSOR_EDGE_Y)
        {
            pitch -= (-AIM_CURSOR_EDGE_Y - aim_y) / 2;
            aim_y = -AIM_CURSOR_EDGE_Y;
        }
        else if (aim_y > AIM_CURSOR_EDGE_Y)
        {
            pitch += (aim_y - AIM_CURSOR_EDGE_Y) / 2;
            aim_y = AIM_CURSOR_EDGE_Y;
        }
        turn_speed = 0;
    }
    else
    {
        // The crosshair comes back to the center, the view levels
        aim_x -= aim_x * 10 / GAME_FPS;
        aim_y -= aim_y * 10 / GAME_FPS;
        if (pitch != 0 && (frame_counter % (GAME_FPS / 30)) == 0)
            pitch -= JO_MAX(1, JO_ABS(pitch) / 6) * (pitch > 0 ? 1 : -1);
        if (up)
            forward = 1;
        else if (down)
            forward = -1;
        if (l && !r)
            side = -1;
        else if (r && !l)
            side = 1;
        if (left)
            turn = -1;
        else if (right)
            turn = 1;
        // The turn speeds up over a few frames
        if (turn != 0)
            turn_speed = JO_MIN(turn_speed + (TURN_SPEED * 256 / GAME_FPS) / TURN_RAMP_FRAMES, TURN_SPEED * 256 / GAME_FPS);
        else
            turn_speed = JO_MAX(turn_speed - (TURN_SPEED * 256 / GAME_FPS) / 2, 0);
        if (turn != 0)
            yaw_fraction += turn * turn_speed;
        else if (turn_speed > 0)
            yaw_fraction += (turn_delta > 0 ? 1 : -1) * turn_speed;
        yaw += yaw_fraction / 256;
        yaw_fraction %= 256;
    }
    yaw = (yaw % 360 + 360) % 360;
    pitch = JO_MAX(-60, JO_MIN(60, pitch));
    turn_delta = yaw - previous_yaw;
    if (turn_delta > 180)
        turn_delta -= 360;
    else if (turn_delta < -180)
        turn_delta += 360;
    // The move: forward along the view, sideways
    speed = WALK_SPEED;
    if (crouched)
        speed = speed * CROUCH_SPEED_PERCENT / 100;
    dx = 0;
    dz = 0;
    if (forward != 0)
    {
        dx += jo_sin(yaw) * forward * speed / GAME_FPS;
        dz += jo_cos(yaw) * forward * speed / GAME_FPS;
    }
    if (side != 0)
    {
        dx += jo_cos(yaw) * side * (crouched ? STRAFE_SPEED * CROUCH_SPEED_PERCENT / 100 : STRAFE_SPEED) / GAME_FPS;
        dz -= jo_sin(yaw) * side * (crouched ? STRAFE_SPEED * CROUCH_SPEED_PERCENT / 100 : STRAFE_SPEED) / GAME_FPS;
    }
    move(dx, dz);
    fall();
    // The bob of the walk
    target_amount = (forward != 0 || side != 0) ? JO_FIXED_1 : 0;
    walk_amount += (target_amount - walk_amount) * 8 / GAME_FPS;
    if (walk_amount > JO_FIXED_1 / 20)
        walk_phase = (walk_phase + jo_fixed2int(walk_amount * 270) / GAME_FPS) % 360;
}

/*
** END OF FILE
*/
