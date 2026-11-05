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
** The guards of the laboratory (class "guard" of the level): an actor of the engine (its animated model cd/GUARD.JOM)
** with a simple behavior:
** - he waits (his place, his animation idle) or patrols between his place and the point patrol_x, patrol_z
** - he sees the agent (in front of him, nothing between them: the walls, the closed doors) or hears his shots: he is
**   surprised, raises his rifle, then shoots short bursts (sometimes on one knee); far away, he misses more
** - hit: he cries, turns red for a moment, blood comes out where he is hit; in the head he dies at once
** - dead: he falls (forward after a shot in the head), the body stays on the floor
*/

#include "demo.h"
#include "guard_faces.h"

/** @brief The size of the guards: 1.25 x their design (the heights below are the ones of the design) */
#define GUARD_SIZE(V)               ((V) * 5 / 4)
/** @brief The knees of a guard on one knee: this lower (world units) */
#define GUARD_KNEEL_DROP            (GUARD_SIZE(22))
/** @brief Heights over the feet (world units: the model is 1.08 times its design):
 *  his eyes, his rifle, the bottom of his head (a hit over it is in the head) */
#define GUARD_EYE_HEIGHT            (GUARD_SIZE(88))
#define GUARD_RIFLE_HEIGHT          (GUARD_SIZE(60))
#define GUARD_HEAD_BOTTOM           (GUARD_SIZE(79))
/** @brief His rifle points this far in front of him (world units) */
#define GUARD_RIFLE_LENGTH          (GUARD_SIZE(58))
/** @brief He sees the agent nearer (world units), in front of him (cosine of half of his field of view: 120 degrees) */
#define GUARD_SIGHT_DISTANCE        (1400)
#define GUARD_SIGHT_COS             (JO_FIXED_1 / 2)
/** @brief He hears the shots of the agent nearer (world units) */
#define GUARD_HEARING_DISTANCE      (500)
/** @brief Blocked by the furniture, he walks around it in a free direction for this long (milliseconds) before he heads
 *  again to his point (turned back at once toward it, he was pushed against the bench again, stuck) */
#define GUARD_DETOUR_TIME           (450)
/** @brief Time between 2 checks of his line of sight (milliseconds): a ray through the cells costs */
#define GUARD_SIGHT_PERIOD          (200)
#define GUARD_IDLE_SIGHT_PERIOD     (350)
/** @brief Speeds (world units per second), turn (degrees per second) */
#define GUARD_WALK_SPEED            (88)
#define GUARD_RUN_SPEED             (270)
#define GUARD_TURN_SPEED            (260)
/** @brief Surprised, then he raises his rifle (milliseconds) */
#define GUARD_SURPRISE_TIME         (450)
#define GUARD_REACTION_TIME         (500)
/** @brief Shots of a burst, time between 2 shots, pause between 2 bursts (milliseconds) */
#define GUARD_BURST                 (3)
#define GUARD_SHOT_TIME             (140)
#define GUARD_BURST_PAUSE           (1000)
/** @brief Damage of a shot that hits the agent */
#define GUARD_DAMAGE                (5)
/** @brief He doesn't see the agent for this time: he looks for him (milliseconds), for this time at most */
#define GUARD_LOST_TIME             (1800)
#define GUARD_SEARCH_TIME           (15000)
/** @brief Searching, he goes where he saw the agent for the last time, then from doorway to doorway (and in front of the
 *  stalls) this near (units): he doesn't know where the agent is (behind a closed door, in a stall) */
#define GUARD_SEARCH_RADIUS         (520)
/** @brief In the room of the agent, he keeps facing him this long after he saw him (milliseconds); later he searches */
#define GUARD_KEEP_TIME             (3000)
/** @brief A shot of the agent that passes this near a guard (units) alerts him, even from behind him */
#define GUARD_BULLET_NEAR           (90)
/** @brief In the room of the agent, a guard that looks for him stops this near (the distance of his shots: he fights from
 *  there, he doesn't run at him); he runs only from another room or from farther than GUARD_RUN_DISTANCE (units) */
#define GUARD_KEEP_DISTANCE         (360)
/** @brief Fighting from farther than this (units), he runs at the agent between 2 bursts, this long at most (milliseconds):
 *  he closes in, he shoots from nearer (standing: a guard on one knee stays) */
#define GUARD_CLOSE_DISTANCE        (520)
#define GUARD_CLOSE_TIME            (1100)
/** @brief Between 2 bursts, standing, he steps aside (milliseconds of the step, its speed: units per second) */
#define GUARD_STEP_TIME             (500)
#define GUARD_STEP_SPEED            (110)
/** @brief After a run, he aims badly for this long (milliseconds): this much less chance to hit */
#define GUARD_WINDED_TIME           (2500)
#define GUARD_WINDED_PENALTY        (30)
/** @brief The scientists (the class scientist: the same body, a white coat, no rifle; they walk, raise their hands when the
 *  agent aims at them from nearer than this (units), run away from the shots). He stands with his hands up this long
 *  (milliseconds) after the agent stopped aiming at him; scared (a shot heard nearer than SCIENTIST_HEAR_DISTANCE, a cry
 *  of a guard, hit), he runs away this long, to places (the doorways, the way nodes) farther from the agent; at the
 *  ends of his walk he stops this long */
#define SCIENTIST_THREAT_DISTANCE   (700)
#define SCIENTIST_CALM_TIME         (2500)
#define SCIENTIST_HEAR_DISTANCE     (1100)
#define SCIENTIST_FEAR_TIME         (9000)
#define SCIENTIST_PAUSE_TIME        (1800)
#define SCIENTIST_FLEE_RADIUS       (1000)
/** @brief The first units of the way to a place must be free (the bench next to him) */
#define SCIENTIST_FLEE_FIRST        (240)
#define SCIENTIST_CHOICE_TIME       (400)
/** @brief A scientist running away, in a room the agent doesn't see for this long (milliseconds) and farther than
 *  SCIENTIST_GONE_DISTANCE: he is gone (out of the level; he doesn't come back) */
#define SCIENTIST_GONE_TIME         (700)
#define SCIENTIST_GONE_DISTANCE     (300)
/** @brief The agent dead: the guards come and look at him from this far (units) */
#define GUARD_BODY_DISTANCE         (190)
/** @brief Blocked by a door longer than this (milliseconds): he goes around it */
#define GUARD_DOOR_WAIT             (1200)
#define GUARD_RUN_DISTANCE          (600)
/** @brief Radius of his body for the walls (world units), the highest step he climbs */
#define GUARD_RADIUS                (20)
#define GUARD_MAX_STEP              (20)
/** @brief A hit in the head kills him (the damage times this) */
#define GUARD_HEAD_MULTIPLIER       (10)
/** @brief The time he does nothing else after a hit (milliseconds): most of the reaction (0.8 s: the hit (the chest, the
 *  head), an arm; 1 s: a leg; 0.3 s on one knee), see tools/make_guard.py */
#define GUARD_HIT_TIME              (650)
#define GUARD_HIT_LEG_TIME          (850)
#define GUARD_HIT_KNEEL_TIME        (320)
/** @brief Red for this time after a hit (milliseconds), its color added to the gouraud tables */
#define GUARD_HURT_TIME             (180)
#define GUARD_HURT_RED              (36)
/** @brief Blood where he is hit: drops (sprites) flying for this time (milliseconds), their size (world units) */
#define BLOOD_MAX                   (8)
#define BLOOD_TIME                  (350)
#define BLOOD_SIZE                  (JO_FIXED_1 * 5)
/** @brief The cap: a hit over the bottom of the cap (world units over the feet) knocks it off the head (no damage), the
 *  center of the cap over the feet (the model: Y down), the falling caps (they lie on the floor after) */
#define GUARD_CAP_BOTTOM            (GUARD_SIZE(92))
#define GUARD_CAP_CENTER            (GUARD_SIZE(95))
#define GUARD_CAP_TOP               (GUARD_SIZE(98))
#define CAP_MAX                     (4)
/** @brief A cap on the floor goes after this long (milliseconds), like the bodies */
#define CAP_FLOOR_TIME              (6000)
typedef struct
{
    jo_pos3D_fixed                  position;
    jo_pos3D_fixed                  speed;          // World units per second (Q16.16)
    int                             yaw;
    int                             spin;           // Degrees (around X), its speed (degrees per second)
    int                             spin_speed;
    jo_fixed                        floor;
    const jo_3d_model               *model;
    const jo_entity                 *owner;         // The guard it fell from (it goes with him when he dies)
    int                             on_floor;       // Milliseconds on the floor (it goes after CAP_FLOOR_TIME)
    bool                            used;
}                                   falling_cap;
static falling_cap                  caps[CAP_MAX];
static int                          next_cap = 0;
/** @brief The bone of the cap in the model of the guards (-1: none), the middle of the cap (model space) */
static int                          cap_bone = -1;
static jo_pos3D_fixed               cap_middle;
/** @brief A shot passing this near the middle of the cap of a guard in his fall hits it (world units) */
#define CAP_HIT_RADIUS              (GUARD_SIZE(6))
/** @brief The bone of the rifle in the model of the guards (-1: none) and the end of its barrel (model space: the point of the
 *  rifle the farthest from its joint, found in the model): the flame of a shot there (it was in front of his belly) */
static int                          rifle_bone = -1;
static jo_pos3D_fixed               rifle_muzzle;
/** @brief The doorways between the rooms (the ways from a room to another): the level has 13 (8 before: the guards
 *  didn't know the doors after the 8th one, the gallery, the airlock, the stairs, the lower floor) */
#define GUARD_MAX_DOORWAYS          (16)
/** @brief The way to a point through several rooms (see way_to()): the points on the 2 sides of each doorway (units from
 *  its middle), the way computed again after this time (ms) or when the point moved */
#define GUARD_WAY_SIDE              (40)
#define GUARD_WAY_NODES             (GUARD_MAX_DOORWAYS * 2)
#define GUARD_WAY_PERIOD            (500)
#define GUARD_WAY_NONE              (0xFFFF)
/** @brief Ways computed again per frame at most (the others keep their next point one more frame): the agent seen again
 *  moved the point of all the hunting guards in the same frame, 16 ways in a frame (a frame of 75 ms) */
#define GUARD_WAY_BUDGET            (1)
/** @brief The load of the frames (see persons_load_update()): 256 when the frames are on time, less after frames of 3 fields
 *  or more (down to PERSONS_LOAD_MIN), back up slowly; under PERSONS_LOAD_MARKS the missed shots of the guards leave no mark
 *  (the ray of the mark: a general alert, 16 guards shooting, 2 ms of rays per frame for the marks). Tried: the far model
 *  of the persons nearer under load: no change in a general alert from the walkway (the guards come up next to the
 *  agent, near whatever the distance) */
#define PERSONS_LOAD_MIN            (96)
#define PERSONS_LOAD_DROP           (48)
#define PERSONS_LOAD_RISE           (3)
#define PERSONS_LOAD_MARKS          (192)
static int                          persons_load = 256;
/** @brief Rays toward the nearest points of the doorways when none of the room is seen (see way_seen()) */
#define GUARD_WAY_FALLBACK_RAYS     (3)
/** @brief Rays toward the nearest points of the doorways of the room (see way_seen()) */
#define GUARD_WAY_ROOM_RAYS         (4)
/** @brief Doorways tried at most when no way is found through the points (see way_to_near()) */
#define GUARD_WAY_NEAR_RAYS         (4)
/** @brief The body stays this long (milliseconds), sinking in the floor during its last GUARD_BODY_SINK, then it goes */
#define GUARD_BODY_TIME             (5000)
#define GUARD_BODY_SINK             (1200)
#define GUARD_BODY_DEPTH            (GUARD_SIZE(24))
/** @brief At most this many bodies on the floor at once: a new death removes the oldest body at once */
#define GUARD_MAX_BODIES            (4)
/** @brief A guard killed on one knee: the change from his pose to the fall (milliseconds) */
#define GUARD_KNEEL_DEATH_BLEND     (250)
/** @brief Far from the agent he kneels more often to shoot (world units), and he aims better on one knee (percent) */
#define GUARD_KNEEL_DISTANCE        (450)
#define GUARD_KNEEL_BONUS           (18)
/** @brief Time to kneel down or to get up (milliseconds: the animations crouch and stand) */
#define GUARD_STANCE_TIME           (550)
/** @brief The aim assistance of the agent: a shot that passes beside a guard, at most AUTO_AIM_MARGIN from his body (world
 *  units: 50 cm) on the sides, over his head or under his feet, hits him in the chest; nearer than AUTO_AIM_DISTANCE. A
 *  shot on him (the head, a leg) goes to his chest too, standing or on one knee (the head: the aiming mode) */
#define AUTO_AIM_MARGIN             (25)
#define AUTO_AIM_DISTANCE           (1600)
/** @brief The aim on the explosive barrels: a shot at most this far over their lids or under them hits them (units) */
#define AUTO_AIM_BARREL_MARGIN      (15)
#define GUARD_CHEST_HEIGHT          (GUARD_SIZE(62))
/** @brief Sound channels: his voice, his rifle (see the channels of demo.h) */
#define GUARD_VOICE_CHANNEL         (CHANNEL_ENEMY)
/** @brief The sounds of the guards: heard up to there (units), their volumes */
#define GUARD_SOUND_RANGE           (2400)
#define GUARD_SHOT_VOLUME           (120)
#define GUARD_VOICE_VOLUME          (120)

typedef enum
{
    GUARD_IDLE,
    GUARD_SURPRISED,
    GUARD_ALERT,
    GUARD_ATTACK,
    GUARD_SEARCH,
    GUARD_DEAD,
    // (the scientists: walking between 2 points, his hands up, running away)
    SCIENTIST_WALK,
    SCIENTIST_HANDS_UP,
    SCIENTIST_FLEE,
}                                   guard_state;

/** @brief Data of a guard: an actor of the engine first */
typedef struct
{
    jo_actor                        actor;
    int                             state;
    int                             timer;          // Milliseconds (the surprise, the reaction, the time between 2 shots)
    int                             shots;          // Shots left in the burst
    int                             sight_timer;    // Milliseconds until the next check of the line of sight
    int                             unseen;         // Milliseconds since he saw the agent
    int                             search;         // Milliseconds of the search
    int                             hit;            // Milliseconds of the animation of a hit
    int                             hurt;           // Milliseconds of red
    int                             closing;        // Milliseconds left of his run at the agent between 2 bursts
    int                             winded;         // Milliseconds left of his bad aim after a run
    int                             step;           // Milliseconds left of his step aside, its side (step_side: 1, -1)
    int                             step_side;
    bool                            scientist;      // A scientist (the class scientist), not a guard
    bool                            met;            // A scientist: the agent threatened him once (his hands went up)
    int                             fear;           // A scientist: milliseconds left of his run away, of his pause
    int                             stuck;          // A scientist: milliseconds without a move on his run away
    int                             hidden_time;    // A scientist running away: milliseconds in a room the agent doesn't see
    jo_fixed                        flee_x;         // A scientist: where he runs to
    jo_fixed                        flee_z;
    int                             last_health;    // His health at the last update (hurt by an explosion: not by a shot)
    int                             moved;          // Distance walked in the last frame (1/256 units): the time of his steps
    int                             gait_speed;     // Speed of the animation of his steps (walk or run), 0: not walking
    bool                            run;            // He runs to the agent (from another room, from far)
    int                             door_wait;      // Milliseconds blocked by a door (open: he goes around its panel)
    int                             detour_time;    // Milliseconds left walking around a piece of furniture
    const jo_entity                 *near[12];      // The solid entities near him (see near_gather()): what can block him
    int                             near_count;
    jo_fixed                        near_x;         // Where he was when they were gathered (gathered again farther)
    jo_fixed                        near_z;
    int                             near_age;       // Milliseconds since (gathered again after NEAR_GATHER_TIME)
    int                             detour_yaw;     // The direction of that walk around
    int                             way_timer;      // Milliseconds until his way is computed again (see way_to())
    jo_fixed                        way_x;          // The next point of his way (the point itself, or a side of a doorway)
    jo_fixed                        way_z;
    jo_fixed                        way_target_x;   // The point the way goes to
    jo_fixed                        way_target_z;
    bool                            way_valid;      // way_x, way_z computed
    int                             dead;           // Milliseconds since his death (the body goes after GUARD_BODY_TIME)
    bool                            hatless;        // His cap was shot off his head
    int                             face;           // His face (see guard_faces.h)
    int                             far_distance;   // His far model farther (world units: the property far_distance)
    bool                            sees;
    bool                            outward;        // He walks toward the end of his patrol
    bool                            crouched;       // He fights on one knee
    int                             stand;          // Milliseconds left of getting up (he doesn't move nor shoot)
    jo_fixed                        home_x;
    jo_fixed                        home_z;
    int                             home_yaw;
    jo_fixed                        patrol_x;
    jo_fixed                        patrol_z;
    jo_fixed                        seen_x;         // Where he saw the agent for the last time (searching: where he
    jo_fixed                        seen_z;         // goes now)
    jo_pos3D_fixed                  muzzle;         // The end of the barrel of his rifle when he was drawn (world)
    int                             muzzle_frame;   // The frame when he was drawn, -1: never
    jo_pos3D_fixed                  cap;            // The middle of his cap when he was drawn (world: it follows his fall)
    int                             cap_frame;      // The frame when it was drawn, -1: never
}                                   guard;

