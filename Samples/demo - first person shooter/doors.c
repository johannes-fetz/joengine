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
** Doors: entities of the class "door". A panel in the opening of a wall that slides up when the agent uses it (B in
** front of it), stays open 5 seconds, then closes unless someone stands in it. Closed, it blocks the agent and the
** bullets (see doors_block() and doors_ray_hit()).
**
** Properties (the level): slide (units the panel goes up), side (0: it goes up; 1 or -1: it slides along its width,
** toward the +X or the -X of its model: a door in a pocket of the wall, the airlock), time (ms of the slide), sound (WAV file: it opens),
** sound_close (WAV file: it closes, else sound), close_hit (ms of the closing sound when the door hits its frame: the
** sound starts so that it hits when the door is closed), pitch (percent: the sounds played higher, a lighter door)
** The model looks toward +Z (its panel in the plane Z = 0, centered): the entity is turned by its yaw (90: a door in a
** wall along Z)
*/

#include "demo.h"

/** @brief An open door closes by itself after this long (ms), whoever opened it (not while someone stands in it); the agent
 *  can close it before (B) */
#define DOOR_OPEN_TIME          (5000)
/** @brief The agent must be this near to use a door (units) and in front of it (the cosine of the angle) */
#define DOOR_USE_DISTANCE       (140)
/** @brief Half thickness of a door for the collisions (units) */
#define DOOR_HALF_THICKNESS     (8)

enum
{
    DOOR_CLOSED,
    DOOR_OPENING,
    DOOR_OPEN,
    DOOR_CLOSING
};

/** @brief Who opens a door (it turns away from him): the agent, or a guard (see doors_open_for()) */
static jo_fixed                 opener_x = 0;
static jo_fixed                 opener_z = 0;
static bool                     opener_set = false;

/** @brief The portals of a doorway: their middle this near the middle of the door (world units, see seal()) */
#define DOORWAY_PORTAL_RADIUS   (24)
/** @brief The rooms on the sides of a doorway: this far from the door (world units) */
#define DOORWAY_SIDE            (60)

typedef struct
{
    int                         slide;          // Units (a sliding door)
    int                         side;           // 0: the panel goes up; 1, -1: it slides along the X axis of its model
    bool                        hinge;          // A hinged door: it turns around the Y axis of its model
    int                         angle;          // Of a hinged door open (degrees)
    int                         direction;      // Of the turn: 1 or -1 (away from the agent who opened it)
    int                         time;           // ms of the slide
    int                         progress;       // 0 (closed) to 65536 (open)
    int                         wait;           // ms left open
    int                         cell;           // 0: the door has portals (a door between 2 rooms), -1: none
    jo_fixed                    x;              // The middle of the doorway
    jo_fixed                    z;
    bool                        sealed;         // Its portals closed (the door closed: the room behind it is not drawn)
    jo_sound                    *sound;         // Its sounds (opening, closing: the same one if no sound_close), JO_NULL: none
    jo_sound                    *sound_close;
    int                         close_hit;      // ms from the start of the closing sound to the door hitting its frame
    bool                        close_due;      // Closing: its sound not started yet (it starts close_hit ms before the end)
    bool                        reopen;         // Closing to open again the other way (a guard behind its panel)
    jo_fixed                    reopen_x;       // Who opens it again (it turns away from him)
    jo_fixed                    reopen_z;
    bool                        bounced;        // Closing: its panel turned onto the agent (it doesn't open again for him)
    bool                        guard_opened;   // Opening for a guard (see bounced)
}                               door_data;

/** @brief Angle of a hinged door open (degrees), the property "angle": the doors of the stalls open less (80), else the panel
 *  lies on the partition, on its plane: the painter's sort of the VDP1 draws one over the other, the door disappears */
#define HINGE_ANGLE             (90)

/** @brief The sounds of the doors, each file loaded once: the doors with the same sound share it (the doors of the
 *  stalls), a door with another pitch gets a copy of it played at another rate (the same samples in the sound RAM) */
#define DOOR_SOUNDS             (8)
static jo_sound                 door_sounds[DOOR_SOUNDS];
static char                     door_sound_names[DOOR_SOUNDS][16];
static int                      door_sound_pitches[DOOR_SOUNDS];
static int                      door_sound_count = 0;
/** @brief The doors are heard up to there (units), their volume */
#define DOOR_SOUND_RANGE        (1600)
#define DOOR_SOUND_VOLUME       (115)

/** @brief The sound of a file played at sound_pitch percent (loaded the first time), JO_NULL if it can't be loaded */
static jo_sound                 *door_sound(const char * const name, const int sound_pitch)
{
    int                         k;
    int                         same;

    if (name[0] == '\0' || jo_strlen(name) >= 16 || door_sound_count >= DOOR_SOUNDS)
        return (JO_NULL);
    same = -1;
    for (k = 0; k < door_sound_count; ++k)
        if (jo_strcmp(door_sound_names[k], name) == 0)
        {
            if (door_sound_pitches[k] == sound_pitch)
                return (&door_sounds[k]);
            same = k;
        }
    if (same >= 0)
    {
        // (the same samples at another rate)
        door_sounds[door_sound_count] = door_sounds[same];
        door_sounds[door_sound_count].sample_rate = (unsigned short)(door_sounds[same].sample_rate * sound_pitch / door_sound_pitches[same]);
    }
    else
    {
        if (!load_sound(name, &door_sounds[door_sound_count]))
            return (JO_NULL);
        door_sounds[door_sound_count].sample_rate = (unsigned short)(door_sounds[door_sound_count].sample_rate * sound_pitch / 100);
    }
    jo_memcpy(door_sound_names[door_sound_count], name, (unsigned int)jo_strlen(name) + 1);
    door_sound_pitches[door_sound_count] = sound_pitch;
    return (&door_sounds[door_sound_count++]);
}

/** @brief The sound of a door (it opens or it closes), from where it is */
static void                     door_play(const jo_entity * const entity, const door_data * const d, const bool closing)
{
    jo_sound                    *sound;

    sound = closing ? d->sound_close : d->sound;
    if (sound == JO_NULL)
        return;
    // (from the middle of the doorway)
    JO_UNUSED_ARG(entity);
    sound_from(sound, d->x, d->z, DOOR_SOUND_VOLUME, DOOR_SOUND_RANGE);
    jo_audio_stop_channel(CHANNEL_DOOR);
    jo_audio_play_sound_on_channel(sound, CHANNEL_DOOR);
}

static void                     seal(jo_entity * const entity);


/** @brief The doors among the entities (built once: the loops over the doors went through all the entities, the rays of
 *  the lamps at the start of the level took 1 s in them) */
static jo_entity                **door_entities = JO_NULL;
static int                      door_entity_count = 0;
/** @brief The guards among the entities (built with the doors: the doors open check at each frame if a guard is near them,
 *  through all the entities, 5 % of the master CPU when the guards hunted the agent) */
static jo_entity                **guard_entities = JO_NULL;
static int                      guard_entity_count = 0;
/** @brief The panes of the windows among the entities (each ray of the guards went through all the entities) */
static jo_entity                **glass_entities = JO_NULL;
static int                      glass_entity_count = 0;
static jo_entity                *door_entities_of = JO_NULL;    // (the entities they were found in: another level, again)
static int                      door_entities_total = -1;

static const jo_entity_class    door_class;
static const jo_entity_class    glass_class;

/** @brief A guard (the class of guards.c) */
static bool                     is_guard(const jo_entity * const e)
{
    return (e->cls != JO_NULL && e->cls->name != JO_NULL && e->cls->name[0] == 'g' && jo_strcmp(e->cls->name, "guard") == 0);
}

static void                     doors_index(void)
{
    int                         k;
    int                         n;
    int                         guards;
    int                         panes;

    if (door_entities_of == jo_entity_get(0) && door_entities_total == jo_entity_count())
        return;
    if (door_entities != JO_NULL)
        jo_free(door_entities);
    if (guard_entities != JO_NULL)
        jo_free(guard_entities);
    if (glass_entities != JO_NULL)
        jo_free(glass_entities);
    glass_entities = JO_NULL;
    glass_entity_count = 0;
    door_entities = JO_NULL;
    door_entity_count = 0;
    guard_entities = JO_NULL;
    guard_entity_count = 0;
    door_entities_of = jo_entity_get(0);
    door_entities_total = jo_entity_count();
    for (k = 0, n = 0, guards = 0, panes = 0; k < jo_entity_count(); ++k)
    {
        if (jo_entity_get(k)->cls == &door_class)
            ++n;
        else if (jo_entity_get(k)->cls == &glass_class)
            ++panes;
        else if (is_guard(jo_entity_get(k)))
            ++guards;
    }
    if (panes > 0 && (glass_entities = (jo_entity **)jo_malloc((unsigned int)panes * sizeof(*glass_entities))) != JO_NULL)
        for (k = 0; k < jo_entity_count(); ++k)
            if (jo_entity_get(k)->cls == &glass_class)
                glass_entities[glass_entity_count++] = jo_entity_get(k);
    if (guards > 0 && (guard_entities = (jo_entity **)jo_malloc((unsigned int)guards * sizeof(*guard_entities))) != JO_NULL)
        for (k = 0; k < jo_entity_count(); ++k)
            if (is_guard(jo_entity_get(k)))
                guard_entities[guard_entity_count++] = jo_entity_get(k);
    if (n == 0 || (door_entities = (jo_entity **)jo_malloc((unsigned int)n * sizeof(*door_entities))) == JO_NULL)
        return;
    for (k = 0; k < jo_entity_count(); ++k)
        if (jo_entity_get(k)->cls == &door_class)
            door_entities[door_entity_count++] = jo_entity_get(k);
}

