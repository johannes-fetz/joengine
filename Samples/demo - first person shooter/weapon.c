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
** The weapon in the hands of the agent: drawn over the world (no depth bias: the world has one), it bobs with the walk
** and lags behind the turns, goes back at each shot. The rules (ammunition, fire rate, reload) are the ones of the
** engine (jo/weapon.h, the definitions of cd/DEFS.JOL); the reload is automatic when the magazine is empty, or
** asked with B (when no door is in front of the agent).
**
** The shots: a ray from the eyes through the crosshair (the center of the screen, or the red crosshair of the aiming
** mode): an impact on the wall hit (a small dark mark that stays), damage to the entity hit.
*/

#include "demo.h"

/** @brief Amplitude of the bob of the walk (units) */
#define BOB_X                   (JO_FIXED_1 * 9 / 4)
#define BOB_Y                   (JO_FIXED_1 * 3 / 2)
/** @brief Breathing: the weapon moves a little when the agent stands still too (thousandths of world units) */
#define BREATH_X                (150)
#define BREATH_Y                (280)
/** @brief The weapon lags behind a turn (units per degree of turn, and the return per frame) */
#define LAG_PER_DEGREE          (JO_FIXED_1 * 3 / 10)
#define LAG_MAX                 (JO_FIXED_1 * 15 / 2)
/** @brief The weapon goes back at a shot (units) */
#define RECOIL_BACK             (JO_FIXED_1 * 9 / 2)
/** @brief Pose of the weapon in the hands (degrees), walking and aiming alike: the barrel points at the
 *  crosshair, turned a little to the left and raised a little */
#define POSE_YAW                (-3)
#define POSE_PITCH              (4)

/** @brief In the aiming mode the weapon moves toward the crosshair: this fraction of the way (on the screen) */
#define AIM_FOLLOW_PERCENT      (65)
/** @brief Impacts on the walls remembered */
#define MAX_IMPACTS             (24)
/** @brief Range of a bullet (units) */
#define BULLET_RANGE            (3000)
/** @brief Size of an impact mark (units) and how much nearer than the wall it is sorted (units) */
#define IMPACT_SIZE             (JO_FIXED_1 * 364 / 100)
#define IMPACT_SORT_BIAS        (60)
/** @brief Some shots (one in FLASH_CHANCE): a small flame at the muzzle for a few frames (the glow appears
 *  and fades in 4 frames: it is lit 3 frames), and the bullet
 *  seen flying (a bright dot, at TRACER_SPEED units per second) */
#define FLASH_CHANCE            (2)
#define FLASH_FRAMES            (GAME_FPS / 10)
/** @brief The flame: a sprite drawn half transparent (cd/FLAME0.BIN to FLAME6.BIN),
 *  from the muzzle along the barrel: length and width (units); then its core (FLAME_CORE percent of its size) over it
 *  (the VDP1 blends at 50 %: the core is brighter). (The additive sprites of the VDP2 add to the VDP2 layers only, not to
 *  the 3D view drawn by the VDP1: the flame was opaque with dark edges) */
#define FLAME_LENGTH            (JO_FIXED_1 * 2)
#define FLAME_WIDTH             (JO_FIXED_1 * 7 / 2)
/** @brief The flame seen: its height at least this part of its width (percent) */
#define FLAME_SEEN_RATIO        (110)
#define FLAME_CORE              (55)
#define TRACER_SPEED            (2400)
#define TRACER_SIZE             (JO_FIXED_1 * 3 / 2)

typedef struct
{
    jo_pos3D_fixed              position;
    short                       yaw;        // Turn of the mark: flat on the wall hit (degrees)
    short                       pitch;      // 0 on a wall, -90 / 90 on a floor or a ceiling
    bool                        used;
    jo_entity                   *door;      // On a door: it moves with it (position: in the space of the panel)
    bool                        front;      // On a door: on the side of its normal
    short                       cell;       // Its cell (see jo_cells_object_cell(): found once, the mark doesn't move), -1: none
}                               impact;

/** @brief The marks on the walls in one mesh (a quad each, in the world: their corners are set when they are made), drawn
 *  cell by cell with the depth bias of the cell: one draw per cell instead of one per mark (a draw of a sprite cost about
 *  85 microseconds: 2 ms per frame for 24 marks). The marks on the doors move with them: drawn one by one */
static jo_3d_mesh               *impact_mesh = JO_NULL;
static void                     impact_mesh_make(const unsigned short gouraud_table);

static jo_3d_model              *model = JO_NULL;
static const jo_weapon_def      *def = JO_NULL;
/** @brief The weapons (cd/DEFS.JOL): their models, their shots, the ones the agent has (the pistol at the start; the
 *  rifle is picked up on the guards) */
#define MAX_WEAPONS             (4)
static jo_3d_model              *models[MAX_WEAPONS];
static jo_sound                 shot_sounds[MAX_WEAPONS];
static jo_sound                 reload_sounds[MAX_WEAPONS];
static bool                     owned[MAX_WEAPONS] = {true, false, false, false};
static bool                     switch_was_down = false;
static jo_sound                 sound_pickup;
static int                      tex_impact;
static jo_sound                 sound_shot;
static jo_sound                 sound_empty;
static jo_sound                 sound_reload;
/** @brief The impacts of the bullets: in the tiles or the concrete (2), on a steel door. A ricochet: an impact and the
 *  whine of the bullet going away, a tone of the SCSP (its pitch falls: the 68000 glides it, no sample in the sound RAM,
 *  JO_COMPILE_WITH_AUDIO_DRIVER_MODULE) */
#define HIT_SOUNDS              (2)
/** @brief 1 bullet in RICOCHET_CHANCE ricochets off a wall */
#define RICOCHET_CHANCE         (3)
/** @brief The impacts are heard up to there (units), their volume */
#define IMPACT_RANGE            (1400)
#define IMPACT_VOLUME           (110)
static jo_sound                 hit_sounds[HIT_SOUNDS];
static jo_sound                 sound_hit_metal;
static bool                     sounds_loaded = false;
static int                      shot_channel = 0;
static int                      impact_channel = 0;
/** @brief Motion of the weapon */
static jo_fixed                 lag_x = 0;
static jo_fixed                 recoil = 0;
static bool                     fire_was_down = false;
static impact                   impacts[MAX_IMPACTS];
static int                      next_impact = 0;
/** @brief The images of the flame turned (FLAME0.BIN to FLAME6.BIN): the first one, their count, the turn
 *  of the first one and the step (degrees, clockwise from up), the length of the flame in them (pixels) */
