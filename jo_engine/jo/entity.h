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
/** @file entity.h
 *  @author Johannes Fetz
 *
 *  @brief Entities: the things of a level that live (barrels that explode, trigger zones, relays, doors, enemies...).
 *         They are read from a level (sections ENTI, PROP and LINK, see Documentation/level_format.md)
 *  @remarks Module JO_COMPILE_WITH_LEVEL_MODULE = 1 in the makefile (and JO_COMPILE_WITH_EFFECTS_MODULE = 1 for the fire,
 *           the smoke, the flash and the shake of the explosions, see effects.h)
 *
 *  - An entity has a class: its behavior. The engine has the generic classes (explosive, trigger, relay...), the game
 *    registers its own classes with jo_entity_register_class() before jo_entities_load()
 *  - Events and actions: when something happens to an entity (EVENT_DEATH, EVENT_ENTER...), the links of the level send
 *    actions to other entities (ACTION_DAMAGE, ACTION_TRIGGER...), with a delay. The game doesn't wire anything
 *  - Damage: health, EVENT_DAMAGE, EVENT_DEATH, damage around a point (the explosions damage the other barrels: chain
 *    reactions); the damage of the player is given to the game (jo_entities_set_area_damage_callback())
 *  - Only the entities near the player (or that wait for something) are updated
 */

#ifndef __JO_ENTITY_H__
# define __JO_ENTITY_H__

#if defined(JO_COMPILE_WITH_LEVEL_SUPPORT) && defined(JO_COMPILE_WITH_3D_SUPPORT)

# include "level.h"

/** @brief Entities at most (they are allocated with their count when a level is loaded) */
# define JO_ENTITY_MAX                      (1024)
/** @brief Classes at most (the engine uses 4) */
# define JO_ENTITY_MAX_CLASSES              (24)
/** @brief Actions waiting for their delay at most */
# define JO_ENTITY_MAX_PENDING              (32)
/** @brief Different models of the entities at most (the models of the classes too: the things broken, the pieces) */
# define JO_ENTITY_MAX_MODELS               (32)
/** @brief The entities farther from the player are not updated (world units), unless they wait for something (timer) */
# define JO_ENTITY_WAKE_DISTANCE            (3000)
/** @brief The models of the entities farther are not drawn (world units), see the property draw_distance */
# define JO_ENTITY_DRAW_DISTANCE            (2000)
/** @brief The far model of the entities is drawn farther (world units), see the properties far_model and far_distance */
# define JO_ENTITY_FAR_DISTANCE             (600)

/** @brief Flags of an entity: the ones of the level (ENTITY_FLAG_xxx of the schema) */
# define JO_ENTITY_FLAG_DISABLED            (0x0001)
/** @brief A solid entity (the collisions, the bullets): set it with jo_entity_set_solid(); set in the flags directly, it is
 *  solid from the next jo_entities_update() (the collisions read a list of the solid entities) */
# define JO_ENTITY_FLAG_SOLID               (0x0002)
# define JO_ENTITY_FLAG_HIDDEN              (0x0004)
/** @brief Flags of an entity: the state */
# define JO_ENTITY_FLAG_DEAD                (0x0100)
# define JO_ENTITY_FLAG_PLAYER_INSIDE       (0x0200)
/** @brief Flag of an entity set by its class: in a doorway between 2 cells (a door), drawn when one of the cells around it
 *  (its position +- its radius on X and Z) is drawn (its position alone: a cell hidden behind the door closed) */
# define JO_ENTITY_FLAG_DOORWAY             (0x0400)

typedef struct __jo_entity                  jo_entity;

/** @brief Class of entities: its callbacks (JO_NULL: nothing to do) */
typedef struct
{
    const char                      *name;          // Name in the level (ENTI.cls)
    unsigned int                    data_size;      // Bytes allocated for each entity (entity->data), 0: none
    /** @brief The entity is created (its properties can be read) */
    void                            (*spawn)(jo_entity * const entity);
    /** @brief Each frame of the game, while the entity is awake (near the player or entity->timer > 0)
     *  @param elapsed Time since the last update (milliseconds)
     */
    void                            (*update)(jo_entity * const entity, const int elapsed);
    /** @brief An action is received
     *  @return true if the class did it, false: the engine does what it does by default (see jo_entity_send())
     */
    bool                            (*action)(jo_entity * const entity, const int action, const int param, jo_entity * const source);
    /** @brief The health reached 0 (before the event DEATH) */
    void                            (*death)(jo_entity * const entity, jo_entity * const source);
    /** @brief Draw the entity (the camera matrix is the current matrix), JO_NULL: its model at its position */
    void                            (*draw)(jo_entity * const entity);
    /** @brief Draw the entity on the slave CPU (see jo_entities_draw_slave(): the slave 3D calls, jo_actor_draw_slave()),
     *  JO_NULL: the master CPU draws it (its draw callback). An entity without draw callback is drawn by both CPUs */
    void                            (*draw_slave)(jo_entity * const entity);
    /** @brief The tint of the entity (the light of the lamps on it, see jo_3d_point_lights_sample()) before it is drawn:
     *  the class changes it (a character hit: red), JO_NULL: the light of the lamps */
    void                            (*tint)(jo_entity * const entity, int * const r, int * const g, int * const b);
}                                   jo_entity_class;