/** @brief The door starts closing: its sound later (see close_hit) */
static void                     door_start_closing(jo_entity * const entity, door_data * const d)
{
    entity->state = DOOR_CLOSING;
    d->close_due = true;
}

static void                     door_spawn(jo_entity * const entity)
{
    door_data                   *d;
    const char                  *name;
    int                         sound_pitch;

    d = (door_data *)entity->data;
    d->slide = jo_entity_get_int(entity, "slide", 150);
    d->side = JO_MAX(-1, JO_MIN(1, jo_entity_get_int(entity, "side", 0)));
    d->hinge = jo_entity_get_int(entity, "hinge", 0) != 0;
    d->angle = jo_entity_get_int(entity, "angle", HINGE_ANGLE);
    d->direction = 1;
    d->time = JO_MAX(100, jo_entity_get_int(entity, "time", 700));
    d->progress = 0;
    d->wait = 0;
    entity->state = DOOR_CLOSED;
    // (the sounds: sound (opening), sound_close (closing, else the same one), pitch (percent: the light doors higher))
    sound_pitch = JO_MAX(25, jo_entity_get_int(entity, "pitch", 100));
    d->sound = door_sound(jo_entity_get_string(entity, "sound", ""), sound_pitch);
    name = jo_entity_get_string(entity, "sound_close", "");
    d->sound_close = name[0] != '\0' ? door_sound(name, sound_pitch) : d->sound;
    // (played faster at a higher pitch: the hit sooner)
    d->close_hit = jo_entity_get_int(entity, "close_hit", 0) * 100 / sound_pitch;
    d->close_due = false;
    d->reopen = false;
    d->guard_opened = false;
    d->bounced = false;
    // The doorway between 2 rooms (door_ROOM_ROOM: a cell of its own, thin): closed, the room behind it is not drawn
    d->cell = -1;
    d->sealed = false;
    // (the middle of the doorway: the panel closed goes from the hinge along the yaw; its sounds come from there)
    d->x = entity->position.x;
    d->z = entity->position.z;
    if (d->hinge)
    {
        d->x += jo_cos(entity->yaw) * (entity->radius / 2);
        d->z += -jo_sin(entity->yaw) * (entity->radius / 2);
    }
    if (entity->name != JO_NULL && entity->name[0] == 'd' && entity->name[4] == '_')
    {
        jo_fixed                x;
        jo_fixed                z;

        // (the portals of the doorway: the middle of the door, a few units around it)
        x = d->x;
        z = d->z;
        d->cell = jo_cells_close_portals(x, z, DOORWAY_PORTAL_RADIUS, false) > 0 ? 0 : -1;
        // (drawn when a room on one of its sides is drawn: the room behind it is hidden when it is closed)
        entity->flags |= JO_ENTITY_FLAG_DOORWAY;
        seal(entity);
    }
}

/** @brief Angle of the panel now (degrees): the yaw of the entity, plus the turn of a hinged door */
static int                      door_angle(const jo_entity * const entity)
{
    const door_data             *d;

    d = (const door_data *)entity->data;
    if (!d->hinge)
        return (entity->yaw);
    return (entity->yaw + d->direction * jo_fixed2int(d->progress * d->angle));
}

/** @brief Axes of the panel: (ax, az) along it (the X axis of the model, turned), the normal is (az, -ax)... see door_normal() */
static void                     door_axes(const jo_entity * const entity, jo_fixed * const ax, jo_fixed * const az)
{
    int                         angle;

    angle = door_angle(entity);
    *ax = jo_cos(angle);
    *az = -jo_sin(angle);
}

/** @brief Normal of the panel (the +Z of the model, turned) */
static void                     door_normal(const jo_entity * const entity, jo_fixed * const nx, jo_fixed * const nz)
{
    int                         angle;

    angle = door_angle(entity);
    *nx = jo_sin(angle);
    *nz = jo_cos(angle);
}

/** @brief Move of the panel of a sliding door now (up, or along its width), from its place closed */
static void                     door_slide_offset(const jo_entity * const entity, jo_pos3D_fixed * const offset)
{
    const door_data             *d;
    jo_fixed                    amount;

    d = (const door_data *)entity->data;
    offset->x = 0;
    offset->y = 0;
    offset->z = 0;
    if (d->hinge)
        return;
    amount = jo_fixed_mult(jo_int2fixed(d->slide), d->progress);
    if (d->side == 0)
    {
        offset->y = -amount;
        return;
    }
    // (along the X axis of the model turned by the yaw: (cos, -sin), see door_axes())
    offset->x = jo_fixed_mult(jo_cos(entity->yaw), amount) * d->side;
    offset->z = -jo_fixed_mult(jo_sin(entity->yaw), amount) * d->side;
}

/** @brief Check if a circle may meet the panel of a door in any position (near its place): false, door_meets_at() is false
 *  for all the angles (the same test at its start). The doors are tested for each move of everybody: this test first, before
 *  the data of the door are read */
static  inline bool             door_near(const jo_entity * const entity, const jo_fixed x, const jo_fixed z, const int radius)
{
    jo_fixed                    dx;
    jo_fixed                    dz;

    dx = x - entity->position.x;
    dz = z - entity->position.z;
    return (JO_ABS(dx) <= jo_int2fixed(entity->radius + radius + DOOR_HALF_THICKNESS) &&
            JO_ABS(dz) <= jo_int2fixed(entity->radius + radius + DOOR_HALF_THICKNESS));
}

/** @brief Check if a circle meets the panel of a door turned by an angle (its rectangle on the ground; a sliding door: the
 *  doorway) */
static bool                     door_meets_at(const jo_entity * const entity, const int angle, const jo_fixed x, const jo_fixed z,
                                              const int radius)
{
    jo_fixed                    ax;
    jo_fixed                    az;
    jo_fixed                    dx;
    jo_fixed                    dz;
    int                         along;
    int                         across;

    dx = x - entity->position.x;
    dz = z - entity->position.z;
    // (far from the panel in any position: no sine, no cosine; the doors are tested for each move of everybody)
    if (JO_ABS(dx) > jo_int2fixed(entity->radius + radius + DOOR_HALF_THICKNESS) ||
        JO_ABS(dz) > jo_int2fixed(entity->radius + radius + DOOR_HALF_THICKNESS))
        return (false);
    ax = jo_cos(angle);
    az = -jo_sin(angle);
    along = jo_fixed2int(jo_fixed_mult(dx, ax) + jo_fixed_mult(dz, az));
    across = jo_fixed2int(jo_fixed_mult(dx, -az) + jo_fixed_mult(dz, ax));
    if (((const door_data *)entity->data)->hinge)
        return (along > -radius && along < entity->radius + radius && JO_ABS(across) < DOOR_HALF_THICKNESS + radius);
    return (JO_ABS(along) < entity->radius + radius && JO_ABS(across) < DOOR_HALF_THICKNESS + radius);
}

/** @brief Check if a circle meets the panel of a door where it is (see door_meets_at()) */
static bool                     door_meets(const jo_entity * const entity, const jo_fixed x, const jo_fixed z, const int radius)
{
    // (far: door_angle() not computed, the data of the door not read)
    return (door_near(entity, x, z, radius) && door_meets_at(entity, door_angle(entity), x, z, radius));
}

/** @brief Check if a hinged door would turn onto the agent: its panel meets him on its way (a few angles), turned the
 *  way it opens for someone at opener_x opener_z (see door_action())
 *  @param direction Its turn: 1 or -1
 *  @param progress Its turn now (0: closed, 65536: open)
 */
static bool                     door_swing_meets_player(const jo_entity * const entity, const int direction, const int progress)
{
    const door_data             *d;
    int                         k;

    d = (const door_data *)entity->data;
    // (the agent in the doorway: the door doesn't close on him, see someone_in())
    if (door_meets_at(entity, entity->yaw, player_x, player_z, PLAYER_RADIUS))
        return (false);
    for (k = 1; k <= 4; ++k)
        if (k * 16384 > progress && door_meets_at(entity, entity->yaw + direction * d->angle * k / 4, player_x, player_z, PLAYER_RADIUS))
            return (true);
    return (false);
}

/** @brief A circle in the door: its panel where it is, or the doorway (a hinged door: the panel closed; it closed on a guard
 *  standing in the doorway, half of him through the door, and he still shot through it) */
static bool                     in_door(const jo_entity * const door, const jo_fixed x, const jo_fixed z, const int radius)
{
    return (door_near(door, x, z, radius) &&
            (door_meets(door, x, z, radius) || (((const door_data *)door->data)->hinge && door_meets_at(door, door->yaw, x, z, radius))));
}