static int                      tex_flame = -1;
#define FLAME_IMAGES            (7)
#define FLAME_FIRST_ANGLE       (-80)
#define FLAME_ANGLE_STEP        (20)
#define FLAME_IMAGE_LENGTH      (48)
/** @brief The flame of this shot: its length (/ 256, at random) and a small turn (degrees) */
static int                      flame_scale = 256;
static int                      flame_turn = 0;
static int                      tex_tracer;
static int                      flash_frames = 0;
/** @brief The bullet in flight: its position (world), its move per frame, the frames left */
static jo_pos3D_fixed           tracer_position;
static jo_pos3D_fixed           tracer_step;
static int                      tracer_frames = 0;
/** @brief Where the weapon was drawn (view space): the muzzle of the next shot */
static jo_fixed                 drawn_x = 0;
static jo_fixed                 drawn_y = 0;
static jo_fixed                 drawn_z = 0;
static int                      drawn_turn_x = 0;

static int                      drawn_turn_y = 0;
/** @brief Distance along the direction of the last ray to what it hit (shoot_ray()) */
static jo_fixed                 last_ray_distance = 0;

void                            weapon_load(void)
{
    int                         k;
    jo_3d_quad                  *quad;

    for (k = 0; k < jo_weapons_count() && k < MAX_WEAPONS; ++k)
    {
        const jo_weapon_def     *d;

        d = jo_weapons_def(k);
        models[k] = d->model[0] != '\0' ? jo_3d_load_model(d->model) : JO_NULL;
#ifdef TEST_WEAPON_MIPMAPS
        if (models[k] != JO_NULL)
        {
            unsigned int        t;

            for (t = 0; t < models[k]->texture_count; ++t)
                jo_3d_set_texture_mipmaps(models[k]->textures[t]);
        }
#endif
        load_sound(d->sound_shot, &shot_sounds[k]);
        shot_sounds[k].volume = (unsigned char)JO_MIN(127, d->shot_volume);
    }
    def = jo_weapons_def(jo_weapons_current());
    model = models[jo_weapons_current()];
    tex_impact = jo_sprite_add_bin_4bits_lut(JO_ROOT_DIR, "IMPACT.BIN", JO_COLOR_Transparent);
    // (the quad of the impacts (the marks on the doors) and their mesh (the marks on the walls) with a neutral gouraud
    // table: the light of the lamps on them, a tint, see impacts_draw())
    if (tex_impact >= 0 && (quad = jo_3d_get_sprite_quad(tex_impact)) != JO_NULL)
    {
        static const jo_color   neutral[4] = {0xC210, 0xC210, 0xC210, 0xC210};
        unsigned short          table;

        table = jo_3d_add_gouraud_tables(neutral, 1);
        if (table != 0)
        {
            quad->data.attributes[0].draw_mode |= JO_VDP1_COLOR_CALC_GOURAUD;
            quad->data.attributes[0].gouraud_table = table;
        }
        impact_mesh_make(table);
    }
    {
        char                    name[16];

        // (the images follow each other: the id of the first one, then + 1 for each turn; 16 colors, the table of colors in the
        // place of the sprite)
        for (k = 0; k < FLAME_IMAGES; ++k)
        {
            name[0] = 'F'; name[1] = 'L'; name[2] = 'A'; name[3] = 'M'; name[4] = 'E';
            name[5] = (char)('0' + k);
            name[6] = '.'; name[7] = 'B'; name[8] = 'I'; name[9] = 'N'; name[10] = '\0';
            if (k == 0)
                tex_flame = jo_sprite_add_bin_4bits_lut(JO_ROOT_DIR, name, JO_COLOR_Transparent);
            else if (jo_sprite_add_bin_4bits_lut(JO_ROOT_DIR, name, JO_COLOR_Transparent) != tex_flame + k)
                tex_flame = -1;
        }
    }
    tex_tracer = jo_sprite_add_bin_4bits_lut(JO_ROOT_DIR, "TRACER.BIN", JO_COLOR_Transparent);
    sounds_loaded = def != JO_NULL;
    sound_shot = shot_sounds[jo_weapons_current()];
    load_sound("HIT1.WAV", &hit_sounds[0]);
    load_sound("HIT2.WAV", &hit_sounds[1]);
    load_sound("HITM.WAV", &sound_hit_metal);
    load_sound("EMPTY.WAV", &sound_empty);
    load_sound("PICKUP.WAV", &sound_pickup);
    // (the impacts: more of the room, as their echo was in their sounds; in the hands of the agent: little of the room)
    hit_sounds[0].reverb = 6;
    hit_sounds[1].reverb = 6;
    sound_empty.reverb = 3;
    sound_pickup.reverb = 3;
    // (the reloads last: if the sound RAM is full, they are copied there when they play)
    for (k = 0; k < jo_weapons_count() && k < MAX_WEAPONS; ++k)
    {
        int                     j;

        // (the weapons with the same reload share its sound: loaded once)
        for (j = 0; j < k && jo_strcmp(jo_weapons_def(j)->sound_reload, jo_weapons_def(k)->sound_reload) != 0; ++j)
            ;
        if (j < k)
            reload_sounds[k] = reload_sounds[j];
        else
        {
            load_sound(jo_weapons_def(k)->sound_reload, &reload_sounds[k]);
            reload_sounds[k].reverb = 3;
        }
    }
    sound_reload = reload_sounds[jo_weapons_current()];
    for (k = 0; k < MAX_IMPACTS; ++k)
        impacts[k].used = false;
}

static void                     play_shot(void)
{
    if (!sounds_loaded)
        return;
    play_sound(&sound_shot, CHANNEL_SHOT_FIRST, CHANNEL_SHOT_COUNT, &shot_channel);
}

#ifdef JO_COMPILE_WITH_AUDIO_DRIVER_SUPPORT

/** @brief The whine of a ricochet (with its impact): a sine from 2600 - 3400 Hz falling to half of it (the Doppler effect
 *  of the bullet going away), a little FM (its second harmonic), the bullet tumbling (a tremolo of 38 or 48 Hz) */