/** @brief An entity */
struct __jo_entity
{
    // (the first 32 bytes: what the collisions read for each entity (jo_entities_is_blocked_at()), 2 lines of the cache)
    unsigned int                    flags;          // JO_ENTITY_FLAG_xxx
    jo_pos3D_fixed                  position;       // Foot (world space, Saturn Y down)
    int                             radius;         // Cylinder: collisions, bullets, zone of a trigger (world units)
    int                             height;
    short                           box_x;          // Box turned with the entity (half sizes, world units) for the collisions
    short                           box_z;          // of the player and the entities (jo_entities_get_blocking()), 0: the cylinder
    int                             yaw;            // Degrees
    const jo_entity_class           *cls;
    const char                      *name;
    int                             health;         // 0: can't be damaged (or dead: JO_ENTITY_FLAG_DEAD)
    jo_3d_mesh                      *mesh;          // Model drawn (JO_NULL: none)
    jo_3d_mesh                      *far_mesh;      // Model drawn far away (JO_NULL: mesh)
    jo_pos3D_fixed                  center;         // Bounding sphere of the model (world space)
    int                             mesh_radius;
    int                             far_distance;   // The far model is drawn farther (world units, on the ground)
    int                             draw_distance;  // The model is not drawn farther (world units, on the ground)
    int                             timer;          // Free for the class (milliseconds): the entity stays awake while > 0
    int                             state;          // Free for the class
    void                            *data;          // Data of the class (jo_entity_class.data_size)
    unsigned short                  first_prop;
    unsigned short                  prop_count;
    unsigned short                  first_link;
    unsigned short                  link_count;
    // (internal) The cells of the entity for its depth bias (jo_entities_draw()): at its foot, then on the 4 sides of a
    // door (-2: not found yet), found at this position in this generation of the cells (jo_cells_generation())
    jo_pos3D_fixed                  cell_position;
    short                           cells[5];
    unsigned short                  cell_generation;
    // (internal) Prepared for the frame (see jo_entities_prepare_draw()): the distance of the view (see
    // jo_entity_view_distance(), -1: not drawn), the tint (packed, see jo_3d_get_tint())
    short                           view_distance;
    unsigned short                  tint;
} __attribute__((aligned(16)));     // (128 bytes: an entity of the table on 8 lines of the cache, see jo_entities_load())

/** @brief Damage around a point given to the game (the player is not an entity)
 *  @param center Center (world space)
 *  @param radius Radius (world units)
 *  @param damage Damage at the center (less farther, 0 at the radius)
 *  @param source Entity that made it (JO_NULL: the game)
 */
typedef void                        (*jo_entity_area_damage_callback)(const jo_pos3D_fixed * const center, const int radius,
                                                                      const int damage, jo_entity * const source);

/** @brief Register a class of the game (before jo_entities_load())
 *  @param cls Class (it must stay in memory). A class with the name of a class of the engine replaces it
 */
void                                jo_entity_register_class(const jo_entity_class * const cls);

/** @brief Create the entities of a level (sections ENTI, PROP, LINK): the previous ones are freed
 *  @param level Level (it can be freed after: what the entities need is copied)
 *  @return Number of entities
 *  @remarks The models are loaded (jo_3d_load_model()), the classes spawn their entities. An unknown class: the entity
 *           only draws its model (the name of the class is printed with JO_DEBUG)
 */
int                                 jo_entities_load(jo_level * const level);

/** @brief Free the entities (and their models) */
void                                jo_entities_free(void);

/** @brief Update the entities: the delayed actions, the classes, the trigger zones
 *  @param player Position of the player (foot, world space)
 *  @param elapsed Time since the last update (milliseconds)
 */
void                                jo_entities_update(const jo_pos3D_fixed * const player, const int elapsed);