/** @brief The agent (with_player) or a guard (alive) stands in the door (it doesn't close on him) */
static bool                     someone_in(const jo_entity * const door, const bool with_player)
{
    jo_entity                   *e;
    int                         k;

    if (with_player && in_door(door, player_x, player_z, PLAYER_RADIUS))
        return (true);
    doors_index();
    for (k = 0; k < guard_entity_count; ++k)
        if ((e = guard_entities[k]) != JO_NULL && !(e->flags & (JO_ENTITY_FLAG_DEAD | JO_ENTITY_FLAG_HIDDEN)) && e->health > 0 &&
            in_door(door, e->position.x, e->position.z, e->radius))
            return (true);
    return (false);
}

/** @brief An open door doesn't close while someone (the agent, a guard alive) is nearer than this to its doorway (units:
 *  3 m) */
#define DOOR_STAY_NEAR          (150)

/** @brief Someone near the doorway of a door (see DOOR_STAY_NEAR): the agent, a guard alive */
static bool                     someone_near(const jo_entity * const door)
{
    const door_data             *d;
    jo_entity                   *e;
    int                         k;

    d = (const door_data *)door->data;
    if (JO_ABS(player_x - d->x) < jo_int2fixed(DOOR_STAY_NEAR) && JO_ABS(player_z - d->z) < jo_int2fixed(DOOR_STAY_NEAR))
        return (true);
    doors_index();
    for (k = 0; k < guard_entity_count; ++k)
        if ((e = guard_entities[k]) != JO_NULL && !(e->flags & (JO_ENTITY_FLAG_DEAD | JO_ENTITY_FLAG_HIDDEN)) && e->health > 0 &&
            JO_ABS(e->position.x - d->x) < jo_int2fixed(DOOR_STAY_NEAR) && JO_ABS(e->position.z - d->z) < jo_int2fixed(DOOR_STAY_NEAR))
            return (true);
    return (false);
}

/** @brief The portals of the doorway closed when the door is closed (the room behind it is not drawn: its panel hides it),
 *  open as soon as it moves */
static void                     seal(jo_entity * const entity)
{
    door_data                   *d;
    bool                        closed;

    d = (door_data *)entity->data;
    closed = entity->state == DOOR_CLOSED;
    if (d->cell < 0 || closed == d->sealed)
        return;
    jo_cells_close_portals(d->x, d->z, DOORWAY_PORTAL_RADIUS, closed);
    d->sealed = closed;
}


static void                     door_update(jo_entity * const entity, const int elapsed)
{
    door_data                   *d;
    int                         step;

    d = (door_data *)entity->data;
    seal(entity);
    step = jo_int_div(elapsed * 65536, d->time);
    switch (entity->state)
    {
        case DOOR_OPENING:
            // A hinged door that turns onto the agent (opened by a guard, the agent behind it): it closes again, its panel
            // went through him (he was pushed in front of the guard). Not a door the agent opened: it turns away from him, he
            // follows its panel (it closed on him, he went through it)
            if (d->hinge && d->guard_opened && door_meets(entity, player_x, player_z, PLAYER_RADIUS) &&
                !door_meets_at(entity, entity->yaw, player_x, player_z, PLAYER_RADIUS))
            {
                d->bounced = true;
                d->reopen = false;
                door_start_closing(entity, d);
                break;
            }
            d->progress += step;
            if (d->progress >= 65536)
            {
                d->progress = 65536;
                entity->state = DOOR_OPEN;
            }
            break;
        case DOOR_OPEN:
            // (it closes after a while, not while someone is near it)
            if (someone_near(entity))
                d->wait = DOOR_OPEN_TIME;
            d->wait -= elapsed;
            if (d->wait <= 0)
                door_start_closing(entity, d);
            break;
        case DOOR_CLOSING:
            d->progress -= step;
            if (someone_in(entity, !d->bounced) && d->progress < 40000 &&
                !(d->hinge && door_swing_meets_player(entity, d->direction, d->progress)))
            {
                // Someone in the door: it opens again (the same way; not onto the agent)
                entity->state = DOOR_OPENING;
                d->close_due = false;
                d->reopen = false;
                break;
            }
            // (its sound: the door hits its frame in the sound when it is closed)
            if (d->close_due && (int)((long long)JO_MAX(d->progress, 0) * d->time / 65536) <= d->close_hit)
            {
                d->close_due = false;
                door_play(entity, d, true);
            }
            if (d->progress <= 0)
            {
                d->progress = 0;
                entity->state = DOOR_CLOSED;
                d->bounced = false;
                // (closed for a guard behind its panel: open again, away from him)
                if (d->reopen)
                {
                    d->reopen = false;
                    opener_x = d->reopen_x;
                    opener_z = d->reopen_z;
                    opener_set = true;
                    jo_entity_send(entity, JO_LEVEL_ACTION_TRIGGER, 0, JO_NULL);
                }
            }
            break;
        default:
            break;
    }
    seal(entity);
    // (awake while it moves)
    entity->timer = entity->state == DOOR_CLOSED ? 0 : 1000;
}

static bool                     door_action(jo_entity * const entity, const int action, const int param, jo_entity * const source)
{
    door_data                   *d;
    int                         direction;
    bool                        by_guard;

    (void)param;
    (void)source;
    if (action != JO_LEVEL_ACTION_USE && action != JO_LEVEL_ACTION_TRIGGER)
        return (false);
    d = (door_data *)entity->data;
    by_guard = opener_set;
    if (!opener_set)
    {
        opener_x = player_x;
        opener_z = player_z;
    }
    opener_set = false;
    if (entity->state == DOOR_CLOSED || entity->state == DOOR_CLOSING)
    {
        direction = d->direction;
        if (d->hinge && entity->state == DOOR_CLOSED)
        {
            // It turns away from the agent: his side of the panel (the normal of the model is its +Z)
            jo_fixed            nx;
            jo_fixed            nz;

            door_normal(entity, &nx, &nz);
            // (opened by a guard: away from him)
            direction = jo_fixed_mult(opener_x - entity->position.x, nx) + jo_fixed_mult(opener_z - entity->position.z, nz) > 0 ? 1 : -1;
        }
        // A guard doesn't open a hinged door onto the agent behind it: it stays closed (its panel went through the agent)
        if (by_guard && d->hinge && door_swing_meets_player(entity, direction, d->progress))
        {
            d->reopen = false;
            return (true);
        }
        d->direction = direction;
        d->guard_opened = by_guard;
        entity->state = DOOR_OPENING;
        // (it closes by itself after DOOR_OPEN_TIME, whoever opened it)
        d->wait = DOOR_OPEN_TIME;
        d->close_due = false;
        door_play(entity, d, false);
    }
    else if (action == JO_LEVEL_ACTION_USE && !in_door(entity, player_x, player_z, PLAYER_RADIUS))
    {
        // B again on an open door: it closes (not on the agent, its panel or its doorway)
        door_start_closing(entity, d);
    }
    return (true);
}

int                             doors_depth_bias(const jo_entity * const entity)
{
    const door_data             *d;
    int                         bias;

    d = (const door_data *)entity->data;
    // (the panel is drawn with the depth of the cell where it is: the middle of the panel. A hinged door open is in the room
    // it turned into; with the cell of its hinge (on the wall between the rooms: the other room), the walls of the room
    // were drawn over it, the door was not seen from the inside)
    if (d->hinge)
    {
        jo_fixed                ax;
        jo_fixed                az;

        door_axes(entity, &ax, &az);
        bias = jo_cells_depth_bias_at(entity->position.x + ax * (entity->radius / 2), entity->position.y - JO_FIXED_1,
                                      entity->position.z + az * (entity->radius / 2));
    }
    else
        bias = jo_cells_depth_bias_at(entity->position.x, entity->position.y, entity->position.z);
    // (a closed door between 2 rooms: in the doorway, the room behind it is not drawn; the room on the other side)
    if (bias < 0 && d->cell >= 0)
    {
        jo_fixed                nx;
        jo_fixed                nz;
        int                     side;

        door_normal(entity, &nx, &nz);
        for (side = -1; side <= 1 && bias < 0; side += 2)
            bias = jo_cells_depth_bias_at(d->x + nx * side * DOORWAY_SIDE, entity->position.y - JO_FIXED_1, d->z + nz * side * DOORWAY_SIDE);
    }
    return (bias);
}

static void                     door_draw(jo_entity * const entity)
{
    door_data                   *d;
    int                         bias;

    d = (door_data *)entity->data;
    if (entity->mesh == JO_NULL || jo_entity_view_distance(entity) < 0)
        return;
    bias = doors_depth_bias(entity);
    if (bias < 0)
        return;
    jo_3d_push_matrix();
    {
        jo_3d_set_depth_bias(WORLD_DEPTH_BIAS + bias);
        if (d->hinge)
        {
            jo_3d_translate_matrix_fixed(entity->position.x, entity->position.y, entity->position.z);
            jo_3d_rotate_matrix_y(door_angle(entity));
        }
        else
        {
            jo_pos3D_fixed      offset;

            door_slide_offset(entity, &offset);
            jo_3d_translate_matrix_fixed(entity->position.x + offset.x, entity->position.y + offset.y, entity->position.z + offset.z);
            jo_3d_rotate_matrix_y(entity->yaw);
        }
        jo_3d_mesh_draw(entity->mesh);
        jo_3d_set_depth_bias(WORLD_DEPTH_BIAS);
    }
    jo_3d_pop_matrix();
}