static void                     play_ricochet(const jo_sound * const hit)
{
    jo_audio_tone               tone;

    jo_memset(&tone, 0, sizeof(tone));
    tone.frequency = (unsigned short)(2600 + jo_random(800));
    tone.end_frequency = (unsigned short)(tone.frequency * (44 + jo_random(16)) / 100);
    tone.glide_ms = (unsigned short)(120 + jo_random(80));
    tone.decay_ms = (unsigned short)(1100 + jo_random(400));
    // (5 dB under the impact)
    tone.volume = (unsigned char)JO_MAX(0, hit->volume - 7);
    tone.pan = hit->pan;
    tone.reverb = hit->reverb;
    tone.modulation = 8;
    tone.modulator_ratio = 4;
    tone.tremolo = 6;
    tone.tremolo_rate = (unsigned char)(27 + jo_random(2));
    tone.waveform = JoToneSine;
    jo_audio_stop_channel(CHANNEL_RICOCHET);
    jo_audio_play_tone(&tone, CHANNEL_RICOCHET);
}

#endif

/** @brief The sound of a bullet on a wall (a ricochet sometimes) or a door (steel), from where it hits */
static void                     play_impact(const jo_pos3D_fixed * const p, const bool metal)
{
    jo_sound                    *sound;

    if (!sounds_loaded)
        return;
    if (metal)
        sound = &sound_hit_metal;
    else
        sound = &hit_sounds[jo_random(HIT_SOUNDS) - 1];
    sound_from(sound, p->x, p->z, IMPACT_VOLUME, IMPACT_RANGE);
    if (sound->volume > 0)
    {
        play_sound(sound, CHANNEL_IMPACT_FIRST, CHANNEL_IMPACT_COUNT, &impact_channel);
#ifdef JO_COMPILE_WITH_AUDIO_DRIVER_SUPPORT
        if (!metal && jo_random(RICOCHET_CHANCE) == 1)
            play_ricochet(sound);
#endif
    }
}

/** @brief The mesh of the marks on the walls (see impact_mesh): made once the texture is loaded */
static void                     impact_mesh_make(const unsigned short gouraud_table)
{
    int                         k;

    if (impact_mesh != JO_NULL || tex_impact < 0)
        return;
    impact_mesh = jo_3d_create_indexed_mesh(MAX_IMPACTS * 4, MAX_IMPACTS);
    if (impact_mesh == JO_NULL)
        return;
    for (k = 0; k < MAX_IMPACTS; ++k)
    {
        impact_mesh->data.polygons[k].vertices[0] = (unsigned short)(k * 4);
        impact_mesh->data.polygons[k].vertices[1] = (unsigned short)(k * 4 + 1);
        impact_mesh->data.polygons[k].vertices[2] = (unsigned short)(k * 4 + 2);
        impact_mesh->data.polygons[k].vertices[3] = (unsigned short)(k * 4 + 3);
    }
    jo_3d_set_mesh_texture(impact_mesh, tex_impact);
    jo_3d_set_mesh_light(impact_mesh, false);
    jo_3d_set_mesh_screen_doors(impact_mesh, false);
    // (seen from both sides, like the sprite: a mark on a thin wall is seen from the other side too; the neutral gouraud
    // table of the sprite: the tint of the light of the lamps)
    for (k = 0; k < MAX_IMPACTS; ++k)
    {
        impact_mesh->data.attributes[k].plane = JO_3D_DUAL_PLANE;
        if (gouraud_table != 0)
        {
            impact_mesh->data.attributes[k].draw_mode |= JO_VDP1_COLOR_CALC_GOURAUD;
            impact_mesh->data.attributes[k].gouraud_table = gouraud_table;
        }
    }
}

/** @brief The quad of a mark on a wall in the mesh: the corners of the sprite (16 x 16, IMPACT_SIZE in the world) turned
 *  by its yaw and its pitch, as the matrix of the sprite was (turned around Y, then around X) */
static void                     impact_mesh_set(const int k)
{
    const impact                *m;
    jo_fixed                    half;
    jo_fixed                    sy;
    jo_fixed                    cy;
    jo_fixed                    sp;
    jo_fixed                    cp;
    jo_pos3D_fixed              *p;
    int                         c;
    jo_fixed                    x;
    jo_fixed                    y;
    jo_fixed                    yy;
    jo_fixed                    zz;

    if (impact_mesh == JO_NULL)
        return;
    m = &impacts[k];
    half = IMPACT_SIZE / 2;
    sy = jo_sin(m->yaw);
    cy = jo_cos(m->yaw);
    sp = jo_sin(m->pitch);
    cp = jo_cos(m->pitch);
    p = impact_mesh->data.points + k * 4;
    // (the corners of the sprite: (-h, -h), (h, -h), (h, h), (-h, h) in its plane, z = 0)
    for (c = 0; c < 4; ++c)
    {
        x = c == 1 || c == 2 ? half : -half;
        y = c >= 2 ? half : -half;
        // Around X by the pitch (jo_3d_rotate_matrix_x: y' = y cos - z sin, z' = y sin + z cos, z = 0)
        yy = jo_fixed_mult(y, cp);
        zz = jo_fixed_mult(y, sp);
        // Around Y by the yaw (jo_3d_rotate_matrix_y: x' = x cos + z sin, z' = -x sin + z cos)
        p[c].x = m->position.x + jo_fixed_mult(x, cy) + jo_fixed_mult(zz, sy);
        p[c].y = m->position.y + yy;
        p[c].z = m->position.z - jo_fixed_mult(x, sy) + jo_fixed_mult(zz, cy);
    }
}

static void                     add_impact(const jo_pos3D_fixed * const p, const jo_pos3D_fixed * const normal, jo_entity * const door)
{
    impacts[next_impact].door = door;
    if (door != JO_NULL)
    {
        // In the space of the panel: drawn where the door is
        doors_to_local(door, p, &impacts[next_impact].position);
        impacts[next_impact].front = impacts[next_impact].position.z > 0;
        impacts[next_impact].pitch = 0;
        impacts[next_impact].cell = -1;
        impacts[next_impact].used = true;
        next_impact = (next_impact + 1) % MAX_IMPACTS;
        return;
    }
    impacts[next_impact].position = *p;
    impacts[next_impact].cell = (short)jo_cells_object_cell(p->x, p->y, p->z);
    // The mark lies on the surface hit: a sprite faces -Z, turned by the yaw it faces (-sin yaw, 0, -cos yaw)
    if (JO_ABS(normal->y) > JO_FIXED_1 / 2)
    {
        impacts[next_impact].yaw = 0;
        impacts[next_impact].pitch = normal->y < 0 ? -90 : 90;
    }
    else
    {
        impacts[next_impact].yaw = jo_atan2_degrees(-normal->x, -normal->z);
        impacts[next_impact].pitch = 0;
    }
    impact_mesh_set(next_impact);
    impacts[next_impact].used = true;
    next_impact = (next_impact + 1) % MAX_IMPACTS;
}

