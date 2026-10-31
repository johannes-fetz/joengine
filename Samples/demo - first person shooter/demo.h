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
** Demo "first person shooter": a first person game in a secret laboratory, in the spirit of the console shooters of the 90s.
**
** The player (an agent) comes in through the ventilation duct, drops into the washroom and goes through the laboratory.
** Controls:
**   D-pad          walk forward and backward, turn
**   A              fire
**   B              open and close the doors; with no door in front of the agent: reload
**   L / R          step to the left / to the right
**   L + R held     aiming mode: the agent stands still, the D-pad moves the red crosshair
**   C              crouch / stand up
**   START          pause
**   X + Y + Z      the wireframe mode (debug)
**
** World: the axes of the Saturn (X right, Y DOWN, Z forward), 1 unit = 2 cm. The rooms are cells (jo/cell.h),
** the doors are entities of the class "door" (doors.c).
*/

#ifndef __DEMO_H__
# define __DEMO_H__

#include <jo/jo.h>

/*
** Player
*/
/** @brief Height of the eyes standing and crouched (world units): the agent as tall as the guards (their eyes at 110, see
 *  GUARD_EYE_HEIGHT in guards.c; at 85, in front of a door he looked at its handle) */
# define EYE_HEIGHT                 (108)
# define EYE_HEIGHT_CROUCHED        (50)
/** @brief Radius of the agent (collisions with the walls and the entities) */
# define PLAYER_RADIUS              (20)
/** @brief A step the agent climbs without a jump (the stairs) */
# define PLAYER_STEP                (24)
/** @brief A move blocked: the half, the quarter, the eighth of it are tried (the edge of a cell, see move() in player.c) */
# define MOVE_HALVINGS              (3)
/** @brief Walking speed (world units per second): 11 m per second (fast, as asked by the author after playing) */
# define WALK_SPEED                 (660)
# define STRAFE_SPEED               (552)
# define CROUCH_SPEED_PERCENT       (50)
/** @brief Turning speed (degrees per second) and the time to reach it (frames): 130 degrees per second */
# define TURN_SPEED                 (272)
# define TURN_RAMP_FRAMES           (8)
/** @brief Looking up and down with the D-pad in the aiming mode: the crosshair moves on the screen (pixels per second) */
# define AIM_CURSOR_SPEED           (400)
/** @brief Beyond this distance from the center, the crosshair turns the view (pixels) */
# define AIM_CURSOR_EDGE_X          (110)
# define AIM_CURSOR_EDGE_Y          (70)
/** @brief Gravity (world units per second per second) and the terminal speed */
# define GRAVITY                    (900)
# define FALL_MAX_SPEED             (700)
/** @brief A cell narrower than this is a duct: the agent crawls in it (crouched) */
# define DUCT_WIDTH                 (90)
/** @brief Health of the agent */
# define PLAYER_MAX_HEALTH          (100)

/** @brief Depth bias of the world: the weapon (depth 0 to 80, no bias) is always drawn over it */
# define WORLD_DEPTH_BIAS           (100)
/** @brief The weapon in the hands: behind the HUD (its sprites at depth 10, the stock of the rifle comes to 7 units), in
 *  front of the world */
# define WEAPON_DEPTH_BIAS          (20)
/** @brief Guard band of the rooms (pixels): the VDP1 clips, the CPU doesn't cut the big polygons (the texture broke along
 *  the cuts) */
# define WORLD_GUARD_BAND           (512)

/** @brief Frames per second of the game: JO_FRAMERATE of the makefile (1: 60, 2: 30). The speeds and the times of the
 *  game are in seconds and milliseconds: they don't change with it */
#ifndef JO_FRAMERATE
# define JO_FRAMERATE               (1)
#endif
# define GAME_FPS                   (60 / JO_FRAMERATE)
/** @brief Milliseconds of a frame */
# define FRAME_MS                   (1000 / GAME_FPS)

/** @brief Sound channels (13): the shots of the agent (3: a burst overlaps), the doors, the pickups, the agent (reload,
 *  empty), the voices of the guards, the impacts of the bullets (2), the shots of the guards (3), the whine of a
 *  ricochet (a tone of the SCSP, see weapon.c) */
# define CHANNEL_SHOT_FIRST         (0)
# define CHANNEL_SHOT_COUNT         (3)
# define CHANNEL_DOOR               (3)
# define CHANNEL_PICKUP             (4)
# define CHANNEL_PLAYER             (5)
# define CHANNEL_ENEMY              (6)
# define CHANNEL_IMPACT_FIRST       (7)
# define CHANNEL_IMPACT_COUNT       (2)
# define CHANNEL_GUARD_SHOT_FIRST   (9)
# define CHANNEL_GUARD_SHOT_COUNT   (3)
# define CHANNEL_RICOCHET           (12)
# define CHANNEL_VOICE              (13)  // The agent hit (see hurt_player())