/** @brief Draw a door on the slave CPU (see door_draw(): the distance of the view prepared by the master CPU) */
static void                     door_draw_slave(jo_entity * const entity)
{
    door_data                   *d;
    int                         bias;

    d = (door_data *)entity->data;
    if (entity->mesh == JO_NULL || entity->view_distance < 0)
        return;
    bias = doors_depth_bias(entity);
    if (bias < 0 || !jo_3d_slave_push_matrix())
        return;
    jo_3d_slave_set_depth_bias(WORLD_DEPTH_BIAS + bias);
    if (d->hinge)
    {
        jo_3d_slave_translate_matrix_fixed(entity->position.x, entity->position.y, entity->position.z);
        jo_3d_slave_rotate_matrix(0, door_angle(entity), 0);
    }
    else
    {
        jo_pos3D_fixed          offset;

        door_slide_offset(entity, &offset);
        jo_3d_slave_translate_matrix_fixed(entity->position.x + offset.x, entity->position.y + offset.y, entity->position.z + offset.z);
        jo_3d_slave_rotate_matrix(0, entity->yaw, 0);
    }
    jo_3d_slave_mesh_draw(entity->mesh);
    jo_3d_slave_set_depth_bias(WORLD_DEPTH_BIAS);
    jo_3d_slave_pop_matrix();
}

static const jo_entity_class    door_class = {"door", sizeof(door_data), door_spawn, door_update, door_action, JO_NULL, door_draw,
                                               door_draw_slave, JO_NULL};

/*
** The class billboard: a sprite standing on the feet of the entity, turned toward the agent around the vertical axis (the
** seats of the toilets). Properties: sprite (STRING, a .BIN of the CD), size_x and size_y (INT, world
** units)
*/
typedef struct
{
    int                         sprite;
    jo_fixed                    scale_x;
    jo_fixed                    scale_y;
    int                         half_height;
}                               billboard_data;

/** @brief The sprites of the billboards: loaded once for all the entities that use them (the name of the file, its id) */
#define BILLBOARD_MAX_SPRITES   (8)
static const char               *billboard_names[BILLBOARD_MAX_SPRITES];
static int                      billboard_sprites[BILLBOARD_MAX_SPRITES];
static int                      billboard_sprite_count = 0;

static void                     billboard_spawn(jo_entity * const entity)
{
    billboard_data              *d;
    const char                  *name;
    int                         k;
    int                         size_x;
    int                         size_y;

    d = (billboard_data *)entity->data;
    d->sprite = -1;
    name = jo_entity_get_string(entity, "sprite", JO_NULL);
    if (name == JO_NULL)
        return;
    for (k = 0; k < billboard_sprite_count; ++k)
        if (jo_strcmp(billboard_names[k], name) == 0)
            d->sprite = billboard_sprites[k];
    if (d->sprite < 0)
    {
        d->sprite = jo_sprite_add_bin_4bits_lut(JO_ROOT_DIR, name, JO_COLOR_Transparent);
        if (d->sprite < 0)
            return;
        if (billboard_sprite_count < BILLBOARD_MAX_SPRITES)
        {
            billboard_names[billboard_sprite_count] = name;
            billboard_sprites[billboard_sprite_count++] = d->sprite;
        }
    }
    size_x = jo_entity_get_int(entity, "size_x", jo_sprite_get_width(d->sprite));
    size_y = jo_entity_get_int(entity, "size_y", jo_sprite_get_height(d->sprite));
    d->scale_x = jo_int2fixed(size_x) / jo_sprite_get_width(d->sprite);
    d->scale_y = jo_int2fixed(size_y) / jo_sprite_get_height(d->sprite);
    d->half_height = size_y / 2;
}

/** @brief The billboards seen this frame: drawn after the entities by billboards_draw() (the sprites are drawn by the
 *  master CPU, the entities may be drawn with the slave CPU) */
#define BILLBOARD_MAX_SEEN      (16)
#define BILLBOARD_SORT_BIAS     (16)
static jo_entity                *billboards_seen[BILLBOARD_MAX_SEEN];
static int                      billboard_seen_count = 0;

static void                     billboard_draw(jo_entity * const entity)
{
    if (((billboard_data *)entity->data)->sprite >= 0 && billboard_seen_count < BILLBOARD_MAX_SEEN)
        billboards_seen[billboard_seen_count++] = entity;
}

void                            billboards_draw(void)
{
    billboard_data              *d;
    jo_entity                   *entity;
    int                         bias;
    int                         k;

    for (k = 0; k < billboard_seen_count; ++k)
    {
        entity = billboards_seen[k];
        d = (billboard_data *)entity->data;
        // In a cell drawn (the sprite is clipped by the VDP1 out of the screen)
        bias = jo_cells_depth_bias_at(entity->position.x, entity->position.y, entity->position.z);
        if (bias < 0)
            continue;
        jo_3d_push_matrix();
        {
            // (sorted nearer than its center: a sprite stands a few units in front of a wall, the parts of the wall cut near
            // the camera have their centers about as near as it: the wall was drawn over it)
            jo_3d_set_depth_bias(WORLD_DEPTH_BIAS + bias - BILLBOARD_SORT_BIAS);
            jo_3d_translate_matrix_fixed(entity->position.x, entity->position.y - jo_int2fixed(d->half_height), entity->position.z);
            jo_3d_rotate_matrix_y(yaw);
            // (the quad of a 3D sprite has the size of the sprite: the matrix scales it)
            jo_3d_set_scale_fixed(d->scale_x, d->scale_y, JO_FIXED_1);
            jo_3d_draw_sprite(d->sprite);
        }
        jo_3d_pop_matrix();
    }
    jo_3d_set_depth_bias(WORLD_DEPTH_BIAS);
    billboard_seen_count = 0;
}

static const jo_entity_class    billboard_class = {"billboard", sizeof(billboard_data), billboard_spawn, JO_NULL, JO_NULL, JO_NULL,
                                                   billboard_draw, JO_NULL, JO_NULL};


/*
** The class glass: the pane of a window (a model in the screen doors of the VDP1), broken by a shot: it falls in pieces
** (shards: they fly away from the shot, fall, stay on the floor, go after GLASS_SHARD_TIME) with the sound of breaking
** glass. Properties: width (INT, units), height (INT); the entity: the middle of the bottom of the pane, its yaw (90: a
** window in a wall along Z, the X axis of the model is the width of the pane)
*/
typedef struct
{
    bool                        broken;
    int                         width;
    int                         height;
}                               glass_data;

/** @brief The shards of all the windows (the oldest ones are taken again), how long they stay (ms) */
#define GLASS_SHARDS            (36)
#define GLASS_SHARD_TIME        (10000)
#define GLASS_SHARD_SPEED       (140)
#define GLASS_GRAVITY           (900)
#define GLASS_SOUND_VOLUME      (127)
#define GLASS_SOUND_RANGE       (1800)
typedef struct
{
    jo_pos3D_fixed              position;
    jo_pos3D_fixed              speed;      // Units per second
    jo_fixed                    floor;
    short                       yaw;
    short                       pitch;
    short                       spin;       // Degrees per second (in the air)
    int                         age;        // ms, -1: free
}                               glass_shard;
static glass_shard              shards[GLASS_SHARDS];
static int                      next_shard = 0;
static jo_3d_mesh               *shard_mesh = JO_NULL;
static jo_sound                 glass_sound;
static bool                     glass_sound_loaded = false;
/** @brief The window that moves and draws the shards of all the windows (and of the glassware, see glass_burst()): the
 *  one broken last (it is near the agent: awake, its cell seen) */
static const jo_entity          *shard_owner = JO_NULL;

static void                     glass_spawn(jo_entity * const entity)
{
    glass_data                  *g;
    int                         k;

    g = (glass_data *)entity->data;
    g->broken = false;
    g->width = jo_entity_get_int(entity, "width", 200);
    g->height = jo_entity_get_int(entity, "height", 100);
    if (shard_mesh == JO_NULL)
    {
        shard_mesh = jo_entity_load_model("SHARD.JOM");
        for (k = 0; k < GLASS_SHARDS; ++k)
            shards[k].age = -1;
    }
    if (!glass_sound_loaded)
        glass_sound_loaded = load_sound("GLASS.WAV", &glass_sound);
}

/** @brief Pieces of glass flying from a point (something made of glass broken: a flask, a screen), spread around it (units),
 *  away from the shot (dir, the horizontal direction), the sound (see glass_break() for the panes of the windows)
 */
