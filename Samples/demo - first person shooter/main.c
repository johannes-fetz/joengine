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
** Main loop: the level (cells and entities), the camera, the frame (world, entities, weapon, HUD)
*/

#include "demo.h"

#ifdef DEMO_BENCH
/** @brief The measures of the benchmark (see bench_print()) */
# define BENCH_BEGIN(N)                 jo_profile_begin(JO_PROFILE_USER(N))
# define BENCH_END(N)                   jo_profile_end(JO_PROFILE_USER(N))
#else
# define BENCH_BEGIN(N)                 do {} while (0)
# define BENCH_END(N)                   do {} while (0)
#endif

#ifdef DEMO_BENCH
static void                     bench_print(void);
static bool                     bench_done(void);
static int                      bench_entities_share;
#endif

jo_fixed                        player_x;
jo_fixed                        player_y;
jo_fixed                        player_z;
int                             yaw;
int                             pitch;
int                             player_health = PLAYER_MAX_HEALTH;
bool                            crouched = false;
bool                            aiming = false;
int                             aim_x = 0;
int                             aim_y = 0;
int                             frame_counter = 0;
bool                            paused = false;

/** @brief The message at the bottom of the screen and the frames left */
static char                     message[48];
static int                      message_frames = 0;
/** @brief The red flash when the agent is hit (frames left) */
static int                      hurt_frames = 0;
/** @brief Red frames of the hurt flash */
#define HURT_FLASH_FRAMES       (8)

/** @brief Height of the eyes now (crouching is smooth) */
static jo_fixed                 eye_height = JO_FIXED_1 * EYE_HEIGHT;

void                            player_eye(jo_pos3D_fixed * const eye)
{
    eye->x = player_x;
    eye->y = player_y - eye_height;
    eye->z = player_z;
}

/** @brief The eyes go down when the agent crouches (a few frames) */
static void                     update_eye_height(void)
{
    jo_fixed                    target;

    target = JO_FIXED_1 * (crouched ? EYE_HEIGHT_CROUCHED : EYE_HEIGHT);
    eye_height += (target - eye_height) * 8 / GAME_FPS;
}

void                            player_aim_direction(jo_pos3D_fixed * const dir)
{
    jo_fixed                    fx;
    jo_fixed                    fy;
    jo_fixed                    fz;
    jo_fixed                    cp;
    jo_fixed                    sp;
    jo_fixed                    cy;
    jo_fixed                    sy;
    jo_fixed                    rx;
    jo_fixed                    ry;
    jo_fixed                    length;

    // The direction through a point of the screen (the crosshair): in the view space, x right, y down, z = the projection
    // distance; then turned by the pitch and the yaw of the camera
    // (jo_3d_get_projection_distance() is Q16.16 already)
    rx = jo_fixed_div(jo_int2fixed(aiming ? aim_x : 0), jo_3d_get_projection_distance());
    ry = jo_fixed_div(jo_int2fixed(aiming ? aim_y : 0), jo_3d_get_projection_distance());
    cp = jo_cos(pitch);
    sp = jo_sin(pitch);
    cy = jo_cos(yaw);
    sy = jo_sin(yaw);
    // View (rx, ry, 1) -> pitch around X (looking down: positive pitch turns +Z toward +Y (down))
    fx = rx;
    fy = jo_fixed_mult(ry, cp) + sp;
    fz = cp - jo_fixed_mult(ry, sp);
    // Yaw around Y: +Z toward +X for a positive yaw
    dir->x = jo_fixed_mult(fx, cy) + jo_fixed_mult(fz, sy);
    dir->y = fy;
    dir->z = jo_fixed_mult(fz, cy) - jo_fixed_mult(fx, sy);
    length = jo_fixed_sqrt(jo_fixed_mult(dir->x, dir->x) + jo_fixed_mult(dir->y, dir->y) + jo_fixed_mult(dir->z, dir->z));
    if (length > 0)
    {
        dir->x = jo_fixed_div(dir->x, length);
        dir->y = jo_fixed_div(dir->y, length);
        dir->z = jo_fixed_div(dir->z, length);
    }
}