static void                         scare_scientists(const jo_entity * const except, const jo_fixed x, const jo_fixed z);

typedef struct
{
    jo_pos3D_fixed                  position;
    jo_pos3D_fixed                  speed;          // World units per second (Q16.16)
    int                             time;           // Milliseconds left
}                                   blood_drop;

static blood_drop                   blood[BLOOD_MAX];
static int                          next_blood = 0;
static int                          tex_blood = -1;
static int                          tex_flash = -1;
/** @brief The flashes of the rifles of the guards: where, the time left (milliseconds) */
#define GUARD_FLASH_MAX             (4)
#define GUARD_FLASH_TIME            (70)
#define GUARD_FLASH_SIZE            (JO_FIXED_1 * 9)
/** @brief The rifles dropped by the dead guards (on the floor, picked up by the agent: the rifle, or its ammunition) */
#define DROP_MAX                    (8)
#define DROP_DISTANCE               (34)
typedef struct
{
    jo_pos3D_fixed                  position;
    int                             yaw;
    bool                            used;
}                                   dropped_rifle;
static dropped_rifle                drops[DROP_MAX];
static jo_3d_model                  *rifle_model = JO_NULL;
/** @brief The cap alone (CAP.JOM, the coordinates of the cap in the model of the guard): the cap of a dead guard on the
 *  floor. Drawn from the model of the guard with all its bones hidden but the cap, it cost the whole model (300 points,
 *  212 polygons rejected): 3 ms per frame for 6 s */
static jo_3d_model                  *cap_model = JO_NULL;
/** @brief The model of the guards without the rifle in the hands (the dead ones: they dropped it) */
static jo_3d_model                  *unarmed_model = JO_NULL;
static jo_3d_model                  *unarmed_far_model = JO_NULL;
static jo_pos3D_fixed               flash_position[GUARD_FLASH_MAX];
static int                          flash_time[GUARD_FLASH_MAX];
static int                          next_flash = 0;
/** @brief The cries of the guards: hit ("ouh", "aie", "argh", "euf", "aaah", "oof"), killed ("aaaaahh", "aargh") */
#define PAIN_SOUND_COUNT            (4)
static jo_sound                     pain_sounds[PAIN_SOUND_COUNT];
/** @brief A cry played a little higher or lower each time (percent of its rate, at random up to this): the 3 cries of
 *  pain sound like more voices */
#define GUARD_CRY_PITCH_RANGE       (12)
/** @brief The shots of their rifles (2: a burst is not the same sound repeated), the channel of the next one */
#define GUARD_SHOT_SOUNDS           (2)
static jo_sound                     guard_shots[GUARD_SHOT_SOUNDS];
static int                          guard_shot_channel = 0;
static bool                         sounds_loaded = false;

/** @brief Distance on the floor to the agent (an octagon, world units) */
static int                          distance_to_agent(const jo_entity * const e)
{
    int                             dx;
    int                             dz;

    dx = JO_ABS(jo_fixed2int(player_x - e->position.x));
    dz = JO_ABS(jo_fixed2int(player_z - e->position.z));
    return (dx > dz ? dx + ((dz * 3) >> 3) : dz + ((dx * 3) >> 3));
}

/** @brief Nothing between 2 points: no wall (with_doors: no closed door either; with_glass: no pane of a window either) */

static bool                         is_clear(const jo_pos3D_fixed * const a, const jo_pos3D_fixed * const b, const bool with_doors,
                                             const bool with_glass)
{
    jo_pos3D_fixed                  dir;
    jo_pos3D_fixed                  normal;
    jo_fixed                        length;
    jo_fixed                        t;
    jo_entity                       *door;

    dir.x = b->x - a->x;
    dir.y = b->y - a->y;
    dir.z = b->z - a->z;
    // (whole units: the squares of the fixed point values overflow)
    length = jo_int2fixed((int)jo_sqrt((unsigned int)(jo_fixed2int(dir.x) * jo_fixed2int(dir.x) + jo_fixed2int(dir.y) * jo_fixed2int(dir.y) +
                                                        jo_fixed2int(dir.z) * jo_fixed2int(dir.z))));
    if (length < JO_FIXED_1)
        return (true);
    dir.x = jo_fixed_div(dir.x, length);
    dir.y = jo_fixed_div(dir.y, length);
    dir.z = jo_fixed_div(dir.z, length);
    if (jo_cells_ray_hit(a, &dir, length, &t, &normal))
        return (false);
    // (a pane of a window: not a way; seen through, see is_line_clear())
    if (with_glass && doors_glass_ray_hit(a, &dir, length, &normal, &door) >= 0)
        return (false);
    return (!with_doors || doors_ray_hit(a, &dir, length, &normal, &door, true) < 0);
}

/** @brief Nothing between 2 points: no wall, no closed door (the sight: through the panes of the windows; the first shot
 *  breaks the pane, see shoot_agent()) */
static bool                         is_line_clear(const jo_pos3D_fixed * const a, const jo_pos3D_fixed * const b)
{
    bool                            clear;

    clear = is_clear(a, b, true, false);
    return (clear);
}

/** @brief He sees the agent: near, in front of him, nothing between his eyes and the eyes of the agent */
static bool                         sees_agent_within(const jo_entity * const e, const int behind)
{
    jo_pos3D_fixed                  eyes;
    jo_pos3D_fixed                  target;
    int                             distance;
    jo_fixed                        dot;

    distance = distance_to_agent(e);
    if (distance > GUARD_SIGHT_DISTANCE || player_health <= 0)
        return (false);
    // (the agent in the ventilation duct: hidden in it, the guards under its hole don't see him, they don't shoot at him)
    if (player_in_duct())
        return (false);
    // In front of him (very near: he feels the agent behind him; behind: this near, he notices him whatever his facing)
    if (distance > behind)
    {
        dot = (jo_fixed_mult(jo_int2fixed(jo_fixed2int(player_x - e->position.x)) / distance, jo_sin(e->yaw)) +
               jo_fixed_mult(jo_int2fixed(jo_fixed2int(player_z - e->position.z)) / distance, jo_cos(e->yaw)));
        if (dot < GUARD_SIGHT_COS)
            return (false);
    }
    // (in a room the agent doesn't see (no way through the doorways between them): no ray, it costs 0.7 ms)
    if (jo_cells_depth_bias_at(e->position.x, e->position.y - JO_FIXED_1, e->position.z) < 0)
        return (false);
    eyes.x = e->position.x;
    eyes.y = e->position.y - jo_int2fixed(GUARD_EYE_HEIGHT);
    eyes.z = e->position.z;
    player_eye(&target);
    return (is_line_clear(&eyes, &target));
}

static bool                         sees_agent(const jo_entity * const e)
{
    return (sees_agent_within(e, 60));
}

/** @brief The solid entities near a person (the benches, the crates, the people): what can block his steps. The test of
 *  the engine looks at the 80 solid entities of the level (260 microseconds each time, several times per frame per
 *  moving person: the detours); this list of at most NEAR_MAX, gathered within NEAR_RADIUS, is used instead, gathered
 *  again when he moved NEAR_AGAIN from where it was gathered, or after NEAR_GATHER_TIME (the others move too) */
#define NEAR_MAX                    (12)
#define NEAR_RADIUS                 (400)
#define NEAR_AGAIN                  (150)
#define NEAR_GATHER_TIME            (500)

static void                         near_gather(const jo_entity * const e, guard * const g)
{
    int                             k;
    const jo_entity                 *o;
    int                             reach;

    g->near_count = 0;
    g->near_x = e->position.x;
    g->near_z = e->position.z;
    g->near_age = 0;
    for (k = 0; (o = jo_entity_get(k)) != JO_NULL && g->near_count < NEAR_MAX; ++k)
    {
        if (o == e || !(o->flags & JO_ENTITY_FLAG_SOLID) || (o->flags & JO_ENTITY_FLAG_DISABLED))
            continue;
        // (a box: its half sizes added, a turned box is within their sum)
        reach = NEAR_RADIUS + (o->box_z > 0 ? o->box_x + o->box_z : o->radius);
        if (JO_ABS(jo_fixed2int(o->position.x - e->position.x)) > reach || JO_ABS(jo_fixed2int(o->position.z - e->position.z)) > reach)
            continue;
        // (the vertical overlap: his body from his feet up to 90)
        if (o->position.y <= e->position.y - jo_int2fixed(90) || o->position.y - jo_int2fixed(o->height) >= e->position.y)
            continue;
        g->near[g->near_count++] = o;
    }
}

/** @brief The list gathered again when it is old or he went far from where it was gathered (see near_gather()); called
 *  only when the list is about to be used (a step, a choice of a way out): the people standing still never gather (19
 *  lists gathered in the same frame took 6 ms every half second) */
static void                         near_update(const jo_entity * const e, guard * const g)
{
    if (g->near_age >= NEAR_GATHER_TIME || JO_ABS(jo_fixed2int(e->position.x - g->near_x)) > NEAR_AGAIN ||
        JO_ABS(jo_fixed2int(e->position.z - g->near_z)) > NEAR_AGAIN)
        near_gather(e, g);
}

/** @brief One of the entities gathered blocks a point (the test of the engine: a cylinder, or a box turned with the entity;
 *  a disabled one (a scientist gone) no longer) */
static bool                         near_blocks(const guard * const g, const jo_fixed x, const jo_fixed z)
{
    int                             k;
    const jo_entity                 *o;
    int                             dx;
    int                             dz;
    int                             lx;
    int                             lz;
    int                             r;

    for (k = 0; k < g->near_count; ++k)
    {
        o = g->near[k];
        if (o->flags & JO_ENTITY_FLAG_DISABLED)
            continue;
        dx = jo_fixed2int(o->position.x - x);
        dz = jo_fixed2int(o->position.z - z);
        if (o->box_z > 0)
        {
            r = o->box_x + o->box_z + GUARD_RADIUS * 2 + 2;
            if (JO_ABS(dx) >= r || JO_ABS(dz) >= r)
                continue;
            lx = jo_fixed2int(dx * jo_cos(o->yaw) - dz * jo_sin(o->yaw));
            lz = jo_fixed2int(dx * jo_sin(o->yaw) + dz * jo_cos(o->yaw));
            if (JO_ABS(lx) < o->box_x + GUARD_RADIUS && JO_ABS(lz) < o->box_z + GUARD_RADIUS)
                return (true);
            continue;
        }
        r = o->radius + GUARD_RADIUS;
        if (JO_ABS(dx) < r && JO_ABS(dz) < r && dx * dx + dz * dz < r * r)
            return (true);
    }
    return (false);
}

/** @brief The furniture (the solid entities: the partitions of the stalls, the toilets, the crates...), the other guards
 *  and the agent stop him (the closed doors: see doors_block())
 */
static bool                         is_blocked(const jo_entity * const e, const jo_fixed x, const jo_fixed z)
{
    int                             dx;
    int                             dz;
    bool                            blocked;

    dx = jo_fixed2int(x - player_x);
    dz = jo_fixed2int(z - player_z);
    if (dx * dx + dz * dz < (GUARD_RADIUS + PLAYER_RADIUS) * (GUARD_RADIUS + PLAYER_RADIUS))
        return (true);
    // (not himself: his cylinder is solid; the entities near him, see near_gather())
    blocked = near_blocks((const guard *)e->data, x, z);
    return (blocked);
}

/** @brief Walk or run in a direction (degrees, like the yaw), stopped by the walls
 *  @return false if he is blocked
 */
static bool                         move_toward(jo_entity * const e, const int direction, const int speed, const int elapsed)
{
    jo_fixed                        step;
    jo_fixed                        nx;
    jo_fixed                        nz;
    int                             floor;

    step = jo_int2fixed(speed) / 1000 * elapsed;
    nx = e->position.x + jo_fixed_mult(jo_sin(direction), step);
    nz = e->position.z + jo_fixed_mult(jo_cos(direction), step);
    if (doors_block(nx, nz, GUARD_RADIUS) && !doors_block(e->position.x, e->position.z, GUARD_RADIUS))
    {
        // A closed door in front of him: he opens it (he waits while it turns; in the doorway of a door that closes: he
        // leaves it)
        doors_open_for(nx, nz, GUARD_RADIUS + 6, e->position.x, e->position.z);
        return (false);
    }
    if (doors_panel_block(nx, nz, GUARD_RADIUS, e->position.x, e->position.z))
    {
        // The panel of a door open toward him: he closes it and opens it the other way
        doors_swing_away(nx, nz, GUARD_RADIUS, e->position.x, e->position.z);
        return (false);
    }
    if (!jo_cells_is_walkable(nx, e->position.y, nz, GUARD_RADIUS, GUARD_MAX_STEP) ||
        !jo_cells_floor(nx, e->position.y, nz, GUARD_MAX_STEP, &floor))
        return (false);
    near_update(e, (guard *)e->data);
    if (is_blocked(e, nx, nz))
        return (false);
    e->position.x = nx;
    e->position.z = nz;
    e->position.y = jo_int2fixed(floor);
    // (the bounding sphere of the model follows him)
    e->center.x = nx;
    e->center.z = nz;
    e->center.y = e->position.y - jo_int2fixed(GUARD_SIZE(50));
    return (true);
}

static bool                         move_forward(jo_entity * const e, const int speed, const int elapsed)
{
    return (move_toward(e, e->yaw, speed, elapsed));
}

/** @brief Walk toward a point: turn, then walk (or run)
 *  @return true when he is there (or blocked)
 */
/** @brief The way to a point through one doorway: the point itself when no wall is between (at the height of his waist), else the doorway
 *  between the rooms that leads there (the shortest way through one doorway seen from him) */
static void                         way_to_near(const jo_entity * const e, const jo_fixed x, const jo_fixed z, jo_fixed * const wx,
                                           jo_fixed * const wz)
{
    jo_pos3D_fixed                  doorways[GUARD_MAX_DOORWAYS * 2];
    jo_pos3D_fixed                  a;
    jo_pos3D_fixed                  b;
    int                             n;
    int                             stalls;
    int                             k;
    int                             best;
    int                             cost;
    int                             count;
    int                             j;
    int                             costs[GUARD_MAX_DOORWAYS * 2];
    int                             order[GUARD_MAX_DOORWAYS * 2];

    *wx = x;
    *wz = z;
    a.x = e->position.x;
    a.y = e->position.y - jo_int2fixed(GUARD_SIZE(50));
    a.z = e->position.z;
    b.x = x;
    b.y = a.y;
    b.z = z;
    if (is_clear(&a, &b, false, true))
        return;
    // (the doorways between the rooms, then the openings of the stalls: only toward the point, or out of the stall where he
    // is: a stall is never a way to elsewhere)
    n = doors_doorways(doorways, GUARD_MAX_DOORWAYS, false);
    stalls = n;
    n += doors_doorways(&doorways[n], GUARD_MAX_DOORWAYS * 2 - n, true);
    // The doorways by cost (the way through them), the first one seen (and that sees the point) is the best: the rays stop
    // there, GUARD_WAY_NEAR_RAYS doorways at most (a ray toward each of the 18 doorways of the level, 25 rays and 16 ms in
    // one frame in a general alert)
    for (k = 0, count = 0; k < n; ++k)
    {
        doorways[k].y = a.y;
        // (the doorway where he is: not a way)
        if (JO_ABS(jo_fixed2int(doorways[k].x - a.x)) + JO_ABS(jo_fixed2int(doorways[k].z - a.z)) < 24)
            continue;
        cost = JO_ABS(jo_fixed2int(doorways[k].x - a.x)) + JO_ABS(jo_fixed2int(doorways[k].z - a.z)) +
               JO_ABS(jo_fixed2int(doorways[k].x - x)) + JO_ABS(jo_fixed2int(doorways[k].z - z));
        for (j = count; j > 0 && costs[j - 1] > cost; --j)
        {
            costs[j] = costs[j - 1];
            order[j] = order[j - 1];
        }
        costs[j] = cost;
        order[j] = k;
        ++count;
    }
    for (j = 0, best = 0; j < count && best < GUARD_WAY_NEAR_RAYS; ++j)
    {
        k = order[j];
        ++best;
        if (!is_clear(&a, &doorways[k], false, true))
            continue;
        if (k >= stalls && !is_clear(&doorways[k], &b, false, true))
            continue;
        *wx = doorways[k].x;
        *wz = doorways[k].z;
        return;
    }
}