/** @brief Prepare the drawing of the entities of the frame (the camera matrix is the current matrix, see
 *  jo_3d_set_depth_bias()): the ones seen, their tint (the light of the lamps, see jo_3d_point_lights_sample()), the share
 *  of the slave CPU (see jo_entities_draw_slave()). Before jo_3d_slave_begin() and the job of the slave CPU
 *  @param slave_share Share of the cost of the entities drawn by the slave CPU (0 to 1024, 0: all by the master CPU): the
 *         entities with a draw_slave callback or without draw callback, the others stay on the master CPU
 *  @remarks Without it, jo_entities_draw() prepares the frame with everything on the master CPU
 */
void                                jo_entities_prepare_draw(const int slave_share);

/** @brief Draw the entities of the master CPU (the camera matrix is the current matrix, see jo_3d_set_depth_bias())
 *  @return Number of entities drawn
 *  @remarks The entities with a draw callback are drawn after the ones of the slave CPU with a draw_slave callback (the
 *  master CPU waits for them: the callbacks of a class share its data, the faces of the characters)
 */
int                                 jo_entities_draw(void);

/** @brief Draw the entities of the slave CPU (call it on the slave CPU, in the job between jo_3d_slave_begin() and
 *  jo_3d_slave_end(), see jo_entities_prepare_draw()): first the ones with a draw_slave callback, then the models
 *  @return Number of entities drawn
 */
int                                 jo_entities_draw_slave(void);

/** @brief Time the master CPU waited for the entities of the slave CPU in the last frame (see jo_entities_draw())
 *  @return Microseconds
 */
int                                 jo_entities_slave_wait_time(void);
int                                 jo_entities_test_counts(const int which);

/** @brief Polygons of the entities of a CPU in the frame (see jo_entities_prepare_draw(): the polygons of their models, plus
 *  8 each), to balance the CPUs
 *  @param slave false: the master CPU, true: the slave CPU
 *  @return Polygons
 */
int                                 jo_entities_polygon_count(const bool slave);

/** @brief Number of entities */
int                                 jo_entity_count(void);

/** @brief An entity
 *  @param index Index (0 to jo_entity_count() - 1: the order of the level)
 */
jo_entity                           *jo_entity_get(const int index);

/** @brief Find an entity by its name
 *  @return The entity, JO_NULL if there is none
 */
jo_entity                           *jo_entity_find(const char * const name);

/** @brief Send an action to an entity now
 *  @param target Target
 *  @param action JO_LEVEL_ACTION_xxx (or an action of the game: from 100)
 *  @param param Parameter (the damage of DAMAGE)
 *  @param source Entity that sent it (JO_NULL: the game)
 *  @remarks If the class doesn't do it: TRIGGER and USE fire the event TRIGGER, DAMAGE damages, KILL kills, ENABLE,
 *           DISABLE and TOGGLE change JO_ENTITY_FLAG_DISABLED. A disabled entity ignores the other actions
 */
void                                jo_entity_send(jo_entity * const target, const int action, const int param, jo_entity * const source);

/** @brief Fire an event of an entity: its links send their actions (now or after their delay)
 *  @param source Entity
 *  @param event JO_LEVEL_EVENT_xxx
 *  @param param Parameter (given to the actions of the links with the parameter -1)
 */
void                                jo_entity_fire(jo_entity * const source, const int event, const int param);

/** @brief Damage an entity: EVENT_DAMAGE, or the death (class death(), EVENT_DEATH) when its health reaches 0
 *  @param entity Entity (nothing if it is dead, disabled or can't be damaged)
 *  @param damage Damage
 *  @param source Entity that did it (JO_NULL: the game, the player)
 */
void                                jo_entity_damage(jo_entity * const entity, const int damage, jo_entity * const source);

/** @brief Damage everything around a point: the entities (their foot) and the game (see jo_entities_set_area_damage_callback())
 *  @param center Center (world space)
 *  @param radius Radius (world units)
 *  @param damage Damage at the center (less farther, 0 at the radius)
 *  @param source Entity that made it (it isn't damaged), JO_NULL: the game
 */
void                                jo_entities_damage_area(const jo_pos3D_fixed * const center, const int radius, const int damage,
                                                            jo_entity * const source);

/** @brief Give the damage around a point to the game (the player)
 *  @param callback Callback, JO_NULL: none
 */
void                                jo_entities_set_area_damage_callback(const jo_entity_area_damage_callback callback);