bool                            shoot_ray(const jo_pos3D_fixed * const origin, const jo_pos3D_fixed * const dir, const int damage,
                                          jo_entity * const source)
{
    jo_fixed                    t_wall;
    jo_fixed                    t_entity;
    jo_pos3D_fixed              normal;
    jo_pos3D_fixed              hit;
    jo_entity                   *e;
    bool                        wall;
    jo_fixed                    t_door;
    jo_pos3D_fixed              door_normal;
    jo_entity                   *door;

    t_wall = jo_int2fixed(BULLET_RANGE);
    wall = jo_cells_ray_hit(origin, dir, jo_int2fixed(BULLET_RANGE), &t_wall, &normal);
    // A closed door is a wall too
    door = JO_NULL;
    t_door = doors_ray_hit(origin, dir, wall ? t_wall : jo_int2fixed(BULLET_RANGE), &door_normal, &door, false);
    if (t_door < 0)
        door = JO_NULL;
    else
    {
        wall = true;
        t_wall = t_door;
        normal = door_normal;
    }
    // A pane of a window: it breaks, the bullet stops there
    {
        jo_entity               *glass;
        jo_pos3D_fixed          glass_normal;
        jo_fixed                t_glass;

        glass = JO_NULL;
        t_glass = doors_glass_ray_hit(origin, dir, wall ? t_wall : jo_int2fixed(BULLET_RANGE), &glass_normal, &glass);
        if (t_glass >= 0 && jo_entities_ray_hit(origin, dir, t_glass, &t_entity, &normal) == JO_NULL)
        {
            last_ray_distance = t_glass;
            doors_glass_shot(glass, dir);
            return (true);
        }
    }
    last_ray_distance = wall ? t_wall : jo_int2fixed(BULLET_RANGE);
    e = jo_entities_ray_hit(origin, dir, wall ? t_wall : jo_int2fixed(BULLET_RANGE), &t_entity, &normal);
    // The cap of a guard in his fall (he is not solid any more): it flies off his head, the bullet stops there (the shots of
    // the agent)
    if (source == JO_NULL)
    {
        jo_fixed                t_cap;

        t_cap = guards_cap_ray_hit(origin, dir, e != JO_NULL ? t_entity : last_ray_distance, true);
        if (t_cap >= 0)
        {
            last_ray_distance = t_cap;
            return (true);
        }
    }
    if (e != JO_NULL && e != source)
    {
        last_ray_distance = t_entity;
        // (a guard: where he is hit, the head; his blood, his cry, see guards.c)
        hit.x = origin->x + jo_fixed_mult(dir->x, t_entity);
        hit.y = origin->y + jo_fixed_mult(dir->y, t_entity);
        hit.z = origin->z + jo_fixed_mult(dir->z, t_entity);
        if (!guards_shot(e, &hit, dir, damage, source) && damage > 0)
            jo_entity_damage(e, damage, source);
        return (true);
    }
    if (wall)
    {
        // The mark a little in front of the wall
        hit.x = origin->x + jo_fixed_mult(dir->x, t_wall) + normal.x;
        hit.y = origin->y + jo_fixed_mult(dir->y, t_wall) + normal.y;
        hit.z = origin->z + jo_fixed_mult(dir->z, t_wall) + normal.z;
        add_impact(&hit, &normal, door);
        play_impact(&hit, door != JO_NULL);
        return (true);
    }
    return (false);
}

/** @brief A point of the view space (x right, y down, z forward) in the world: turned by the pitch and the yaw of the camera
 *  (as player_aim_direction()), from the eyes */
static void                     view_to_world(const jo_pos3D_fixed * const eye, const jo_fixed vx, const jo_fixed vy, const jo_fixed vz,
                                              jo_pos3D_fixed * const w)
{
    jo_fixed                    cp;
    jo_fixed                    sp;
    jo_fixed                    cy;
    jo_fixed                    sy;
    jo_fixed                    fy;
    jo_fixed                    fz;

    cp = jo_cos(pitch);
    sp = jo_sin(pitch);
    cy = jo_cos(yaw);
    sy = jo_sin(yaw);
    fy = jo_fixed_mult(vy, cp) + jo_fixed_mult(vz, sp);
    fz = jo_fixed_mult(vz, cp) - jo_fixed_mult(vy, sp);
    w->x = eye->x + jo_fixed_mult(vx, cy) + jo_fixed_mult(fz, sy);
    w->y = eye->y + fy;
    w->z = eye->z + jo_fixed_mult(fz, cy) - jo_fixed_mult(vx, sy);
}

void                            camera_view_to_world(const jo_fixed vx, const jo_fixed vy, const jo_fixed vz, jo_pos3D_fixed * const w)
{
    jo_pos3D_fixed              eye;

    player_eye(&eye);
    view_to_world(&eye, vx, vy, vz, w);
}

/** @brief The muzzle of the weapon where it was drawn (view space: x right, y down, z forward): the end of the silencer
 *  turned by the pose of the weapon, and the direction of the barrel (a unit vector) */
static void                     muzzle_view(jo_pos3D_fixed * const muzzle, jo_pos3D_fixed * const barrel)
{
    jo_fixed                    my;
    jo_fixed                    mz;
    jo_fixed                    cx;
    jo_fixed                    sx;
    jo_fixed                    cy;
    jo_fixed                    sy;

    // (turned around X then Y, as the matrix of weapon_draw(): the muzzle is (0, muzzle_y, muzzle_z) in the model)
    cx = jo_cos(drawn_turn_x);
    sx = jo_sin(drawn_turn_x);
    cy = jo_cos(drawn_turn_y);
    sy = jo_sin(drawn_turn_y);
    my = jo_fixed_mult(def->muzzle_y, cx) - jo_fixed_mult(def->muzzle_z, sx);
    mz = jo_fixed_mult(def->muzzle_y, sx) + jo_fixed_mult(def->muzzle_z, cx);
    muzzle->x = drawn_x + jo_fixed_mult(mz, sy);
    muzzle->y = drawn_y + my;
    muzzle->z = drawn_z + jo_fixed_mult(mz, cy);
    barrel->x = jo_fixed_mult(cx, sy);
    barrel->y = -sx;
    barrel->z = jo_fixed_mult(cx, cy);
}