void                            show_message(const char * const text)
{
    int                         k;

    for (k = 0; text[k] != '\0' && k < (int)sizeof(message) - 1; ++k)
        message[k] = text[k];
    message[k] = '\0';
    message_frames = GAME_FPS * 3;
}

/** @brief The low work RAM given to jo_malloc() (its end left free) */
#define LWRAM_HEAP_ADDRESS              (0x00200000)
#define LWRAM_HEAP_SIZE                 (0xE0000)

/** @brief The send of the sounds to the echo of the room (the DSP of the SCSP: the sounds have no echo of their own):
 *  -12 dB */
#define SOUND_REVERB                    (5)

bool                            load_sound(const char * const name, jo_sound * const sound)
{
    if (!jo_audio_load_wav(name, sound))
        return (false);
    sound->reverb = SOUND_REVERB;
    // (in the sound RAM: the work RAM stays free, a sound is not copied when it plays; full: it stays in the work RAM)
    jo_audio_make_resident(sound);
    return (true);
}

void                            play_sound(jo_sound * const sound, const int first, const int count, int * const next)
{
    int                         channel;

    channel = first + *next;
    *next = (*next + 1) % count;
    jo_audio_stop_channel((unsigned char)channel);
    jo_audio_play_sound_on_channel(sound, (unsigned char)channel);
}

#ifdef JO_COMPILE_WITH_AUDIO_DSP_SUPPORT

/** @brief The echoes of the rooms (the reverberation of the DSP of the SCSP: decay ms, damping, early reflections, tail) */
static const jo_audio_reverb    room_echoes[] =
{
    // Tiles (the washroom: RT60 0.85 s under 1 kHz, 0.4 s over 4 kHz; its corridor, the hall)
    {850, 60, 100, 100},
    // Concrete (the gallery, the lower hall, the stairs)
    {650, 75, 90, 90},
    // Furnished rooms (the storage room, the laboratory, the room of the guards, the office): duller
    {600, 95, 70, 80},
    // The bottling room: 2 floors high, the tanks
    {1400, 55, 60, 110},
    // Steel (the duct, the airlock): short, many reflections
    {350, 25, 127, 70},
};

/** @brief The echo of each cell (the rooms in the order of the level, then the duct; the cells of
 *  the doors after them keep the echo of the room the agent comes from) */
static const unsigned char      cell_echoes[] =
{
    0,  // washroom
    0,  // corridor
    2,  // storage
    0,  // hall
    2,  // lab
    2,  // guards
    2,  // office
    1,  // gallery
    4,  // airlock
    1,  // stairs
    1,  // lowhall
    3,  // bottling
    3,  // walkway
    3,  // stairs2
    3,  // landing
    4,  // duct
    4,  // ducthole
    4,  // ductend
};

/** @brief The echo of the room of the agent (when he goes to another room) */
static void                     update_room_echo(void)
{
    static int                  echo = -1;
    int                         index;

    index = jo_cells_find(player_x, player_y, player_z, 32);
    if (index < 0 || index >= (int)(sizeof(cell_echoes) / sizeof(cell_echoes[0])) || cell_echoes[index] == echo)
        return;
    echo = cell_echoes[index];
    jo_audio_set_reverb(&room_echoes[echo]);
}

#endif

/** @brief The pan of a sound on a side of the agent (90 degrees): the other speaker 15 dB lower (the SCSP lowers it by
 *  3 dB per 8 of pan), still heard */
#define SOUND_PAN_MAX                   (40)

void                            sound_from(jo_sound * const sound, const jo_fixed x, const jo_fixed z, const int volume, const int range)
{
    int                         dx;
    int                         dz;
    int                         distance;
    int                         side;

    dx = jo_fixed2int(x - player_x);
    dz = jo_fixed2int(z - player_z);
    distance = (int)jo_sqrt((unsigned int)(dx * dx + dz * dz));
    // (the right of the agent: (cos yaw, -sin yaw); a sound on his right louder in the right speaker)
    side = distance > 0 ? jo_fixed2int(dx * jo_cos(yaw) - dz * jo_sin(yaw)) * SOUND_PAN_MAX / distance : 0;
    sound->pan = (char)side;
    sound->volume = (unsigned char)JO_MAX(0, JO_MIN(127, volume * JO_MAX(0, range - distance) / range));
}