void                            glass_burst(const jo_pos3D_fixed * const center, const jo_pos3D_fixed * const dir, const int count, const int spread,
                                            const jo_entity * const owner)
{
    glass_shard                 *s;
    int                         k;
    int                         floor;

    if (shard_mesh == JO_NULL)
    {
        shard_mesh = jo_entity_load_model("SHARD.JOM");
        for (k = 0; k < GLASS_SHARDS; ++k)
            shards[k].age = -1;
    }
    if (!glass_sound_loaded)
        glass_sound_loaded = load_sound("GLASS.WAV", &glass_sound);
    shard_owner = owner;
    for (k = 0; k < count && k < GLASS_SHARDS; ++k)
    {
        s = &shards[next_shard];
        next_shard = (next_shard + 1) % GLASS_SHARDS;
        s->position.x = center->x + jo_int2fixed(jo_random(2 * spread + 1) - 1 - spread);
        s->position.y = center->y - jo_int2fixed(jo_random(spread + 1) - 1);
        s->position.z = center->z + jo_int2fixed(jo_random(2 * spread + 1) - 1 - spread);
        // (with the shot, spread, up a little more than the panes: they jump off the table)
        s->speed.x = jo_fixed_mult(dir->x, jo_int2fixed(GLASS_SHARD_SPEED / 2)) + jo_int2fixed(jo_random(80) - 40);
        s->speed.y = -jo_int2fixed(40 + jo_random(60));
        s->speed.z = jo_fixed_mult(dir->z, jo_int2fixed(GLASS_SHARD_SPEED / 2)) + jo_int2fixed(jo_random(80) - 40);
        s->yaw = (short)jo_random(360);
        s->pitch = (short)jo_random(360);
        s->spin = (short)(jo_random(720) - 360);
        s->floor = jo_cells_floor(s->position.x + s->speed.x / 4, s->position.y, s->position.z + s->speed.z / 4, 0x7fff, &floor) ?
                   jo_int2fixed(floor) - JO_FIXED_1 : center->y + jo_int2fixed(100);
        s->age = 0;
    }
    if (glass_sound_loaded)
    {
        sound_from(&glass_sound, center->x, center->z, GLASS_SOUND_VOLUME, GLASS_SOUND_RANGE);
        jo_audio_stop_channel(CHANNEL_IMPACT_FIRST + 1);
        jo_audio_play_sound_on_channel(&glass_sound, CHANNEL_IMPACT_FIRST + 1);
    }
}

/** @brief The pane breaks: its pieces fly away from the shot (dir), the sound */
static void                     glass_break(jo_entity * const entity, const jo_pos3D_fixed * const dir)
{
    glass_data                  *g;
    glass_shard                 *s;
    jo_fixed                    ax;
    jo_fixed                    az;
    int                         along;
    int                         up;
    int                         k;
    int                         floor;

    g = (glass_data *)entity->data;
    g->broken = true;
    shard_owner = entity;
    ax = jo_cos(entity->yaw);
    az = -jo_sin(entity->yaw);
    for (k = 0; k < GLASS_SHARDS / 2; ++k)
    {
        s = &shards[next_shard];
        next_shard = (next_shard + 1) % GLASS_SHARDS;
        // (a grid of 6 x 3 pieces over the pane, a little at random)
        along = (k % 6) * g->width / 6 - g->width / 2 + g->width / 12 + jo_random(g->width / 8) - g->width / 16;
        up = (k / 6) * g->height / 3 + g->height / 6 + jo_random(g->height / 6) - g->height / 12;
        s->position.x = entity->position.x + ax * along;
        s->position.y = entity->position.y - jo_int2fixed(up);
        s->position.z = entity->position.z + az * along;
        // (with the shot, spread, up a little)
        s->speed.x = jo_fixed_mult(dir->x, jo_int2fixed(GLASS_SHARD_SPEED)) + jo_int2fixed(jo_random(80) - 40);
        s->speed.y = -jo_int2fixed(jo_random(60));
        s->speed.z = jo_fixed_mult(dir->z, jo_int2fixed(GLASS_SHARD_SPEED)) + jo_int2fixed(jo_random(80) - 40);
        s->yaw = (short)jo_random(360);
        s->pitch = (short)jo_random(360);
        s->spin = (short)(jo_random(720) - 360);
        s->floor = jo_cells_floor(s->position.x + s->speed.x / 4, s->position.y, s->position.z + s->speed.z / 4, 0x7fff, &floor) ?
                   jo_int2fixed(floor) - JO_FIXED_1 : entity->position.y + jo_int2fixed(100);
        s->age = 0;
    }
    if (glass_sound_loaded)
    {
        sound_from(&glass_sound, entity->position.x, entity->position.z, GLASS_SOUND_VOLUME, GLASS_SOUND_RANGE);
        jo_audio_stop_channel(CHANNEL_IMPACT_FIRST + 1);
        jo_audio_play_sound_on_channel(&glass_sound, CHANNEL_IMPACT_FIRST + 1);
    }
}

/** @brief The pieces of glass of the windows and of the glassware: moved and drawn by the thing broken last (see
 *  glass_update(), props.c) */
void                            glass_shards_update(const jo_entity * const entity, const int elapsed)
{
    glass_shard                 *s;
    int                         k;

    if (entity != shard_owner)
        return;
    for (k = 0, s = shards; k < GLASS_SHARDS; ++k, ++s)
    {
        if (s->age < 0)
            continue;
        s->age += elapsed;
        if (s->age >= GLASS_SHARD_TIME)
        {
            s->age = -1;
            continue;
        }
        if (s->position.y >= s->floor)
            continue;
        s->speed.y += jo_int2fixed(GLASS_GRAVITY) * elapsed / 1000;
        s->position.x += s->speed.x / 1000 * elapsed;
        s->position.y += s->speed.y / 1000 * elapsed;
        s->position.z += s->speed.z / 1000 * elapsed;
        s->yaw = (short)((s->yaw + s->spin * elapsed / 1000 + 360) % 360);
        s->pitch = (short)((s->pitch + s->spin * elapsed / 2000 + 360) % 360);
        if (s->position.y >= s->floor)
        {
            // (on the floor: flat)
            s->position.y = s->floor;
            s->pitch = 90;
        }
    }
}

static void                     glass_update(jo_entity * const entity, const int elapsed)
{
    glass_shards_update(entity, elapsed);
}

static void                     glass_draw(jo_entity * const entity)
{
    int                         bias;
    int                         side;

    if (!((glass_data *)entity->data)->broken && entity->mesh != JO_NULL && jo_entity_view_distance(entity) >= 0)
    {
        // (the pane on the wall between 2 rooms: the cell on the side of the agent first, the room behind the glass is sorted
        // behind it. With the cell of the other side, the frame of the window behind the glass was drawn over it: triangles
        // of the frame without the screen doors at the corners of the window)
        side = jo_fixed_mult(player_x - entity->position.x, jo_sin(entity->yaw)) + jo_fixed_mult(player_z - entity->position.z, jo_cos(entity->yaw)) >= 0 ? 8 : -8;
        bias = jo_cells_depth_bias_at(entity->position.x + jo_sin(entity->yaw) * side, entity->position.y - jo_int2fixed(20),
                                      entity->position.z + jo_cos(entity->yaw) * side);
        if (bias < 0)
            bias = jo_cells_depth_bias_at(entity->position.x - jo_sin(entity->yaw) * side, entity->position.y - jo_int2fixed(20),
                                          entity->position.z - jo_cos(entity->yaw) * side);
        if (bias >= 0)
        {
            jo_3d_push_matrix();
            jo_3d_set_depth_bias(WORLD_DEPTH_BIAS + bias);
            jo_3d_translate_matrix_fixed(entity->position.x, entity->position.y, entity->position.z);
            jo_3d_rotate_matrix_y(entity->yaw);
            jo_3d_mesh_draw(entity->mesh);
            jo_3d_pop_matrix();
        }
    }
    glass_shards_draw(entity);
}

/** @brief Draw the pieces of glass (see glass_shards_update()) */
void                            glass_shards_draw(const jo_entity * const entity)
{
    glass_shard                 *s;
    int                         bias;
    int                         k;

#ifdef TEST_SHARDS
    {
        int n = 0, m = 0;
        for (k = 0, s = shards; k < GLASS_SHARDS; ++k, ++s)
            if (s->age >= 0) { ++n; if (jo_cells_depth_bias_at(s->position.x, s->position.y - JO_FIXED_1, s->position.z) >= 0) ++m; }
        jo_printf(0, 2, "shards %d drawn %d owner %d mesh %d y %d fl %d", n, m, entity == shard_owner, shard_mesh != JO_NULL,
                  jo_fixed2int(shards[0].position.y), jo_fixed2int(shards[0].floor));
    }
#endif
    if (shard_mesh == JO_NULL || entity != shard_owner)
        return;
    for (k = 0, s = shards; k < GLASS_SHARDS; ++k, ++s)
    {
        if (s->age < 0 || (bias = jo_cells_depth_bias_at(s->position.x, s->position.y - JO_FIXED_1, s->position.z)) < 0)
            continue;
        jo_3d_push_matrix();
        jo_3d_set_depth_bias(WORLD_DEPTH_BIAS + bias);
        jo_3d_translate_matrix_fixed(s->position.x, s->position.y, s->position.z);
        jo_3d_rotate_matrix_y(s->yaw);
        jo_3d_rotate_matrix_x(s->pitch);
        jo_3d_mesh_draw(shard_mesh);
        jo_3d_pop_matrix();
    }
    jo_3d_set_depth_bias(WORLD_DEPTH_BIAS);
}