/** @brief The muzzle of the weapon where it was drawn (world) */
static void                     muzzle_position(const jo_pos3D_fixed * const eye, jo_pos3D_fixed * const muzzle)
{
    jo_pos3D_fixed              m;
    jo_pos3D_fixed              barrel;

    muzzle_view(&m, &barrel);
    view_to_world(eye, m.x, m.y, m.z, muzzle);
}

/** @brief The flame: from the muzzle along the barrel, on the screen (an image turned along the barrel seen, half transparent) */
static void                     flame_draw(void)
{
    jo_pos3D_fixed              m;
    jo_pos3D_fixed              barrel;
    jo_fixed                    projection;
    jo_fixed                    tip_x;
    jo_fixed                    tip_y;
    jo_fixed                    tip_z;
    jo_fixed                    x0;
    jo_fixed                    y0;
    jo_fixed                    x1;
    jo_fixed                    y1;
    jo_fixed                    length;
    jo_fixed                    dx;
    jo_fixed                    dy;
    jo_fixed                    half_width;
    jo_fixed                    half_height;
    int                         angle;
    int                         image;

    if (tex_flame < 0)
        return;
    muzzle_view(&m, &barrel);
    length = FLAME_LENGTH / 256 * flame_scale;
    tip_x = m.x + jo_fixed_mult(barrel.x, length);
    tip_y = m.y + jo_fixed_mult(barrel.y, length);
    tip_z = m.z + jo_fixed_mult(barrel.z, length);
    if (m.z < JO_FIXED_8 || tip_z < JO_FIXED_8)
        return;
    // The base and the tip on the screen (pixels from the middle)
    projection = jo_3d_get_projection_distance();
    x0 = jo_fixed_mult(jo_fixed_div(m.x, m.z), projection);
    y0 = jo_fixed_mult(jo_fixed_div(m.y, m.z), projection);
    x1 = jo_fixed_mult(jo_fixed_div(tip_x, tip_z), projection);
    y1 = jo_fixed_mult(jo_fixed_div(tip_y, tip_z), projection);
    dx = x1 - x0;
    dy = y1 - y0;
    // (the barrel points into the screen: the flame seen is short, it is never shorter than FLAME_SEEN_RATIO of its
    // width, a burst around the muzzle)
    half_width = jo_fixed_mult(jo_fixed_div(FLAME_WIDTH, m.z), projection) / 2 * flame_scale / 256;
    half_height = jo_fixed_sqrt(jo_fixed_mult(dx, dx) + jo_fixed_mult(dy, dy)) / 2;
    half_height = JO_MAX(half_height, half_width * FLAME_SEEN_RATIO / 100);
    // (the direction of the flame on the screen, a unit vector: from the muzzle toward the point hit, seen at the
    // crosshair (the barrel points a little beside it: the pose of the weapon); the base of the image on the muzzle)
    dx = jo_int2fixed(aiming ? aim_x : 0) - x0;
    dy = jo_int2fixed(aiming ? aim_y : 0) - y0;
    length = jo_fixed_sqrt(jo_fixed_mult(dx, dx) + jo_fixed_mult(dy, dy));
    if (length > JO_FIXED_1)
    {
        dx = jo_fixed_div(dx, length);
        dy = jo_fixed_div(dy, length);
    }
    else
    {
        dx = 0;
        dy = -JO_FIXED_1;
    }
    // The image turned nearest to the barrel seen (clockwise from up), scaled to the length of the flame, its middle half
    // the flame from the muzzle
    angle = jo_atan2_degrees(dx, -dy) + flame_turn;
    if (angle > 180)
        angle -= 360;
    image = (angle - FLAME_FIRST_ANGLE + FLAME_ANGLE_STEP / 2) / FLAME_ANGLE_STEP;
    if (angle < FLAME_FIRST_ANGLE)
        image = 0;
    image = JO_MAX(0, JO_MIN(FLAME_IMAGES - 1, image));
    jo_sprite_enable_half_transparency();
    jo_sprite_change_sprite_scale_xy_fixed(2 * half_height / FLAME_IMAGE_LENGTH, 2 * half_height / FLAME_IMAGE_LENGTH);
    jo_sprite_draw3D(tex_flame + image, jo_fixed2int(x0 + jo_fixed_mult(dx, half_height)), jo_fixed2int(y0 + jo_fixed_mult(dy, half_height)),
                     jo_fixed2int(m.z) - 1);
    // (the core: smaller, over it)
    half_height = half_height * FLAME_CORE / 100;
    jo_sprite_change_sprite_scale_xy_fixed(2 * half_height / FLAME_IMAGE_LENGTH, 2 * half_height / FLAME_IMAGE_LENGTH);
    jo_sprite_draw3D(tex_flame + image, jo_fixed2int(x0 + jo_fixed_mult(dx, half_height)), jo_fixed2int(y0 + jo_fixed_mult(dy, half_height)),
                     jo_fixed2int(m.z) - 2);
    jo_sprite_change_sprite_scale_xy_fixed(JO_FIXED_1, JO_FIXED_1);
    jo_sprite_disable_half_transparency();
}

/** @brief The flame at the muzzle, and the bullet flying from the muzzle to the point hit by the last ray */
static void                     start_flash(const jo_pos3D_fixed * const eye, const jo_pos3D_fixed * const dir)
{
    jo_pos3D_fixed              muzzle;
    jo_pos3D_fixed              hit;
    jo_fixed                    distance;
    int                         count;

    flash_frames = JO_MAX(1, FLASH_FRAMES);
    muzzle_position(eye, &muzzle);
    // (each flame a little different: its length and a small turn)
    flame_scale = 210 + (jo_random(92) - 1);
    flame_turn = (jo_random(21) - 1) - 10;
    hit.x = eye->x + jo_fixed_mult(dir->x, last_ray_distance);
    hit.y = eye->y + jo_fixed_mult(dir->y, last_ray_distance);
    hit.z = eye->z + jo_fixed_mult(dir->z, last_ray_distance);
    // (the frames of the flight: the distance at the speed of the bullet, whole units: no overflow of the squares)
    distance = jo_fixed2int(last_ray_distance);
    count = JO_MAX(1, distance * GAME_FPS / TRACER_SPEED);
    tracer_step.x = (hit.x - muzzle.x) / count;
    tracer_step.y = (hit.y - muzzle.y) / count;
    tracer_step.z = (hit.z - muzzle.z) / count;
    // (half a step from the muzzle: the flame is there)
    tracer_position.x = muzzle.x + tracer_step.x / 2;
    tracer_position.y = muzzle.y + tracer_step.y / 2;
    tracer_position.z = muzzle.z + tracer_step.z / 2;
    tracer_frames = count;
}