/** @brief The grunt of the agent when he is hit (a recording): played by the SCSP (the 68000
 *  driver programs its slot, the DSP adds the echo of the room), a little higher or lower each time (its sample rate) */
static jo_sound                 hurt_sound;
static bool                     hurt_sound_loaded = false;
/** @brief The grunt again after this (frames: a burst of the guards is one grunt, not a stutter) */
#define HURT_SOUND_FRAMES               (10)
/** @brief Its pitch: from - to + this percent */
#define HURT_SOUND_PITCH_RANGE          (6)

void                            hurt_player(const int damage)
{
    static int                  last_sound_frame = -HURT_SOUND_FRAMES;
    jo_sound                    grunt;

#ifdef DEMO_BENCH
    // (the benchmark: the agent is not hurt, the scene goes on)
    JO_UNUSED_ARG(damage);
    return;
#endif
    if (player_health <= 0)
        return;
    player_health = JO_MAX(0, player_health - damage);
    hurt_frames = HURT_FLASH_FRAMES;
    jo_effect_flash(120, 0, 0, HURT_FLASH_FRAMES);
    if (hurt_sound_loaded && frame_counter - last_sound_frame >= HURT_SOUND_FRAMES)
    {
        last_sound_frame = frame_counter;
        // (a copy played at another rate: the same samples in the sound RAM)
        grunt = hurt_sound;
        grunt.sample_rate = (unsigned short)(hurt_sound.sample_rate * (100 - HURT_SOUND_PITCH_RANGE +
                                                                       jo_random(2 * HURT_SOUND_PITCH_RANGE + 1) - 1) / 100);
        jo_audio_stop_channel(CHANNEL_VOICE);
        jo_audio_play_sound_on_channel(&grunt, CHANNEL_VOICE);
    }
}

/** @brief The damage of an explosion (an explosive barrel, see jo_entities_damage_area()) on the agent: less far from it */
static void                     player_area_damage(const jo_pos3D_fixed * const center, const int radius, const int damage,
                                                   jo_entity * const source)
{
    int                         dx;
    int                         dy;
    int                         dz;
    int                         d;

    JO_UNUSED_ARG(source);
    // (the distance to the middle of the body)
    dx = JO_ABS(jo_fixed2int(player_x - center->x));
    dy = JO_ABS(jo_fixed2int(player_y - center->y) - EYE_HEIGHT / 2);
    dz = JO_ABS(jo_fixed2int(player_z - center->z));
    if (dx >= radius || dy >= radius || dz >= radius)
        return;
    d = (int)jo_sqrt((unsigned int)(dx * dx + dy * dy + dz * dz)) - PLAYER_RADIUS;
    if (d >= radius)
        return;
    hurt_player(damage * (radius - JO_MAX(d, 0)) / radius);
}

/** @brief The camera: the matrix of the view (the current matrix must be the identity) */
static void                     set_camera(void)
{
    jo_pos3D_fixed              eye;

    player_eye(&eye);
    jo_3d_rotate_matrix_x(pitch);
    jo_3d_rotate_matrix_y(-yaw);
    jo_3d_translate_matrix_fixed(-eye.x, -eye.y, -eye.z);
}

/** @brief The share of the cost of the entities drawn by the slave CPU (0 to 1024, see jo_entities_prepare_draw()): all
 *  of them (the characters cost the most: about 4 ms for one near the camera), the master CPU draws the cells they leave
 *  (the share of the cells, see jo_cells_draw_prepared_on_both_cpus()) */
#define ENTITIES_SLAVE_SHARE    (1024)

/** @brief The share of the entities drawn by the slave CPU in this frame: ENTITIES_SLAVE_SHARE, less when the master CPU
 *  waits for the slave CPU while it has no cells left to take (the share of the cells at 0): from the walkway over the
 *  bottling room, 4 guards and the whole room on the slave CPU made it 29 ms while the master CPU waited 4 to 7 ms; the
 *  entities go back to the slave CPU when the master CPU draws a good part of the cells again (the slave CPU has time) */