/*
** Shared state (main.c)
*/
extern jo_fixed                     player_x;
extern jo_fixed                     player_y;       // The feet (Saturn Y: down)
extern jo_fixed                     player_z;
extern int                          yaw;            // Degrees (0: toward +Z, 90: toward +X)
extern int                          pitch;          // Degrees (positive: looking down)
extern int                          player_health;
extern bool                         crouched;
extern bool                         aiming;         // L + R: the aiming mode
extern int                          aim_x;          // The crosshair on the screen (pixels from the center) in the aiming mode
extern int                          aim_y;
extern int                          frame_counter;
extern bool                         paused;

/** @brief Position of the eyes (world) */
void                                player_eye(jo_pos3D_fixed * const eye);
/** @brief Direction of the line of sight through the crosshair (unit vector, world) */
void                                player_aim_direction(jo_pos3D_fixed * const dir);
/** @brief A message at the bottom of the screen for a few seconds ("Picked up some ammo.") */
void                                show_message(const char * const text);
/** @brief The agent is hit */
void                                hurt_player(const int damage);
/** @brief Load a sound of the CD (a WAV file, 16 bits) and keep it in the sound RAM
 *  @return false if it can't be loaded
 */
bool                                load_sound(const char * const name, jo_sound * const sound);
/** @brief Play a sound on one of count channels from first, in turn (*next): the oldest one is stopped if it still
 *  plays (a burst, several guards: no shot is lost, the end of the echo of the oldest one is cut) */
void                                play_sound(jo_sound * const sound, const int first, const int count, int * const next);
/** @brief A sound from a place of the level: its volume (base volume 0 to 127, lower with the distance: silent at range
 *  units) and its side (the pan: from the direction the agent looks) */
void                                sound_from(jo_sound * const sound, const jo_fixed x, const jo_fixed z, const int volume, const int range);

/*
** Player (player.c)
*/
void                                player_init(void);
/** @brief Check if the agent is in the ventilation duct (a narrow cell over the floor of the rooms) */
bool                                player_in_duct(void);
/** @brief Input and movement, once per frame (not when the game is paused) */
void                                player_update(void);
/** @brief Bob of the walk (0 to 65536, a sine) and the amount of the walk (0: still, 65536: full speed) */
jo_fixed                            player_bob(void);
jo_fixed                            player_walk_amount(void);
/** @brief Turn of the last frame (degrees, positive: to the right): the weapon lags behind */
int                                 player_turn_delta(void);

/*
** Weapon (weapon.c)
*/
void                                weapon_load(void);
void                                weapon_update(void);
/** @brief The agent picks up a weapon (by the name of its definition): a new one (a message, the sound), else its
 *  ammunition (a magazine) */
bool                                weapon_pickup(const char * const name);
/** @brief Reload the weapon at its next update (B with no door in front of the agent; nothing when the magazine is full) */
void                                weapon_request_reload(void);
/** @brief Draw the weapon in the hands (after the world, the camera matrix is not needed) */
void                                weapon_draw(void);
/** @brief Draw the impacts of the bullets on the walls (the camera matrix is the current matrix) */
void                                impacts_draw(void);
/** @brief A bullet along a ray: the impact on the walls, the damage on the entities
 *  @return true if something was hit
 */
bool                                shoot_ray(const jo_pos3D_fixed * const origin, const jo_pos3D_fixed * const dir, const int damage,
                                              jo_entity * const source);

/*
** Doors (doors.c)
*/
/** @brief Register the class door (before jo_entities_load()) */
void                                doors_register(void);
/** @brief The glows of the lamps on (after the level is loaded: the class lamp adds them) */
void                                doors_glows_init(void);
/** @brief The red alarm lights (they pulse) and the point lights of the engine: once per frame, before the drawing */
void                                doors_lights_update(const int elapsed);
/** @brief Open the door in front of the agent (B)
 *  @return true if a door was used
 */
bool                                doors_use(void);
/** @brief Check if a position is blocked by a door not open yet (the doorway: nobody goes through before it is open) */
bool                                doors_block(const jo_fixed x, const jo_fixed z, const int radius);
/** @brief The depth bias of the cell a door is drawn in (the room its panel is in, or the room on a side of its doorway),
 *  negative: not seen (the impacts on it are drawn with it) */
int                                 doors_depth_bias(const jo_entity * const door);
/** @brief The first pane of a window (not broken) hit by a ray (see doors_ray_hit()): its distance, -1: none */
jo_fixed                            doors_glass_ray_hit(const jo_pos3D_fixed * const origin, const jo_pos3D_fixed * const dir,
                                                        const jo_fixed max_t, jo_pos3D_fixed * const normal, jo_entity ** const glass);
/** @brief A pane shot: it breaks (its pieces fly in the direction of the shot) */
void                                doors_glass_shot(jo_entity * const glass, const jo_pos3D_fixed * const dir);
/** @brief The panel of an open hinged door is in the way of a circle going from (from_x, from_z) to (x, z) (it stands in
 *  the room like a thin wall; a circle already on it may leave it) */