/** @brief The agent fires: the ray through the crosshair, with the spread of the weapon */
static void                     fire(void)
{
    jo_pos3D_fixed              eye;
    jo_pos3D_fixed              dir;
    int                         spread;

    player_eye(&eye);
    player_aim_direction(&dir);
    spread = def != JO_NULL ? def->spread : 0;
    if (spread > 0)
    {
        // (the offset at 1000 units: a random part of it, perpendicular to the view)
        dir.x += ((jo_random(2 * spread + 1) - 1) - spread) * 65536 / 1000;
        dir.y += ((jo_random(2 * spread + 1) - 1) - spread) * 65536 / 1000;
    }
    // The aim assistance: without the aiming mode, a guard near the middle of the view is hit in the
    // chest (in the aiming mode the agent aims himself: the head)
    if (!aiming)
        guards_auto_aim(&eye, &dir);
    shoot_ray(&eye, &dir, def != JO_NULL ? def->damage : 25, JO_NULL);
    play_shot();
    // (the guards near the agent hear the shot, the guards near the bullet too; a rifle without a silencer: the whole wing)
    guards_hear_shot(&eye, &dir, last_ray_distance, def != JO_NULL && def->automatic);
    // Some shots: the flame at the muzzle and the bullet seen flying to what it hit
    // (an automatic rifle without a silencer: the flame at each shot)
    if (def != JO_NULL && (def->automatic || jo_random(FLASH_CHANCE) == 1))
        start_flash(&eye, &dir);
    recoil = RECOIL_BACK;
}

/** @brief The reload asked by the player (B), done at the next update */
static bool                     reload_requested = false;

bool                            weapon_pickup(const char * const name)
{
    int                         k;
    const jo_weapon_def         *d;

    for (k = 0; k < jo_weapons_count() && k < MAX_WEAPONS; ++k)
    {
        d = jo_weapons_def(k);
        if (jo_strcmp(d->name, name) != 0)
            continue;
        jo_audio_play_sound_on_channel(&sound_pickup, CHANNEL_PICKUP);
        if (!owned[k])
        {
            // A new weapon (its magazine is full)
            owned[k] = true;
            show_message(k == 1 ? "PICKED UP AN AK-47." : "PICKED UP A WEAPON.");
            return (true);
        }
        // Already in the hands: its ammunition
        jo_weapons_give_ammo(d->ammo_type, d->magazine);
        show_message("PICKED UP SOME AMMO.");
        return (true);
    }
    return (false);
}

void                            weapon_request_reload(void)
{
    reload_requested = true;
}

/** @brief The weapon turns toward the guard the aim assistance would hit: the point of his chest at the
 *  convergence distance in the view (x right, y down), followed smoothly (LOCK_FOLLOW per frame at 30 frames per second) */
#define LOCK_FOLLOW_PERCENT     (30)
static jo_fixed                 lock_x = 0;
static jo_fixed                 lock_y = 0;

static void                     update_lock(void)
{
    jo_pos3D_fixed              eye;
    jo_pos3D_fixed              dir;
    jo_fixed                    tx;
    jo_fixed                    ty;
    jo_fixed                    fz;
    jo_fixed                    vx;
    jo_fixed                    vy;
    jo_fixed                    vz;
    jo_fixed                    cp;
    jo_fixed                    sp;
    jo_fixed                    convergence;
    int                         follow;

    tx = 0;
    ty = 0;
    if (!aiming && def != JO_NULL)
    {
        player_eye(&eye);
        player_aim_direction(&dir);
        if (guards_auto_aim(&eye, &dir))
        {
            // (the world direction in the view: the inverse of view_to_world())
            fz = jo_fixed_mult(dir.x, jo_sin(yaw)) + jo_fixed_mult(dir.z, jo_cos(yaw));
            vx = jo_fixed_mult(dir.x, jo_cos(yaw)) - jo_fixed_mult(dir.z, jo_sin(yaw));
            cp = jo_cos(pitch);
            sp = jo_sin(pitch);
            vz = jo_fixed_mult(dir.y, sp) + jo_fixed_mult(fz, cp);
            vy = jo_fixed_mult(dir.y, cp) - jo_fixed_mult(fz, sp);
            if (vz > JO_FIXED_1 / 4)
            {
                convergence = jo_int2fixed(def->convergence);
                tx = jo_fixed_mult(jo_fixed_div(vx, vz), convergence);
                ty = jo_fixed_mult(jo_fixed_div(vy, vz), convergence);
                // (15 degrees at most: a guard next to the agent)
                tx = JO_MAX(-convergence * 27 / 100, JO_MIN(convergence * 27 / 100, tx));
                ty = JO_MAX(-convergence * 27 / 100, JO_MIN(convergence * 27 / 100, ty));
            }
        }
    }
    follow = JO_MIN(100, LOCK_FOLLOW_PERCENT * 30 / GAME_FPS);
    lock_x += (tx - lock_x) / 100 * follow;
    lock_y += (ty - lock_y) / 100 * follow;
}