/** @brief The VBLANK count for the period of the ways */
# define guard_vblank_count()               jo_core_get_vblank_count()

/** @brief The ways between the rooms: the points on the 2 sides of the doorways (at the height of the waist), the length
 *  between 2 of them when nothing is between (GUARD_WAY_NONE: a wall), computed once per level (the walls don't move, the
 *  doors are not walls for the way: a guard opens them) */
static jo_pos3D_fixed               way_nodes[GUARD_WAY_NODES];
static unsigned short               way_lengths[GUARD_WAY_NODES][GUARD_WAY_NODES];
static short                        way_cells[GUARD_WAY_NODES];     // The cell (room) of each point
static int                          way_node_count = -1;
/** @brief The ways computed in this frame (see GUARD_WAY_BUDGET): its frame count (not its VBLANK: in a slow frame the VBLANK
 *  changed during the update of the guards, more ways were computed in it and the frame was even slower) */
static int                          way_budget_frame = -1;
static int                          way_budget_used = 0;
/** @brief The points seen from the point the guards go to (the agent: the same for all of them), computed once for them all
 *  (see way_to()): the point, the VBLANK when they were computed, the lengths (-1: not seen) */
static jo_fixed                     way_end_x;
static jo_fixed                     way_end_z;
static jo_fixed                     way_end_y;
static unsigned int                 way_end_vblank = 0;
static bool                         way_end_valid = false;
static int                          way_end_lengths[GUARD_WAY_NODES];

/** @brief Length between 2 points on the floor (the longest axis plus 3/8 of the other one, within 6 %) */
static int                          way_length(const jo_fixed ax, const jo_fixed az, const jo_fixed bx, const jo_fixed bz)
{
    int                             dx;
    int                             dz;

    dx = JO_ABS(jo_fixed2int(ax - bx));
    dz = JO_ABS(jo_fixed2int(az - bz));
    return (dx > dz ? dx + ((dz * 3) >> 3) : dz + ((dx * 3) >> 3));
}

static void                         way_build(void)
{
    int                             i;
    int                             j;
    int                             length;

    way_node_count = doors_doorway_sides(way_nodes, GUARD_WAY_NODES, GUARD_WAY_SIDE);
    for (i = 0; i < way_node_count; ++i)
    {
        way_cells[i] = (short)jo_cells_find(way_nodes[i].x, way_nodes[i].y, way_nodes[i].z, PLAYER_STEP);
        way_nodes[i].y -= jo_int2fixed(GUARD_SIZE(50));
    }
    way_end_valid = false;
    for (i = 0; i < way_node_count; ++i)
    {
        way_lengths[i][i] = 0;
        for (j = i + 1; j < way_node_count; ++j)
        {
            length = way_length(way_nodes[i].x, way_nodes[i].z, way_nodes[j].x, way_nodes[j].z);
            // (the 2 sides of a doorway: through it)
            if ((j == i + 1 && (i & 1) == 0) || is_clear(&way_nodes[i], &way_nodes[j], false, true))
                way_lengths[i][j] = (unsigned short)JO_MIN(length, GUARD_WAY_NONE - 1);
            else
                way_lengths[i][j] = GUARD_WAY_NONE;
            way_lengths[j][i] = way_lengths[i][j];
        }
    }
}

/** @brief The lengths to the points of the doorways seen from a point (-1: not seen, or nearer than near)
 *  @remarks The rays only toward the points in the same room and the other sides of its doorways (the walls hide the
 *  others; a doorway of another room is reached through the doorways of this one), toward all of them when no point is
 *  in this room or none is seen
 */
static void                         way_seen(const jo_pos3D_fixed * const p, int lengths[GUARD_WAY_NODES], const int near)
{
    int                             cell;
    int                             seen;
    int                             tested;
    int                             best;
    int                             i;
    int                             length;
    int                             nearest;
    int                             distance_nearest;
    int                             distance[GUARD_WAY_NODES];
    int                             room[GUARD_WAY_ROOM_RAYS + 1];
    int                             room_distance[GUARD_WAY_ROOM_RAYS + 1];
    int                             room_count;
    int                             j;

    cell = jo_cells_find(p->x, p->y + jo_int2fixed(GUARD_SIZE(50)), p->z, PLAYER_STEP);
    // The points in this room and the other sides of its doorways (through them): the nearest GUARD_WAY_ROOM_RAYS of them
    // get a ray (a ray costs 0.4 to 0.7 ms; the hall has 5 doors, 10 points: 7 ms for one way in a general alert)
    nearest = -1;
    room_count = 0;
    for (i = 0, seen = 0; i < way_node_count; ++i)
    {
        distance[i] = way_length(p->x, p->z, way_nodes[i].x, way_nodes[i].z);
        lengths[i] = -1;
        if (cell < 0 || (way_cells[i] != cell && way_cells[i ^ 1] != cell))
            continue;
        // (tested: not tested again below)
        length = distance[i];
        distance[i] = -1;
        if (length < near)
            continue;
        if (nearest < 0 || length < distance_nearest)
        {
            nearest = i;
            distance_nearest = length;
        }
        // (kept by distance, the nearest first)
        for (j = room_count; j > 0 && room_distance[j - 1] > length; --j)
        {
            room[j] = room[j - 1];
            room_distance[j] = room_distance[j - 1];
        }
        room[j] = i;
        room_distance[j] = length;
        if (room_count < GUARD_WAY_ROOM_RAYS)
            ++room_count;
    }
    for (j = 0; j < room_count; ++j)
        if (is_clear(p, &way_nodes[room[j]], false, true))
        {
            lengths[room[j]] = room_distance[j];
            ++seen;
        }
    // The room has points but none is seen (a tank, a stall between): the nearest one, the furniture is gone around on the
    // way (see go_to_way()). The rays toward the points of the other rooms found nothing (walls) and cost 4 ms per way: 2
    // ways in a frame made a frame of 36 ms in the bottling room
    if (seen == 0 && nearest >= 0)
    {
        lengths[nearest] = distance_nearest;
        return;
    }
    // None (a room without a doorway, the openings of the bottling room, or between 2 cells): the nearest other points, a few
    // of them (the rays toward the 26 points cost 10 ms)
    for (tested = 0; seen == 0 && tested < GUARD_WAY_FALLBACK_RAYS; ++tested)
    {
        best = -1;
        for (i = 0; i < way_node_count; ++i)
            if (distance[i] >= 0 && (best < 0 || distance[i] < distance[best]))
                best = i;
        if (best < 0)
            break;
        length = distance[best];
        distance[best] = -1;
        if (length >= near && is_clear(p, &way_nodes[best], false, true))
        {
            lengths[best] = length;
            ++seen;
        }
    }
}

/** @brief The way to a point: the point itself when no wall is between (at the height of his waist), else the first point
 *  of the shortest way through the doorways (several rooms: Dijkstra on the sides of the doorways), else the old way
 *  through one doorway (way_to_near(): the stalls). Computed again every GUARD_WAY_PERIOD or when the point moved (the
 *  rays of the way at each frame for each guard who hunted the agent slowed the game down)
 */
static void                         way_to_inner(jo_entity * const e, const jo_fixed x, const jo_fixed z, jo_fixed * const wx,
                                           jo_fixed * const wz, const int elapsed)
{
    guard                           *g;
    jo_pos3D_fixed                  a;
    jo_pos3D_fixed                  b;
    int                             from_start[GUARD_WAY_NODES];
    int                             to_end[GUARD_WAY_NODES];
    int                             best[GUARD_WAY_NODES];
    int                             first[GUARD_WAY_NODES];
    int                             way[GUARD_WAY_NODES];
    int                             n;
    bool                            done[GUARD_WAY_NODES];
    int                             total;
    int                             i;
    int                             j;
    int                             k;

    g = (guard *)e->data;
    g->way_timer -= elapsed;
    // (the same way: the time is not over, the point didn't move, the next point not reached; or the ways of this frame
    // are already computed: the next point of the last way one more frame, if it is not reached)
    if (frame_counter != way_budget_frame)
    {
        way_budget_frame = frame_counter;
        way_budget_used = 0;
    }
    if ((g->way_timer > 0 && way_length(x, z, g->way_target_x, g->way_target_z) < 32 &&
         way_length(e->position.x, e->position.z, g->way_x, g->way_z) >= 20) ||
        (way_budget_used >= GUARD_WAY_BUDGET && g->way_valid && way_length(e->position.x, e->position.z, g->way_x, g->way_z) >= 20))
    {
        *wx = g->way_x;
        *wz = g->way_z;
        return;
    }
    // (no way yet and the ways of this frame computed: toward the point itself for one frame, the guards alerted together)
    if (way_budget_used >= GUARD_WAY_BUDGET)
    {
        *wx = x;
        *wz = z;
        return;
    }
    ++way_budget_used;
    // (a part of the period depends on his position: the guards alerted together don't compute their ways in the same frame)
    g->way_timer = GUARD_WAY_PERIOD - ((int)(e->position.x >> 16) & 127);
    g->way_target_x = x;
    g->way_target_z = z;
    *wx = x;
    *wz = z;
    a.x = e->position.x;
    a.y = e->position.y - jo_int2fixed(GUARD_SIZE(50));
    a.z = e->position.z;
    b.x = x;
    // (at the height of the agent when the point is near him, the agent on another floor: the rays from the point toward the
    // doorways went under the floor, nothing seen, the rays toward all the doorways for all the guards. Else at the height of
    // the guard: his place, his patrol)
    b.y = way_length(x, z, player_x, player_z) < 64 ? player_y - jo_int2fixed(GUARD_SIZE(50)) : a.y;
    b.z = z;
    if (!is_clear(&a, &b, false, true))
    {
        if (way_node_count < 0)
            way_build();
        // The points of the doorways seen from him (not the one where he is) and from the point: the rays only toward the
        // points in his room (and in the room of the point), all of them when there is none (a room without a door: the
        // openings of the bottling room). A ray through the rooms costs about 0.4 ms
        way_seen(&a, from_start, 0);
        if (!way_end_valid || guard_vblank_count() - way_end_vblank >= GUARD_WAY_PERIOD * 60 / 1000 ||
            way_length(x, z, way_end_x, way_end_z) >= 8 || b.y != way_end_y)
        {
            way_seen(&b, way_end_lengths, 0);
            way_end_x = x;
            way_end_y = b.y;
            way_end_z = z;
            way_end_vblank = guard_vblank_count();
            way_end_valid = true;
        }
        for (i = 0; i < way_node_count; ++i)
        {
            to_end[i] = way_end_lengths[i];
            best[i] = from_start[i] >= 0 ? from_start[i] : 0x7fffffff;
            first[i] = -1;
            done[i] = false;
        }
        // Dijkstra from him (the nodes reached first: their first point of the way)
        for (;;)
        {
            k = -1;
            for (i = 0; i < way_node_count; ++i)
                if (!done[i] && best[i] != 0x7fffffff && (k < 0 || best[i] < best[k]))
                    k = i;
            if (k < 0)
                break;
            done[k] = true;
            for (j = 0; j < way_node_count; ++j)
                if (!done[j] && way_lengths[k][j] != GUARD_WAY_NONE && best[k] + way_lengths[k][j] < best[j])
                {
                    best[j] = best[k] + way_lengths[k][j];
                    first[j] = k;
                }
        }
        total = 0x7fffffff;
        k = -1;
        for (i = 0; i < way_node_count; ++i)
            if (to_end[i] >= 0 && best[i] != 0x7fffffff && best[i] + to_end[i] < total)
            {
                total = best[i] + to_end[i];
                k = i;
            }
        if (k >= 0)
        {
            // The points of the way, from its last one back (first[]: the point before each one, -1: seen from him)
            for (n = 0; k >= 0 && n < GUARD_WAY_NODES; k = first[k])
                way[n++] = k;
            // (the first one where he doesn't stand yet: nearer than 20, he reached it)
            if (n > 0 && from_start[way[n - 1]] < 20)
                --n;
            // His next point; the one after it instead when he sees it (one ray): the side of a doorway left out, the
            // shortest way went back through it, the guard of the lower floor stayed at the foot of the stairs (2 cells: the
            // stairs, no doorway between them)
            k = n > 0 ? way[n - 1] : -1;
            if (n > 1 && is_clear(&a, &way_nodes[way[n - 2]], false, true))
                k = way[n - 2];
            *wx = k >= 0 ? way_nodes[k].x : x;
            *wz = k >= 0 ? way_nodes[k].z : z;
        }
        else
            way_to_near(e, x, z, wx, wz);
    }
    g->way_x = *wx;
    g->way_z = *wz;
    g->way_valid = true;
}


static void                         way_to(jo_entity * const e, const jo_fixed x, const jo_fixed z, jo_fixed * const wx,
                                           jo_fixed * const wz, const int elapsed)
{
    way_to_inner(e, x, z, wx, wz, elapsed);
}

static bool                         go_to_way(jo_entity * const e, const jo_fixed tx, const jo_fixed tz, const bool run, const int elapsed)
{
    int                             left;
    jo_fixed                        x;
    jo_fixed                        z;

    guard                           *g;

    g = (guard *)e->data;
    if (JO_ABS(jo_fixed2int(tx - e->position.x)) + JO_ABS(jo_fixed2int(tz - e->position.z)) < 20)
        return (true);
    // (around a piece of furniture: he keeps his direction a moment)
    if (g->detour_time > 0)
    {
        g->detour_time -= elapsed;
        jo_entity_turn_to(e, e->position.x + jo_sin(g->detour_yaw) * 100, e->position.z + jo_cos(g->detour_yaw) * 100,
                          GUARD_TURN_SPEED * elapsed / 1000);
        if (move_toward(e, g->detour_yaw, run ? GUARD_RUN_SPEED : GUARD_WALK_SPEED, elapsed))
            return (false);
        g->detour_time = 0;
    }
    // (in another room: through the doorways first)
    way_to(e, tx, tz, &x, &z, elapsed);
    left = jo_entity_turn_to(e, x, z, GUARD_TURN_SPEED * elapsed / 1000);
    // (he turns on the spot first)
    if (left >= 40 || move_forward(e, run ? GUARD_RUN_SPEED : GUARD_WALK_SPEED, elapsed))
        return (false);
    // Blocked by a door: he waits for it to open; still blocked after a while (the panel of a door open in his way), he
    // goes around it like a piece of furniture
    if (doors_block(e->position.x + jo_sin(e->yaw) * (GUARD_RADIUS + 8), e->position.z + jo_cos(e->yaw) * (GUARD_RADIUS + 8), GUARD_RADIUS))
    {
        ((guard *)e->data)->door_wait += elapsed;
        if (((guard *)e->data)->door_wait < GUARD_DOOR_WAIT)
            return (false);
    }
    else
        ((guard *)e->data)->door_wait = 0;
    // Blocked by the furniture (a crate, a bench): he goes around it, on a side then the other (45, 90, then 135 degrees),
    // the first direction free for a few steps, kept for a moment (GUARD_DETOUR_TIME)
    {
        static const int            turns[6] = {45, -45, 90, -90, 135, -135};
        int                         k;
        int                         around;

        for (k = 0; k < 6; ++k)
        {
            around = (e->yaw + turns[k] * (((int)(e->position.x >> 16) & 64) ? -1 : 1) + 360) % 360;
            if (!is_blocked(e, e->position.x + jo_sin(around) * 40, e->position.z + jo_cos(around) * 40) &&
                move_toward(e, around, run ? GUARD_RUN_SPEED : GUARD_WALK_SPEED, elapsed))
            {
                g->detour_yaw = around;
                g->detour_time = GUARD_DETOUR_TIME;
                return (false);
            }
        }
    }
    return (true);
}

