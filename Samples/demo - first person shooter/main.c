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

/** @brief The share of the polygons of the cells drawn by the master CPU (0 to 1024, see jo_cells_draw_prepared_share()),
 *  balanced at each frame (see balance_cells_share()): the slave CPU draws the other part. The master CPU draws the first
 *  part: its commands come first in the table, 2 polygons at the same depth are drawn in the same order as with a CPU.
 *  (Down to 0: all the guards on alert, the master CPU draws the entities and the slave CPU still waited with 64 / 1024) */
#define CELLS_SHARE_MIN         (0)
#define CELLS_SHARE_MAX         (896)
static int                      cells_share = 400;

/** @brief The job of the slave CPU: its part of the cells */
static void                     slave_cells(void)
{
    // (written by the master CPU before the job: read without the cache)
    jo_cells_draw_prepared_share(true, 1, *(volatile int *)JO_CACHE_THROUGH(&cells_share));
}

/** @brief More polygons of the cells to the slave CPU when it waited in the last frame, fewer when the master CPU waited for
 *  it (or when its command table was nearly full: its polygons would be lost) */
static void                     balance_cells_share(void)
{
    int                         step;

    // (1 / 1024 of the polygons per 100 microseconds, 40 / 1024 at most per frame)
    step = JO_MAX(-40, JO_MIN(40, jo_core_slave_idle_time() / 100));
    if (step > 0 && jo_3d_slave_get_command_count() > jo_3d_slave_get_max_commands() * 3 / 4)
        step = -16;
    cells_share = JO_MAX(CELLS_SHARE_MIN, JO_MIN(CELLS_SHARE_MAX, cells_share - step));
}


/** @brief The world: the cells seen from the camera (shared by the 2 CPUs), the entities, the impacts */
static void                     draw_world(void)
{
    jo_pos3D_fixed              eye;

    player_eye(&eye);
    jo_3d_push_matrix();
    {
        set_camera();
        jo_3d_set_depth_bias(WORLD_DEPTH_BIAS);
        jo_cells_prepare(&eye);
        jo_3d_slave_begin();
        jo_core_exec_on_slave(slave_cells);
        jo_cells_draw_prepared_share(false, 0, cells_share);
        jo_cells_draw_mirror(JO_NULL);
        // The glows of the lamps (VDP2 layers, placed with the camera: after the cells, their rectangles)
        jo_3d_glows_draw(&eye);
        jo_entities_draw();
        billboards_draw();
        // The fire and the smoke of the explosions (the explosive barrels)
        jo_effects_draw();
        impacts_draw();
        guards_draw_blood();
        jo_core_wait_for_slave();
        balance_cells_share();
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
        jo_entities_update(&feet, FRAME_MS);
        doors_lights_update(FRAME_MS);
        guards_update_blood(FRAME_MS);
        jo_effects_update();
    }
    draw_world();
    weapon_draw();
    hud_draw();
    draw_texts();
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
    // The weapons (cd/DEFS.JOL), the classes of the game, then the level: its rooms and its entities
    level = jo_level_load("DEFS.JOL");
    if (level != JO_NULL)
    {
        jo_weapons_load(level);
        jo_level_free(level);
    }
    doors_register();
    guards_register();
    level = jo_level_load("LEVEL.JOL");
    if (level != JO_NULL)
    {
        jo_cells_load(level);
        jo_entities_set_area_damage_callback(player_area_damage);
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
    jo_core_add_callback(my_draw);
    jo_core_run();
}

/*
** END OF FILE
*/