void                            weapon_update(void)
{
    bool                        fire_down;
    bool                        reload;
    unsigned int                events;

    // The weapon in the hands (it changes when the previous one is down)
    def = jo_weapons_def(jo_weapons_current());
    model = models[jo_weapons_current()];
    sound_shot = shot_sounds[jo_weapons_current()];
    sound_reload = reload_sounds[jo_weapons_current()];
    // X: the previous weapon, Y: the next one (of the ones the agent has; X + Y + Z together: the wireframe mode)
    {
        bool                    x;
        bool                    y;
        int                     step;
        int                     k;
        int                     n;

        x = jo_is_pad1_key_pressed(JO_KEY_X);
        y = jo_is_pad1_key_pressed(JO_KEY_Y);
        step = 0;
        if (!switch_was_down && !jo_is_pad1_key_pressed(JO_KEY_Z) && (x != y))
            step = x ? -1 : 1;
        switch_was_down = x || y;
        n = JO_MIN(jo_weapons_count(), MAX_WEAPONS);
        for (k = 1; step != 0 && k < n; ++k)
        {
            int                 index;

            index = (jo_weapons_current() + step * k + n * k) % n;
            if (owned[index])
            {
                jo_weapons_select(index);
                break;
            }
        }
    }
    fire_down = jo_is_pad1_key_pressed(JO_KEY_A);
    // The reload: automatic when the magazine is empty and the trigger is pulled, or asked (B) when the magazine is not full
    reload = fire_down && !fire_was_down && jo_weapons_ammo() == 0 && jo_weapons_reserve() > 0;
    if (reload_requested && def != JO_NULL && jo_weapons_ammo() < def->magazine && jo_weapons_reserve() > 0 &&
        jo_weapons_reload_progress() == 0)
        reload = true;
    reload_requested = false;
    events = jo_weapons_update(fire_down, fire_down && !fire_was_down, reload, false);
    fire_was_down = fire_down;
    if (events & JO_WEAPON_EVENT_FIRE)
        fire();
    if ((events & JO_WEAPON_EVENT_EMPTY) && sounds_loaded && !reload)
        jo_audio_play_sound_on_channel(&sound_empty, CHANNEL_PLAYER);
    if ((events & JO_WEAPON_EVENT_RELOAD) && sounds_loaded)
        jo_audio_play_sound_on_channel(&sound_reload, CHANNEL_PLAYER);
    // The flame and the bullet in flight
    if (flash_frames > 0 && !(events & JO_WEAPON_EVENT_FIRE))
        --flash_frames;

    if (tracer_frames > 0 && !(events & JO_WEAPON_EVENT_FIRE))
    {
        --tracer_frames;
        tracer_position.x += tracer_step.x;
        tracer_position.y += tracer_step.y;
        tracer_position.z += tracer_step.z;
    }
    update_lock();
    // The lag behind the turns and the recoil fade
    lag_x += player_turn_delta() * LAG_PER_DEGREE;
    lag_x = JO_MAX(-LAG_MAX, JO_MIN(LAG_MAX, lag_x));
    // (per second: the same motion at 30 or 60 frames per second)
    lag_x -= lag_x * 8 / GAME_FPS;
    recoil -= recoil * 10 / GAME_FPS;
}

void                            weapon_draw(void)
{
    jo_fixed                    bob;
    jo_fixed                    amount;
    jo_fixed                    x;
    jo_fixed                    y;
    jo_fixed                    z;
    jo_fixed                    reload_drop;
    jo_fixed                    target_x;
    jo_fixed                    target_y;
    jo_fixed                    convergence;
    int                         progress;
    jo_fixed                    projection;

    if (model == JO_NULL || def == JO_NULL)
        return;
    // In the aiming mode the weapon points at the red crosshair: the target is the point of the line of sight through the
    // crosshair at the convergence distance (view space: x right, y down); the weapon also slides a little toward it
    projection = jo_3d_get_projection_distance();
    convergence = jo_int2fixed(def->convergence);
    // (without the aiming mode: the guard the aim assistance would hit, see update_lock())
    target_x = aiming ? jo_fixed_mult(jo_fixed_div(jo_int2fixed(aim_x), projection), convergence) : lock_x;
    target_y = aiming ? jo_fixed_mult(jo_fixed_div(jo_int2fixed(aim_y), projection), convergence) : lock_y;
    bob = player_bob();
    amount = player_walk_amount();
    // The weapon moves on a figure of eight: x with the phase, y twice as fast (up at each step)
    x = def->x + jo_fixed_mult(jo_fixed_mult(bob, BOB_X), amount) + lag_x
        + jo_fixed_mult(target_x, def->z) / def->convergence * AIM_FOLLOW_PERCENT / 100;
    y = def->y + jo_fixed_mult(jo_fixed_mult(JO_ABS(bob), BOB_Y), amount) - jo_fixed_mult(recoil, JO_FIXED_1 / 4)
        + jo_fixed_mult(target_y, def->z) / def->convergence * AIM_FOLLOW_PERCENT / 100;
    // Breathing: the weapon moves a little all the time, up and down in 3 s, a slow drift
    // on the side (the time: the frames at GAME_FPS)
    x += jo_sin((frame_counter * 2 * 30 / GAME_FPS + 90) % 360) * BREATH_X / 1000;
    y += jo_sin((frame_counter * 4 * 30 / GAME_FPS) % 360) * BREATH_Y / 1000;
    // (the kick: back a little and up, the hand at the bottom of the screen stays in view)
    z = def->z - recoil / 2;
    // The reload: the weapon goes down and comes back
    progress = jo_weapons_reload_progress();
    if (progress > 0)
    {
        reload_drop = jo_int2fixed(JO_MIN(progress, 256 - progress) * 30 / 128);
        y += reload_drop;
    }
    // The change of weapon: it goes out of the view
    y += jo_int2fixed(jo_weapons_lowered() * 50 / 256);
    jo_3d_push_matrix();
    {
        jo_3d_set_depth_bias(WEAPON_DEPTH_BIAS);
        jo_3d_translate_matrix_fixed(x, y, z);
        // The barrel points at the crosshair (the target at the convergence distance), a little up at the recoil
        // (small angles: the ratio of the offset to the distance, in degrees x 57)
        // (a rifle: no pose turn, its barrel on the crosshair (the middle of the screen out of the aiming mode), the same
        // pose with and without the aiming mode)
        drawn_turn_y = -jo_fixed2int(jo_fixed_div(x - target_x, convergence) * 57) + (def->automatic ? 0 : POSE_YAW);
        jo_3d_rotate_matrix_y(drawn_turn_y);
        // (a positive turn around X raises the barrel: the weapon is under the eyes, it points up at the target)
        drawn_turn_x = jo_fixed2int(jo_fixed_div(y - target_y, convergence) * 57) + jo_fixed2int(recoil) + (def->automatic ? 0 : POSE_PITCH);
        jo_3d_rotate_matrix_x(drawn_turn_x);
        // (the light of the lamps on the weapon (the red alarm): at the eyes of the agent)
        {
            jo_pos3D_fixed      eye;
            int                 r;
            int                 g;
            int                 b;

            player_eye(&eye);
            jo_3d_point_lights_sample(&eye, &r, &g, &b);
            jo_3d_set_tint(r, g, b);
        }
        jo_3d_model_draw(model);
        jo_3d_set_tint(0, 0, 0);
        jo_3d_set_depth_bias(WORLD_DEPTH_BIAS);
    }
    jo_3d_pop_matrix();
    drawn_x = x;
    drawn_y = y;
    drawn_z = z;
    if (flash_frames > 0)
        flame_draw();
}

