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
** Breakable things: entities of the class "breakable". The monitors of the computers (their screen animated: a cursor
** blinks, the text scrolls) and the glassware of the benches (racks of test tubes, flasks). A shot breaks them: pieces
** of glass fly (the shards of the windows, see glass_burst() in doors.c), the sound, and the model after (the screen
** dark and cracked, the tubes in stubs). They stay solid (the casing, the rack): the bullets stop on them.
**
** Properties (the level): dead_model (JOM file: the thing broken; none: hidden), shards (pieces of glass, 0: none),
** frames (frames of the animation of the screen, 0 or 1: none; the frames after the first one: the textures of
** cd/SCRFRM.JOM, see screen_frames.h), draw_distance. The model looks toward -Z, turned by the yaw of the entity.
*/

#include "demo.h"
#include "screen_frames.h"

/** @brief A frame of the animation of a screen lasts this long (milliseconds) */
#define SCREEN_FRAME_TIME           (420)
/** @brief The pieces of glass spread this far around a broken thing (units) */
#define BREAK_SPREAD                (8)

typedef struct
{
    jo_3d_mesh                      *dead;          // The model broken (JO_NULL: hidden when broken)
    short                           shards;         // Pieces of glass when it breaks
    short                           frames;         // Frames of the animation of its screen (0: none)
    short                           frame;          // The frame shown
    short                           timer;          // Milliseconds left for this frame
    bool                            broken;
}                                   breakable_data;

/** @brief The sprites of the frames of the screens (-1: not loaded), the polygon of the screen in the model of the
 *  monitors (found once: the polygon with the texture of the first frame), -1: none */
static int                          frame_sprites[SCREEN_FRAME_COUNT];
static bool                         frames_loaded = false;
static jo_3d_mesh                   *screen_mesh = JO_NULL;
static int                          screen_polygon = -1;

/** @brief The sprites of the frames of the screens (the first one: in the model of the monitor; the others: in SCRFRM.JOM,
 *  loaded here only to put them in the VRAM) */
static void                         load_frames(void)
{
    int                             k;

    if (frames_loaded)
        return;
    frames_loaded = true;
    (void)jo_entity_load_model("SCRFRM.JOM");
    for (k = 0; k < SCREEN_FRAME_COUNT; ++k)
        frame_sprites[k] = jo_3d_model_texture(screen_frame_textures[k]);
}

static void                         breakable_spawn(jo_entity * const entity)
{
    breakable_data                  *b;
    const char                      *dead;

    b = (breakable_data *)entity->data;
    b->broken = false;
    b->shards = (short)jo_entity_get_int(entity, "shards", 6);
    b->frames = (short)jo_entity_get_int(entity, "frames", 0);
    b->frame = 0;
    // (the screens not all on the same frame: a phase from the position)
    b->timer = (short)(((int)(entity->position.x >> 16) * 7 + (int)(entity->position.z >> 16) * 3) % SCREEN_FRAME_TIME);
    dead = jo_entity_get_string(entity, "dead_model", "");
    b->dead = dead[0] != '\0' ? jo_entity_load_model(dead) : JO_NULL;
    if (b->frames > 1)
        load_frames();
}

static void                         breakable_update(jo_entity * const entity, const int elapsed)
{
    breakable_data                  *b;

    b = (breakable_data *)entity->data;
    glass_shards_update(entity, elapsed);
    if (b->broken || b->frames <= 1)
        return;
    b->timer = (short)(b->timer - elapsed);
    if (b->timer <= 0)
    {
        b->timer = SCREEN_FRAME_TIME;
        b->frame = (short)((b->frame + 1) % b->frames);
        // (the entity is kept awake: the screens far from the agent go on, he comes back to a changed screen)
        entity->timer = SCREEN_FRAME_TIME;
    }
}

/** @brief It breaks: the pieces of glass fly away from the agent (the shot came from him), the model after */
static void                         breakable_death(jo_entity * const entity, jo_entity * const source)
{
    breakable_data                  *b;
    jo_pos3D_fixed                  center;
    jo_pos3D_fixed                  dir;
    int                             dx;
    int                             dz;
    int                             length;

    JO_UNUSED_ARG(source);
    b = (breakable_data *)entity->data;
    b->broken = true;
    center = entity->position;
    center.y -= jo_int2fixed(entity->height * 2 / 3);
    dx = jo_fixed2int(entity->position.x - player_x);
    dz = jo_fixed2int(entity->position.z - player_z);
    length = JO_MAX(1, (int)jo_sqrt((unsigned int)(dx * dx + dz * dz)));
    dir.x = jo_int2fixed(dx) / length;
    dir.y = 0;
    dir.z = jo_int2fixed(dz) / length;
    if (b->shards > 0)
        glass_burst(&center, &dir, b->shards, BREAK_SPREAD, entity);
    if (b->dead != JO_NULL)
        entity->mesh = b->dead;
    else
        entity->flags |= JO_ENTITY_FLAG_HIDDEN;
}

/** @brief The polygon of the screen in the model of the monitors (the texture of the first frame), once */
static void                         find_screen_polygon(jo_3d_mesh * const mesh)
{
    unsigned int                    k;

    if (mesh == screen_mesh || frame_sprites[0] < 0)
        return;
    screen_mesh = mesh;
    screen_polygon = -1;
    for (k = 0; k < mesh->data.polygon_count; ++k)
        if (mesh->data.attributes[k].texture_id == frame_sprites[0])
        {
            screen_polygon = (int)k;
            return;
        }
}

static void                         breakable_draw(jo_entity * const entity)
{
    breakable_data                  *b;
    int                             id;

    b = (breakable_data *)entity->data;
    glass_shards_draw(entity);
    if (entity->mesh == JO_NULL || jo_entity_view_distance(entity) < 0)
        return;
    // (the frame of the animation of the screen: the texture of its polygon, on the model shared by the monitors)
    if (!b->broken && b->frames > 1 && frames_loaded)
    {
        find_screen_polygon(entity->mesh);
        id = frame_sprites[b->frame < SCREEN_FRAME_COUNT ? b->frame : 0];
        if (screen_polygon >= 0 && id >= 0)
            entity->mesh->data.attributes[screen_polygon].texture_id = (unsigned short)id;
    }
    jo_3d_push_matrix();
    {
        jo_3d_translate_matrix_fixed(entity->position.x, entity->position.y, entity->position.z);
        if (entity->yaw != 0)
            jo_3d_rotate_matrix_y(entity->yaw);
        jo_3d_mesh_draw(entity->mesh);
    }
    jo_3d_pop_matrix();
}

static const jo_entity_class        breakable_class = {"breakable", sizeof(breakable_data), breakable_spawn, breakable_update,
                                                       JO_NULL, breakable_death, breakable_draw};

void                                props_register(void)
{
    int                             k;

    for (k = 0; k < SCREEN_FRAME_COUNT; ++k)
        frame_sprites[k] = -1;
    frames_loaded = false;
    screen_mesh = JO_NULL;
    jo_entity_register_class(&breakable_class);
}