#define ENTITIES_SHARE_STEP     (64)
#define ENTITIES_SHARE_MIN      (256)

static int                      entities_slave_share(void)
{
    static int                  share = ENTITIES_SLAVE_SHARE;
    int                         cells;

    // (the slave CPU idle: back to it, whatever the cells (with the cells of the master CPU as the condition, the share
    // stayed down once the master CPU was the slower one: 24 ms of guards on it in a general alert))
    // (jo_cells_get_share() is the share of the MASTER CPU: at its maximum, the master CPU draws all the cells and can't take
    // more of them, only entities. With only the test of the share at 0, 9 scientists in front of the agent kept all the
    // entities on the slave CPU (23 ms of characters) while the master CPU waited 16 ms for it: 28 frames per second)
    cells = jo_cells_get_share();
    if ((cells <= 128 || cells >= 1024 - 128) && jo_core_master_wait_time() > 300)
        share = JO_MAX(ENTITIES_SHARE_MIN, share - ENTITIES_SHARE_STEP);
    else if (jo_core_slave_idle_time() > 1000)
        share = JO_MIN(ENTITIES_SLAVE_SHARE, share + ENTITIES_SHARE_STEP);
#ifdef DEMO_BENCH
# ifdef DEMO_BENCH_SHARE
    share = DEMO_BENCH_SHARE;
# endif
    bench_entities_share = share;
#endif
    return (share);
}

/** @brief What the slave CPU draws before its part of the cells: its entities (the job of the slave CPU) */
static void                     slave_entities(void)
{
    jo_entities_draw_slave();
}

/** @brief The world: the cells seen from the camera (shared by the 2 CPUs), the entities (the slave CPU), the impacts */
static void                     draw_world(void)
{
    jo_pos3D_fixed              eye;

    player_eye(&eye);
    jo_3d_push_matrix();
    {
        set_camera();
        jo_3d_set_depth_bias(WORLD_DEPTH_BIAS);
        BENCH_BEGIN(1);
        jo_cells_prepare(&eye);
        // The entities seen, their tints, the ones of the slave CPU (before its job)
        jo_entities_prepare_draw(entities_slave_share());
        BENCH_END(1);
        jo_3d_slave_begin();
        BENCH_BEGIN(2);
        // (the entities of the slave CPU, then the cells shared by the 2 CPUs: half of the polygons of the frame to each
        // one, the share balanced at each frame)
        jo_cells_draw_prepared_on_both_cpus(slave_entities, jo_entities_polygon_count(true));
        jo_cells_draw_mirror(JO_NULL);
        // The glows of the lamps (VDP2 layers, placed with the camera: after the cells, their rectangles)
        jo_3d_glows_draw(&eye);
        // (the entities of the master CPU: the ones with a draw callback after the ones of the slave CPU)
        jo_entities_draw();
        billboards_draw();
        // The fire and the smoke of the explosions (the explosive barrels)
        jo_effects_draw();
        impacts_draw();
        guards_draw_blood();
        BENCH_END(2);
        jo_core_wait_for_slave();
        jo_cells_balance_share();
        jo_3d_slave_end();
    }
    jo_3d_pop_matrix();
}

/** @brief The message and the pause */
static void                     draw_texts(void)
{
    static bool                 message_shown = false;

    if (message_frames > 0)
    {
        --message_frames;
        jo_printf(2, 25, "%s", message);
        message_shown = true;
    }
    else if (message_shown)
    {
        // (cleared once: a printf each frame cost 2 % of the master CPU)
        jo_printf(2, 25, "                                      ");
        message_shown = false;
    }
    if (paused)
        jo_printf(17, 13, "PAUSED");
    else
        jo_printf(17, 13, "      ");
}