void                            glass_shards_draw_slave(const jo_entity * const entity)
{
    glass_shard                 *s;
    int                         bias;
    int                         k;

    if (shard_mesh == JO_NULL || entity != shard_owner)
        return;
    for (k = 0, s = shards; k < GLASS_SHARDS; ++k, ++s)
    {
        if (s->age < 0 || (bias = jo_cells_depth_bias_at(s->position.x, s->position.y - JO_FIXED_1, s->position.z)) < 0 ||
            !jo_3d_slave_push_matrix())
            continue;
        jo_3d_slave_set_depth_bias(WORLD_DEPTH_BIAS + bias);
        jo_3d_slave_translate_matrix_fixed(s->position.x, s->position.y, s->position.z);
        jo_3d_slave_rotate_matrix(s->pitch, s->yaw, 0);
        jo_3d_slave_mesh_draw(shard_mesh);
        jo_3d_slave_pop_matrix();
    }
    jo_3d_slave_set_depth_bias(WORLD_DEPTH_BIAS);
}

/** @brief Draw a pane on the slave CPU (see glass_draw(): the distance of the view prepared by the master CPU) */
static void                     glass_draw_slave(jo_entity * const entity)
{
    int                         bias;
    int                         side;

    if (!((glass_data *)entity->data)->broken && entity->mesh != JO_NULL && entity->view_distance >= 0)
    {
        side = jo_fixed_mult(player_x - entity->position.x, jo_sin(entity->yaw)) + jo_fixed_mult(player_z - entity->position.z, jo_cos(entity->yaw)) >= 0 ? 8 : -8;
        bias = jo_cells_depth_bias_at(entity->position.x + jo_sin(entity->yaw) * side, entity->position.y - jo_int2fixed(20),
                                      entity->position.z + jo_cos(entity->yaw) * side);
        if (bias < 0)
            bias = jo_cells_depth_bias_at(entity->position.x - jo_sin(entity->yaw) * side, entity->position.y - jo_int2fixed(20),
                                          entity->position.z - jo_cos(entity->yaw) * side);
        if (bias >= 0 && jo_3d_slave_push_matrix())
        {
            jo_3d_slave_set_depth_bias(WORLD_DEPTH_BIAS + bias);
            jo_3d_slave_translate_matrix_fixed(entity->position.x, entity->position.y, entity->position.z);
            jo_3d_slave_rotate_matrix(0, entity->yaw, 0);
            jo_3d_slave_mesh_draw(entity->mesh);
            jo_3d_slave_pop_matrix();
        }
    }
    glass_shards_draw_slave(entity);
}

static const jo_entity_class    glass_class = {"glass", sizeof(glass_data), glass_spawn, glass_update, JO_NULL, JO_NULL, glass_draw,
                                                glass_draw_slave, JO_NULL};

jo_fixed                        doors_glass_ray_hit(const jo_pos3D_fixed * const origin, const jo_pos3D_fixed * const dir, const jo_fixed max_t,
                                                    jo_pos3D_fixed * const normal, jo_entity ** const glass)
{
    jo_entity                   *e;
    const glass_data            *g;
    jo_fixed                    nx;
    jo_fixed                    nz;
    jo_fixed                    denominator;
    jo_fixed                    t;
    jo_fixed                    best;
    int                         along;
    int                         y;
    int                         k;

    best = -1;
    doors_index();
    for (k = 0; k < glass_entity_count; ++k)
    {
        e = glass_entities[k];
        if (((const glass_data *)e->data)->broken)
            continue;
        g = (const glass_data *)e->data;
        // (the plane of the pane: its normal is the +Z of the model turned by the yaw)
        nx = jo_sin(e->yaw);
        nz = jo_cos(e->yaw);
        denominator = jo_fixed_mult(dir->x, nx) + jo_fixed_mult(dir->z, nz);
        if (JO_ABS(denominator) < 64)
            continue;
        t = jo_fixed_div(jo_fixed_mult(e->position.x - origin->x, nx) + jo_fixed_mult(e->position.z - origin->z, nz), denominator);
        if (t <= 0 || t > max_t || (best >= 0 && t >= best))
            continue;
        along = jo_fixed2int(jo_fixed_mult(origin->x + jo_fixed_mult(dir->x, t) - e->position.x, jo_cos(e->yaw)) -
                             jo_fixed_mult(origin->z + jo_fixed_mult(dir->z, t) - e->position.z, jo_sin(e->yaw)));
        y = jo_fixed2int(e->position.y - (origin->y + jo_fixed_mult(dir->y, t)));
        if (JO_ABS(along) > g->width / 2 || y < 0 || y > g->height)
            continue;
        best = t;
        if (glass != JO_NULL)
            *glass = e;
        if (normal != JO_NULL)
        {
            normal->x = denominator < 0 ? nx : -nx;
            normal->y = 0;
            normal->z = denominator < 0 ? nz : -nz;
        }
    }
    return (best);
}

void                            doors_glass_shot(jo_entity * const glass, const jo_pos3D_fixed * const dir)
{
    if (glass != JO_NULL && glass->cls == &glass_class && !((glass_data *)glass->data)->broken)
        glass_break(glass, dir);
}

/*
** The class lamp: the glow of a lamp on a ceiling (the engine glows, see jo/glow.h: a halo of light added to the image by
** the VDP2, hidden by the walls). Properties: r, g, b (INT, 0 to 255), size (INT, units), intensity (INT, 0 to 256);
** alarm (INT, 1: a red alarm light that pulses, a point light of the engine
** added to the gouraud tables of the model of its cell, see jo/light.h)
*/

/** @brief The light of an alarm: radius (units, half of its strength there), strength, the pulse (ms: 2 s), never
 *  completely off (its light from ALARM_LIGHT_MIN to 256) */
#define ALARM_RADIUS            (130)
#define ALARM_STRENGTH          (420)
#define ALARM_PERIOD            (2000)
#define ALARM_LIGHT_MIN         (44)
#define ALARM_MAX               (4)

/** @brief The alarms: their point lights, their glows (their light: the intensity of the glow on) */
static int                      alarm_lights[ALARM_MAX];
static int                      alarm_glows[ALARM_MAX];
static int                      alarm_glow_light[ALARM_MAX];
static int                      alarm_count = 0;
/** @brief The time of the pulse (ms) */
static int                      alarm_time = 0;
/** @brief The meshes already lit by a point light (2 alarms in a room: its mesh added once) */
static const jo_3d_mesh         *alarm_meshes[ALARM_MAX];
static int                      alarm_mesh_count = 0;

/** @brief The model of the cell of an alarm lit by its point light (once per model) */
static void                     alarm_add_mesh(const jo_entity * const entity)
{
    const jo_cell               *cell;
    int                         index;
    int                         k;

    // (the cell under the lamp: its floor under the point)
    index = jo_cells_find(entity->position.x, entity->position.y, entity->position.z, 1000);
    if (index < 0)
        return;
    cell = jo_cells_get(index);
    if (cell->mesh == JO_NULL)
        return;
    for (k = 0; k < alarm_mesh_count; ++k)
        if (alarm_meshes[k] == cell->mesh)
            return;
    if (alarm_mesh_count < ALARM_MAX && jo_3d_point_lights_add_mesh(cell->mesh))
        alarm_meshes[alarm_mesh_count++] = cell->mesh;
}

static void                     lamp_spawn(jo_entity * const entity)
{
    jo_pos3D_fixed              p;
    jo_pos3D_fixed              down;
    int                         glow;
    int                         intensity;
    int                         light;

    intensity = jo_entity_get_int(entity, "intensity", 200);
    glow = jo_3d_glow_add(&entity->position, (unsigned char)jo_entity_get_int(entity, "r", 255),
                          (unsigned char)jo_entity_get_int(entity, "g", 255), (unsigned char)jo_entity_get_int(entity, "b", 255),
                          jo_entity_get_int(entity, "size", 40), intensity);
    if (jo_entity_get_int(entity, "alarm", 0) == 0 || alarm_count >= ALARM_MAX)
        return;
    alarm_add_mesh(entity);
    // (a little under the ceiling: the light of the walls under it; toward the floor)
    p = entity->position;
    p.y += jo_int2fixed(10);
    light = jo_3d_point_light_add(&p, 255, 36, 24, ALARM_RADIUS, ALARM_STRENGTH);
    if (light < 0)
        return;
    down.x = 0;
    down.y = JO_FIXED_1;
    down.z = 0;
    jo_3d_point_light_set_spot(light, &down, 170);
    alarm_lights[alarm_count] = light;
    alarm_glows[alarm_count] = glow;
    alarm_glow_light[alarm_count] = intensity;
    ++alarm_count;
}

/*
** The sheen of the duct: the steel of the duct shines where the agent looks. A soft white point light on the camera, a
** spot along the view, without shadows (jo/light.h), added to the gouraud tables of the models of the cells of the duct:
** the walls in front of the agent and near him get brighter, the bright spot moves with him (a reflection of his light on
** the metal). It moves with the camera: its light is computed again when the camera moves or turns, only on the models of
** the duct (the meshes out of its reach are skipped), the VDP1 draws nothing more
*/

/** @brief The sheen: radius (units, half of its strength there), strength (256: the color doubled in front of it), the
  angle of its cone (degrees) */
#define SHEEN_RADIUS            (20)
#define SHEEN_STRENGTH          (700)
#define SHEEN_ANGLE             (90)