bool                                doors_panel_block(const jo_fixed x, const jo_fixed z, const int radius, const jo_fixed from_x,
                                                      const jo_fixed from_z);
/** @brief A guard opens the closed door in front of him (a circle on the floor meets it): it turns away from him, it
 *  closes by itself after a while
 *  @return true if a door opens */
bool                                doors_open_for(const jo_fixed x, const jo_fixed z, const int radius, const jo_fixed from_x, const jo_fixed from_z);
/** @brief The middles of the doorways between the rooms (the ways of the guards from a room to another), or of the
 *  openings of the stalls (stalls: true)
 *  @return Their number */
int                                 doors_doorways(jo_pos3D_fixed * const out, const int max, const bool stalls);
/** @brief The 2 sides of the doorways between the rooms (the ways of the guards, see guards.c): out[2k] and out[2k + 1] are
 *  the points in front of the doorway k on each side of its wall, at a distance (units) from its middle
 *  @return Count of points (2 per doorway)
 */
int                                 doors_doorway_sides(jo_pos3D_fixed * const out, const int max, const int distance);
/** @brief A guard stopped by the panel of an open hinged door turned toward him (the circle at x z, coming from from_x
 *  from_z): the door closes and opens again away from him
 *  @return true if a door does it */
bool                                doors_swing_away(const jo_fixed x, const jo_fixed z, const int radius, const jo_fixed from_x,
                                                     const jo_fixed from_z);
/** @brief First door panel hit by a ray (the bullets, the sight of the guards)
 *  @param door Output: the door hit (JO_NULL: not needed)
 *  @param turning_closed A hinged door that turns is closed (the sight of the guards), else its panel where it is (a bullet)
 *  @return Distance of the hit (Q16.16), -1 if none
 */
jo_fixed                            doors_ray_hit(const jo_pos3D_fixed * const origin, const jo_pos3D_fixed * const dir, const jo_fixed max_t,
                                                  jo_pos3D_fixed * const normal, jo_entity ** const door, const bool turning_closed);
/** @brief A point in the space of the panel of a door (along it, up, in front of it): the impacts move with the door */
void                                doors_to_local(const jo_entity * const door, const jo_pos3D_fixed * const world, jo_pos3D_fixed * const local);
/** @brief A point of the space of the panel back in the world, where the door is now
 *  @return Yaw of the panel now (degrees: its +Z faces (sin yaw, cos yaw))
 */
int                                 doors_to_world(const jo_entity * const door, const jo_pos3D_fixed * const local, jo_pos3D_fixed * const world);
/** @brief Draw the billboards (the class billboard of doors.c: the seats of the toilets) seen by jo_entities_draw() */
void                                billboards_draw(void);

/*
** The guards (guards.c)
*/

/** @brief Register the class guard (before the level is loaded) */
void                                guards_register(void);
/** @brief Their sounds, the sprite of the blood */
void                                guards_load(void);
/** @brief A shot on the cap of a guard in his fall (the cap follows the fall: the place where it was drawn)
 *  @param max_t Distance of what the shot hits first (world, fixed)
 *  @param knock true: the cap flies off his head
 *  @return Distance of the cap on the shot, -1: no cap hit
 */
jo_fixed                            guards_cap_ray_hit(const jo_pos3D_fixed * const origin, const jo_pos3D_fixed * const dir, const jo_fixed max_t,
                                                       const bool knock);
/** @brief A bullet hits an entity: a guard is hurt (in the head: killed), his blood, his cry
 *  @return false if the entity is not a guard (the caller damages it) */
bool                                guards_shot(jo_entity * const e, const jo_pos3D_fixed * const hit, const jo_pos3D_fixed * const dir,
                                            const int damage, jo_entity * const source);
/** @brief The aim assistance: the direction of a shot toward the chest of a guard near it (in the middle of the view)
 *  @return true if the direction is changed */
bool                                guards_auto_aim(const jo_pos3D_fixed * const eye, jo_pos3D_fixed * const dir);
/** @brief A point of the view space (x right, y down, z forward: the camera of the agent) in the world */
void                                camera_view_to_world(const jo_fixed vx, const jo_fixed vy, const jo_fixed vz, jo_pos3D_fixed * const w);
/** @brief The agent fires: the guards near him hear it, the guards near the bullet too (from eye along dir, it stopped at
 *  length) */
void                                guards_hear_shot(const jo_pos3D_fixed * const eye, const jo_pos3D_fixed * const dir, const jo_fixed length);
/** @brief The drops of blood: their flight (each frame), their sprites (in the world) */
void                                guards_update_blood(const int elapsed);
void                                guards_draw_blood(void);

/*
** HUD (hud.c)
*/
void                                hud_load(void);
void                                hud_draw(void);

#endif /* !__DEMO_H__ */

/*
** END OF FILE
*/