/** @brief First solid entity hit by a ray (its cylinder)
 *  @param origin Origin (world space)
 *  @param dir Direction (unit vector, Q16.16)
 *  @param max_t Length of the ray (world units, Q16.16)
 *  @param t Distance of the hit (output, Q16.16)
 *  @param normal Normal of the cylinder at the hit (output, unit vector)
 *  @return The entity, JO_NULL if nothing is hit
 */
jo_entity                           *jo_entities_ray_hit(const jo_pos3D_fixed * const origin, const jo_pos3D_fixed * const dir,
                                                         const jo_fixed max_t, jo_fixed * const t, jo_pos3D_fixed * const normal);

/** @brief Check if a circle on the ground is in a solid entity (collisions of the player)
 *  @param x Center (world space)
 *  @param z Center
 *  @param radius Radius (world units)
 *  @return true if it is blocked
 */
bool                                jo_entities_is_blocked(const jo_fixed x, const jo_fixed z, const int radius);

/** @brief Like jo_entities_is_blocked(), with the heights: a solid entity blocks only a body at its level (a low piece of
 *  furniture doesn't block the player in a duct over it)
 *  @param x Position of the feet
 *  @param y Position of the feet (Saturn Y: down)
 *  @param z Position of the feet
 *  @param radius Radius of the body (world units)
 *  @param height Height of the body (world units)
 *  @param except Entity not checked (the body itself), JO_NULL: none
 *  @return true if a solid entity is in the way (the entity goes from its feet up to its height)
 */
bool                                jo_entities_is_blocked_at(const jo_fixed x, const jo_fixed y, const jo_fixed z, const int radius,
                                                              const int height, const jo_entity * const except);

/** @brief Make an entity solid or not (see JO_ENTITY_FLAG_SOLID)
 *  @param entity Entity
 *  @param solid true: solid
 */
void                                jo_entity_set_solid(jo_entity * const entity, const bool solid);

/** @brief The solid entity in a circle on the ground (collisions of a vehicle)
 *  @param x Center (world space)
 *  @param z Center
 *  @param radius Radius (world units)
 *  @param except Entity not checked (the vehicle itself), JO_NULL: none
 *  @return The first entity found, JO_NULL: none
 */
jo_entity                           *jo_entities_get_blocking(const jo_fixed x, const jo_fixed z, const int radius,
                                                              const jo_entity * const except);

/** @brief Property of an entity (PROP)
 *  @param entity Entity
 *  @param key Name
 *  @param default_value Value when the entity doesn't have it
 *  @return The value (an INT property read as fixed is converted, and the opposite)
 */
int                                 jo_entity_get_int(const jo_entity * const entity, const char * const key, const int default_value);
jo_fixed                            jo_entity_get_fixed(const jo_entity * const entity, const char * const key, const jo_fixed default_value);
const char                          *jo_entity_get_string(const jo_entity * const entity, const char * const key,
                                                          const char * const default_value);

/** @brief Change of level asked by an entity zone_link (the player went in it): the game fades the screen, loads the
 *  level (jo_entities_load(), jo_cells_load()...), puts the player on the entity spawn, then calls
 *  jo_entities_clear_zone_request()
 *  @param level JOL file of the level (output, valid until the next request)
 *  @param spawn Name of the entity of the level where the player appears (output, "": the game chooses)
 *  @return true if a change of level is asked
 */
bool                                jo_entities_get_zone_request(const char ** const level, const char ** const spawn);

/** @brief The change of level is done (see jo_entities_get_zone_request()) */
void                                jo_entities_clear_zone_request(void);

/** @brief Load a model for the entities (loaded once: shared by all the entities)
 *  @param filename JOM file
 *  @return Its mesh, JO_NULL if it can't be loaded
 */
jo_3d_mesh                          *jo_entity_load_model(const char * const filename);

/** @brief Load a model for the entities (loaded once: shared by all the entities)
 *  @param filename JOM file
 *  @return The model, JO_NULL if it can't be loaded
 */
jo_3d_model                         *jo_entity_get_model(const char * const filename);

/** @brief Change the model of an entity (the dead model of an explosive...)
 *  @param entity Entity
 *  @param mesh Mesh (JO_NULL: none)
 *  @remarks The far model is not used any more (see the property far_model)
 */
void                                jo_entity_set_mesh(jo_entity * const entity, jo_3d_mesh * const mesh);

/** @brief Check if the model of an entity is drawn (near enough, in the view)
 *  @param entity Entity
 *  @return Distance from the player on the ground (world units), -1: not drawn
 *  @remarks For the draw callback of the classes (the camera matrix is the current matrix)
 */
int                                 jo_entity_view_distance(const jo_entity * const entity);