static void                     my_draw(void)
{
    static bool                 start_was_down = false;
    bool                        start_down;
    jo_pos3D_fixed              feet;

    ++frame_counter;
    // X + Y + Z: the wireframe mode (the polygons drawn as their outline, to find a bug), on and off (when the last of the 3
    // is pressed)
    if (jo_is_pad1_key_pressed(JO_KEY_X) && jo_is_pad1_key_pressed(JO_KEY_Y) && jo_is_pad1_key_pressed(JO_KEY_Z) &&
        (jo_is_pad1_key_down(JO_KEY_X) || jo_is_pad1_key_down(JO_KEY_Y) || jo_is_pad1_key_down(JO_KEY_Z)))
        jo_3d_set_wireframe(!jo_3d_is_wireframe());
    start_down = jo_is_pad1_key_pressed(JO_KEY_START);
    if (start_down && !start_was_down)
        paused = !paused;
    start_was_down = start_down;
    if (!paused)
    {
        player_update();
        update_eye_height();
#ifdef JO_COMPILE_WITH_AUDIO_DSP_SUPPORT
        update_room_echo();
#endif
        weapon_update();
        feet.x = player_x;
        feet.y = player_y;
        feet.z = player_z;
        BENCH_BEGIN(0);
        jo_entities_update(&feet, FRAME_MS);
        BENCH_END(0);
        doors_lights_update(FRAME_MS);
        guards_update_blood(FRAME_MS);
        jo_effects_update();
    }
#ifdef DEMO_BENCH
    if (bench_done())
    {
        bench_print();
        return;
    }
#endif
    draw_world();
    weapon_draw();
    hud_draw();
    draw_texts();
#ifdef DEMO_BENCH
    bench_print();
#endif
}

/** @brief Put the agent on the spawn of the level */
static void                     place_player(void)
{
    jo_entity                   *spawn;
    int                         floor;

    spawn = jo_entity_find("start");
    if (spawn != JO_NULL)
    {
        player_x = spawn->position.x;
        player_y = spawn->position.y;
        player_z = spawn->position.z;
        yaw = spawn->yaw;
    }
    pitch = 0;
    (void)floor;
}

#ifdef DEMO_BENCH
/*
** Benchmark, disabled by default (make _CCFLAGS="-DDEMO_BENCH ..." JO_COMPILE_WITH_PROFILER_MODULE=1): the agent placed at a
** view (DEMO_BENCH_VIEW), no input needed, the game measured from the frame DEMO_BENCH_START to DEMO_BENCH_END, then the
** summary on a black screen (late frames, averages and maximums of the profiler in microseconds, the share of the entities
** of the slave CPU). Options:
**   DEMO_BENCH_HIDDEN         the persons don't see the agent (they patrol: the same scene in every run)
**   DEMO_BENCH_CROWD=N        N persons gathered in rows in front of the agent (view 4: the laboratory), the scientists first
**   DEMO_BENCH_SCIENTISTS     with DEMO_BENCH_CROWD: the guards become scientists
**   DEMO_BENCH_SHARE=S        the share of the entities of the slave CPU fixed (0 to 1024)
**   DEMO_BENCH_FREEZE=F       the game paused at the frame F (the same picture in every run, to compare them)
**   DEMO_BENCH_PCPROF         the program counter sampling during the measures (tools/pcprof_report.py on a save state)
** Measured (Mednafen, 7/11/2026, 9 scientists, view 4): 37 late frames out of 384 before the parallel drawing of the persons
** (see guards_register()) and their models in the high work RAM (see guards_reserve_fast_memory()), 0 after
*/
# ifndef DEMO_BENCH_VIEW
#  define DEMO_BENCH_VIEW       (1)
# endif
/** @brief The views: x, z, yaw, pitch */
static const int                bench_views[][4] =
{
    {0, 0, 0, 0},
    // 1: the laboratory from its corner (south-east): its 2 scientists and its 2 guards
    {1720, 1880, 235, 0},
    // 2: the laboratory from the door of the hall
    {1280, 1380, 20, 0},
    // 3: the office from the door of the laboratory (its scientist and its guard)
    {1610, 1380, 180, 0},
    // 4: the laboratory from its east side (for DEMO_BENCH_CROWD: the persons gathered in front of the agent)
    {1720, 1640, 270, 0},
    // 5: the bottling room from the walkway (its guards below)
    {3480, 1480, 180, 15},
};