/** @brief Go to a point (see go_to_way()), his steps with the distance walked: the animation of the steps (walk, run) plays
 *  at the speed of his feet (guard_update()), standing (turning on the spot, waiting for a door) he stands still. (The
 *  steps played with the time: he slid on the floor, a moonwalk)
 *  @return true if he is there (or can't go farther)
 */
static bool                         go_to(jo_entity * const e, const jo_fixed tx, const jo_fixed tz, const bool run, const int elapsed)
{
    guard                           *g;
    jo_fixed                        x0;
    jo_fixed                        z0;
    int                             dx;
    int                             dz;
    bool                            there;

    g = (guard *)e->data;
    x0 = e->position.x;
    z0 = e->position.z;
    there = go_to_way(e, tx, tz, run, elapsed);
    // (the distance: the longest axis plus half of the other one, within 12 %)
    dx = JO_ABS(e->position.x - x0) >> 8;
    dz = JO_ABS(e->position.z - z0) >> 8;
    g->moved = JO_MAX(dx, dz) + JO_MIN(dx, dz) / 2;
    if (g->moved > 0)
    {
        jo_actor_play(e, run ? "run" : "walk", 200);
        g->gait_speed = run ? GUARD_RUN_SPEED : GUARD_WALK_SPEED;
    }
    else if (!there)
    {
        jo_actor_play(e, "idle", 200);
        g->gait_speed = 0;
    }
    return (there);
}

/** @brief Read a position written by the slave CPU (the bones of a guard it drew, see guard_bone()): without the cache of
 *  the master CPU, it may hold the old value */
static void                         read_slave_position(const jo_pos3D_fixed * const written, jo_pos3D_fixed * const p)
{
    p->x = *(const volatile jo_fixed *)JO_CACHE_THROUGH(&written->x);
    p->y = *(const volatile jo_fixed *)JO_CACHE_THROUGH(&written->y);
    p->z = *(const volatile jo_fixed *)JO_CACHE_THROUGH(&written->z);
}

/** @brief Read a frame number written by the slave CPU (see read_slave_position()) */
static int                          read_slave_frame(const int * const written)
{
    return (*(const volatile int *)JO_CACHE_THROUGH(written));
}

/** @brief The end of his rifle in the world (the aim pose) */
static void                         muzzle_position(const jo_entity * const e, jo_pos3D_fixed * const p, const bool kneeling)
{
    const guard                     *g;

    // (drawn in this frame or the last one: the end of the barrel of his model)
    g = (const guard *)e->data;
    if (g != JO_NULL && read_slave_frame(&g->muzzle_frame) >= frame_counter - 1)
    {
        read_slave_position(&g->muzzle, p);
        return;
    }
    p->x = e->position.x + jo_sin(e->yaw) * GUARD_RIFLE_LENGTH - jo_cos(e->yaw) * 2;
    p->y = e->position.y - jo_int2fixed(kneeling ? GUARD_RIFLE_HEIGHT - GUARD_KNEEL_DROP : GUARD_RIFLE_HEIGHT);
    p->z = e->position.z + jo_cos(e->yaw) * GUARD_RIFLE_LENGTH + jo_sin(e->yaw) * 2;
}

/** @brief A shot at the agent: the sound, the agent is hit or the bullet hits a wall next to him */
static void                         shoot_agent_inner(jo_entity * const e, const guard * const g);

static void                         shoot_agent(jo_entity * const e, const guard * const g)
{
    shoot_agent_inner(e, g);
}

static void                         shoot_agent_inner(jo_entity * const e, const guard * const g)
{
    jo_pos3D_fixed                  muzzle;
    jo_pos3D_fixed                  target;
    jo_pos3D_fixed                  dir;
    jo_fixed                        length;
    int                             distance;
    int                             chance;

    distance = distance_to_agent(e);
    muzzle_position(e, &muzzle, g->crouched);
    // (his shots: the scientists around run away)
    scare_scientists(e, e->position.x, e->position.z);
    // (the flash at the muzzle)
    flash_position[next_flash] = muzzle;
    flash_time[next_flash] = GUARD_FLASH_TIME;
    next_flash = (next_flash + 1) % GUARD_FLASH_MAX;
    if (sounds_loaded)
    {
        // (from where he is: less loud far away, on the side where he is)
        jo_sound                    *shot;

        shot = &guard_shots[jo_random(GUARD_SHOT_SOUNDS) - 1];
        sound_from(shot, e->position.x, e->position.z, GUARD_SHOT_VOLUME, GUARD_SOUND_RANGE);
        // (never silent: a shot is heard in the whole level)
        shot->volume = (unsigned char)JO_MAX(shot->volume, 25);
        play_sound(shot, CHANNEL_GUARD_SHOT_FIRST, CHANNEL_GUARD_SHOT_COUNT, &guard_shot_channel);
    }
    // A pane of a window between him and the agent (he sees him through it): the bullet breaks it, it stops there
    player_eye(&target);
    dir.x = target.x - muzzle.x;
    dir.y = target.y - muzzle.y;
    dir.z = target.z - muzzle.z;
    length = jo_int2fixed((int)jo_sqrt((unsigned int)(jo_fixed2int(dir.x) * jo_fixed2int(dir.x) + jo_fixed2int(dir.y) * jo_fixed2int(dir.y) +
                                                        jo_fixed2int(dir.z) * jo_fixed2int(dir.z))));
    if (length >= JO_FIXED_1)
    {
        jo_entity                   *glass;
        jo_pos3D_fixed              normal;

        dir.x = jo_fixed_div(dir.x, length);
        dir.y = jo_fixed_div(dir.y, length);
        dir.z = jo_fixed_div(dir.z, length);
        glass = JO_NULL;
        if (doors_glass_ray_hit(&muzzle, &dir, length, &normal, &glass) >= 0 && glass != JO_NULL)
        {
            doors_glass_shot(glass, &dir);
            return;
        }
    }
    // He hits more often near, less when the agent walks or crouches
    chance = 70 - distance / 14 - jo_fixed2int(player_walk_amount() * 20) - (crouched ? 15 : 0) + (g->crouched ? GUARD_KNEEL_BONUS : 0) -
             (g->winded > 0 ? GUARD_WINDED_PENALTY : 0);
    if (chance < 8)
        chance = 8;
    if ((jo_random(100) - 1) < chance)
    {
        hurt_player(GUARD_DAMAGE + (jo_random(4) - 1));
        return;
    }
    // Missed: the bullet goes next to the agent (a mark on the wall behind him; a ray through the cells, 1.5 ms: not for a
    // guard in a room the agent doesn't see, nor when the frames are slow (a general alert: 16 guards shooting, 2 ms of
    // rays per frame for their marks))
    if (persons_load < PERSONS_LOAD_MARKS || jo_cells_depth_bias_at(e->position.x, e->position.y - JO_FIXED_1, e->position.z) < 0)
        return;
    player_eye(&target);
    target.x += jo_int2fixed(jo_random(2) == 2 ? 16 + (jo_random(30) - 1) : -16 - (jo_random(30) - 1));
    target.y += jo_int2fixed((jo_random(60) - 1) - 20);
    target.z += jo_int2fixed((jo_random(50) - 1) - 25);
    dir.x = target.x - muzzle.x;
    dir.y = target.y - muzzle.y;
    dir.z = target.z - muzzle.z;
    length = jo_int2fixed((int)jo_sqrt((unsigned int)(jo_fixed2int(dir.x) * jo_fixed2int(dir.x) + jo_fixed2int(dir.y) * jo_fixed2int(dir.y) +
                                                        jo_fixed2int(dir.z) * jo_fixed2int(dir.z))));
    if (length < JO_FIXED_1)
        return;
    dir.x = jo_fixed_div(dir.x, length);
    dir.y = jo_fixed_div(dir.y, length);
    dir.z = jo_fixed_div(dir.z, length);
    shoot_ray(&muzzle, &dir, 0, e);
}

/** @brief After a bone of a guard is drawn: the end of the barrel of his rifle in the world (the matrix of the bone: the view
 *  space, back in the world with the camera, see camera_view_to_world()) */
static void                         guard_bone(jo_entity * const e, const int bone, const jo_fixed (* const matrix)[3])
{
    guard                           *g;
    jo_pos3D_fixed                  view;

    g = (guard *)e->data;
    if (g == JO_NULL)
        return;
    if (bone == cap_bone)
    {
        jo_3d_transform_point_by(matrix, &cap_middle, &view);
        camera_view_to_world(view.x, view.y, view.z, &g->cap);
        g->cap_frame = frame_counter;
        return;
    }
    if (bone != rifle_bone)
        return;
    jo_3d_transform_point_by(matrix, &rifle_muzzle, &view);
    camera_view_to_world(view.x, view.y, view.z, &g->muzzle);
    g->muzzle_frame = frame_counter;
}

/** @brief The middle of the cap of the model of the guards (the middle of the box around the points of its bone) */
static void                         find_cap_middle(const jo_3d_model * const model)
{
    const jo_3d_bone                *b;
    const jo_3d_model_group         *group;
    const jo_pos3D_fixed            *p;
    jo_pos3D_fixed                  low;
    jo_pos3D_fixed                  high;
    int                             k;

    if (model == JO_NULL || cap_bone < 0 || (b = &model->bones[cap_bone])->group == JO_3D_BONE_NO_GROUP)
        return;
    group = &model->groups[b->group];
    p = model->mesh->data.points + group->first_point;
    low = high = *p;
    for (k = 1, ++p; k < group->point_count; ++k, ++p)
    {
        low.x = JO_MIN(low.x, p->x);
        low.y = JO_MIN(low.y, p->y);
        low.z = JO_MIN(low.z, p->z);
        high.x = JO_MAX(high.x, p->x);
        high.y = JO_MAX(high.y, p->y);
        high.z = JO_MAX(high.z, p->z);
    }
    cap_middle.x = low.x + (high.x - low.x) / 2;
    cap_middle.y = low.y + (high.y - low.y) / 2;
    cap_middle.z = low.z + (high.z - low.z) / 2;
}

/** @brief The bone of the rifle of the model of the guards and the end of its barrel (once) */
static void                         find_rifle_muzzle(const jo_3d_model * const model)
{
    const jo_3d_bone                *b;
    const jo_3d_model_group         *group;
    const jo_pos3D_fixed            *p;
    int                             best;
    int                             d;
    int                             k;

    if (rifle_bone >= 0 || model == JO_NULL || (rifle_bone = jo_3d_model_find_bone(model, "rifle")) < 0)
        return;
    b = &model->bones[rifle_bone];
    if (b->group == JO_3D_BONE_NO_GROUP)
    {
        rifle_bone = -1;
        return;
    }
    group = &model->groups[b->group];
    best = -1;
    for (k = 0, p = model->mesh->data.points + group->first_point; k < group->point_count; ++k, ++p)
    {
        // (whole units: no overflow)
        d = JO_SQUARE(jo_fixed2int(p->x - b->pivot.x)) + JO_SQUARE(jo_fixed2int(p->y - b->pivot.y)) + JO_SQUARE(jo_fixed2int(p->z - b->pivot.z));
        if (d > best)
        {
            best = d;
            rifle_muzzle = *p;
        }
    }
}

static void                         person_spawn(jo_entity * const e, const bool scientist)
{
    guard                           *g;

    // (a new level: the ways between its rooms are computed again, see way_to())
    way_node_count = -1;
#ifdef DEMO_NO_GUARDS
    // (tests: a level without guards, to look at the rooms in peace)
    if (!scientist)
    {
        e->flags |= JO_ENTITY_FLAG_DISABLED | JO_ENTITY_FLAG_HIDDEN;
        e->flags &= ~JO_ENTITY_FLAG_SOLID;
        return;
    }
#endif
    jo_actor_spawn(e);
    g = (guard *)e->data;
    if (g == JO_NULL)
        return;
    g->scientist = scientist;
    if (!scientist)
        find_rifle_muzzle(g->actor.model);
    g->actor.bone_callback = guard_bone;
    g->muzzle_frame = -1;
    g->cap_frame = -1;
    g->state = scientist ? SCIENTIST_WALK : GUARD_IDLE;
    g->hatless = scientist;
    g->fear = 0;
    g->stuck = 0;
    g->near_count = 0;
    g->near_age = NEAR_GATHER_TIME + jo_random(NEAR_GATHER_TIME);
    g->met = false;
    g->hidden_time = 0;
    g->detour_time = 0;
    g->closing = 0;
    g->winded = 0;
    g->step = 0;
    g->last_health = e->health;
    g->home_x = e->position.x;
    g->home_z = e->position.z;
    g->home_yaw = e->yaw;
    g->patrol_x = e->position.x + jo_int2fixed(jo_entity_get_int(e, "patrol_x", 0));
    g->patrol_z = e->position.z + jo_int2fixed(jo_entity_get_int(e, "patrol_z", 0));
    g->outward = true;
    g->sight_timer = (int)(e->position.x >> 16) & 127;
    g->face = jo_entity_get_int(e, "face", 0);
    if (g->face < 0 || g->face >= GUARD_FACE_COUNT)
        g->face = 0;
    g->far_distance = e->far_distance;
    // (his way computed at his first move, see way_to())
    g->way_timer = 0;
    g->way_valid = false;
}

static void                         guard_spawn(jo_entity * const e)
{
    person_spawn(e, false);
}

static void                         scientist_spawn(jo_entity * const e)
{
    person_spawn(e, true);
}

/** @brief A cry of a guard (hit, dying), from where he is: it replaces the cry before (one voice channel) */
static void                         guard_cry(const jo_entity * const e, jo_sound * const sound)
{
    jo_sound                        cry;

    // (a copy played at another rate: the same samples, another voice)
    cry = *sound;
    cry.sample_rate = (unsigned short)(sound->sample_rate * (100 - GUARD_CRY_PITCH_RANGE + jo_random(2 * GUARD_CRY_PITCH_RANGE + 1) - 1) / 100);
    sound_from(&cry, e->position.x, e->position.z, GUARD_VOICE_VOLUME, GUARD_SOUND_RANGE);
    jo_audio_stop_channel(GUARD_VOICE_CHANNEL);
    jo_audio_play_sound_on_channel(&cry, GUARD_VOICE_CHANNEL);
    // (his cry: the scientists around run away)
    scare_scientists(e, e->position.x, e->position.z);
}

/** @brief The fall of a dead guard: shot in the head he slips and falls on his back; in the body, his arms open wide and he
 *  sinks, or he turns on himself and goes down (see tools/make_guard.py) */
static const char                   *death_animation(const bool head)
{
    // (shot in the head: he goes down on his back; elsewhere: on his belly, or turned around by the shot)
    if (head)
        return ("death_back");
    switch (jo_random(3))
    {
        case 1:
            return ("death_front");
        case 2:
            return ("death_twist");
        default:
            return ("death_back");
    }
}

/** @brief He sees or hears the agent: surprised first, then his rifle up */
static void                         alert(jo_entity * const e, guard * const g)
{
    g->seen_x = player_x;
    g->seen_z = player_z;
    g->unseen = 0;
    if (g->state == GUARD_IDLE || g->state == GUARD_SEARCH)
    {
        g->state = GUARD_SURPRISED;
        g->timer = GUARD_SURPRISE_TIME;
        // (he flinches: the rifle comes up at once, the move of the aim)
        jo_actor_restart(e, "aim", 80);
    }
}