/** @brief A magnified view (binoculars, a sniper scope): the entities are drawn and detailed as far as they look
 *  @param scale The magnification (1: normal): the distances of jo_entity_view_distance() are divided by it
 */
void                                jo_entities_set_lod_scale(const int scale);

/** @brief Turn an entity toward a point on the ground (its model looks toward +Z turned by its yaw: the direction
 *  (sin yaw, cos yaw))
 *  @param entity Entity
 *  @param x Point (world space)
 *  @param z Point
 *  @param max_step The most it turns (degrees)
 *  @return The angle left (degrees, 0 to 180)
 */
int                                 jo_entity_turn_to(jo_entity * const entity, const jo_fixed x, const jo_fixed z, const int max_step);

/*
** Actors: entities with an animated model (a skeleton, see jo_3d_model_draw_pose()). The class "actor" of the engine plays
** the animation of its property animation (a character who waits, a flag); the classes of the game (enemies) start their
** data with a jo_actor and call jo_actor_spawn(), jo_actor_update() and jo_actor_draw()
*/

/** @brief Called when an actor is drawn, after each bone (a weapon in a hand, a muzzle flash)
 *  @param entity The actor
 *  @param bone Index of the bone
 *  @param matrix Matrix of the bone (see jo_3d_transform_point_by())
 */
typedef void                        (*jo_actor_bone_callback)(jo_entity * const entity, const int bone, const jo_fixed (* const matrix)[3]);

/** @brief Data of an actor (the first member of the data of a class of actors) */
typedef struct
{
    jo_3d_model                     *model;
    jo_3d_model                     *far_model;     // (property far_model, with a skeleton too) JO_NULL: model
    short                           animation;      // Index in the model, -1: none
    short                           previous;       // Animation before the change (blended), -1: none
    int                             time;           // Milliseconds from the start of the animation
    int                             previous_time;
    int                             blend;          // Milliseconds left of the blend with the previous animation
    int                             blend_duration;
    jo_actor_bone_callback          bone_callback;  // JO_NULL: none
}                                   jo_actor;

/** @brief Create the actor of an entity: its model (the model of the entity, it has a skeleton), the animation of its
 *  property animation ("idle" by default)
 *  @param entity Entity (its data starts with a jo_actor)
 */
void                                jo_actor_spawn(jo_entity * const entity);

/** @brief Advance the animation of an actor
 *  @param entity Entity
 *  @param elapsed Milliseconds
 */
void                                jo_actor_update(jo_entity * const entity, const int elapsed);

/** @brief Draw an actor (its model in the pose of its animation) at its position, turned by its yaw
 *  @param entity Entity
 */
void                                jo_actor_draw(jo_entity * const entity);

/** @brief Draw an actor on the slave CPU (call it on the slave CPU, see jo_entities_draw_slave()): its model in the pose of
 *  its animation at its position, turned by its yaw, with the distance and the tint prepared (see jo_entities_prepare_draw())
 *  @param entity Entity
 *  @param far_distance The far model farther (world units, on the ground), -1: the one of the entity
 *  @param hidden_bones Bones not drawn (bits, see jo_3d_model_set_hidden_bones())
 */
void                                jo_actor_draw_slave(jo_entity * const entity, const int far_distance, const unsigned int hidden_bones);

/** @brief Play an animation (nothing if it is already played)
 *  @param entity Entity
 *  @param name Name of the animation
 *  @param blend Milliseconds of the change (blend from the previous pose), 0: at once
 *  @return false if the model doesn't have this animation
 */
bool                                jo_actor_play(jo_entity * const entity, const char * const name, const int blend);

/** @brief Play an animation again from its start
 *  @param entity Entity
 *  @param name Name of the animation
 *  @param blend Milliseconds of the change
 *  @return false if the model doesn't have this animation
 */
bool                                jo_actor_restart(jo_entity * const entity, const char * const name, const int blend);

/** @brief Check if the animation of an actor is at its end (the last key of an animation that doesn't loop)
 *  @param entity Entity
 *  @return true at the end (a loop: never)
 */
bool                                jo_actor_is_done(const jo_entity * const entity);

/** @brief Check which animation an actor plays
 *  @param entity Entity
 *  @param name Name of the animation
 *  @return true if it plays it
 */
bool                                jo_actor_is_playing(const jo_entity * const entity, const char * const name);

#endif /* !JO_COMPILE_WITH_LEVEL_SUPPORT && JO_COMPILE_WITH_3D_SUPPORT */

#endif /* !__JO_ENTITY_H__ */

/*
** END OF FILE
*/