/** @brief The point light of the sheen (-1: none), on (the camera in the duct), where it was computed last */
static int                      sheen_light = -1;
static bool                     sheen_on = false;
static jo_pos3D_fixed           sheen_eye;
static int                      sheen_yaw;
static int                      sheen_pitch;

/** @brief Check if a cell is a part of the duct: narrower than DUCT_WIDTH, over the floor of the rooms (see player.c) */
static bool                     sheen_is_duct(const jo_cell * const cell)
{
    return (cell->floor0 < 0 && (cell->x1 - cell->x0 <= DUCT_WIDTH || cell->z1 - cell->z0 <= DUCT_WIDTH));
}

/** @brief The models of the duct lit by the sheen, its light (off until the camera is in the duct) */
static void                     sheen_init(void)
{
    const jo_cell               *cell;
    jo_pos3D_fixed              p;
    int                         k;

    sheen_light = -1;
    sheen_on = false;
    for (k = 0; k < jo_cells_count(); ++k)
    {
        cell = jo_cells_get(k);
        if (cell->mesh != JO_NULL && sheen_is_duct(cell))
            jo_3d_point_lights_add_mesh(cell->mesh);
    }
    JO_ZERO(p.x);
    JO_ZERO(p.y);
    JO_ZERO(p.z);
    sheen_light = jo_3d_point_light_add(&p, 220, 235, 255, SHEEN_RADIUS, SHEEN_STRENGTH);
    if (sheen_light < 0)
        return;
    jo_3d_point_light_set_shadows(sheen_light, false);
    // (a sheen on the walls: the weapon in front of the camera doesn't get its light, see weapon.c)
    jo_3d_point_light_set_sampled(sheen_light, false);
    jo_3d_point_light_enable(sheen_light, false);
}

/** @brief The sheen follows the camera in the duct (computed again only when the camera moved or turned) */
static void                     sheen_update(void)
{
    jo_pos3D_fixed              eye;
    jo_pos3D_fixed              dir;
    jo_fixed                    c;
    int                         index;
    bool                        in;

    if (sheen_light < 0)
        return;
    player_eye(&eye);
    index = jo_cells_find(eye.x, eye.y, eye.z, PLAYER_STEP);
    in = index >= 0 && sheen_is_duct(jo_cells_get(index));
    if (in != sheen_on)
    {
        sheen_on = in;
        jo_3d_point_light_enable(sheen_light, in);
        // (the position below is set again: the light was off)
        sheen_yaw = -1000;
    }
    if (!in || (eye.x == sheen_eye.x && eye.y == sheen_eye.y && eye.z == sheen_eye.z && yaw == sheen_yaw && pitch == sheen_pitch))
        return;
    sheen_eye = eye;
    sheen_yaw = yaw;
    sheen_pitch = pitch;
    // (the view: yaw 0 toward +Z, 90 toward +X; a positive pitch looks down, the Saturn Y goes down)
    c = jo_cos(pitch);
    dir.x = jo_fixed_mult(jo_sin(yaw), c);
    dir.y = jo_sin(pitch);
    dir.z = jo_fixed_mult(jo_cos(yaw), c);
    jo_3d_point_light_set_position(sheen_light, &eye);
    jo_3d_point_light_set_spot(sheen_light, &dir, SHEEN_ANGLE);
}

void                            doors_lights_update(const int elapsed)
{
    int                         light;
    int                         k;
    static int                  frame = 0;

    sheen_update();
    if (alarm_count == 0)
    {
        jo_3d_point_lights_update();
        return;
    }
    alarm_time = (alarm_time + elapsed) % ALARM_PERIOD;
    // (every other frame of the game: each change of the light costs CPU time on the polygons of its room)
    if ((++frame & 1) == 0)
    {
        light = ALARM_LIGHT_MIN + jo_fixed2int(((jo_sin(alarm_time * 360 / ALARM_PERIOD) + JO_FIXED_1) / 2) * (256 - ALARM_LIGHT_MIN));
        for (k = 0; k < alarm_count; ++k)
        {
            jo_3d_point_light_set_intensity(alarm_lights[k], light);
            jo_3d_glow_set_intensity(alarm_glows[k], alarm_glow_light[k] * (64 + light * 3 / 4) / 256);
        }
    }
    jo_3d_point_lights_update();
}

static const jo_entity_class    lamp_class = {"lamp", 0, lamp_spawn, JO_NULL, JO_NULL, JO_NULL, JO_NULL, JO_NULL, JO_NULL};

/** @brief The walls hide the glows of the lamps behind them */
static bool                     lamp_ray_hit(const jo_pos3D_fixed * const origin, const jo_pos3D_fixed * const dir, const jo_fixed max_t)
{
    jo_fixed                    t;
    jo_pos3D_fixed              normal;
    jo_entity                   *door;

    return (jo_cells_ray_hit(origin, dir, max_t, &t, &normal) || doors_ray_hit(origin, dir, max_t, &normal, &door, true) >= 0);
}

void                            doors_glows_init(void)
{
    jo_3d_glows_set_occlusion(lamp_ray_hit);
    // (the light of the alarms: the walls and the closed doors stop it, on the rooms and on the guards and the doors)
    jo_3d_point_lights_set_occlusion(lamp_ray_hit);
    // (a lamp seen through a door: its glow stays in the door)
    jo_3d_glows_set_clip(jo_cells_view_rect_at);
    // (a soft halo: short faint rays, the star of the default was too strong)
    jo_3d_glows_set_rays(55, 30);
    jo_3d_glows_enable(true);
    sheen_init();
}

void                            doors_register(void)
{
    jo_3d_glows_clear();
    jo_3d_point_lights_clear();
    alarm_mesh_count = 0;
    alarm_count = 0;
    jo_entity_register_class(&lamp_class);
    jo_entity_register_class(&door_class);
    jo_entity_register_class(&billboard_class);
    jo_entity_register_class(&glass_class);
}

bool                            doors_use(void)
{
    jo_entity                   *e;
    jo_pos3D_fixed              dir;
    int                         dx;
    int                         dz;
    int                         dist;
    int                         k;
    jo_fixed                    facing;
    jo_entity                   *best;
    int                         best_dist;

    best = JO_NULL;
    best_dist = 0;
    player_aim_direction(&dir);
    doors_index();
    for (k = 0; k < door_entity_count; ++k)
    {
        e = door_entities[k];
        if (e->cls != &door_class || (e->flags & JO_ENTITY_FLAG_DEAD))
            continue;
        // (whole units: the squares of the fixed point distances overflow. A hinged door: from the middle of its panel)
        dx = jo_fixed2int(e->position.x - player_x);
        dz = jo_fixed2int(e->position.z - player_z);
        if (((door_data *)e->data)->hinge)
        {
            // (from the middle of the doorway: where the panel is when the door is closed, not the swung panel)
            dx += jo_fixed2int(jo_cos(e->yaw) * (e->radius / 2));
            dz += jo_fixed2int(-jo_sin(e->yaw) * (e->radius / 2));
        }
        dist = (int)jo_sqrt((unsigned int)(dx * dx + dz * dz));
        if (dist > DOOR_USE_DISTANCE || dist == 0)
            continue;
        // In front of the agent (the door toward where he looks): the nearest one of them (the doors of the stalls are side
        // by side: the first one found was the neighbor)
        facing = dx * dir.x + dz * dir.z;
        if (facing < jo_int2fixed(dist) / 2 || (best != JO_NULL && dist >= best_dist))
            continue;
        best = e;
        best_dist = dist;
    }
    if (best == JO_NULL)
        return (false);
    jo_entity_send(best, JO_LEVEL_ACTION_USE, 0, JO_NULL);
    return (true);
}

/** @brief Origin of the panel of a door now: its hinge, or its middle raised by the slide */
static void                     door_origin(const jo_entity * const e, jo_pos3D_fixed * const o)
{
    const door_data             *d;

    d = (const door_data *)e->data;
    *o = e->position;
    if (!d->hinge)
    {
        jo_pos3D_fixed          offset;

        door_slide_offset(e, &offset);
        o->x += offset.x;
        o->y += offset.y;
        o->z += offset.z;
    }
}

void                            doors_to_local(const jo_entity * const door, const jo_pos3D_fixed * const world, jo_pos3D_fixed * const local)
{
    jo_pos3D_fixed              o;
    jo_fixed                    ax;
    jo_fixed                    az;
    jo_fixed                    nx;
    jo_fixed                    nz;

    door_origin(door, &o);
    door_axes(door, &ax, &az);
    door_normal(door, &nx, &nz);
    local->x = jo_fixed_mult(world->x - o.x, ax) + jo_fixed_mult(world->z - o.z, az);
    local->y = world->y - o.y;
    local->z = jo_fixed_mult(world->x - o.x, nx) + jo_fixed_mult(world->z - o.z, nz);
}

int                             doors_to_world(const jo_entity * const door, const jo_pos3D_fixed * const local, jo_pos3D_fixed * const world)
{
    jo_pos3D_fixed              o;
    jo_fixed                    ax;
    jo_fixed                    az;
    jo_fixed                    nx;
    jo_fixed                    nz;

    door_origin(door, &o);
    door_axes(door, &ax, &az);
    door_normal(door, &nx, &nz);
    world->x = o.x + jo_fixed_mult(local->x, ax) + jo_fixed_mult(local->z, nx);
    world->y = o.y + local->y;
    world->z = o.z + jo_fixed_mult(local->x, az) + jo_fixed_mult(local->z, nz);
    return (door_angle(door));
}