/** @brief A scientist is scared (a shot, a cry, hit): he runs away from the agent, see scientist_think() */
static void                         scare(jo_entity * const e, guard * const g, const bool hurt)
{
    // (never threatened by the agent (his hands never went up) and not hurt: the shots don't scare him, he stays at his
    // work (scared by the shots heard through the walls, he was gone before the agent ever saw him))
    if (g->state == GUARD_DEAD || (!g->met && !hurt))
        return;
    g->met = true;
    g->fear = SCIENTIST_FEAR_TIME;
    if (g->state != SCIENTIST_FLEE)
    {
        g->state = SCIENTIST_FLEE;
        // (a new place to run to, chosen in scientist_think())
        g->flee_x = e->position.x;
        g->flee_z = e->position.z;
        g->timer = 0;
    }
}

/** @brief The agent aims at him, even roughly: his weapon toward him (25 degrees), or he is near (300 units) and the agent faces
 *  him (45 degrees) */
static bool                         agent_aims_at(const jo_entity * const e)
{
    jo_pos3D_fixed                  dir;
    int                             dx;
    int                             dz;
    int                             distance;
    jo_fixed                        dot;

    dx = jo_fixed2int(e->position.x - player_x);
    dz = jo_fixed2int(e->position.z - player_z);
    distance = (int)jo_sqrt((unsigned int)(dx * dx + dz * dz));
    if (distance < 1)
        return (true);
    player_aim_direction(&dir);
    // (the cosine of the angle between the aim and the direction to him, in the horizontal plane)
    dot = (dir.x * dx + dir.z * dz) / distance;
    // (roughly at him is enough: 45 degrees near, 25 farther)
    return (dot > (distance < 300 ? JO_FIXED_1 * 45 / 64 : JO_FIXED_1 * 29 / 32));
}

/** @brief The straight line from him to a point can be walked (the first units of it, every 30: a bench is 60 deep, sampled
 *  every 60 it was skipped): the floor (the cells) and the furniture near him (near_gather() first) */
static bool                         furniture_clear(const jo_entity * const e, const jo_fixed x, const jo_fixed z, const int first)
{
    int                             dx;
    int                             dz;
    int                             length;
    int                             k;
    int                             n;
    jo_fixed                        px;
    jo_fixed                        pz;

    dx = jo_fixed2int(x - e->position.x);
    dz = jo_fixed2int(z - e->position.z);
    length = (int)jo_sqrt((unsigned int)(dx * dx + dz * dz));
    n = JO_MIN(first, length);
    for (k = 30; k <= n; k += 30)
    {
        px = e->position.x + jo_int2fixed(dx * k / length);
        pz = e->position.z + jo_int2fixed(dz * k / length);
        if (!jo_cells_is_walkable(px, e->position.y, pz, GUARD_RADIUS, GUARD_MAX_STEP) || near_blocks((const guard *)e->data, px, pz))
            return (false);
    }
    return (true);
}

/** @brief The next place a scared scientist runs to: a doorway or a way node near him, the farthest from the agent (away
 *  from him), the whole way to it free of furniture, in flee_x flee_z; none: a few steps away from the agent
 *  @return false when no way is free around him (he cowers where he is) */
static bool                         flee_point(const jo_entity * const e, guard * const g)
{
    jo_pos3D_fixed                  points[GUARD_WAY_NODES + 8];
    int                             scores[GUARD_WAY_NODES + 8];
    int                             count;
    int                             k;
    int                             best;
    int                             score;
    int                             dx;
    int                             dz;
    int                             from_agent;
    int                             from_him;
    int                             tried;

    if (way_node_count < 0)
        way_build();
    for (k = 0; k < way_node_count; ++k)
        points[k] = way_nodes[k];
    count = way_node_count + doors_doorways(&points[way_node_count], 8, true);
    near_update(e, g);
    // The score of each place (cheap): far from the agent, not too far from him (the nearest of the far places), a
    // little at random (blocked by the furniture on his way to one, he tries another). Then the way to the best ones is
    // tested, in their order, the first one free wins: testing the way to every place made the frame lag (33 tests of the
    // furniture per place, 20 places)
    for (k = 0; k < count; ++k)
    {
        scores[k] = -1;
        dx = jo_fixed2int(points[k].x - e->position.x);
        dz = jo_fixed2int(points[k].z - e->position.z);
        from_him = JO_MAX(JO_ABS(dx), JO_ABS(dz)) + JO_MIN(JO_ABS(dx), JO_ABS(dz)) / 2;
        if (from_him < 80 || from_him > SCIENTIST_FLEE_RADIUS)
            continue;
        dx = jo_fixed2int(points[k].x - player_x);
        dz = jo_fixed2int(points[k].z - player_z);
        from_agent = JO_MAX(JO_ABS(dx), JO_ABS(dz)) + JO_MIN(JO_ABS(dx), JO_ABS(dz)) / 2;
        // (not right at the agent; a place nearer to him than the scientist is stays possible: cornered at the end of a
        // laboratory, the door being on the agent's side, he runs past him rather than staying in his corner)
        if (from_agent < 200)
            continue;
        scores[k] = JO_MAX(1, from_agent * 2 - from_him + jo_random(240) + 1000);
    }
    for (tried = 0; tried < 4; ++tried)
    {
        best = -1;
        score = 0;
        for (k = 0; k < count; ++k)
            if (scores[k] > score)
            {
                best = k;
                score = scores[k];
            }
        if (best < 0)
            break;
        scores[best] = -1;
        // (the first steps free: the way after goes through the doorways (way_to()), around the furniture (go_to_way()))
        if (furniture_clear(e, points[best].x, points[best].z, SCIENTIST_FLEE_FIRST))
        {
            g->flee_x = points[best].x;
            g->flee_z = points[best].z;
            return (true);
        }
    }
    // (no place: a few steps away from the agent, the nearest direction around that is free of furniture (45 degrees by 45))
    {
        int                         away;
        int                         turn;

        // (the yaw of a direction: x = sin, z = cos; already running away: he keeps his direction first (between 2 benches
        // he ran back and forth, from the direction away from the agent to its neighbours and back))
        away = jo_atan2_degrees(e->position.x - player_x, e->position.z - player_z);
        if (JO_ABS(((e->yaw - away + 540) % 360) - 180) < 100)
            away = e->yaw;
        // (the 7th direction, straight at the agent, is the last resort: cornered between the benches, the only way out
        // is past him)
        static const int            turns[8] = {0, 45, -45, 90, -90, 135, -135, 180};

        for (k = 0; k < 8; ++k)
        {
            turn = turns[k];
            g->flee_x = e->position.x + jo_sin((away + turn + 720) % 360) * 200;
            g->flee_z = e->position.z + jo_cos((away + turn + 720) % 360) * 200;
            if (furniture_clear(e, g->flee_x, g->flee_z, 200))
                return (true);
        }
    }
    return (false);
}

/** @brief A scientist: he walks between his place and the end of his patrol (a pause at each end); the agent aims at him
 *  from near: he stops, his hands up, facing him, until the agent looks elsewhere for a while; scared (see scare()),
 *  he runs away from the agent, from place to place, until his fear is gone, then he walks again */
static void                         scientist_think(jo_entity * const e, guard * const g, const int elapsed)
{
    bool                            threatened;

    // (the agent aims at him: he saw him, or he is near (he hears him, he turns around))
    threatened = player_health > 0 && g->sees && distance_to_agent(e) < SCIENTIST_THREAT_DISTANCE && agent_aims_at(e);
    switch (g->state)
    {
        case SCIENTIST_WALK:
            if (threatened)
            {
                // (threatened once: from then on the shots scare him, see scare())
                g->met = true;
                g->state = SCIENTIST_HANDS_UP;
                g->timer = 0;
                g->gait_speed = 0;
                jo_actor_restart(e, "hands_up", 120);
                break;
            }
            // (in a room not seen: he waits there)
            if (jo_cells_depth_bias_at(e->position.x, e->position.y - JO_FIXED_1, e->position.z) < 0)
                break;
            if (g->fear > 0)
            {
                // (the pause at an end of his walk: he stands, he looks around)
                g->fear -= elapsed;
                jo_actor_play(e, "idle", 300);
                g->gait_speed = 0;
                break;
            }
            if (g->patrol_x == g->home_x && g->patrol_z == g->home_z)
            {
                if (go_to(e, g->home_x, g->home_z, false, elapsed))
                {
                    jo_actor_play(e, "idle", 300);
                    if (e->yaw != g->home_yaw)
                        jo_entity_turn_to(e, e->position.x + jo_sin(g->home_yaw) * 100, e->position.z + jo_cos(g->home_yaw) * 100,
                                          GUARD_TURN_SPEED * elapsed / 1000);
                }
            }
            else if (go_to(e, g->outward ? g->patrol_x : g->home_x, g->outward ? g->patrol_z : g->home_z, false, elapsed))
            {
                g->outward = !g->outward;
                g->fear = SCIENTIST_PAUSE_TIME;
            }
            break;
        case SCIENTIST_HANDS_UP:
            jo_entity_turn_to(e, player_x, player_z, GUARD_TURN_SPEED * elapsed / 1000);
            if (jo_actor_is_playing(e, "hands_up") && jo_actor_is_done(e))
                jo_actor_restart(e, "hands_hold", 0);
            if (threatened)
                g->timer = 0;
            else
            {
                g->timer += elapsed;
                if (g->timer > SCIENTIST_CALM_TIME)
                {
                    // (he lowers his hands, he walks on)
                    g->state = SCIENTIST_WALK;
                    g->fear = SCIENTIST_PAUSE_TIME / 2;
                    jo_actor_play(e, "idle", 250);
                }
            }
            break;
        case SCIENTIST_FLEE:
            g->fear -= elapsed;
            if (g->fear <= 0)
            {
                g->state = SCIENTIST_WALK;
                g->fear = SCIENTIST_PAUSE_TIME;
                break;
            }
            // (out of sight in another room: he is gone)
            if (jo_cells_depth_bias_at(e->position.x, e->position.y - JO_FIXED_1, e->position.z) < 0 &&
                distance_to_agent(e) > SCIENTIST_GONE_DISTANCE)
            {
                g->hidden_time += elapsed;
                if (g->hidden_time >= SCIENTIST_GONE_TIME)
                {
                    e->flags |= JO_ENTITY_FLAG_DISABLED | JO_ENTITY_FLAG_HIDDEN;
                    e->flags &= ~JO_ENTITY_FLAG_SOLID;
                    g->state = SCIENTIST_WALK;
                    g->gait_speed = 0;
                    break;
                }
            }
            else
                g->hidden_time = 0;
            // (at the place (or blocked): the next one farther from the agent; stuck on the furniture: another one)
            {
                bool            there;

                there = go_to(e, g->flee_x, g->flee_z, true, elapsed);
                g->stuck = g->moved < 2 ? g->stuck + elapsed : 0;
                // (a new place at most every SCIENTIST_CHOICE_TIME: the choice tests the furniture around, it costs;
                // blocked at a place, he waits there a moment, then chooses)
                g->timer -= elapsed;
                if ((there || g->stuck > 600) && g->timer <= 0)
                {
                    g->stuck = 0;
                    g->timer = SCIENTIST_CHOICE_TIME;
                    // (nowhere to run: he stops, his hands up, until he is calm)
                    if (!flee_point(e, g))
                    {
                        g->state = SCIENTIST_HANDS_UP;
                        g->timer = 0;
                        g->gait_speed = 0;
                        jo_actor_restart(e, "hands_up", 120);
                    }
                }
            }
            break;
        default:
            break;
    }
}

static void                         drop_rifle(const jo_entity * const e)
{
    int                             k;

    for (k = 0; k < DROP_MAX; ++k)
    {
        if (drops[k].used)
            continue;
        drops[k].used = true;
        drops[k].position.x = e->position.x + jo_sin(e->yaw) * 26 + jo_cos(e->yaw) * 10;
        drops[k].position.y = e->position.y;
        drops[k].position.z = e->position.z + jo_cos(e->yaw) * 26 - jo_sin(e->yaw) * 10;
        drops[k].yaw = (e->yaw + 60 + (jo_random(60) - 1)) % 360;
        return;
    }
}

static const jo_entity_class        guard_class;
static const jo_entity_class        scientist_class;

/** @brief A guard or a scientist (the entities with the data of a guard) */
static  __jo_force_inline bool      is_person(const jo_entity * const e)
{
    return (e->cls == &guard_class || e->cls == &scientist_class);
}

/** @brief Keeps at most GUARD_MAX_BODIES - 1 bodies on the floor (before a new one): the oldest ones go at once */
static void                         remove_oldest_bodies(void)
{
    jo_entity                       *e;
    jo_entity                       *oldest;
    guard                           *g;
    int                             bodies;
    int                             k;

    for (;;)
    {
        bodies = 0;
        oldest = JO_NULL;
        for (k = 0; k < jo_entity_count(); ++k)
        {
            e = jo_entity_get(k);
            if (!is_person(e) || e->data == JO_NULL || (e->flags & JO_ENTITY_FLAG_HIDDEN))
                continue;
            g = (guard *)e->data;
            if (g->state != GUARD_DEAD)
                continue;
            ++bodies;
            if (oldest == JO_NULL || g->dead > ((guard *)oldest->data)->dead)
                oldest = e;
        }
        if (bodies < GUARD_MAX_BODIES)
            return;
        oldest->flags |= JO_ENTITY_FLAG_HIDDEN;
    }
}

static void                         guard_death(jo_entity * const e, jo_entity * const source)
{
    guard                           *g;

    JO_UNUSED_ARG(source);
    remove_oldest_bodies();
    g = (guard *)e->data;
    g->state = GUARD_DEAD;
    g->dead = 0;
    // (his cap shot off before: it goes with him)
    {
        int                         k;

        for (k = 0; k < CAP_MAX; ++k)
            if (caps[k].used && caps[k].owner == e)
                caps[k].used = false;
    }
    // (killed by an explosion: he falls; shot, guards_shot() chooses the fall after this)
    jo_actor_restart(e, death_animation(false), g->crouched ? GUARD_KNEEL_DEATH_BLEND : 60);
    if (sounds_loaded)
    {
        // (his last cry: one of the 2 longest cries of pain, lower)
        jo_sound                    last;

        last = pain_sounds[jo_random(2) - 1];
        last.sample_rate = (unsigned short)(last.sample_rate * 90 / 100);
        guard_cry(e, &last);
    }
    // The bullets go through the body, the agent walks over it
    e->flags &= ~JO_ENTITY_FLAG_SOLID;
    // He drops his rifle (in front of him, on the floor): the model without it, the same skeleton
    if (g->scientist)
        return;
    drop_rifle(e);
    if (unarmed_model != JO_NULL)
        g->actor.model = unarmed_model;
    if (unarmed_far_model != JO_NULL && g->actor.far_model != JO_NULL)
        g->actor.far_model = unarmed_far_model;
}

/** @brief He gets up from one knee (an animation: he pushes on his legs) */
static void                         stand_up(jo_entity * const e, guard * const g)
{
    g->crouched = false;
    g->stand = GUARD_STANCE_TIME;
    jo_actor_restart(e, "stand", 60);
}

/** @brief The number of entries of a table */
#define GUARD_COUNT_OF(T)           ((int)(sizeof(T) / sizeof((T)[0])))

/** @brief The animations that end with the rifle up: standing (aim, its shot, getting up), on one knee (kneeling, its
 *  shot, a hit); a held pose follows them (aim_hold, kneel) */
static const char * const           rifle_up_standing[] = {"aim", "fire", "stand"};
static const char * const           rifle_up_kneeling[] = {"crouch", "crouch_fire", "hit_kneel"};

/** @brief His rifle up in his stance: the move up (aim, crouch) from another pose, then the pose held. A shot, a hit,
 *  getting up end in the pose: it is held after them (playing the move again went back to its first key, the rifle
 *  down or the guard standing, then up again: a jump between 2 poses) */
static void                         hold_rifle(jo_entity * const e, const guard * const g, const int blend)
{
    const char * const              *ends;
    int                             count;
    int                             k;

    ends = g->crouched ? rifle_up_kneeling : rifle_up_standing;
    count = g->crouched ? GUARD_COUNT_OF(rifle_up_kneeling) : GUARD_COUNT_OF(rifle_up_standing);
    if (jo_actor_is_playing(e, g->crouched ? "kneel" : "aim_hold"))
        return;
    for (k = 0; k < count; ++k)
        if (jo_actor_is_playing(e, ends[k]))
        {
            if (jo_actor_is_done(e))
                jo_actor_restart(e, g->crouched ? "kneel" : "aim_hold", 0);
            return;
        }
    jo_actor_restart(e, ends[0], blend);
}