static void                     bench_place(void)
{
    player_x = jo_int2fixed(bench_views[DEMO_BENCH_VIEW][0]);
    player_y = 0;
    player_z = jo_int2fixed(bench_views[DEMO_BENCH_VIEW][1]);
    yaw = bench_views[DEMO_BENCH_VIEW][2];
    pitch = bench_views[DEMO_BENCH_VIEW][3];
# ifdef DEMO_BENCH_CROWD
    // (DEMO_BENCH_CROWD persons in rows in front of the agent; DEMO_BENCH_SCIENTISTS: all of them scientists)
#  ifdef DEMO_BENCH_SCIENTISTS
    guards_bench_gather(player_x, player_z, yaw, DEMO_BENCH_CROWD, true);
#  else
    guards_bench_gather(player_x, player_z, yaw, DEMO_BENCH_CROWD, false);
#  endif
# endif
    jo_profile_set_name(JO_PROFILE_USER(0), "ent_upd");
    jo_profile_set_name(JO_PROFILE_USER(1), "prepare");
    jo_profile_set_name(JO_PROFILE_USER(2), "m_draw");
}

# ifndef DEMO_BENCH_START
#  define DEMO_BENCH_START      (64)
# endif
# ifndef DEMO_BENCH_END
#  define DEMO_BENCH_END        (64 + 32 * 12)
# endif
/** @brief The measures summed from DEMO_BENCH_START to DEMO_BENCH_END (frames of the game), then shown on a black screen */
static const jo_profile_slot    bench_slots[] =
{
    JO_PROFILE_FRAME, JO_PROFILE_CALLBACKS, JO_PROFILE_WAIT_SLAVE, JO_PROFILE_SLAVE, JO_PROFILE_VDP1_DRAW, JO_PROFILE_POLYGONS,
    JO_PROFILE_USER(0), JO_PROFILE_USER(1), JO_PROFILE_USER(2),
};
static const char               *bench_names[] =
{
    "FRAME", "MASTER", "WAITSLAVE", "SLAVE", "VDP1", "POLYGONS", "UPDATE", "PREPARE", "MDRAW",
};
#define BENCH_SLOTS             ((int)(sizeof(bench_slots) / sizeof(bench_slots[0])))

static bool                     bench_done(void)
{
    return (frame_counter > DEMO_BENCH_END);
}

static void                     bench_print(void)
{
    static unsigned int         first_late = 0;
    static unsigned int         first_vblank = 0;
    static unsigned int         late = 0;
    static unsigned int         vblanks = 0;
    static unsigned int         sums[BENCH_SLOTS];
    static unsigned int         maxs[BENCH_SLOTS];
    static int                  samples = 0;
    static int                  min_share = 1024;
    static int                  max_share = 0;
    int                         k;

# ifdef DEMO_BENCH_PCPROF
    // (the program counter sampling over the frames of the measures, the histograms at the end of the low work RAM, read in
    // a save state by tools/pcprof_report.py)
    if (frame_counter == DEMO_BENCH_START)
        jo_profile_sampling_start((unsigned int *)0x002E0000, (unsigned int *)0x002F0000);
    else if (frame_counter == DEMO_BENCH_END)
        jo_profile_sampling_stop();
# endif
# ifdef DEMO_BENCH_FREEZE
    // (the scene frozen at this frame (the pause): the same picture in every run, to compare them)
    if (frame_counter == DEMO_BENCH_FREEZE)
        paused = true;
# endif
    if (frame_counter == DEMO_BENCH_START)
    {
        first_late = jo_core_get_late_frame_count();
        first_vblank = jo_core_get_vblank_count();
    }
    if (frame_counter > DEMO_BENCH_START && frame_counter <= DEMO_BENCH_END)
    {
        min_share = JO_MIN(min_share, bench_entities_share);
        max_share = JO_MAX(max_share, bench_entities_share);
    }
#ifdef JO_COMPILE_WITH_PROFILER_SUPPORT
    // (the averages of the profiler change every 32 frames: summed when they change)
    if (frame_counter > DEMO_BENCH_START && frame_counter <= DEMO_BENCH_END && ((frame_counter - DEMO_BENCH_START) & 31) == 1)
    {
        unsigned int            avg;
        unsigned int            max;

        for (k = 0; k < BENCH_SLOTS; ++k)
            if (jo_profile_get(bench_slots[k], &avg, &max))
            {
                sums[k] += avg;
                maxs[k] = JO_MAX(maxs[k], max);
            }
        ++samples;
    }
#endif
    if (frame_counter == DEMO_BENCH_END)
    {
        late = jo_core_get_late_frame_count() - first_late;
        vblanks = jo_core_get_vblank_count() - first_vblank;
        // (the summary on a black screen: the 3D is no longer drawn)
        jo_3d_set_frame_clear(true);
    }
    if (frame_counter == DEMO_BENCH_END + 4)
    {
        jo_clear_screen();
        jo_printf(1, 1, "FRAMES %d VBLANKS %d LATE %d", DEMO_BENCH_END - DEMO_BENCH_START, (int)vblanks, (int)late);
        jo_printf(1, 2, "SHARE %d TO %d", min_share, max_share);
        for (k = 0; k < BENCH_SLOTS && samples > 0; ++k)
            jo_printf(1, 4 + k, "%s %d MAX %d", bench_names[k], (int)(sums[k] / samples), (int)maxs[k]);
    }
}
#endif