jo_fixed                        doors_ray_hit(const jo_pos3D_fixed * const origin, const jo_pos3D_fixed * const dir, const jo_fixed max_t,
                                              jo_pos3D_fixed * const normal, jo_entity ** const door, const bool turning_closed)
{
    jo_entity                   *e;
    door_data                   *d;
    jo_fixed                    best;
    jo_fixed                    nx;
    jo_fixed                    nz;
    jo_fixed                    denominator;
    jo_fixed                    t;
    jo_fixed                    hx;
    jo_fixed                    hy;
    jo_fixed                    hz;
    jo_fixed                    ax;
    jo_fixed                    az;
    jo_pos3D_fixed              offset;
    int                         along;
    int                         top;
    int                         k;

    best = -1;
    doors_index();
    for (k = 0; k < door_entity_count; ++k)
    {
        e = door_entities[k];
        if (e->cls != &door_class)
            continue;
        d = (door_data *)e->data;
        // The plane of the panel: its normal (the +Z of the model turned by the yaw, and by the turn of a hinged door; a hinged
        // door that turns, not open yet: closed for the sight of the guards (turning_closed), they don't see nor shoot
        // through it; a bullet hits the panel where it is: its impact was in the doorway, in the air)
        if (d->hinge && e->state != DOOR_OPEN && turning_closed)
        {
            nx = jo_sin(e->yaw);
            nz = jo_cos(e->yaw);
        }
        else
            door_normal(e, &nx, &nz);
        denominator = jo_fixed_mult(dir->x, nx) + jo_fixed_mult(dir->z, nz);
        if (JO_ABS(denominator) < 256)
            continue;
        t = jo_fixed_div(jo_fixed_mult(e->position.x - origin->x, nx) + jo_fixed_mult(e->position.z - origin->z, nz), denominator);
        if (t <= 0 || t > max_t || (best >= 0 && t >= best))
            continue;
        hx = origin->x + jo_fixed_mult(dir->x, t);
        hy = origin->y + jo_fixed_mult(dir->y, t);
        hz = origin->z + jo_fixed_mult(dir->z, t);
        if (d->hinge && e->state != DOOR_OPEN && turning_closed)
        {
            ax = jo_cos(e->yaw);
            az = -jo_sin(e->yaw);
        }
        else
            door_axes(e, &ax, &az);
        // The panel moved by the slide x progress (up, or along its width): from its top to its bottom (Saturn Y: up is
        // negative)
        door_slide_offset(e, &offset);
        along = jo_fixed2int(jo_fixed_mult(hx - e->position.x - offset.x, ax) + jo_fixed_mult(hz - e->position.z - offset.z, az));
        top = jo_fixed2int(e->position.y + offset.y);
        if (d->hinge ? (along < 0 || along > e->radius) : JO_ABS(along) > e->radius)
            continue;
        if (jo_fixed2int(hy) > top || jo_fixed2int(hy) < top - e->height)
            continue;
        best = t;
        if (door != JO_NULL)
            *door = e;
        if (normal != JO_NULL)
        {
            // (toward the shooter)
            normal->x = denominator < 0 ? nx : -nx;
            normal->y = 0;
            normal->z = denominator < 0 ? nz : -nz;
        }
    }
    return (best);
}

bool                            doors_block(const jo_fixed x, const jo_fixed z, const int radius)
{
    jo_entity                   *e;
    door_data                   *d;
    int                         k;

    doors_index();

    for (k = 0; k < door_entity_count; ++k)
    {
        e = door_entities[k];
        if (!door_near(e, x, z, radius) || e->cls != &door_class)
            continue;
        d = (door_data *)e->data;
        // (open: nobody goes through a door before it is open, the doorway too; a hinged door: its panel where it is too)
        if (d->progress >= 65536)
            continue;
        if (door_meets(e, x, z, radius) || (d->hinge && door_meets_at(e, e->yaw, x, z, radius)))
            return (true);
    }
    return (false);
}


bool                            doors_panel_block(const jo_fixed x, const jo_fixed z, const int radius, const jo_fixed from_x,
                                                  const jo_fixed from_z)
{
    jo_entity                   *e;
    const door_data             *d;
    int                         k;

    doors_index();

    for (k = 0; k < door_entity_count; ++k)
    {
        e = door_entities[k];
        if (!door_near(e, x, z, radius) || e->cls != &door_class)
            continue;
        d = (const door_data *)e->data;
        // (the panel of an open hinged door stands in the room: a thin wall; one going up is out of the way; the circle
        // already on it may leave it: the panel turned onto someone)
        if (!d->hinge || d->progress <= 65536 * 7 / 10)
            continue;
        if (door_meets(e, x, z, radius) && !door_meets(e, from_x, from_z, radius))
            return (true);
    }
    return (false);
}

bool                            doors_swing_away(const jo_fixed x, const jo_fixed z, const int radius, const jo_fixed from_x,
                                                 const jo_fixed from_z)
{
    jo_entity                   *e;
    door_data                   *d;
    int                         k;

    doors_index();

    for (k = 0; k < door_entity_count; ++k)
    {
        e = door_entities[k];
        if (!door_near(e, x, z, radius) || e->cls != &door_class)
            continue;
        d = (door_data *)e->data;
        if (!d->hinge || e->state != DOOR_OPEN || d->progress <= 65536 * 7 / 10)
            continue;
        if (!door_meets(e, x, z, radius) || door_meets(e, from_x, from_z, radius))
            continue;
        // (the panel turned toward him: it closes, then opens again away from him)
        d->reopen = true;
        d->reopen_x = from_x;
        d->reopen_z = from_z;
        door_start_closing(e, d);
        return (true);
    }
    return (false);
}

bool                            doors_open_for(const jo_fixed x, const jo_fixed z, const int radius, const jo_fixed from_x, const jo_fixed from_z)
{
    jo_entity                   *e;
    int                         k;

    doors_index();

    for (k = 0; k < door_entity_count; ++k)
    {
        e = door_entities[k];
        if (e->cls != &door_class || e->state != DOOR_CLOSED || !door_meets(e, x, z, radius))
            continue;
        // (a trigger: it closes by itself after a while)
        opener_x = from_x;
        opener_z = from_z;
        opener_set = true;
        jo_entity_send(e, JO_LEVEL_ACTION_TRIGGER, 0, JO_NULL);
        return (true);
    }
    return (false);
}

/** @brief The way out of a stall for the guards: this far in front of its door (world units) */
#define STALL_WAY_OUT           (40)

int                             doors_doorways(jo_pos3D_fixed * const out, const int max, const bool stalls)
{
    jo_entity                   *e;
    int                         k;
    int                         n;

    doors_index();

    for (k = 0, n = 0; k < door_entity_count && n < max; ++k)
    {
        e = door_entities[k];
        // (the doors between the rooms: door_ROOM_ROOM in the level; stalls: the doors of the stalls, stallN)
        if (e->cls != &door_class || e->name == JO_NULL)
            continue;
        if (stalls ? (e->name[0] != 's' || e->name[1] != 't') : (e->name[0] != 'd' || e->name[4] != '_'))
            continue;
        out[n] = e->position;
        if (((door_data *)e->data)->hinge)
        {
            // (the middle of the doorway: the panel closed goes from the hinge along the yaw)
            out[n].x += jo_cos(e->yaw) * (e->radius / 2);
            out[n].z += -jo_sin(e->yaw) * (e->radius / 2);
            // (a stall: in front of it, in the room, out of the swing of its door and past the ends of the partitions)
            if (stalls)
            {
                out[n].x += jo_sin(e->yaw) * STALL_WAY_OUT;
                out[n].z += jo_cos(e->yaw) * STALL_WAY_OUT;
            }
        }
        ++n;
    }
    return (n);
}

int                             doors_doorway_sides(jo_pos3D_fixed * const out, const int max, const int distance)
{
    jo_pos3D_fixed              middle;
    jo_fixed                    nx;
    jo_fixed                    nz;
    jo_entity                   *e;
    int                         k;
    int                         n;

    doors_index();
    for (k = 0, n = 0; k < door_entity_count && n + 2 <= max; ++k)
    {
        e = door_entities[k];
        // (the doors between the rooms: door_ROOM_ROOM in the level, not the stalls)
        if (e->cls != &door_class || e->name == JO_NULL || e->name[0] != 'd' || e->name[4] != '_')
            continue;
        middle = e->position;
        if (((door_data *)e->data)->hinge)
        {
            middle.x += jo_cos(e->yaw) * (e->radius / 2);
            middle.z += -jo_sin(e->yaw) * (e->radius / 2);
        }
        // (across the wall: the closed panel is along the yaw, the wall too)
        nx = jo_sin(e->yaw) * distance;
        nz = jo_cos(e->yaw) * distance;
        out[n] = middle;
        out[n].x += nx;
        out[n].z += nz;
        out[n + 1] = middle;
        out[n + 1].x -= nx;
        out[n + 1].z -= nz;
        n += 2;
    }
    return (n);
}

/*
** END OF FILE
*/