/** @brief The next place where a guard searches the agent: a side of a doorway or the front of a stall near him (a random one,
 *  not where he is), seen_x seen_z (unchanged if none)
 */
static void                         next_search_point(const jo_entity * const e, guard * const g)
{
    jo_pos3D_fixed                  points[GUARD_WAY_NODES + 8];
    int                             near[GUARD_WAY_NODES + 8];
    int                             count;
    int                             n;
    int                             k;
    int                             length;

    if (way_node_count < 0)
        way_build();
    for (k = 0; k < way_node_count; ++k)
        points[k] = way_nodes[k];
    count = way_node_count + doors_doorways(&points[way_node_count], 8, true);
    for (k = 0, n = 0; k < count; ++k)
    {
        length = way_length(e->position.x, e->position.z, points[k].x, points[k].z);
        if (length >= 60 && length <= GUARD_SEARCH_RADIUS)
            near[n++] = k;
    }
    if (n == 0)
        return;
    k = near[jo_random(n) - 1];
    g->seen_x = points[k].x;
    g->seen_z = points[k].z;
}

static void                         guard_update_inner(jo_entity * const e, const int elapsed);

static void                         guard_update(jo_entity * const e, const int elapsed)
{
    guard_update_inner(e, elapsed);
}

static void                         guard_update_inner(jo_entity * const e, const int elapsed)
{
    guard                           *g;

    g = (guard *)e->data;
    if (g != JO_NULL)
        g->near_age += elapsed;
    if (g == JO_NULL)
    {
        jo_actor_update(e, elapsed);
        return;
    }
    // His steps: the time of the animation with the distance his feet walked (see go_to())
    if (g->gait_speed > 0 && g->state != GUARD_DEAD && (jo_actor_is_playing(e, "walk") || jo_actor_is_playing(e, "run")))
        jo_actor_update(e, JO_MIN(jo_int_div(g->moved * 1000, g->gait_speed * 256), elapsed * 2));
    else
        jo_actor_update(e, elapsed);
    g->moved = 0;
    if (g->hurt > 0)
        g->hurt -= elapsed;
    if (g->stand > 0)
        g->stand -= elapsed;
    if (g->winded > 0)
        g->winded -= elapsed;
    // Hurt by something else than a shot (an explosion near him): he jerks, he knows (a shot did it in guards_shot())
    if (g->state != GUARD_DEAD && e->health < g->last_health && g->hurt <= 0)
    {
        g->hurt = GUARD_HURT_TIME;
        g->hit = g->crouched ? GUARD_HIT_KNEEL_TIME : GUARD_HIT_TIME;
        if (g->scientist)
            scare(e, g, true);
        else
            alert(e, g);
        jo_actor_restart(e, g->crouched ? "hit_kneel" : "hit", 40);
    }
    g->last_health = e->health;
    if (g->state == GUARD_DEAD)
    {
        // The body: it stays a while, sinks in the floor, goes
        if (e->flags & JO_ENTITY_FLAG_HIDDEN)
            return;
        g->dead += elapsed;
        if (g->dead > GUARD_BODY_TIME - GUARD_BODY_SINK)
            e->position.y += jo_int2fixed(GUARD_BODY_DEPTH) * elapsed / GUARD_BODY_SINK;
        if (g->dead >= GUARD_BODY_TIME)
            e->flags |= JO_ENTITY_FLAG_HIDDEN;
        return;
    }
    // He looks at the agent (a ray: not every frame)
    g->sight_timer -= elapsed;
    if (g->sight_timer <= 0)
    {
        g->sight_timer = g->state == GUARD_IDLE ? GUARD_IDLE_SIGHT_PERIOD : GUARD_SIGHT_PERIOD;
        // (a scientist notices the agent near him whatever his facing: he hears him, he turns around)
        g->sees = g->scientist ? sees_agent_within(e, SCIENTIST_THREAT_DISTANCE * 2 / 3) : sees_agent(e);
    }
    if (g->sees)
    {
        g->seen_x = player_x;
        g->seen_z = player_z;
        g->unseen = 0;
    }
    else
        g->unseen += elapsed;
    if (g->hit > 0)
    {
        g->hit -= elapsed;
        return;
    }
    if (g->scientist)
    {
        scientist_think(e, g, elapsed);
        return;
    }
    switch (g->state)
    {
        case GUARD_IDLE:
            if (g->sees)
            {
                alert(e, g);
                break;
            }
            // (in a room not seen: he waits there, his patrol paused; the moves test the walls, the doors and the
            // furniture: the bigger level fell to 22 frames per second)
            if (jo_cells_depth_bias_at(e->position.x, e->position.y - JO_FIXED_1, e->position.z) < 0)
                break;
            if (g->patrol_x == g->home_x && g->patrol_z == g->home_z)
            {
                // (no patrol: he waits at his place, turned as in the level)
                if (go_to(e, g->home_x, g->home_z, false, elapsed))
                {
                    jo_actor_play(e, "idle", 300);
                    if (e->yaw != g->home_yaw)
                        jo_entity_turn_to(e, e->position.x + jo_sin(g->home_yaw) * 100, e->position.z + jo_cos(g->home_yaw) * 100,
                                          GUARD_TURN_SPEED * elapsed / 1000);
                }
            }
            else if (go_to(e, g->outward ? g->patrol_x : g->home_x, g->outward ? g->patrol_z : g->home_z, false, elapsed))
                g->outward = !g->outward;
            break;
        case GUARD_SURPRISED:
            jo_entity_turn_to(e, g->seen_x, g->seen_z, GUARD_TURN_SPEED * elapsed / 1000);
            g->timer -= elapsed;
            if (g->timer <= 0)
            {
                g->state = GUARD_ALERT;
                g->timer = GUARD_REACTION_TIME;
                // (on one knee to shoot from far, sometimes near)
                g->crouched = distance_to_agent(e) > GUARD_KNEEL_DISTANCE ? jo_random(3) != 1 : jo_random(4) == 1;
            }
            break;
        case GUARD_ALERT:
            // He turns toward the agent and raises his rifle
            jo_entity_turn_to(e, g->seen_x, g->seen_z, GUARD_TURN_SPEED * elapsed / 1000);
            hold_rifle(e, g, 150);
            g->timer -= elapsed;
            if (g->timer <= 0)
            {
                g->state = GUARD_ATTACK;
                g->shots = GUARD_BURST;
                g->timer = 0;
            }
            break;
        case GUARD_ATTACK:
            if (g->unseen > GUARD_LOST_TIME)
            {
                g->state = GUARD_SEARCH;
                g->search = 0;
                g->closing = 0;
                if (g->crouched)
                    stand_up(e, g);
                break;
            }
            g->timer -= elapsed;
            // (between 2 bursts: a step aside, the rifle up; blocked, the step ends there)
            if (g->step > 0)
            {
                g->step -= elapsed;
                if (!move_toward(e, (e->yaw + (g->step_side > 0 ? 90 : 270)) % 360, GUARD_STEP_SPEED, elapsed))
                    g->step = 0;
                if (g->step > 0)
                {
                    jo_entity_turn_to(e, g->seen_x, g->seen_z, GUARD_TURN_SPEED * elapsed / 1000);
                    break;
                }
            }
            // (between 2 bursts, far from the agent: he runs at him, then his rifle up again)
            if (g->closing > 0)
            {
                g->closing -= elapsed;
                if (g->closing > 0 && g->timer > 0 && distance_to_agent(e) > GUARD_KEEP_DISTANCE &&
                    !go_to(e, g->seen_x, g->seen_z, true, elapsed))
                    break;
                g->closing = 0;
                g->gait_speed = 0;
                g->winded = GUARD_WINDED_TIME / 2;
                g->timer = JO_MAX(g->timer, GUARD_REACTION_TIME / 2);
            }
            jo_entity_turn_to(e, g->seen_x, g->seen_z, GUARD_TURN_SPEED * elapsed / 1000);
            hold_rifle(e, g, 120);
            if (g->timer > 0)
                break;
            if (!g->sees)
            {
                g->timer = GUARD_SHOT_TIME;
                break;
            }
            jo_actor_restart(e, g->crouched ? "crouch_fire" : "fire", 30);
            shoot_agent(e, g);
            if (--g->shots > 0)
                g->timer = GUARD_SHOT_TIME;
            else
            {
                g->shots = GUARD_BURST - 1 + (jo_random(3) - 1);
                g->timer = GUARD_BURST_PAUSE + (jo_random(700) - 1);
                // (far from the agent, standing: he closes in between 2 bursts, mostly)
                if (!g->crouched && g->sees && distance_to_agent(e) > GUARD_CLOSE_DISTANCE && jo_random(3) != 1)
                {
                    g->closing = GUARD_CLOSE_TIME;
                    g->timer = JO_MAX(g->timer, GUARD_CLOSE_TIME + GUARD_REACTION_TIME / 2);
                }
                // (standing, a step aside between 2 bursts: harder to hit)
                else if (!g->crouched && jo_random(3) == 1)
                {
                    g->step = GUARD_STEP_TIME;
                    g->step_side = jo_random(2) == 2 ? 1 : -1;
                    jo_actor_restart(e, g->step_side > 0 ? "step_l" : "step_r", 60);
                    g->timer = JO_MAX(g->timer, GUARD_STEP_TIME + 100);
                }
                // (he changes his stance between 2 bursts: he kneels or gets up, then shoots again)
                else if (jo_random(4) == 1)
                {
                    if (g->crouched)
                        stand_up(e, g);
                    else
                    {
                        g->crouched = true;
                        jo_actor_restart(e, "crouch", 80);
                    }
                    g->timer = JO_MAX(g->timer, GUARD_STANCE_TIME);
                }
            }
            break;
        case GUARD_SEARCH:
            // (getting up: he looks for the agent when he is up)
            if (g->stand > 0)
                break;
            if (g->sees)
            {
                g->state = GUARD_ALERT;
                g->timer = GUARD_REACTION_TIME / 2;
                // (out of breath after his run: he shoots badly a moment)
                if (g->run && g->gait_speed > 0)
                    g->winded = GUARD_WINDED_TIME;
                break;
            }
            g->search += elapsed;
            // (the agent dead: they come and look at him, from a few steps)
            if (player_health <= 0)
            {
                if (distance_to_agent(e) > GUARD_BODY_DISTANCE)
                    go_to(e, player_x, player_z, g->run, elapsed);
                else
                {
                    jo_entity_turn_to(e, player_x, player_z, GUARD_TURN_SPEED * elapsed / 1000);
                    jo_actor_play(e, "idle", 300);
                    g->gait_speed = 0;
                }
                break;
            }
            // (he goes where he saw the agent for the last time, around the furniture and through the doors, then he searches
            // the doorways and the stalls near there: he doesn't know where the agent is; after GUARD_SEARCH_TIME he gives up)
            // (in the room of the agent and near: he stays at the distance of his shots, he turns toward him)
            {
                int                 distance;
                bool                same_room;

                distance = distance_to_agent(e);
                // (only near: far, he runs whatever the room, 2 searches of the cells per guard and per frame)
                same_room = distance <= JO_MAX(GUARD_KEEP_DISTANCE, GUARD_RUN_DISTANCE) &&
                            jo_cells_find(e->position.x, e->position.y, e->position.z, PLAYER_STEP) ==
                            jo_cells_find(player_x, player_y, player_z, PLAYER_STEP);
                // (he saw him a moment ago: not a hidden agent, in a stall of the room)
                if (same_room && distance < GUARD_KEEP_DISTANCE && g->unseen < GUARD_KEEP_TIME)
                {
                    jo_entity_turn_to(e, player_x, player_z, GUARD_TURN_SPEED * elapsed / 1000);
                    hold_rifle(e, g, 150);
                    g->gait_speed = 0;
                    break;
                }
                // (searching the doorways and the stalls later: he walks)
                g->run = (!same_room || distance > GUARD_RUN_DISTANCE) && g->unseen < 2 * GUARD_KEEP_TIME;
            }
            // (at the point: nobody, the next place near there. Blocked on his way (go_to() is true too: a guard in the doorway in
            // front of him), he goes on until the end of the search: he gave up in the doorway)
            if (g->search > GUARD_SEARCH_TIME)
            {
                // Nobody: back to his place
                g->state = GUARD_IDLE;
                g->outward = false;
            }
            else if (go_to(e, g->seen_x, g->seen_z, g->run, elapsed) && way_length(e->position.x, e->position.z, g->seen_x, g->seen_z) < 60)
                next_search_point(e, g);
            break;
        default:
            break;
    }
}

/** @brief The faces: the sprites of their textures (-1: not loaded), the polygons of the models of the guards that have
 *  them (the models are shared by the guards: the textures of his face are set on them before each guard is drawn) */
#define FACE_MAX_MODELS             (6)
#define FACE_MAX_POLYGONS           (40)
typedef struct
{
    const jo_3d_model               *model;
    int                             count;
    int                             face;           // The face set last on the model (-1: none yet)
    unsigned short                  polygons[FACE_MAX_POLYGONS];
    unsigned char                   tiles[FACE_MAX_POLYGONS];
}                                   face_polygons;
static int                          face_sprites[GUARD_FACE_COUNT][GUARD_FACE_TILES];
static bool                         faces_loaded = false;
static face_polygons                face_models[FACE_MAX_MODELS];
static int                          face_model_count = 0;
/** @brief The VDP1 commands of a guard at most (his model near, his far model), the commands kept for what is drawn after
 *  the entities (the billboards, the impacts, the blood, the weapon, the HUD): with too few commands left this frame (a
 *  big room, the guards near), a guard is drawn with his far model, or not at all (the table of the VDP1 full: an error) */
#define GUARD_NEAR_COMMANDS         (160)
#define GUARD_FAR_COMMANDS          (60)
#define GUARD_RESERVE_COMMANDS      (90)

/** @brief The load of the last frame (its fields), once per frame (see persons_load) */
static void                         persons_load_update(void)
{
    static unsigned int             last_vblank = 0;
    unsigned int                    vblank;
    int                             fields;

    vblank = guard_vblank_count();
    fields = (int)(vblank - last_vblank);
    last_vblank = vblank;
    if (fields >= 3 && fields < 30)
        persons_load = JO_MAX(PERSONS_LOAD_MIN, persons_load - PERSONS_LOAD_DROP);
    else
        persons_load = JO_MIN(256, persons_load + PERSONS_LOAD_RISE);
}

/** @brief The polygons of a model that have the textures of the first face (found the first time) */
static face_polygons                *find_face_polygons(const jo_3d_model * const model)
{
    face_polygons                   *f;
    unsigned int                    k;
    int                             t;
    int                             id;

    if (model == JO_NULL || model->mesh == JO_NULL)
        return (JO_NULL);
    for (t = 0; t < face_model_count; ++t)
        if (face_models[t].model == model)
            return (&face_models[t]);
    if (face_model_count >= FACE_MAX_MODELS)
        return (JO_NULL);
    f = &face_models[face_model_count++];
    f->model = model;
    f->count = 0;
    f->face = -1;
    for (k = 0; k < model->mesh->data.polygon_count && f->count < FACE_MAX_POLYGONS; ++k)
    {
        id = model->mesh->data.attributes[k].texture_id;
        for (t = 0; t < GUARD_FACE_TILES; ++t)
        {
            if (face_sprites[0][t] >= 0 && id == face_sprites[0][t])
            {
                f->polygons[f->count] = (unsigned short)k;
                f->tiles[f->count++] = (unsigned char)t;
                break;
            }
        }
    }
    return (f);
}

/** @brief Set the textures of a face on a model of the guards
 *  @param force Written even if it is the face set last: on the master CPU (the slave CPU sets the faces of the guards it
 *         draws, the master CPU may hold the old textures in its cache: it writes them all, they go to its cache too)
 */