/** @brief The bullet in flight (world space, in draw_world()) */
static void                     tracer_draw(void)
{
    int                         bias;

    if (tracer_frames <= 0 || tex_tracer < 0)
        return;
    bias = jo_cells_depth_bias_at(tracer_position.x, tracer_position.y, tracer_position.z);
    if (bias < 0)
        return;
    jo_3d_push_matrix();
    {
        jo_3d_set_depth_bias(WORLD_DEPTH_BIAS + bias - IMPACT_SORT_BIAS);
        jo_3d_select_window(jo_cells_window_of(bias));
        jo_3d_translate_matrix_fixed(tracer_position.x, tracer_position.y, tracer_position.z);
        // (facing the camera)
        jo_3d_rotate_matrix_y(yaw);
        jo_3d_rotate_matrix_x(-pitch);
        jo_3d_set_scale_fixed(TRACER_SIZE / 8, TRACER_SIZE / 8, JO_FIXED_1);
        jo_3d_draw_sprite(tex_tracer);
    }
    jo_3d_pop_matrix();
    jo_3d_select_window(JO_3D_NEAR_WINDOW);
    jo_3d_set_depth_bias(WORLD_DEPTH_BIAS);
}

void                            impacts_draw(void)
{
    int                         k;
    int                         bias;
    jo_3d_polygon_range         ranges[MAX_IMPACTS];
    int                         range_count;
    int                         cell;
    bool                        drawn[MAX_IMPACTS];
    int                         j;

    tracer_draw();
    if (tex_impact < 0)
        return;
    // The marks on the walls: one draw per cell seen (the marks of the cell, its depth bias)
    if (impact_mesh != JO_NULL)
    {
        for (k = 0; k < MAX_IMPACTS; ++k)
            drawn[k] = !impacts[k].used || impacts[k].door != JO_NULL;
        for (k = 0; k < MAX_IMPACTS; ++k)
        {
            if (drawn[k])
                continue;
            cell = impacts[k].cell;
            bias = jo_cells_depth_bias_of(cell);
            range_count = 0;
            for (j = k; j < MAX_IMPACTS; ++j)
                if (!drawn[j] && impacts[j].cell == cell)
                {
                    drawn[j] = true;
                    ranges[range_count].first = (unsigned short)j;
                    ranges[range_count++].count = 1;
                }
            if (bias < 0)
                continue;
            // (sorted before the wall, see IMPACT_SORT_BIAS; the light of the lamps at the first mark of the cell: the red
            // alarm is the same in the whole room)
            {
                int             r;
                int             g;
                int             b;

                jo_3d_point_lights_sample(&impacts[k].position, &r, &g, &b);
                jo_3d_set_tint(r, g, b);
            }
            jo_3d_set_depth_bias(WORLD_DEPTH_BIAS + bias - IMPACT_SORT_BIAS);
            // (in a room seen through a door: the far window, clipped to the door)
            jo_3d_select_window(jo_cells_window_of(bias));
            jo_3d_mesh_draw_ranges(impact_mesh, impact_mesh->data.point_count, ranges, range_count);
            jo_3d_select_window(JO_3D_NEAR_WINDOW);
            jo_3d_set_tint(0, 0, 0);
        }
        jo_3d_set_depth_bias(WORLD_DEPTH_BIAS);
    }
    for (k = 0; k < MAX_IMPACTS; ++k)
    {
        if (!impacts[k].used || (impacts[k].door == JO_NULL && impact_mesh != JO_NULL))
            continue;
        // (on a door: the cell the door is drawn in (the cell of its hinge was the room behind it when it is closed: the
        // impacts were hidden), its position is in the space of the panel)
        if (impacts[k].door != JO_NULL)
            bias = doors_depth_bias(impacts[k].door);
        else
            bias = jo_cells_depth_bias_of(impacts[k].cell);
        if (bias < 0)
            continue;
        jo_3d_push_matrix();
        {
            // A small mark flat on the surface hit, sorted before the wall (the sort uses the nearest vertex of a polygon: the
            // nearest corner of a wall quad is up to a few dozen units nearer than the mark)
            jo_3d_set_depth_bias(WORLD_DEPTH_BIAS + bias - IMPACT_SORT_BIAS);
            jo_3d_select_window(jo_cells_window_of(bias));
            if (impacts[k].door != JO_NULL)
            {
                // On a door: where the panel is now, flat on it (the sprite faces -Z: the side of the shooter)
                jo_pos3D_fixed  w;
                int             door_yaw;

                door_yaw = doors_to_world(impacts[k].door, &impacts[k].position, &w);
                jo_3d_translate_matrix_fixed(w.x, w.y, w.z);
                jo_3d_rotate_matrix_y(door_yaw + (impacts[k].front ? 180 : 0));
            }
            else
            {
                jo_3d_translate_matrix_fixed(impacts[k].position.x, impacts[k].position.y, impacts[k].position.z);
                jo_3d_rotate_matrix_y(impacts[k].yaw);
                jo_3d_rotate_matrix_x(impacts[k].pitch);
            }
            // (the sprite is 16 x 16: the quad of a 3D sprite has the size of the sprite, the matrix scales it; the scale of
            // the 2D sprites does nothing on it)
            jo_3d_set_scale_fixed(IMPACT_SIZE / 16, IMPACT_SIZE / 16, JO_FIXED_1);
            // (the light of the lamps on it: the red alarm)
            {
                jo_pos3D_fixed  at;
                int             r;
                int             g;
                int             b;

                at = impacts[k].door != JO_NULL ? impacts[k].door->position : impacts[k].position;
                jo_3d_point_lights_sample(&at, &r, &g, &b);
                jo_3d_set_tint(r, g, b);
            }
            jo_3d_draw_sprite(tex_impact);
            jo_3d_set_tint(0, 0, 0);
        }
        jo_3d_pop_matrix();
        jo_3d_select_window(JO_3D_NEAR_WINDOW);
    }
    jo_3d_set_depth_bias(WORLD_DEPTH_BIAS);
}

/*
** END OF FILE
*/
