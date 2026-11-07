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
/** @file weapon.h
 *  @author Johannes Fetz
 *
 *  @brief Weapons of the player: the definitions (section WEAP of a level, for instance a file of definitions shared by
 *         the levels, see Documentation/level_format.md) and their rules: ammunition, fire rate (automatic or one shot
 *         per press), reload, change of weapon. The game draws the weapon of jo_weapons_current() and makes the shots
 *         (the bullets, the sounds, the flash) on the events of jo_weapons_update()
 *  @remarks Module JO_COMPILE_WITH_LEVEL_MODULE = 1 in the makefile
 */

#ifndef __JO_WEAPON_H__
# define __JO_WEAPON_H__

#ifdef JO_COMPILE_WITH_LEVEL_SUPPORT

/** @brief Weapons at most */
# define JO_WEAPON_MAX                      (8)
/** @brief Types of ammunition at most */
# define JO_WEAPON_MAX_AMMO_TYPES           (8)

/** @brief Events of jo_weapons_update() (bits) */
# define JO_WEAPON_EVENT_FIRE               (0x01)  // A shot: the bullet, the sound, the flash, the recoil
# define JO_WEAPON_EVENT_EMPTY              (0x02)  // The trigger is pressed, nothing to fire (a click)
# define JO_WEAPON_EVENT_RELOAD             (0x04)  // The reload starts
# define JO_WEAPON_EVENT_RELOAD_HALF        (0x08)  // The middle of the reload: the new magazine goes in (the sound)
# define JO_WEAPON_EVENT_RELOAD_DONE        (0x10)  // The magazine is full
# define JO_WEAPON_EVENT_SWITCH             (0x20)  // The weapon changed (jo_weapons_current()): the new one comes up

/** @brief Definition of a weapon (the fields of the section WEAP) */
typedef struct
{
    const char                      *name;
    const char                      *model;         // JOM file ("": none)
    const char                      *flash;         // Base name of the muzzle flashes
    const char                      *sound_shot;    // PCM files
    const char                      *sound_reload;
    jo_fixed                        x;              // Position of the model in front of the camera (world units)
    jo_fixed                        y;
    jo_fixed                        z;
    jo_fixed                        muzzle_y;       // The muzzle in the model
    jo_fixed                        muzzle_z;
    int                             convergence;
    int                             automatic;
    int                             fire_delay;  // Frames of the game (JO_FRAMERATE VBLANKs each) between 2 shots
    int                             damage;
    int                             spread;
    int                             recoil;
    int                             kick;
    int                             magazine;
    int                             ammo_type;
    int                             ammo_start;
    int                             ammo_max;
    int                             reload_time;  // Frames of the game (counted in VBLANKs: the same time when the frame rate goes down)
    int                             switch_time;  // Frames of the game
    int                             flash_size;
    int                             casing;
    int                             shot_volume;
}                                   jo_weapon_def;

/** @brief Read the definitions of the weapons (section WEAP): the player has all of them, with full magazines and the
 *  reserve of ammo_start; the first one is taken
 *  @param level Level (it can be freed after)
 *  @return Number of weapons
 */
int                                 jo_weapons_load(jo_level * const level);

/** @brief Number of weapons */
int                                 jo_weapons_count(void);

/** @brief Definition of a weapon
 *  @param index Index (0 to jo_weapons_count() - 1: the order of the section)
 */
const jo_weapon_def                 *jo_weapons_def(const int index);

/** @brief The weapon in the hands (index) */
int                                 jo_weapons_current(void);

/** @brief Take a weapon (a selection in a menu): the current one goes down, the new one comes up (like the switch button
 *  of jo_weapons_update(), which takes the next one)
 *  @param index Index of the weapon (nothing happens when it is the current one, or while a weapon changes)
 */
void                                jo_weapons_select(const int index);

/** @brief Rules of the weapons, once per frame of the game
 *  @param fire_held The fire button is held (automatic weapons)
 *  @param fire_pressed The fire button was pressed in this frame (the other weapons)
 *  @param reload_pressed The reload button was pressed
 *  @param switch_pressed The button to change of weapon was pressed: the next weapon
 *  @return JO_WEAPON_EVENT_xxx (bits)
 */
unsigned int                        jo_weapons_update(const bool fire_held, const bool fire_pressed, const bool reload_pressed,
                                                      const bool switch_pressed);

/** @brief Bullets in the magazine of the current weapon */
int                                 jo_weapons_ammo(void);

/** @brief Reserve of ammunition of the current weapon (its type) */
int                                 jo_weapons_reserve(void);

/** @brief Reload of the current weapon: 0 (not reloading), else 1 to 256 */
int                                 jo_weapons_reload_progress(void);

/** @brief The weapon goes down and up when it is changed: 0 (in the hands) to 256 (out of the view) */
int                                 jo_weapons_lowered(void);

/** @brief Give ammunition (a pickup)
 *  @param ammo_type Type of ammunition
 *  @param count Bullets (the reserve doesn't go over ammo_max, nor under 0; a reserve already over it stays)
 */
void                                jo_weapons_give_ammo(const int ammo_type, const int count);

#endif /* !JO_COMPILE_WITH_LEVEL_SUPPORT */

#endif /* !__JO_WEAPON_H__ */

/*
** END OF FILE
*/