static void                         set_face(const jo_3d_model * const model, const int face, const bool force)
{
    face_polygons                   *f;
    int                             k;
    int                             id;

    if (!faces_loaded || model == JO_NULL || model->mesh == JO_NULL || (f = find_face_polygons(model)) == JO_NULL)
        return;
    // (the face set last: the same textures, nothing to write. The guards near each other often have the same face)
    if (f->face == face && !force)
        return;
    f->face = face;
    for (k = 0; k < f->count; ++k)
    {
        id = face_sprites[face][f->tiles[k]];
        if (id >= 0)
            model->mesh->data.attributes[f->polygons[k]].texture_id = (unsigned short)id;
    }
}

/** @brief The tint of a guard (the light of the lamps on him, see jo_entities_prepare_draw()): red for a moment after a
 *  hit */
static void                         guard_tint(jo_entity * const e, int * const r, int * const g, int * const b)
{
    const guard                     *gd;

    gd = (const guard *)e->data;
    if (gd != JO_NULL && gd->hurt > 0)
    {
        *r = GUARD_HURT_RED * gd->hurt / GUARD_HURT_TIME;
        *g = 0;
        *b = 0;
    }
}

/** @brief Draw a guard on the master CPU: his face, his cap shot off (its bone hidden) */
static void                         guard_draw(jo_entity * const e)
{
    guard                           *g;
    int                             left;

    g = (guard *)e->data;
    if (g != JO_NULL)
    {
        // (the commands of the slave CPU too: its count now, or in the last frame if it had more, it still draws)
        left = jo_vdp1_command_max() - __jo_vdp1_frame_command_count() - GUARD_RESERVE_COMMANDS -
               JO_MAX(jo_3d_slave_get_current_command_count(), jo_3d_slave_get_command_count());
        if (left < GUARD_FAR_COMMANDS)
            return;
        e->far_distance = left < GUARD_NEAR_COMMANDS ? 0 : g->far_distance;
        set_face(g->actor.model, g->face, true);
        set_face(g->actor.far_model, g->face, true);
    }
    if (g != JO_NULL && g->hatless && cap_bone >= 0)
        jo_3d_model_set_hidden_bones(1u << cap_bone);
    jo_actor_draw(e);
    jo_3d_model_set_hidden_bones(0);
}

/** @brief Draw a guard on the slave CPU (see jo_entities_draw_slave()): like guard_draw(), with the commands of the slave
 *  CPU (nothing written in the entity: the master CPU reads it) */
static void                         guard_draw_slave(jo_entity * const e)
{
    guard                           *g;
    int                             left;
    int                             far_distance;

    g = (guard *)e->data;
    far_distance = e->far_distance;
    if (g != JO_NULL)
    {
        // (the table of the VDP1 is shared by the 2 CPUs: the commands of the slave CPU so far, the polygons of the cells
        // of the master CPU as planned (not its count now: it depends on how far it got, a run would differ from another);
        // the slave CPU drops its last commands when the table is full)
        left = JO_MIN(jo_3d_slave_get_max_commands() - jo_3d_slave_get_current_command_count(),
                      jo_vdp1_command_max() - jo_cells_prepared_polygon_count() * jo_cells_get_share() / 1024 -
                      jo_3d_slave_get_current_command_count()) - GUARD_RESERVE_COMMANDS;
        if (left < GUARD_FAR_COMMANDS)
            return;
        far_distance = left < GUARD_NEAR_COMMANDS ? 0 : g->far_distance;
        set_face(g->actor.model, g->face, false);
        set_face(g->actor.far_model, g->face, false);
    }
    jo_actor_draw_slave(e, far_distance, g != JO_NULL && g->hatless && cap_bone >= 0 ? 1u << cap_bone : 0);
}

static const jo_entity_class        guard_class = {"guard", sizeof(guard), guard_spawn, guard_update, JO_NULL, guard_death, guard_draw,
                                                   guard_draw_slave, guard_tint};
static const jo_entity_class        scientist_class = {"scientist", sizeof(guard), scientist_spawn, guard_update, JO_NULL, guard_death,
                                                       guard_draw, guard_draw_slave, guard_tint};

/** @brief Blood: drops flying out of the hit, falling */
static void                         add_blood(const jo_pos3D_fixed * const p, const jo_pos3D_fixed * const dir)
{
    int                             k;
    blood_drop                      *b;

    for (k = 0; k < 3; ++k)
    {
        b = &blood[next_blood];
        next_blood = (next_blood + 1) % BLOOD_MAX;
        b->position = *p;
        // (along the bullet, a little up and aside)
        b->speed.x = dir->x * (40 + (jo_random(60) - 1)) + jo_int2fixed((jo_random(61) - 1) - 30);
        b->speed.y = dir->y * 40 - jo_int2fixed(30 + (jo_random(40) - 1));
        b->speed.z = dir->z * (40 + (jo_random(60) - 1)) + jo_int2fixed((jo_random(61) - 1) - 30);
        b->time = BLOOD_TIME - (jo_random(120) - 1);
    }
}

/** @brief The cap flies off the head (along the bullet, up), spins, falls on the floor */
static void                         knock_cap(const jo_entity * const e, const jo_pos3D_fixed * const dir, const jo_pos3D_fixed * const from)
{
    falling_cap                     *c;
    int                             floor;

    c = &caps[next_cap];
    next_cap = (next_cap + 1) % CAP_MAX;
    // (from its place on his head: over his feet standing, else where it was drawn, see guard_bone())
    if (from != JO_NULL)
        c->position = *from;
    else
    {
        c->position = e->position;
        c->position.y += cap_middle.y != 0 ? cap_middle.y : -jo_int2fixed(GUARD_CAP_CENTER);
    }
    c->speed.x = dir->x * 140;
    c->speed.y = -jo_int2fixed(160);
    c->speed.z = dir->z * 140;
    c->yaw = e->yaw;
    c->spin = 0;
    c->spin_speed = 500 + (jo_random(300) - 1);
    c->floor = jo_cells_floor(e->position.x, e->position.y, e->position.z, 8, &floor) ? jo_int2fixed(floor) : e->position.y;
    c->model = ((const jo_actor *)e->data)->model;
    c->owner = e;
    c->used = true;
}

jo_fixed                            guards_cap_ray_hit(const jo_pos3D_fixed * const origin, const jo_pos3D_fixed * const dir, const jo_fixed max_t,
                                                       const bool knock)
{
    jo_entity                       *e;
    jo_entity                       *best;
    guard                           *g;
    jo_fixed                        best_t;
    jo_fixed                        t;
    jo_fixed                        ox;
    jo_fixed                        oy;
    jo_fixed                        oz;
    jo_pos3D_fixed                  cap;
    int                             dx;
    int                             dy;
    int                             dz;
    int                             k;

    best = JO_NULL;
    best_t = max_t;
    for (k = 0; (e = jo_entity_get(k)) != JO_NULL; ++k)
    {
        // (a guard in his fall, his cap on his head, drawn in the last frame: its place follows the fall)
        if (e->cls != &guard_class || (g = (guard *)e->data) == JO_NULL || g->state != GUARD_DEAD || g->hatless ||
            read_slave_frame(&g->cap_frame) < frame_counter - 1 || (e->flags & JO_ENTITY_FLAG_HIDDEN))
            continue;
        read_slave_position(&g->cap, &cap);
        ox = cap.x - origin->x;
        oy = cap.y - origin->y;
        oz = cap.z - origin->z;
        if (JO_ABS(ox) > best_t || JO_ABS(oy) > best_t || JO_ABS(oz) > best_t)
            continue;
        // (the point of the shot nearest to the middle of the cap)
        t = jo_fixed_mult(ox, dir->x) + jo_fixed_mult(oy, dir->y) + jo_fixed_mult(oz, dir->z);
        if (t <= 0 || t >= best_t)
            continue;
        dx = jo_fixed2int(ox - jo_fixed_mult(dir->x, t));
        dy = jo_fixed2int(oy - jo_fixed_mult(dir->y, t));
        dz = jo_fixed2int(oz - jo_fixed_mult(dir->z, t));
        if (dx * dx + dy * dy + dz * dz > CAP_HIT_RADIUS * CAP_HIT_RADIUS)
            continue;
        best = e;
        best_t = t;
    }
    if (best == JO_NULL)
        return (-1);
    if (knock)
    {
        g = (guard *)best->data;
        g->hatless = true;
        read_slave_position(&g->cap, &cap);
        knock_cap(best, dir, &cap);
    }
    return (best_t);
}

bool                                guards_shot(jo_entity * const e, const jo_pos3D_fixed * const hit, const jo_pos3D_fixed * const dir,
                                                const int damage, jo_entity * const source)
{
    guard                           *g;
    bool                            head;
    int                             height;
    int                             lateral;
    const char                      *reaction;

    if (!is_person(e))
        return (false);
    g = (guard *)e->data;
    if (g == JO_NULL || g->state == GUARD_DEAD)
        return (true);
    // (the height of the hit over his feet: Y down)
    height = jo_fixed2int(e->position.y - hit->y);
    // The cap: shot off his head (he is not hurt, but he knows); over his head without it: missed
    if (!g->crouched && height >= GUARD_CAP_BOTTOM)
    {
        if (g->hatless || cap_bone < 0)
            return (true);
        g->hatless = true;
        knock_cap(e, dir, JO_NULL);
        if (damage > 0)
            alert(e, g);
        return (true);
    }
    head = height >= GUARD_HEAD_BOTTOM - (g->crouched ? GUARD_KNEEL_DROP : 0);
    add_blood(hit, dir);
    g->hurt = GUARD_HURT_TIME;
    if (damage <= 0)
        return (true);
    if (g->scientist)
        scare(e, g, true);
    else
    {
        alert(e, g);
        // (he knows where the agent is: he fights back at once)
        if (g->state == GUARD_SURPRISED)
        {
            g->state = GUARD_ALERT;
            g->timer = GUARD_REACTION_TIME / 2;
        }
    }
    jo_entity_damage(e, head ? damage * GUARD_HEAD_MULTIPLIER : damage, source);
    if (e->health <= 0 || (e->flags & JO_ENTITY_FLAG_DEAD))
    {
        // (shot in the head or in the back: he falls forward; in the chest: backward. On one knee: the fall starts from
        // there, slower, he does not stand up first)
        jo_actor_restart(e, death_animation(head), g->crouched ? GUARD_KNEEL_DEATH_BLEND : 60);
        return (true);
    }
    // Hit: he cries, his body jerks (the head, a side or the chest)
    if (sounds_loaded)
    {
        guard_cry(e, &pain_sounds[jo_random(PAIN_SOUND_COUNT) - 1]);
    }
    // (where: the head, a leg, an arm (a side of the body: the hit across his facing), the chest)
    lateral = jo_fixed2int(jo_fixed_mult(hit->x - e->position.x, jo_cos(e->yaw)) - jo_fixed_mult(hit->z - e->position.z, jo_sin(e->yaw)));
    // (he does nothing else for most of the reaction (GUARD_HIT_TIME: he staggers, his hand on the wound), then he
    // fights back while he straightens up)
    g->hit = GUARD_HIT_TIME;
    // (on one knee: he stays on it, his body jerks; standing up for the reaction was a jump between 2 poses)
    if (g->crouched)
    {
        reaction = "hit_kneel";
        g->hit = GUARD_HIT_KNEEL_TIME;
    }
    else if (height < GUARD_SIZE(46) - (g->crouched ? GUARD_SIZE(20) : 0))
    {
        // (a leg: it gives way under him, he limps a moment)
        reaction = "hit_leg";
        g->hit = GUARD_HIT_LEG_TIME;
    }
    else if (lateral > 7)
        reaction = "hit_arm_l";
    else if (lateral < -7)
        reaction = "hit_arm_r";
    else
        reaction = "hit";
    jo_actor_restart(e, reaction, 40);
    return (true);
}

/** @brief The entities the auto aim can choose (the guards, the explosive barrels), in their order: the classes of the
 *  entities don't change after the loading of the level (about 300 entities, 30 of them)
 */
#define AUTO_AIM_MAX_TARGETS                (256)
static unsigned short               auto_aim_targets[AUTO_AIM_MAX_TARGETS];
/** @brief Number of targets, -1: more than AUTO_AIM_MAX_TARGETS (all the entities are looked at) */
static int                          auto_aim_target_count;
/** @brief The entities of the list: the first one, their number, the generation of the cells (a level loaded) */
static jo_entity                    *auto_aim_first_entity = JO_NULL;
static int                          auto_aim_entity_count = -1;
static unsigned short               auto_aim_generation;

/** @brief Check if the class of an entity can be a target of the auto aim (see guards_auto_aim()) */
static bool                         is_auto_aim_class(const jo_entity * const e)
{
    if (e->cls == &guard_class)
        return (e->data != JO_NULL);
    return (e->cls != JO_NULL && e->cls->name != JO_NULL && e->cls->name[0] == 'e' && jo_strcmp(e->cls->name, "explosive") == 0);
}

/** @brief The list of the targets of the auto aim, made again after the loading of a level */
static void                         update_auto_aim_targets(void)
{
    jo_entity                       *e;
    int                             k;

    if (jo_entity_get(0) == auto_aim_first_entity && jo_entity_count() == auto_aim_entity_count &&
        jo_cells_generation() == auto_aim_generation)
        return;
    auto_aim_first_entity = jo_entity_get(0);
    auto_aim_entity_count = jo_entity_count();
    auto_aim_generation = jo_cells_generation();
    auto_aim_target_count = 0;
    for (k = 0; (e = jo_entity_get(k)) != JO_NULL; ++k)
        if (is_auto_aim_class(e))
        {
            if (auto_aim_target_count == AUTO_AIM_MAX_TARGETS)
            {
                auto_aim_target_count = -1;
                return;
            }
            auto_aim_targets[auto_aim_target_count++] = (unsigned short)k;
        }
}

bool                                guards_auto_aim(const jo_pos3D_fixed * const eye, jo_pos3D_fixed * const dir)
{
    jo_entity                       *e;
    guard                           *g;
    jo_pos3D_fixed                  chest;
    jo_pos3D_fixed                  best_dir;
    int                             hx;
    int                             hz;
    int                             hlen;
    int                             tx;
    int                             tz;
    int                             ty;
    int                             forward;
    int                             lateral;
    int                             allowed;
    int                             score;
    int                             best_score;
    int                             length;
    int                             ray_y;
    int                             feet;
    int                             k;
    bool                            found;

    // The direction of the shot on the ground (x 1024)
    hx = jo_fixed2int(dir->x * 1024);
    hz = jo_fixed2int(dir->z * 1024);
    hlen = (int)jo_sqrt((unsigned int)(hx * hx + hz * hz));
    if (hlen < 64)
        return (false);
    found = false;
    best_score = 0x7fffffff;
    update_auto_aim_targets();
    // (the list of the targets, else all the entities)
    for (k = 0; (e = jo_entity_get(auto_aim_target_count < 0 ? k : k < auto_aim_target_count ? auto_aim_targets[k] : -1)) != JO_NULL; ++k)
    {
        int                         radius;
        int                         top;
        int                         margin_side;
        int                         margin_height;
        int                         target_height;

        if (e->flags & (JO_ENTITY_FLAG_DEAD | JO_ENTITY_FLAG_HIDDEN))
            continue;
        if (e->cls == &guard_class && e->data != JO_NULL)
        {
            g = (guard *)e->data;
            if (g->state == GUARD_DEAD)
                continue;
            radius = GUARD_RADIUS;
            top = g->crouched ? GUARD_SIZE(80) : GUARD_CAP_TOP;
            margin_side = AUTO_AIM_MARGIN;
            margin_height = AUTO_AIM_MARGIN;
            target_height = g->crouched ? GUARD_CHEST_HEIGHT - GUARD_KNEEL_DROP : GUARD_CHEST_HEIGHT;
        }
        else if (e->cls != JO_NULL && e->cls->name != JO_NULL && e->cls->name[0] == 'e' && (e->flags & JO_ENTITY_FLAG_SOLID) &&
                 e->health > 0 && jo_strcmp(e->cls->name, "explosive") == 0)
        {
            // An explosive barrel: a little over its lid or under it (not on its sides: the barrels near the guards)
            radius = e->radius;
            top = e->height;
            margin_side = 0;
            margin_height = AUTO_AIM_BARREL_MARGIN;
            target_height = e->height / 2;
        }
        else
            continue;
        if (distance_to_agent(e) > AUTO_AIM_DISTANCE)
            continue;
        // (world units: in front of the agent, the distance from the line of the shot on the ground)
        tx = jo_fixed2int(e->position.x - eye->x);
        tz = jo_fixed2int(e->position.z - eye->z);
        // (the divisions by the division unit, see jo_int_div(): about 30 candidates per shot)
        forward = jo_int_div(tx * hx + tz * hz, hlen);
        if (forward < radius)
            continue;
        lateral = jo_int_div(JO_ABS(tx * hz - tz * hx), hlen);
        allowed = radius + margin_side;
        if (lateral > allowed)
            continue;
        // (the height of the shot over him: within the vertical tolerance)
        ray_y = jo_fixed2int(eye->y) + jo_int_div(jo_fixed2int(dir->y * forward) * 1024, hlen);
        feet = jo_fixed2int(e->position.y);
        // (the shot at most the margin over his head or under his feet)
        if (ray_y > feet + margin_height || ray_y < feet - top - margin_height)
            continue;
        chest = e->position;
        chest.y -= jo_int2fixed(target_height);
        if (!is_line_clear(eye, &chest))
            continue;
        // The shot already hits a barrel: it stays where the agent aimed (a guard: his chest, standing or on one knee; the
        // head is aimed in the aiming mode)
        if (e->cls != &guard_class && lateral <= radius && ray_y <= feet && ray_y >= feet - top)
        {
            if (lateral < best_score)
            {
                best_score = lateral;
                best_dir = *dir;
                found = true;
            }
            continue;
        }
        // (the nearest to the line of the shot, relative to the tolerance)
        score = jo_int_div(lateral * 1024, JO_MAX(allowed, 1));
        if (found && score >= best_score)
            continue;
        tx = jo_fixed2int(chest.x - eye->x);
        ty = jo_fixed2int(chest.y - eye->y);
        tz = jo_fixed2int(chest.z - eye->z);
        length = (int)jo_sqrt((unsigned int)(tx * tx + ty * ty + tz * tz));
        if (length < 1)
            continue;
        best_dir.x = (chest.x - eye->x) / length;
        best_dir.y = (chest.y - eye->y) / length;
        best_dir.z = (chest.z - eye->z) / length;
        best_score = score;
        found = true;
    }
    if (found)
        *dir = best_dir;
    return (found);
}