void                            jo_main(void)
{
    jo_level                    *level;

    jo_core_init(JO_COLOR_Black);
    // The low work RAM extends the memory of jo_malloc() (after the high work RAM: the rooms of the bigger level need it;
    // the SH-2 DMA reads it, not the SCU DMA nor the DSP: the engine checks it, see jo_3d_enable_dsp())
    jo_add_memory_zone((unsigned char *)LWRAM_HEAP_ADDRESS, LWRAM_HEAP_SIZE);
    // The VDP1 commands of the weapon and of the HUD (drawn after the polygons of the slave CPU are added): kept for them (45 at
    // most measured, all the guards alerted in the bottling room: the table was full, "Too many VDP1 commands in a frame")
    jo_3d_slave_set_kept_commands(56);
    jo_core_tv_on();
    // 75 degrees of horizontal field of view (about 60 degrees of vertical field of view on a 4:3 screen)
    jo_3d_perspective_angle(75);
    jo_3d_display_level(5);
    jo_3d_set_guard_band(WORLD_GUARD_BAND);
    // The rooms cover the whole screen at each frame: the frame buffer is not cleared (the polygon that cleared it cost
    // the VDP1 4 ms per frame)
    jo_3d_set_frame_clear(false);
    // (the high work RAM kept for the models of the persons, given to them after the level is loaded: see guards_load())
    guards_reserve_fast_memory();
    // The weapons (cd/DEFS.JOL), the classes of the game, then the level: its rooms and its entities
    level = jo_level_load("DEFS.JOL");
    if (level != JO_NULL)
    {
        jo_weapons_load(level);
        jo_level_free(level);
    }
    doors_register();
    guards_register();
    props_register();
    level = jo_level_load("LEVEL.JOL");
    if (level != JO_NULL)
    {
        jo_cells_load(level);
        jo_entities_set_area_damage_callback(player_area_damage);
        // (the table of the entities and their data in the high work RAM, see guards_reserve_fast_memory())
        guards_release_entity_memory();
        jo_entities_load(level);
        jo_level_free(level);
    }
    doors_glows_init();
    hud_load();
    guards_load();
    weapon_load();
    hurt_sound_loaded = load_sound("HURT.WAV", &hurt_sound);
    player_init();
    place_player();
    jo_effect_fade(255, 0);
    jo_effect_fade(0, 20);
    // The music: the track 2 of the CD (music.wav next to the makefile: the cue sheet puts the audio files after the
    // data track), in a loop. Started after the loading: the CD drive reads the files or plays the music, not both
    // (at half the level of the CD audio, -6 dB: at the full level the music covered the sounds of the game)
    jo_audio_set_cd_volume(JO_AUDIO_CD_VOLUME_MAX - 1);
    jo_audio_play_cd_track(2, 2, true);
#ifdef DEMO_BENCH
    bench_place();
#endif
    jo_core_add_callback(my_draw);
    jo_core_run();
}

/*
** END OF FILE
*/