/** @brief A shot passes near a guard: the bullet, from eye along dir (length: where it stopped), at most GUARD_BULLET_NEAR from
 *  him on the floor (his back to the agent, far from him: he hears it whiz) */
static bool                         bullet_passes_near(const jo_entity * const e, const jo_pos3D_fixed * const eye,
                                                       const jo_pos3D_fixed * const dir, const jo_fixed length)
{
    int                             px;
    int                             pz;
    int                             along;
    int                             across;

    px = jo_fixed2int(e->position.x - eye->x);
    pz = jo_fixed2int(e->position.z - eye->z);
    // (whole units times a direction less than 1: no overflow up to 32000 units)
    along = (px * dir->x + pz * dir->z) >> 16;
    if (along < 0 || along > jo_fixed2int(length) + GUARD_BULLET_NEAR)
        return (false);
    across = (px * dir->z - pz * dir->x) >> 16;
    return (JO_ABS(across) <= GUARD_BULLET_NEAR);
}

/** @brief The scientists near a sound at x z (the shots of a rifle, the cry of a guard hit): scared, they run away */
static void                         scare_scientists(const jo_entity * const except, const jo_fixed x, const jo_fixed z)
{
    jo_entity                       *e;
    guard                           *g;
    int                             k;
    int                             dx;
    int                             dz;

    for (k = 0; (e = jo_entity_get(k)) != JO_NULL; ++k)
    {
        if (e == except || e->cls != &scientist_class || e->data == JO_NULL)
            continue;
        g = (guard *)e->data;
        dx = JO_ABS(jo_fixed2int(x - e->position.x));
        dz = JO_ABS(jo_fixed2int(z - e->position.z));
        if (JO_MAX(dx, dz) + (JO_MIN(dx, dz) * 3 >> 3) <= SCIENTIST_HEAR_DISTANCE)
            scare(e, g, false);
    }
}

void                                guards_hear_shot(const jo_pos3D_fixed * const eye, const jo_pos3D_fixed * const dir, const jo_fixed length,
                                                     const bool loud)
{
    jo_entity                       *e;
    guard                           *g;
    int                             k;

    // The shots of the agent: the guards near him hear them (surprised, they turn to him), the guards near the bullet too.
    // The silenced pistol (not loud) makes no noise: only the guards that can see him from there (around them, through
    // the doorways) notice it, or those the bullet passes near
    for (k = 0; (e = jo_entity_get(k)) != JO_NULL; ++k)
    {
        if (!is_person(e) || e->data == JO_NULL || (e->flags & JO_ENTITY_FLAG_DISABLED))
            continue;
        g = (guard *)e->data;
        if (g->scientist)
        {
            // (a scientist: scared by a shot near him, by the loud rifle farther)
            if (g->state != GUARD_DEAD && ((loud ? distance_to_agent(e) <= SCIENTIST_HEAR_DISTANCE :
                                                   distance_to_agent(e) <= GUARD_HEARING_DISTANCE + 200 && sees_agent_within(e, GUARD_HEARING_DISTANCE + 200)) ||
                                           bullet_passes_near(e, eye, dir, length)))
                scare(e, g, false);
            continue;
        }
        if (g->state != GUARD_IDLE && g->state != GUARD_SEARCH)
            continue;
        if (!(loud ? distance_to_agent(e) <= GUARD_HEARING_DISTANCE :
                     distance_to_agent(e) <= GUARD_HEARING_DISTANCE && sees_agent_within(e, GUARD_HEARING_DISTANCE)) &&
            !bullet_passes_near(e, eye, dir, length))
            continue;
        alert(e, g);
    }
}


void                                guards_update_blood(const int elapsed)
{
    int                             k;
    blood_drop                      *b;

    persons_load_update();


    for (k = 0; k < BLOOD_MAX; ++k)
    {
        b = &blood[k];
        if (b->time <= 0)
            continue;
        b->time -= elapsed;
        b->position.x += b->speed.x / 1000 * elapsed;
        b->position.y += b->speed.y / 1000 * elapsed;
        b->position.z += b->speed.z / 1000 * elapsed;
        // (gravity: 400 units per second per second)
        b->speed.y += jo_int2fixed(400) / 1000 * elapsed;
    }
    for (k = 0; k < GUARD_FLASH_MAX; ++k)
        if (flash_time[k] > 0)
            flash_time[k] -= elapsed;
    // The falling caps (lying on the floor: on their side, they stay)
    for (k = 0; k < CAP_MAX; ++k)
    {
        falling_cap                 *c;

        c = &caps[k];
        if (!c->used)
            continue;
        if (c->spin_speed == 0)
        {
            // (on the floor: it goes after a while, like the bodies)
            c->on_floor += elapsed;
            if (c->on_floor >= CAP_FLOOR_TIME)
                c->used = false;
            continue;
        }
        c->position.x += c->speed.x / 1000 * elapsed;
        c->position.y += c->speed.y / 1000 * elapsed;
        c->position.z += c->speed.z / 1000 * elapsed;
        c->speed.y += jo_int2fixed(500) / 1000 * elapsed;
        c->spin = (c->spin + c->spin_speed * elapsed / 1000) % 360;
        // (on the floor: the half height of the cap)
        if (c->position.y >= c->floor - jo_int2fixed(3))
        {
            c->position.y = c->floor - jo_int2fixed(3);
            c->spin = 90;
            c->spin_speed = 0;
            c->on_floor = 0;
        }
    }
    // The agent walks over a dropped rifle: he takes it
    for (k = 0; k < DROP_MAX; ++k)
    {
        if (!drops[k].used || JO_ABS(jo_fixed2int(drops[k].position.x - player_x)) > DROP_DISTANCE ||
            JO_ABS(jo_fixed2int(drops[k].position.z - player_z)) > DROP_DISTANCE ||
            JO_ABS(jo_fixed2int(drops[k].position.y - player_y)) > 40)
            continue;
        if (weapon_pickup("AK-47"))
            drops[k].used = false;
    }
}

void                                guards_draw_blood(void)
{
    int                             k;
    int                             bias;
    blood_drop                      *b;

    // The falling caps: the model of the cap (or the model of the guard with all its bones hidden but the cap), turned
    // around the middle of the cap
    for (k = 0; k < CAP_MAX && (cap_bone >= 0 || cap_model != JO_NULL); ++k)
    {
        if (!caps[k].used || caps[k].model == JO_NULL)
            continue;
        bias = jo_cells_depth_bias_at(caps[k].position.x, caps[k].position.y, caps[k].position.z);
        if (bias < 0)
            continue;
        jo_3d_push_matrix();
        {
            jo_3d_set_depth_bias(WORLD_DEPTH_BIAS + bias);
            jo_3d_select_window(jo_cells_window_of(bias));
            jo_3d_translate_matrix_fixed(caps[k].position.x, caps[k].position.y, caps[k].position.z);
            jo_3d_rotate_matrix_y(caps[k].yaw);
            jo_3d_rotate_matrix_x(caps[k].spin);
            // (the middle of the cap at its position: GUARD_CAP_CENTER was 10 units too low, the cap on the floor was
            // drawn 7 units under the floor, never seen)
            if (cap_middle.y != 0)
                jo_3d_translate_matrix_fixed(-cap_middle.x, -cap_middle.y, -cap_middle.z);
            else
                jo_3d_translate_matrix_fixed(0, jo_int2fixed(GUARD_CAP_CENTER), 0);
            if (cap_model != JO_NULL)
                jo_3d_model_draw(cap_model);
            else
            {
                jo_3d_model_set_hidden_bones(~(1u << cap_bone));
                jo_3d_model_draw_pose(caps[k].model, JO_NULL, JO_NULL, JO_NULL);
                jo_3d_model_set_hidden_bones(0);
            }
        }
        jo_3d_pop_matrix();
        jo_3d_select_window(JO_3D_NEAR_WINDOW);
    }
    // The dropped rifles
    for (k = 0; k < DROP_MAX && rifle_model != JO_NULL; ++k)
    {
        if (!drops[k].used)
            continue;
        bias = jo_cells_depth_bias_at(drops[k].position.x, drops[k].position.y, drops[k].position.z);
        if (bias < 0)
            continue;
        jo_3d_push_matrix();
        {
            jo_3d_set_depth_bias(WORLD_DEPTH_BIAS + bias);
            jo_3d_select_window(jo_cells_window_of(bias));
            jo_3d_translate_matrix_fixed(drops[k].position.x, drops[k].position.y, drops[k].position.z);
            jo_3d_rotate_matrix_y(drops[k].yaw);
            jo_3d_model_draw(rifle_model);
        }
        jo_3d_pop_matrix();
        jo_3d_select_window(JO_3D_NEAR_WINDOW);
    }
    for (k = 0; k < GUARD_FLASH_MAX && tex_flash >= 0; ++k)
    {
        if (flash_time[k] <= 0)
            continue;
        bias = jo_cells_depth_bias_at(flash_position[k].x, flash_position[k].y, flash_position[k].z);
        if (bias < 0)
            continue;
        jo_3d_push_matrix();
        {
            jo_3d_set_depth_bias(WORLD_DEPTH_BIAS + bias - 30);
            jo_3d_select_window(jo_cells_window_of(bias));
            jo_3d_translate_matrix_fixed(flash_position[k].x, flash_position[k].y, flash_position[k].z);
            jo_3d_rotate_matrix_y(yaw);
            jo_3d_rotate_matrix_x(-pitch);
            jo_3d_set_scale_fixed(GUARD_FLASH_SIZE / 16, GUARD_FLASH_SIZE / 16, JO_FIXED_1);
            jo_3d_draw_sprite(tex_flash);
        }
        jo_3d_pop_matrix();
        jo_3d_select_window(JO_3D_NEAR_WINDOW);
    }
    if (tex_blood < 0)
    {
        jo_3d_set_depth_bias(WORLD_DEPTH_BIAS);
        return;
    }
    for (k = 0; k < BLOOD_MAX; ++k)
    {
        b = &blood[k];
        if (b->time <= 0)
            continue;
        bias = jo_cells_depth_bias_at(b->position.x, b->position.y, b->position.z);
        if (bias < 0)
            continue;
        jo_3d_push_matrix();
        {
            // (in front of the body it comes out of)
            jo_3d_set_depth_bias(WORLD_DEPTH_BIAS + bias - 30);
            jo_3d_select_window(jo_cells_window_of(bias));
            jo_3d_translate_matrix_fixed(b->position.x, b->position.y, b->position.z);
            // (facing the camera)
            jo_3d_rotate_matrix_y(yaw);
            jo_3d_rotate_matrix_x(-pitch);
            jo_3d_set_scale_fixed(BLOOD_SIZE / 8, BLOOD_SIZE / 8, JO_FIXED_1);
            jo_3d_draw_sprite(tex_blood);
        }
        jo_3d_pop_matrix();
        jo_3d_select_window(JO_3D_NEAR_WINDOW);
    }
    jo_3d_set_depth_bias(WORLD_DEPTH_BIAS);
}

void                                guards_register(void)
{
    jo_entity_register_class(&guard_class);
    jo_entity_register_class(&scientist_class);
}

/** @brief The textures of the faces (cd/GFACES.JOM: its textures stay in the VRAM, the model is freed) */
static void                         load_faces(void)
{
    jo_3d_model                     *m;
    int                             v;
    int                             t;

    m = jo_3d_load_model("GFACES.JOM");
    if (m == JO_NULL)
        return;
    jo_3d_free_model(m);
    for (v = 0; v < GUARD_FACE_COUNT; ++v)
        for (t = 0; t < GUARD_FACE_TILES; ++t)
            face_sprites[v][t] = jo_3d_model_texture(guard_face_textures[v][t]);
    faces_loaded = true;
}

void                                guards_load(void)
{
    tex_blood = jo_sprite_add_bin_4bits_lut(JO_ROOT_DIR, "BLOOD.BIN", JO_COLOR_Transparent);
    tex_flash = jo_sprite_add_bin_4bits_lut(JO_ROOT_DIR, "GFLASH.BIN", JO_COLOR_Transparent);
    rifle_model = jo_3d_load_model("RIFLEW.JOM");
    cap_model = jo_3d_load_model("CAP.JOM");
    unarmed_model = jo_3d_load_model("GUARDN.JOM");
    if (unarmed_model != JO_NULL)
        cap_bone = jo_3d_model_find_bone(unarmed_model, "cap");
    find_cap_middle(unarmed_model);
    unarmed_far_model = jo_3d_load_model("GUARDNF.JOM");
    load_faces();
    // (the face polygons of the models of the guards found now, by the master CPU: the slave CPU reads them when it draws
    // the guards, it must not find them)
    if (faces_loaded)
    {
        jo_entity                   *e;
        const guard                 *g;
        int                         k;

        for (k = 0; (e = jo_entity_get(k)) != JO_NULL; ++k)
            if ((e->cls == &guard_class || e->cls == &scientist_class) && (g = (const guard *)e->data) != JO_NULL)
            {
                find_face_polygons(g->actor.model);
                find_face_polygons(g->actor.far_model);
            }
        find_face_polygons(unarmed_model);
        find_face_polygons(unarmed_far_model);
    }
    sounds_loaded = load_sound("GSHOT1.WAV", &guard_shots[0]) && load_sound("GSHOT2.WAV", &guard_shots[1]) &&
                    load_sound("GPAIN1.WAV", &pain_sounds[0]) && load_sound("GPAIN2.WAV", &pain_sounds[1]) &&
                    load_sound("GPAIN3.WAV", &pain_sounds[2]) && load_sound("GPAIN4.WAV", &pain_sounds[3]);
    // (far from the agent: more of the room than of the shot)
    guard_shots[0].reverb = 7;
    guard_shots[1].reverb = 7;
    // The ways between the rooms (the doors are loaded): at the loading, not at the first hunt (about 0.2 s of rays)
    way_build();
}

/*
** END OF FILE
*/
