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
** Entities: see jo/entity.h. The generic classes of the engine are at the end of this file:
**
** - entity: the default class (a model, an unknown class)
** - relay: passes the actions TRIGGER on to its links (with their delays): a sequence of events
** - trigger: a zone (its cylinder) that fires ENTER and EXIT when the player goes in and out
** - explosive: explodes when its health reaches 0 (fire, smoke, flash, shake, sound), damages what is around it after a
**   short delay (chain reactions), then shows its dead model
*/

#include <stdbool.h>
#include "jo/conf.h"

#if defined(JO_COMPILE_WITH_LEVEL_SUPPORT) && defined(JO_COMPILE_WITH_3D_SUPPORT)

#include "jo/types.h"
#include "jo/sega_saturn.h"
#include "jo/smpc.h"
#include "jo/core.h"
#include "jo/tools.h"
#include "jo/malloc.h"
#include "jo/colors.h"
#include "jo/vdp1_command_pipeline.h"
#include "jo/sprites.h"
#include "jo/fs.h"
#include "jo/math.h"
#include "jo/list.h"
#include "jo/image.h"
#include "jo/tga.h"
#include "jo/3d.h"
#include "jo/model.h"
#include "jo/audio.h"
#include "jo/level.h"
#include "jo/effects.h"
#include "jo/entity.h"
#include "jo/vehicle.h"
#include "jo/cell.h"
#include "jo/light.h"
#include "jo/time.h"

/** @brief Events and actions sent at once in a row at most (then they wait for the next update: no endless loop) */
#define JO_ENTITY_MAX_DEPTH                 (8)

/** @brief An action that waits for its delay */
typedef struct
{
    short                           target;         // -1: free
    short                           source;         // -1: the game
    unsigned short                  action;
    short                           time;           // Milliseconds left
    int                             param;
}                                   __jo_entity_pending;

typedef struct
{
    const char                      *filename;
    jo_3d_model                     *model;
}                                   __jo_entity_model;

static const jo_entity_class        *__jo_entity_classes[JO_ENTITY_MAX_CLASSES];
static int                          __jo_entity_class_count = 0;
/** @brief The entities of the level: allocated with their count (a static array of JO_ENTITY_MAX cost its whole size to
 *  every program, a big level no longer fitted in the work RAM) */
static jo_entity                    *__jo_entities = JO_NULL;
/** @brief The memory of the table (__jo_entities: aligned on 16 bytes, a line of the cache) */
static void                         *__jo_entities_memory = JO_NULL;
static int                          __jo_entity_count = 0;
/** @brief The entities with the flag JO_ENTITY_FLAG_SOLID, in their order (see __jo_entities_list_solids()): the collisions
 *  read them, not all the entities (most entities are not solid: each one cost a line of the cache) */
static unsigned short               __jo_entity_solids[JO_ENTITY_MAX];
static int                          __jo_entity_solid_count = 0;

/** @brief List the solid entities (after the loading, at each jo_entities_update(), see jo_entity_set_solid()) */
static void                         __jo_entities_list_solids(void)
{
    int                             k;
    int                             n;

    for (k = 0, n = 0; k < __jo_entity_count; ++k)
        if (__jo_entities[k].flags & JO_ENTITY_FLAG_SOLID)
            __jo_entity_solids[n++] = (unsigned short)k;
    __jo_entity_solid_count = n;
}
static jo_level_prop_record         *__jo_entity_props = JO_NULL;
static unsigned int                 __jo_entity_prop_count = 0;
static jo_level_link_record         *__jo_entity_links = JO_NULL;
static unsigned int                 __jo_entity_link_count = 0;
static char                         *__jo_entity_strings = JO_NULL;
static unsigned int                 __jo_entity_strings_size = 0;
static __jo_entity_pending          __jo_entity_pendings[JO_ENTITY_MAX_PENDING];
static __jo_entity_model            __jo_entity_models[JO_ENTITY_MAX_MODELS];
static int                          __jo_entity_model_count = 0;
static int                          __jo_entity_depth = 0;
static jo_pos3D_fixed               __jo_entity_player;
static jo_entity_area_damage_callback __jo_entity_area_damage = JO_NULL;
static void                         __jo_entity_register_engine_classes(void);

/*
** Classes, strings, properties
*/

void                                jo_entity_register_class(const jo_entity_class * const cls)
{
    int                             k;

    for (k = 0; k < __jo_entity_class_count; ++k)
        if (jo_strcmp(__jo_entity_classes[k]->name, cls->name) == 0)
        {
            __jo_entity_classes[k] = cls;
            return;
        }
    if (__jo_entity_class_count >= JO_ENTITY_MAX_CLASSES)
    {
#ifdef JO_DEBUG
        jo_core_error("Too many entity classes (max=%d)", JO_ENTITY_MAX_CLASSES);
#endif
        return;
    }
    __jo_entity_classes[__jo_entity_class_count++] = cls;
}

static const jo_entity_class        *__jo_entity_find_class(const char * const name)
{
    int                             k;

    for (k = 0; k < __jo_entity_class_count; ++k)
        if (jo_strcmp(__jo_entity_classes[k]->name, name) == 0)
            return (__jo_entity_classes[k]);
    return (JO_NULL);
}

static const char                   *__jo_entity_string(const unsigned int offset)
{
    if (__jo_entity_strings == JO_NULL || offset >= __jo_entity_strings_size)
        return ("");
    return (__jo_entity_strings + offset);
}

static const jo_level_prop_record   *__jo_entity_find_prop(const jo_entity * const entity, const char * const key)
{
    unsigned int                    k;
    const jo_level_prop_record      *p;

    for (k = 0, p = __jo_entity_props + entity->first_prop; k < entity->prop_count; ++k, ++p)
        if (jo_strcmp(__jo_entity_string(p->key), key) == 0)
            return (p);
    return (JO_NULL);
}

int                                 jo_entity_get_int(const jo_entity * const entity, const char * const key, const int default_value)
{
    const jo_level_prop_record      *p;

    p = __jo_entity_find_prop(entity, key);
    if (p == JO_NULL || p->type == JO_LEVEL_PROPERTY_STRING)
        return (default_value);
    return (p->type == JO_LEVEL_PROPERTY_FIXED ? jo_fixed2int(p->value) : p->value);
}

jo_fixed                            jo_entity_get_fixed(const jo_entity * const entity, const char * const key, const jo_fixed default_value)
{
    const jo_level_prop_record      *p;

    p = __jo_entity_find_prop(entity, key);
    if (p == JO_NULL || p->type == JO_LEVEL_PROPERTY_STRING)
        return (default_value);
    return (p->type == JO_LEVEL_PROPERTY_FIXED ? p->value : jo_int2fixed(p->value));
}

const char                          *jo_entity_get_string(const jo_entity * const entity, const char * const key,
                                                          const char * const default_value)
{
    const jo_level_prop_record      *p;

    p = __jo_entity_find_prop(entity, key);
    if (p == JO_NULL || p->type != JO_LEVEL_PROPERTY_STRING)
        return (default_value);
    return (__jo_entity_string((unsigned int)p->value));
}

/*
** Models
*/

jo_3d_model                         *jo_entity_get_model(const char * const filename)
{
    int                             k;
    jo_3d_model                     *model;

    if (filename == JO_NULL || *filename == '\0')
        return (JO_NULL);
    for (k = 0; k < __jo_entity_model_count; ++k)
        if (jo_strcmp(__jo_entity_models[k].filename, filename) == 0)
            return (__jo_entity_models[k].model);
    if (__jo_entity_model_count >= JO_ENTITY_MAX_MODELS)
    {
#ifdef JO_DEBUG
        jo_core_error("Too many entity models (max=%d)", JO_ENTITY_MAX_MODELS);
#endif
        return (JO_NULL);
    }
    model = jo_3d_load_model(filename);
    __jo_entity_models[__jo_entity_model_count].filename = filename;
    __jo_entity_models[__jo_entity_model_count].model = model;
    ++__jo_entity_model_count;
    return (model);
}

jo_3d_mesh                          *jo_entity_load_model(const char * const filename)
{
    jo_3d_model                     *model;

    model = jo_entity_get_model(filename);
    return (model != JO_NULL ? model->mesh : JO_NULL);
}

void                                jo_entity_set_mesh(jo_entity * const entity, jo_3d_mesh * const mesh)
{
    jo_pos3D_fixed                  center;
    int                             cx;
    int                             cz;

    entity->mesh = mesh;
    entity->far_mesh = JO_NULL;
    if (mesh == JO_NULL)
        return;
    // Bounding sphere in the world: on the axis of the model, bigger (any yaw)
    entity->mesh_radius = jo_3d_mesh_get_bounding_sphere(mesh, &center);
    cx = JO_ABS(jo_fixed2int(center.x));
    cz = JO_ABS(jo_fixed2int(center.z));
    entity->mesh_radius += cx > cz ? cx + cz / 2 : cz + cx / 2;
    entity->center.x = entity->position.x;
    entity->center.y = entity->position.y + center.y;
    entity->center.z = entity->position.z;
}

/*
** Loading
*/

static void                         __jo_entities_free_looks(void);

void                                jo_entities_free(void)
{
    int                             k;

    for (k = 0; k < __jo_entity_count; ++k)
        if (__jo_entities[k].data != JO_NULL && __jo_entities[k].cls->data_size > 0)
            jo_free(__jo_entities[k].data);
    __jo_entity_count = 0;
    __jo_entity_solid_count = 0;
    if (__jo_entities_memory != JO_NULL)
        jo_free(__jo_entities_memory);
    __jo_entities_memory = JO_NULL;
    __jo_entities = JO_NULL;
    for (k = 0; k < __jo_entity_model_count; ++k)
        if (__jo_entity_models[k].model != JO_NULL)
            jo_3d_free_model(__jo_entity_models[k].model);
    __jo_entity_model_count = 0;
    if (__jo_entity_props != JO_NULL)
        jo_free(__jo_entity_props);
    if (__jo_entity_links != JO_NULL)
        jo_free(__jo_entity_links);
    if (__jo_entity_strings != JO_NULL)
        jo_free(__jo_entity_strings);
    __jo_entity_props = JO_NULL;
    __jo_entity_links = JO_NULL;
    __jo_entity_strings = JO_NULL;
    __jo_entity_prop_count = 0;
    __jo_entity_link_count = 0;
    __jo_entity_strings_size = 0;
    for (k = 0; k < JO_ENTITY_MAX_PENDING; ++k)
        __jo_entity_pendings[k].target = -1;
    // (the sprites of the explosions are loaded again with the next level: the program can free their VRAM with the
    // textures of the models, see jo_3d_model_free_textures_from())
    __jo_entities_free_looks();
}

/** @brief Copy the records of a section (they stay after the level is freed) */
static void                         *__jo_entity_copy(const void * const src, const unsigned int size)
{
    void                            *copy;

    if (src == JO_NULL || size == 0)
        return (JO_NULL);
    copy = jo_malloc(size);
    if (copy != JO_NULL)
        jo_memcpy(copy, src, size);
    return (copy);
}

int                                 jo_entities_load(jo_level * const level)
{
    const jo_level_enti_record      *er;
    const jo_level_prop_record      *pr;
    const jo_level_link_record      *lr;
    const jo_level_directory_entry  *strs;
    unsigned int                    count;
    unsigned int                    k;
    jo_entity                       *e;

    jo_entities_free();
    if (__jo_entity_class_count == 0 || __jo_entity_find_class("entity") == JO_NULL)
        __jo_entity_register_engine_classes();
    // What the entities need after the level is freed: the strings, the properties, the links
    strs = jo_level_find_section(level, JO_LEVEL_SECTION_STRS);
    if (strs != JO_NULL)
    {
        __jo_entity_strings = (char *)__jo_entity_copy(jo_level_section_data(level, JO_LEVEL_SECTION_STRS), strs->size);
        __jo_entity_strings_size = __jo_entity_strings != JO_NULL ? strs->size : 0;
    }
    pr = JO_LEVEL_RECORDS(level, PROP, prop, &__jo_entity_prop_count);
    __jo_entity_props = (jo_level_prop_record *)__jo_entity_copy(pr, __jo_entity_prop_count * sizeof(*pr));
    if (__jo_entity_props == JO_NULL)
        __jo_entity_prop_count = 0;
    lr = JO_LEVEL_RECORDS(level, LINK, link, &__jo_entity_link_count);
    __jo_entity_links = (jo_level_link_record *)__jo_entity_copy(lr, __jo_entity_link_count * sizeof(*lr));
    if (__jo_entity_links == JO_NULL)
        __jo_entity_link_count = 0;
    er = JO_LEVEL_RECORDS(level, ENTI, enti, &count);
    if (count > JO_ENTITY_MAX)
    {
#ifdef JO_DEBUG
        jo_core_error("Too many entities: %d (max=%d)", count, JO_ENTITY_MAX);
#endif
        count = JO_ENTITY_MAX;
    }
    if (count > 0)
    {
        __jo_entities_memory = jo_malloc(count * sizeof(*__jo_entities) + 15);
        __jo_entities = (jo_entity *)(((unsigned int)__jo_entities_memory + 15) & ~15u);
        if (__jo_entities_memory == JO_NULL)
        {
#ifdef JO_DEBUG
            jo_core_error("Out of memory: %d entities", count);
#endif
            count = 0;
        }
    }
    for (k = 0; k < count; ++k, ++er)
    {
        e = &__jo_entities[k];
        jo_memset(e, 0, sizeof(*e));
        e->name = __jo_entity_string(er->name);
        e->cls = __jo_entity_find_class(__jo_entity_string(er->cls));
        if (e->cls == JO_NULL)
        {
#ifdef JO_DEBUG
            jo_core_error("%s: unknown entity class %s", e->name, __jo_entity_string(er->cls));
#endif
            e->cls = __jo_entity_find_class("entity");
        }
        e->position.x = jo_int2fixed(er->x);
        e->position.y = jo_int2fixed(er->y);
        e->position.z = jo_int2fixed(er->z);
        e->yaw = er->yaw;
        e->health = er->health;
        e->flags = er->flags & (JO_ENTITY_FLAG_DISABLED | JO_ENTITY_FLAG_SOLID | JO_ENTITY_FLAG_HIDDEN);
        e->radius = er->radius;
        e->height = er->height;
        e->box_x = 0;
        e->box_z = 0;
        // (properties and links outside of their sections: none)
        if (er->first_prop + er->prop_count <= __jo_entity_prop_count)
        {
            e->first_prop = er->first_prop;
            e->prop_count = er->prop_count;
        }
        if (er->first_link + er->link_count <= __jo_entity_link_count)
        {
            e->first_link = er->first_link;
            e->link_count = er->link_count;
        }
        // The model, the model far away (fewer polygons), the distances (on the ground)
        jo_entity_set_mesh(e, jo_entity_load_model(__jo_entity_string(er->model)));
        e->far_mesh = jo_entity_load_model(jo_entity_get_string(e, "far_model", ""));
        e->far_distance = jo_entity_get_int(e, "far_distance", JO_ENTITY_FAR_DISTANCE);
        e->draw_distance = jo_entity_get_int(e, "draw_distance", JO_ENTITY_DRAW_DISTANCE);
        // (a box turned with the entity instead of its cylinder for the collisions: a wall of furniture, a partition)
        e->box_x = (short)jo_entity_get_int(e, "box_x", 0);
        e->box_z = (short)jo_entity_get_int(e, "box_z", 0);
        if (e->cls->data_size > 0)
        {
            e->data = jo_malloc(e->cls->data_size);
            if (e->data != JO_NULL)
                jo_memset(e->data, 0, e->cls->data_size);
        }
    }
    __jo_entity_count = (int)count;
    // The classes spawn their entities (all the entities exist: they can find each other)
    for (k = 0; k < count; ++k)
        if (__jo_entities[k].cls->spawn != JO_NULL)
            __jo_entities[k].cls->spawn(&__jo_entities[k]);
    __jo_entities_list_solids();
    return (__jo_entity_count);
}

int                                 jo_entity_count(void)
{
    return (__jo_entity_count);
}

jo_entity                           *jo_entity_get(const int index)
{
    return (index >= 0 && index < __jo_entity_count ? &__jo_entities[index] : JO_NULL);
}

jo_entity                           *jo_entity_find(const char * const name)
{
    int                             k;

    for (k = 0; k < __jo_entity_count; ++k)
        if (jo_strcmp(__jo_entities[k].name, name) == 0)
            return (&__jo_entities[k]);
    return (JO_NULL);
}

/*
** Events, actions, damage
*/

static void                         __jo_entity_queue(const int target, const int action, const int param, const int source, const int delay)
{
    int                             k;
    __jo_entity_pending             *p;

    for (k = 0, p = __jo_entity_pendings; k < JO_ENTITY_MAX_PENDING; ++k, ++p)
        if (p->target < 0)
        {
            p->target = (short)target;
            p->source = (short)source;
            p->action = (unsigned short)action;
            p->time = (short)JO_MIN(delay, 32767);
            p->param = param;
            return;
        }
#ifdef JO_DEBUG
    jo_core_error("Too many delayed actions (max=%d)", JO_ENTITY_MAX_PENDING);
#endif
}

void                                jo_entity_fire(jo_entity * const source, const int event, const int param)
{
    unsigned int                    k;
    int                             target;
    const jo_level_link_record      *l;

    for (k = 0, l = __jo_entity_links + source->first_link; k < source->link_count; ++k, ++l)
    {
        if (l->event != event)
            continue;
        target = l->target == JO_LEVEL_NO_INDEX ? (int)(source - __jo_entities) : (int)l->target;
        if (target >= __jo_entity_count)
            continue;
        // No delay: now (unless too many events in a row: at the next update)
        if (l->delay == 0 && __jo_entity_depth < JO_ENTITY_MAX_DEPTH)
            jo_entity_send(&__jo_entities[target], l->action, l->param == -1 ? param : l->param, source);
        else
            __jo_entity_queue(target, l->action, l->param == -1 ? param : l->param, (int)(source - __jo_entities), l->delay);
    }
}

void                                jo_entity_send(jo_entity * const target, const int action, const int param, jo_entity * const source)
{
    if ((target->flags & JO_ENTITY_FLAG_DISABLED) && action != JO_LEVEL_ACTION_ENABLE && action != JO_LEVEL_ACTION_TOGGLE)
        return;
    ++__jo_entity_depth;
    if (target->cls->action == JO_NULL || !target->cls->action(target, action, param, source))
    {
        switch (action)
        {
            case JO_LEVEL_ACTION_TRIGGER:
            case JO_LEVEL_ACTION_USE:
                jo_entity_fire(target, JO_LEVEL_EVENT_TRIGGER, param);
                break;
            case JO_LEVEL_ACTION_DAMAGE:
                jo_entity_damage(target, param, source);
                break;
            case JO_LEVEL_ACTION_KILL:
                jo_entity_damage(target, JO_MAX(target->health, 1), source);
                break;
            case JO_LEVEL_ACTION_ENABLE:
                target->flags &= ~JO_ENTITY_FLAG_DISABLED;
                break;
            case JO_LEVEL_ACTION_DISABLE:
                target->flags |= JO_ENTITY_FLAG_DISABLED;
                break;
            case JO_LEVEL_ACTION_TOGGLE:
                target->flags ^= JO_ENTITY_FLAG_DISABLED;
                break;
            default:
                break;
        }
    }
    --__jo_entity_depth;
}

void                                jo_entity_damage(jo_entity * const entity, const int damage, jo_entity * const source)
{
    if (damage <= 0 || entity->health <= 0 || (entity->flags & (JO_ENTITY_FLAG_DEAD | JO_ENTITY_FLAG_DISABLED)))
        return;
    entity->health -= damage;
    if (entity->health > 0)
    {
        jo_entity_fire(entity, JO_LEVEL_EVENT_DAMAGE, damage);
        return;
    }
    entity->health = 0;
    entity->flags |= JO_ENTITY_FLAG_DEAD;
    ++__jo_entity_depth;
    if (entity->cls->death != JO_NULL)
        entity->cls->death(entity, source);
    jo_entity_fire(entity, JO_LEVEL_EVENT_DEATH, damage);
    --__jo_entity_depth;
}

void                                jo_entities_set_area_damage_callback(const jo_entity_area_damage_callback callback)
{
    __jo_entity_area_damage = callback;
}

void                                jo_entities_damage_area(const jo_pos3D_fixed * const center, const int radius, const int damage,
                                                            jo_entity * const source)
{
    int                             k;
    int                             dx;
    int                             dy;
    int                             dz;
    int                             d;
    jo_entity                       *e;

    for (k = 0, e = __jo_entities; k < __jo_entity_count; ++k, ++e)
    {
        if (e == source || e->health <= 0 || (e->flags & (JO_ENTITY_FLAG_DEAD | JO_ENTITY_FLAG_DISABLED)))
            continue;
        // Distance to the middle of its cylinder, less its radius
        dx = JO_ABS(jo_fixed2int(e->position.x - center->x));
        dy = JO_ABS(jo_fixed2int(e->position.y - center->y) - e->height / 2);
        dz = JO_ABS(jo_fixed2int(e->position.z - center->z));
        if (dx >= radius + e->radius || dy >= radius + e->height || dz >= radius + e->radius)
            continue;
        d = (int)jo_sqrt((unsigned int)(dx * dx + dy * dy + dz * dz)) - e->radius;
        if (d < radius)
            jo_entity_damage(e, damage * (radius - JO_MAX(d, 0)) / radius, source);
    }
    if (__jo_entity_area_damage != JO_NULL)
        __jo_entity_area_damage(center, radius, damage, source);
}

/*
** Update, drawing
*/

void                                jo_entities_update(const jo_pos3D_fixed * const player, const int elapsed)
{
    int                             k;
    int                             dx;
    int                             dz;
    __jo_entity_pending             *p;
    jo_entity                       *e;

    __jo_entity_player = *player;
    // (the flags set directly since the last update)
    __jo_entities_list_solids();
    // The delayed actions (the ones added now wait for the next update)
    for (k = 0, p = __jo_entity_pendings; k < JO_ENTITY_MAX_PENDING; ++k, ++p)
    {
        if (p->target < 0)
            continue;
        p->time -= (short)elapsed;
        if (p->time > 0)
            continue;
        e = &__jo_entities[p->target];
        p->target = -1;
        jo_entity_send(e, p->action, p->param, p->source >= 0 ? &__jo_entities[p->source] : JO_NULL);
    }
    // The classes: the entities near the player or that wait for something
    for (k = 0, e = __jo_entities; k < __jo_entity_count; ++k, ++e)
    {
        if (e->cls->update == JO_NULL || (e->flags & JO_ENTITY_FLAG_DISABLED))
            continue;
        dx = JO_ABS(jo_fixed2int(e->position.x - player->x));
        dz = JO_ABS(jo_fixed2int(e->position.z - player->z));
        if (e->timer > 0 || (dx < JO_ENTITY_WAKE_DISTANCE && dz < JO_ENTITY_WAKE_DISTANCE))
            e->cls->update(e, elapsed);
    }
}

/** @brief The light of the lamps on a door: this far from it on the side of the player (world units: out of the cell of
 *  its doorway, 50 units deep on each side, closed with the door) */
# define __JO_ENTITY_DOOR_LIGHT_SIDE        (60)

/** @brief The distances of the levels of detail are divided by it (see jo_entities_set_lod_scale()) */
static int                          __jo_entity_lod_scale = 1;

void                                jo_entities_set_lod_scale(const int scale)
{
    __jo_entity_lod_scale = JO_MAX(scale, 1);
}

int                                 jo_entity_view_distance(const jo_entity * const entity)
{
    int                             dx;
    int                             dz;
    int                             distance;
    jo_pos3D_fixed                  v;

    if (entity->mesh == JO_NULL)
        return (-1);
    // Distance on the ground from the player (an octagon): each polygon costs CPU time, even small far away
    dx = JO_ABS(jo_fixed2int(entity->position.x - __jo_entity_player.x));
    dz = JO_ABS(jo_fixed2int(entity->position.z - __jo_entity_player.z));
    // (a magnified view: the distance of the details as it is seen)
    distance = dx > dz ? dx + ((dz * 3) >> 3) : dz + ((dx * 3) >> 3);
    // (no division most of the time: a scale of 1. JO_OPAQUE(): else GCC divides in all the cases)
    if (__jo_entity_lod_scale != 1)
    {
        JO_OPAQUE(distance);
        distance /= __jo_entity_lod_scale;
    }
    if (distance > entity->draw_distance)
        return (-1);
    jo_3d_transform_point(&entity->center, &v);
    if (!jo_3d_is_sphere_visible(&v, jo_int2fixed(entity->mesh_radius),
                                 jo_int2fixed(entity->draw_distance * __jo_entity_lod_scale + entity->mesh_radius)))
        return (-1);
    return (distance);
}

/** @brief Depth bias of an entity in the cells (see jo_cells_depth_bias_at()): a door hidden in its cell, the cells on its
 *  sides
 *  @return Bias, -1: not seen
 *  @remarks The cells of the entity are kept while it doesn't move (about 130 searches a frame in a level full of doors, 5 for
 *           a door not seen). The sides use the radius and the height of the entity (they don't change after the loading)
 */
static int                          __jo_entity_depth_bias(jo_entity * const e)
{
    int                             side;
    int                             bias;
    jo_fixed                        reach;

    if (e->cell_generation != jo_cells_generation() || e->cell_position.x != e->position.x ||
        e->cell_position.y != e->position.y || e->cell_position.z != e->position.z)
    {
        e->cell_generation = jo_cells_generation();
        e->cell_position = e->position;
        e->cells[0] = (short)jo_cells_object_cell(e->position.x, e->position.y, e->position.z);
        e->cells[1] = -2;
    }
    bias = jo_cells_depth_bias_of(e->cells[0]);
    if (bias >= 0 || !(e->flags & JO_ENTITY_FLAG_DOORWAY))
        return (bias);
    // (a door: the cells on both sides of it)
    if (e->cells[1] == -2)
    {
        reach = jo_int2fixed(e->radius);
        for (side = 0; side < 4; ++side)
            e->cells[1 + side] = (short)jo_cells_object_cell(e->position.x + (side == 0 ? reach : side == 1 ? -reach : 0),
                                                             e->position.y - jo_int2fixed(e->height / 2),
                                                             e->position.z + (side == 2 ? reach : side == 3 ? -reach : 0));
    }
    for (side = 0; side < 4 && bias < 0; ++side)
        bias = jo_cells_depth_bias_of(e->cells[1 + side]);
    return (bias);
}

/** @brief An entity drawn in the frame (see jo_entities_prepare_draw()) */
typedef struct
{
    short                           entity;         // Index
    short                           bias;           // Depth bias of its cell (see jo_cells_depth_bias_at())
}                                   __jo_entity_drawn;

/** @brief The entities drawn in the frame (see jo_entities_prepare_draw()): the ones of the master CPU, then the ones of the
 *  slave CPU from the end of the table (first the ones with a draw_slave callback, see jo_entities_draw_slave()) */
static __jo_entity_drawn            __jo_entity_draw_list[JO_ENTITY_MAX];
static int                          __jo_entity_master_count = 0;
static int                          __jo_entity_slave_count = 0;
static int                          __jo_entity_slave_callbacks = 0;    // Of the slave CPU: with a draw_slave callback (first)
static int                          __jo_entity_draw_base_bias = 0;
static bool                         __jo_entity_draw_prepared = false;
/** @brief The entities with a draw_slave callback are drawn (written by the slave CPU, read without the cache) */
static volatile int                 __jo_entity_slave_done = 0;
static int                          __jo_entity_slave_wait = 0;

int                                 jo_entities_test_counts(const int which)
{
    return (which == 0 ? __jo_entity_master_count : which == 1 ? __jo_entity_slave_count : __jo_entity_slave_callbacks);
}

static int                          __jo_entity_slave_polygons = 0;
static int                          __jo_entity_master_polygons = 0;

int                                 jo_entities_polygon_count(const bool slave)
{
    return (slave ? __jo_entity_slave_polygons : __jo_entity_master_polygons);
}

int                                 jo_entities_slave_wait_time(void)
{
    return (__jo_entity_slave_wait);
}

/** @brief The light of the lamps on an entity: at its middle (a door: its side toward the player; at its middle, in the
 *  doorway, the light of the room behind it was on the closed door) */
static void                         __jo_entity_light(const jo_entity * const e, int * const r, int * const g, int * const b)
{
    jo_pos3D_fixed                  middle;
    int                             dx;
    int                             dz;
    int                             d;

    middle = e->position;
    middle.y -= jo_int2fixed(e->height / 2);
    if (e->flags & JO_ENTITY_FLAG_DOORWAY)
    {
        dx = jo_fixed2int(__jo_entity_player.x - middle.x);
        dz = jo_fixed2int(__jo_entity_player.z - middle.z);
        d = JO_MAX(JO_ABS(dx), JO_ABS(dz));
        if (d > 0)
        {
            middle.x += jo_int2fixed(dx * __JO_ENTITY_DOOR_LIGHT_SIDE / d);
            middle.z += jo_int2fixed(dz * __JO_ENTITY_DOOR_LIGHT_SIDE / d);
        }
    }
    jo_3d_point_lights_sample(&middle, r, g, b);
}

/** @brief The model of an entity at a distance (the far one farther) */
static  __jo_force_inline jo_3d_mesh *__jo_entity_mesh_at(const jo_entity * const e, const int distance)
{
    return (e->far_mesh != JO_NULL && distance > e->far_distance ? e->far_mesh : e->mesh);
}

void                                jo_entities_prepare_draw(const int slave_share)
{
    int                             k;
    int                             distance;
    int                             bias;
    int                             r;
    int                             g;
    int                             b;
    int                             cost;
    int                             master_cost;
    int                             slave_cost;
    bool                            slave;
    jo_entity                       *e;
    const jo_3d_mesh                *mesh;
    __jo_entity_drawn               *master;
    __jo_entity_drawn               *last;
    __jo_entity_drawn               *callbacks;

    // The share: the costs of the 2 CPUs weighted by their shares (the polygons of the model drawn)
    master_cost = 1;
    slave_cost = 1;
    master = __jo_entity_draw_list;
    last = __jo_entity_draw_list + JO_ENTITY_MAX;
    __jo_entity_draw_base_bias = jo_3d_get_depth_bias();
    __jo_entity_slave_callbacks = 0;
    for (k = 0, e = __jo_entities; k < __jo_entity_count; ++k, ++e)
    {
        e->view_distance = -1;
        if (e->flags & (JO_ENTITY_FLAG_DISABLED | JO_ENTITY_FLAG_HIDDEN))
            continue;
        // (an entity drawn by the engine: too far or not seen, nothing else to do: the cell and the lights cost more)
        distance = -1;
        if (e->cls->draw == JO_NULL && (distance = jo_entity_view_distance(e)) < 0)
            continue;
        // In a cell: hidden if the cell is not seen, behind the nearer cells (see jo_cells_depth_bias_at())
        bias = __jo_entity_depth_bias(e);
        if (bias < 0)
            continue;
        // (a class that draws it: drawn even without a model or out of the view, the class decides (a sprite, the pieces
        // of a broken thing); the distance for its slave CPU draw, and its share)
        if (e->cls->draw != JO_NULL)
            distance = jo_entity_view_distance(e);
        e->view_distance = (short)JO_MIN(distance, 32767);
        // The tint: the light of the lamps on it, changed by the class
        __jo_entity_light(e, &r, &g, &b);
        if (e->cls->tint != JO_NULL)
            e->cls->tint(e, &r, &g, &b);
        jo_3d_set_tint(r, g, b);
        e->tint = jo_3d_get_tint();
        jo_3d_set_tint(0, 0, 0);
        // The CPU: the one with the smaller cost for its share (an entity with a draw callback without draw_slave callback:
        // the master CPU)
        mesh = distance >= 0 ? __jo_entity_mesh_at(e, distance) : JO_NULL;
        cost = mesh != JO_NULL ? (int)mesh->data.polygon_count + 8 : 8;
        slave = slave_share > 0 && (e->cls->draw == JO_NULL || e->cls->draw_slave != JO_NULL) &&
                slave_cost * (1024 - slave_share) < master_cost * slave_share;
        if (slave && master < last)
        {
            slave_cost += cost;
            --last;
            // (the ones with a callback first: from the end of the table, the others before them)
            if (e->cls->draw_slave != JO_NULL)
            {
                callbacks = __jo_entity_draw_list + JO_ENTITY_MAX - 1 - __jo_entity_slave_callbacks;
                *last = *callbacks;
                callbacks->entity = (short)k;
                callbacks->bias = (short)bias;
                ++__jo_entity_slave_callbacks;
            }
            else
            {
                last->entity = (short)k;
                last->bias = (short)bias;
            }
            // (its tinted tables: the slave CPU doesn't write them)
            if (e->tint != 0 && mesh != JO_NULL)
            {
                jo_3d_set_packed_tint(e->tint);
                jo_3d_prepare_tint(mesh);
                jo_3d_set_tint(0, 0, 0);
            }
        }
        else if (master < last)
        {
            master_cost += cost;
            master->entity = (short)k;
            master->bias = (short)bias;
            ++master;
        }
    }
    __jo_entity_master_count = (int)(master - __jo_entity_draw_list);
    __jo_entity_slave_count = (int)(__jo_entity_draw_list + JO_ENTITY_MAX - last);
    __jo_entity_master_polygons = master_cost - 1;
    __jo_entity_slave_polygons = slave_cost - 1;
    __jo_entity_slave_done = 0;
    __jo_entity_draw_prepared = true;
}

/** @brief Draw an entity of the list on the master CPU */
static void                         __jo_entity_draw(const __jo_entity_drawn * const d)
{
    jo_entity                       *e;

    e = &__jo_entities[d->entity];
    if (d->bias != 0)
    {
        jo_3d_set_depth_bias(__jo_entity_draw_base_bias + d->bias);
        // (in a cell seen through a portal: the far window, clipped to the portals, see jo_cells_window_of())
        jo_3d_select_window(jo_cells_window_of(d->bias));
    }
    jo_3d_set_packed_tint(e->tint);
    if (e->cls->draw != JO_NULL)
        e->cls->draw(e);
    else
    {
        jo_3d_push_matrix();
        {
            jo_3d_translate_matrix_fixed(e->position.x, e->position.y, e->position.z);
            if (e->yaw != 0)
                jo_3d_rotate_matrix_y(e->yaw);
            jo_3d_mesh_draw(__jo_entity_mesh_at(e, e->view_distance));
        }
        jo_3d_pop_matrix();
    }
    jo_3d_set_tint(0, 0, 0);
    if (d->bias != 0)
    {
        jo_3d_set_depth_bias(__jo_entity_draw_base_bias);
        jo_3d_select_window(JO_3D_NEAR_WINDOW);
    }
}

int                                 jo_entities_draw(void)
{
    int                             k;
    int                             count;
    int                             pass;
    bool                            waited;
    unsigned int                    start;
    const __jo_entity_drawn         *d;

    if (!__jo_entity_draw_prepared)
        jo_entities_prepare_draw(0);
    __jo_entity_draw_prepared = false;
    count = 0;
    waited = false;
    start = 0;
    __jo_entity_slave_wait = 0;
    // The models first, then the entities drawn by their class: after the ones of the slave CPU (the callbacks of a class
    // share its data)
    for (pass = 0; pass < 2; ++pass)
        for (k = 0, d = __jo_entity_draw_list; k < __jo_entity_master_count; ++k, ++d)
        {
            if ((__jo_entities[d->entity].cls->draw != JO_NULL) != (pass == 1))
                continue;
            if (pass == 1 && !waited)
            {
                waited = true;
                if (__jo_entity_slave_callbacks > 0)
                {
                    unsigned int    spins;

                    // (the job of the slave CPU draws them: jo_entities_draw_slave(). Not called: after a while, without
                    // them, the game goes on)
                    start = jo_time_get_frc();
                    for (spins = 0; !*(volatile int *)JO_CACHE_THROUGH(&__jo_entity_slave_done) && spins < 4000000; ++spins)
                        ;
#ifdef JO_DEBUG
                    if (spins >= 4000000)
                        jo_core_error("jo_entities_draw_slave() not called by the slave CPU");
#endif
                    // (microseconds per tick of the timer: clock / 32, see jo_time_init())
                    __jo_entity_slave_wait = (int)(((jo_time_get_frc() - start) & 0xFFFF) * 1000) /
                                             ((JO_TV_WIDTH == 352 || JO_TV_WIDTH == 704) ? 895 : 839);
                }
            }
            __jo_entity_draw(d);
            ++count;
        }
    return (count);
}

int                                 jo_entities_draw_slave(void)
{
    int                             k;
    int                             count;
    jo_entity                       *e;
    const __jo_entity_drawn         *d;

    count = 0;
    // (from the end of the table: the ones with a draw_slave callback first)
    for (k = 0, d = __jo_entity_draw_list + JO_ENTITY_MAX - 1; k < __jo_entity_slave_count; ++k, --d)
    {
        if (k == __jo_entity_slave_callbacks)
            *(volatile int *)JO_CACHE_THROUGH(&__jo_entity_slave_done) = 1;
        e = &__jo_entities[d->entity];
        jo_3d_slave_set_depth_bias(__jo_entity_draw_base_bias + d->bias);
        jo_3d_slave_select_window(jo_cells_window_of(d->bias));
        jo_3d_slave_set_tint(e->tint);
        if (e->cls->draw_slave != JO_NULL)
            e->cls->draw_slave(e);
        else if (jo_3d_slave_push_matrix())
        {
            jo_3d_slave_translate_matrix_fixed(e->position.x, e->position.y, e->position.z);
            if (e->yaw != 0)
                jo_3d_slave_rotate_matrix(0, e->yaw, 0);
            jo_3d_slave_mesh_draw(__jo_entity_mesh_at(e, e->view_distance));
            jo_3d_slave_pop_matrix();
        }
        ++count;
    }
    if (__jo_entity_slave_count <= __jo_entity_slave_callbacks)
        *(volatile int *)JO_CACHE_THROUGH(&__jo_entity_slave_done) = 1;
    jo_3d_slave_set_tint(0);
    jo_3d_slave_set_depth_bias(__jo_entity_draw_base_bias);
    jo_3d_slave_select_window(JO_3D_NEAR_WINDOW);
    return (count);
}

/*
** Collisions
*/

/** @brief Square root of a 64 bits number */
static unsigned int                 __jo_entity_sqrt64(unsigned long long value)
{
    unsigned long long              r;
    unsigned long long              b;

    r = 0;
    b = 1ULL << 62;
    while (b > value)
        b >>= 2;
    while (b != 0)
    {
        if (value >= r + b)
        {
            value -= r + b;
            r = (r >> 1) + b;
        }
        else
            r >>= 1;
        b >>= 2;
    }
    return ((unsigned int)r);
}

/** @brief A ray and the turned box of a solid entity (box_x / box_z, see __jo_entity_blocks()): the slabs in its space
 *  @param e Entity (box_z > 0)
 *  @param origin Origin of the ray (world)
 *  @param dir Direction (unit vector)
 *  @param max_t Farthest distance
 *  @param t Distance of the hit (output)
 *  @param normal Normal of the face hit (world, output)
 *  @return true if the ray enters the box before max_t (the origin in the box: no hit, like the cylinders)
 */
static bool                         __jo_entity_ray_box(const jo_entity * const e, const jo_pos3D_fixed * const origin,
                                                        const jo_pos3D_fixed * const dir, const jo_fixed max_t, jo_fixed * const t,
                                                        jo_pos3D_fixed * const normal)
{
    jo_fixed                        c;
    jo_fixed                        s;
    jo_fixed                        o[2];
    jo_fixed                        d[2];
    jo_fixed                        half[2];
    jo_fixed                        t_in;
    jo_fixed                        t_out;
    jo_fixed                        t0;
    jo_fixed                        t1;
    jo_fixed                        swap;
    jo_fixed                        px;
    jo_fixed                        pz;
    int                             axis;
    int                             sign;
    int                             k;

    c = jo_cos(e->yaw);
    s = jo_sin(e->yaw);
    px = origin->x - e->position.x;
    pz = origin->z - e->position.z;
    // (the same space as __jo_entity_blocks())
    o[0] = jo_fixed_mult(px, c) - jo_fixed_mult(pz, s);
    o[1] = jo_fixed_mult(px, s) + jo_fixed_mult(pz, c);
    d[0] = jo_fixed_mult(dir->x, c) - jo_fixed_mult(dir->z, s);
    d[1] = jo_fixed_mult(dir->x, s) + jo_fixed_mult(dir->z, c);
    half[0] = jo_int2fixed(e->box_x);
    half[1] = jo_int2fixed(e->box_z);
    if (JO_ABS(o[0]) < half[0] && JO_ABS(o[1]) < half[1])
        return (false);
    t_in = 0;
    t_out = max_t;
    axis = -1;
    sign = 0;
    for (k = 0; k < 2; ++k)
    {
        if (JO_ABS(d[k]) < 16)
        {
            if (JO_ABS(o[k]) >= half[k])
                return (false);
            continue;
        }
        t0 = jo_fixed_div(-half[k] - o[k], d[k]);
        t1 = jo_fixed_div(half[k] - o[k], d[k]);
        if (t0 > t1)
        {
            swap = t0;
            t0 = t1;
            t1 = swap;
        }
        if (t0 > t_in)
        {
            t_in = t0;
            axis = k;
            // (the face toward the origin)
            sign = d[k] > 0 ? -1 : 1;
        }
        if (t1 < t_out)
            t_out = t1;
        if (t_in > t_out)
            return (false);
    }
    if (axis < 0 || t_in <= 0 || t_in >= max_t)
        return (false);
    *t = t_in;
    // (the normal back in the world: the inverse turn)
    if (axis == 0)
    {
        normal->x = sign * c;
        normal->z = -sign * s;
    }
    else
    {
        normal->x = sign * s;
        normal->z = sign * c;
    }
    normal->y = 0;
    return (true);
}

jo_entity                           *jo_entities_ray_hit(const jo_pos3D_fixed * const origin, const jo_pos3D_fixed * const dir,
                                                         const jo_fixed max_t, jo_fixed * const t, jo_pos3D_fixed * const normal)
{
    int                             k;
    int                             px;
    int                             pz;
    int                             range;
    int                             b;
    int                             c;
    jo_fixed                        a;
    jo_fixed                        best;
    jo_fixed                        hit;
    jo_fixed                        y;
    long long                       disc;
    jo_entity                       *e;
    jo_entity                       *found;
    jo_pos3D_fixed                  box_hit_normal;

    box_hit_normal.x = 0;
    box_hit_normal.y = 0;
    box_hit_normal.z = 0;
    // The cylinder in the plane XZ: |p - d t|^2 = r^2 (p: center - origin), a t^2 - 2 b t + c = 0
    a = jo_fixed_mult(dir->x, dir->x) + jo_fixed_mult(dir->z, dir->z);
    if (a < 64)
        return (JO_NULL);
    range = jo_fixed2int(max_t);
    best = max_t;
    found = JO_NULL;
    for (k = 0; k < __jo_entity_solid_count; ++k)
    {
        e = &__jo_entities[__jo_entity_solids[k]];
        if (!(e->flags & JO_ENTITY_FLAG_SOLID) || (e->flags & JO_ENTITY_FLAG_DISABLED))
            continue;
        px = jo_fixed2int(e->position.x - origin->x);
        pz = jo_fixed2int(e->position.z - origin->z);
        if (JO_ABS(px) > range + e->radius || JO_ABS(pz) > range + e->radius)
            continue;
        if (e->box_z > 0)
        {
            // A turned box (a thin partition: its cylinder of the radius of its length stopped the bullets around it)
            jo_pos3D_fixed          box_normal;

            if (!__jo_entity_ray_box(e, origin, dir, best, &hit, &box_normal))
                continue;
            y = origin->y + jo_fixed_mult(dir->y, hit);
            if (y > e->position.y || y < e->position.y - jo_int2fixed(e->height))
                continue;
            best = hit;
            found = e;
            box_hit_normal = box_normal;
            continue;
        }
        b = px * (dir->x >> 1) + pz * (dir->z >> 1);
        c = px * px + pz * pz - e->radius * e->radius;
        // Away from the ray, or the origin in the cylinder
        if (b <= 0 || c <= 0)
            continue;
        // (b is b / 2 in Q16: no overflow up to 30000 world units)
        disc = (long long)b * b * 4 - (long long)a * ((long long)c << 16);
        if (disc < 0)
            continue;
        hit = (jo_fixed)((((long long)b * 2 - __jo_entity_sqrt64((unsigned long long)disc)) << 16) / a);
        if (hit < 0 || hit >= best)
            continue;
        // The height of the hit: between the foot and the top
        y = origin->y + jo_fixed_mult(dir->y, hit);
        if (y > e->position.y || y < e->position.y - jo_int2fixed(e->height))
            continue;
        best = hit;
        found = e;
    }
    if (found == JO_NULL)
        return (JO_NULL);
    *t = best;
    if (found->box_z > 0)
    {
        *normal = box_hit_normal;
        return (found);
    }
    normal->x = (origin->x + jo_fixed_mult(dir->x, best) - found->position.x) / found->radius;
    normal->y = 0;
    normal->z = (origin->z + jo_fixed_mult(dir->z, best) - found->position.z) / found->radius;
    return (found);
}

/** @brief Check if a solid entity blocks a circle on the ground (its cylinder, or its turned box: box_x / box_z) */
static bool                         __jo_entity_blocks(const jo_entity * const e, const jo_fixed x, const jo_fixed z, const int radius)
{
    int                             dx;
    int                             dz;
    int                             lx;
    int                             lz;
    int                             r;

    dx = jo_fixed2int(e->position.x - x);
    dz = jo_fixed2int(e->position.z - z);
    // (far: no sine nor cosine. The turned box grown by the radius is in the square of half side box_x + box_z + 2 x radius
    // around its center: |dx| <= |lx| + |lz|, 2 more units for the rounding)
    r = e->box_z > 0 ? e->box_x + e->box_z + JO_MULT_BY_2(radius) + 2 : e->radius + radius;
    if (JO_ABS(dx) >= r || JO_ABS(dz) >= r)
        return (false);
    if (e->box_z > 0)
    {
        // A box turned with the entity (a vehicle: longer than wide): in its space (it looks toward (sin yaw, cos yaw))
        lx = jo_fixed2int(dx * jo_cos(e->yaw) - dz * jo_sin(e->yaw));
        lz = jo_fixed2int(dx * jo_sin(e->yaw) + dz * jo_cos(e->yaw));
        return (JO_ABS(lx) < e->box_x + radius && JO_ABS(lz) < e->box_z + radius);
    }
    r = e->radius + radius;
    return (JO_ABS(dx) < r && JO_ABS(dz) < r && dx * dx + dz * dz < r * r);
}

void                                jo_entity_set_solid(jo_entity * const entity, const bool solid)
{
    if (solid)
        entity->flags |= JO_ENTITY_FLAG_SOLID;
    else
        entity->flags &= ~JO_ENTITY_FLAG_SOLID;
    __jo_entities_list_solids();
}

jo_entity                           *jo_entities_get_blocking(const jo_fixed x, const jo_fixed z, const int radius,
                                                              const jo_entity * const except)
{
    int                             k;
    jo_entity                       *e;

    for (k = 0; k < __jo_entity_solid_count; ++k)
    {
        e = &__jo_entities[__jo_entity_solids[k]];
        if (e == except || !(e->flags & JO_ENTITY_FLAG_SOLID) || (e->flags & JO_ENTITY_FLAG_DISABLED))
            continue;
        if (__jo_entity_blocks(e, x, z, radius))
            return (e);
    }
    return (JO_NULL);
}

bool                                jo_entities_is_blocked(const jo_fixed x, const jo_fixed z, const int radius)
{
    return (jo_entities_get_blocking(x, z, radius, JO_NULL) != JO_NULL);
}

bool                                jo_entities_is_blocked_at(const jo_fixed x, const jo_fixed y, const jo_fixed z, const int radius,
                                                              const int height, const jo_entity * const except)
{
    int                             k;
    jo_entity                       *e;
    jo_fixed                        top;

    // The solid entities near (x, z), then the vertical overlap: the entity goes from its feet up to its height (Saturn Y:
    // down), the body from y up to height
    top = y - jo_int2fixed(height);
    for (k = 0; k < __jo_entity_solid_count; ++k)
    {
        e = &__jo_entities[__jo_entity_solids[k]];
        if (e == except || !(e->flags & JO_ENTITY_FLAG_SOLID) || (e->flags & JO_ENTITY_FLAG_DISABLED))
            continue;
        if (top >= e->position.y || y <= e->position.y - jo_int2fixed(e->height))
            continue;
        if (__jo_entity_blocks(e, x, z, radius))
            return (true);
    }
    return (false);
}

/*
** Classes of the engine
*/

/** @brief trigger: a zone (the cylinder) that fires ENTER and EXIT when the player goes in and out.
 *  Property: once (INT, 0): 1, the zone is disabled after the first ENTER
 */
/** @brief Check if the player went in or out of the cylinder of an entity
 *  @return 1: he went in, -1: he went out, 0: no change
 */
static int                          __jo_entity_player_crossing(jo_entity * const entity)
{
    int                             dx;
    int                             dz;
    jo_fixed                        y;
    bool                            inside;

    dx = jo_fixed2int(__jo_entity_player.x - entity->position.x);
    dz = jo_fixed2int(__jo_entity_player.z - entity->position.z);
    y = __jo_entity_player.y;
    inside = JO_ABS(dx) < entity->radius && JO_ABS(dz) < entity->radius && dx * dx + dz * dz < entity->radius * entity->radius &&
             y <= entity->position.y + JO_FIXED_8 && y >= entity->position.y - jo_int2fixed(entity->height);
    if (inside == ((entity->flags & JO_ENTITY_FLAG_PLAYER_INSIDE) != 0))
        return (0);
    entity->flags ^= JO_ENTITY_FLAG_PLAYER_INSIDE;
    return (inside ? 1 : -1);
}

static void                         __jo_trigger_update(jo_entity * const entity, const int elapsed)
{
    int                             crossing;

    JO_UNUSED_ARG(elapsed);
    crossing = __jo_entity_player_crossing(entity);
    if (crossing == 0)
        return;
    jo_entity_fire(entity, crossing > 0 ? JO_LEVEL_EVENT_ENTER : JO_LEVEL_EVENT_EXIT, 0);
    if (crossing > 0 && jo_entity_get_int(entity, "once", 0))
        entity->flags |= JO_ENTITY_FLAG_DISABLED;
}

/*
** zone_link: a passage to another level (stairs going down to another level). When the player goes in its
** cylinder (or when it receives TRIGGER or USE), the change of level is asked to the game (see
** jo_entities_get_zone_request()): the game fades the screen, loads the level and puts the player on its spawn.
** Properties: level (STRING, the JOL file), spawn (STRING, the entity of the other level where the player appears)
*/

static char                         __jo_zone_level[16];
static char                         __jo_zone_spawn[24];
static bool                         __jo_zone_requested = false;

static void                         __jo_zone_link_request(jo_entity * const entity)
{
    const char                      *level;
    const char                      *spawn;

    level = jo_entity_get_string(entity, "level", "");
    spawn = jo_entity_get_string(entity, "spawn", "");
    if (*level == '\0' || __jo_zone_requested)
        return;
    jo_memcpy(__jo_zone_level, level, (unsigned int)JO_MIN(jo_strlen(level), (int)sizeof(__jo_zone_level) - 1));
    __jo_zone_level[JO_MIN(jo_strlen(level), (int)sizeof(__jo_zone_level) - 1)] = '\0';
    jo_memcpy(__jo_zone_spawn, spawn, (unsigned int)JO_MIN(jo_strlen(spawn), (int)sizeof(__jo_zone_spawn) - 1));
    __jo_zone_spawn[JO_MIN(jo_strlen(spawn), (int)sizeof(__jo_zone_spawn) - 1)] = '\0';
    __jo_zone_requested = true;
}

static void                         __jo_zone_link_update(jo_entity * const entity, const int elapsed)
{
    int                             crossing;

    JO_UNUSED_ARG(elapsed);
    crossing = __jo_entity_player_crossing(entity);
    // (the first update: the player may start in it, he must go out first)
    if (entity->state == 0)
    {
        entity->state = 1;
        return;
    }
    if (crossing == 0)
        return;
    jo_entity_fire(entity, crossing > 0 ? JO_LEVEL_EVENT_ENTER : JO_LEVEL_EVENT_EXIT, 0);
    if (crossing > 0)
        __jo_zone_link_request(entity);
}

static bool                         __jo_zone_link_action(jo_entity * const entity, const int action, const int param, jo_entity * const source)
{
    JO_UNUSED_ARG(param);
    JO_UNUSED_ARG(source);
    if (action != JO_LEVEL_ACTION_TRIGGER && action != JO_LEVEL_ACTION_USE)
        return (false);
    __jo_zone_link_request(entity);
    return (true);
}

bool                                jo_entities_get_zone_request(const char ** const level, const char ** const spawn)
{
    if (!__jo_zone_requested)
        return (false);
    *level = __jo_zone_level;
    *spawn = __jo_zone_spawn;
    return (true);
}

void                                jo_entities_clear_zone_request(void)
{
    __jo_zone_requested = false;
}

/** @brief explosive: what it loaded (shared by the explosives with the same properties) */
typedef struct
{
    char                            fire[12];       // Base names of the sprites, PCM file
    char                            smoke[12];
    char                            sound[16];
#ifdef JO_COMPILE_WITH_EFFECTS_SUPPORT
    jo_effect_def                   fireball;
    jo_effect_def                   smoke_puff;
#endif
#ifdef JO_COMPILE_WITH_AUDIO_SUPPORT
    jo_sound                        pcm;
    bool                            has_sound;
#endif
}                                   __jo_explosive_look;

# define __JO_EXPLOSIVE_MAX_LOOKS           (4)

static __jo_explosive_look          __jo_explosive_looks[__JO_EXPLOSIVE_MAX_LOOKS];
static int                          __jo_explosive_look_count = 0;

static void                         __jo_entities_free_looks(void)
{
#ifdef JO_COMPILE_WITH_AUDIO_SUPPORT
    int                             k;

    for (k = 0; k < __jo_explosive_look_count; ++k)
        if (__jo_explosive_looks[k].has_sound)
            jo_audio_free_pcm(&__jo_explosive_looks[k].pcm);
#endif
    __jo_explosive_look_count = 0;
}

#ifdef JO_COMPILE_WITH_EFFECTS_SUPPORT

/** @brief Load the frames of an animation: NAME1.BIN, NAME2.BIN...
 *  @return Number of frames loaded
 */
static int                          __jo_explosive_load_frames(const char * const name, short * const sprites, const int count)
{
    char                            filename[16];
    int                             len;
    int                             k;
    int                             id;

    len = jo_strlen(name);
    if (len == 0 || len > 8)
        return (0);
    jo_memcpy(filename, name, (unsigned int)len);
    for (k = 0; k < JO_MIN(count, JO_EFFECT_MAX_FRAMES); ++k)
    {
        filename[len] = (char)('1' + k);
        filename[len + 1] = '.';
        filename[len + 2] = 'B';
        filename[len + 3] = 'I';
        filename[len + 4] = 'N';
        filename[len + 5] = '\0';
        id = jo_sprite_add_bin(JO_ROOT_DIR, filename, JO_COLOR_Transparent);
        if (id < 0)
            break;
        // The VDP1 stops drawing each line after its last visible pixel
        jo_sprite_add_end_codes(id);
        sprites[k] = (short)id;
    }
    return (k);
}

#endif

static __jo_explosive_look          *__jo_explosive_get_look(const jo_entity * const entity)
{
    const char                      *fire;
    const char                      *smoke;
    const char                      *sound;
    __jo_explosive_look             *look;
    int                             k;

    fire = jo_entity_get_string(entity, "fire", "FIRE");
    smoke = jo_entity_get_string(entity, "smoke", "SMOKE");
    sound = jo_entity_get_string(entity, "sound", "");
    for (k = 0, look = __jo_explosive_looks; k < __jo_explosive_look_count; ++k, ++look)
        if (jo_strcmp(look->fire, fire) == 0 && jo_strcmp(look->smoke, smoke) == 0 && jo_strcmp(look->sound, sound) == 0)
            return (look);
    if (__jo_explosive_look_count >= __JO_EXPLOSIVE_MAX_LOOKS)
        return (&__jo_explosive_looks[0]);
    look = &__jo_explosive_looks[__jo_explosive_look_count++];
    jo_memset(look, 0, sizeof(*look));
    jo_memcpy(look->fire, fire, (unsigned int)JO_MIN(jo_strlen(fire) + 1, (int)sizeof(look->fire) - 1));
    jo_memcpy(look->smoke, smoke, (unsigned int)JO_MIN(jo_strlen(smoke) + 1, (int)sizeof(look->smoke) - 1));
    jo_memcpy(look->sound, sound, (unsigned int)JO_MIN(jo_strlen(sound) + 1, (int)sizeof(look->sound) - 1));
#ifdef JO_COMPILE_WITH_EFFECTS_SUPPORT
    // Fire: opaque (the VDP1 half transparency costs a lot on big sprites), it grows. Smoke: it goes up
    look->fireball.frame_count = (unsigned char)__jo_explosive_load_frames(fire, look->fireball.sprites,
                                                                           jo_entity_get_int(entity, "fire_frames", 4));
    look->fireball.life = 12;
    look->fireball.flags = JO_EFFECT_RANDOM_FLIP;
    look->fireball.size_start = 16;
    look->fireball.size_end = 40;
    look->fireball.rise = 8;
    look->smoke_puff.frame_count = (unsigned char)__jo_explosive_load_frames(smoke, look->smoke_puff.sprites,
                                                                             jo_entity_get_int(entity, "smoke_frames", 2));
    look->smoke_puff.life = 30;
    look->smoke_puff.flags = JO_EFFECT_RANDOM_FLIP;
    look->smoke_puff.size_start = 20;
    look->smoke_puff.size_end = 44;
    look->smoke_puff.rise = 20;
#endif
#ifdef JO_COMPILE_WITH_AUDIO_SUPPORT
    look->has_sound = *sound != '\0' && jo_audio_load_pcm(sound, JoSoundMono8Bit, &look->pcm);
#endif
    return (look);
}

/** @brief explosive: the state */
# define __JO_EXPLOSIVE_WAITING             (0)
# define __JO_EXPLOSIVE_BLAST               (1)     // Exploded, the damage around it comes after the delay
# define __JO_EXPLOSIVE_DONE                (2)

static void                         __jo_explosive_spawn(jo_entity * const entity)
{
    entity->data = __jo_explosive_get_look(entity);
    entity->state = __JO_EXPLOSIVE_WAITING;
    // (the dead model loaded with the level: its textures are sprites, loaded at the first explosion the sprites of the
    // level were used up in the middle of the game)
    (void)jo_entity_load_model(jo_entity_get_string(entity, "dead_model", ""));
}

static void                         __jo_explosive_death(jo_entity * const entity, jo_entity * const source)
{
    const __jo_explosive_look       *look;
    jo_3d_mesh                      *dead;
#ifdef JO_COMPILE_WITH_EFFECTS_SUPPORT
    jo_pos3D_fixed                  p;
    int                             size;
    int                             k;
    int                             dx;
    int                             dz;
    int                             near;
    int                             flash;
#endif

    JO_UNUSED_ARG(source);
    look = (const __jo_explosive_look *)entity->data;
    JO_UNUSED_ARG(look);
#ifdef JO_COMPILE_WITH_EFFECTS_SUPPORT
    // (module JO_COMPILE_WITH_EFFECTS_MODULE: without it, no fire, smoke, flash or shake)
    size = jo_entity_get_int(entity, "size", 100);
    // Fireballs one after the other around the middle, then the smoke
    if (look->fireball.frame_count > 0)
    {
        for (k = 0; k < 5; ++k)
        {
            p = entity->position;
            p.y -= jo_int2fixed(entity->height / 2);
            if (k > 0)
            {
                p.x += jo_int2fixed(((k & 1) ? 1 : -1) * size / 5);
                p.z += jo_int2fixed(((k & 2) ? 1 : -1) * size / 6);
                p.y -= jo_int2fixed(k * size / 10);
            }
            jo_effect_add(&look->fireball, &p, k);
        }
    }
    if (look->smoke_puff.frame_count > 0)
    {
        for (k = 0; k < 3; ++k)
        {
            p = entity->position;
            p.y -= jo_int2fixed(entity->height + k * size / 8);
            p.x += jo_int2fixed((k - 1) * size / 8);
            jo_effect_add(&look->smoke_puff, &p, 6 + k * 3);
        }
    }
    // Flash and shake: less far from the player
    dx = JO_ABS(jo_fixed2int(entity->position.x - __jo_entity_player.x));
    dz = JO_ABS(jo_fixed2int(entity->position.z - __jo_entity_player.z));
    near = JO_MAX(0, 256 - (dx + dz) * 256 / 3000);
    flash = jo_entity_get_int(entity, "flash", 120) * near / 256;
    if (flash > 0)
        jo_effect_flash(flash, flash * 3 / 4, flash / 3, 8);
    if (near > 64)
        jo_effect_shake(jo_entity_get_int(entity, "shake", 6) * near / 256, 12);
#endif
#ifdef JO_COMPILE_WITH_AUDIO_SUPPORT
    if (look->has_sound)
        jo_audio_play_sound((jo_sound *)&look->pcm);
#endif
    // The dead model (not solid without it), the damage around after the delay
    dead = jo_entity_load_model(jo_entity_get_string(entity, "dead_model", ""));
    jo_entity_set_mesh(entity, dead);
    if (dead == JO_NULL)
        entity->flags &= ~JO_ENTITY_FLAG_SOLID;
    entity->state = __JO_EXPLOSIVE_BLAST;
    entity->timer = JO_MAX(1, jo_entity_get_int(entity, "delay", 150));
}

static void                         __jo_explosive_update(jo_entity * const entity, const int elapsed)
{
    jo_pos3D_fixed                  center;

    if (entity->state != __JO_EXPLOSIVE_BLAST)
        return;
    entity->timer -= elapsed;
    if (entity->timer > 0)
        return;
    entity->timer = 0;
    entity->state = __JO_EXPLOSIVE_DONE;
    center = entity->position;
    center.y -= jo_int2fixed(entity->height / 2);
    jo_entities_damage_area(&center, jo_entity_get_int(entity, "radius", 200), jo_entity_get_int(entity, "damage", 100), entity);
}

/** @brief The angle equal to d modulo 360 in [-180, 180[ (as ((d % 360) + 540) % 360 - 180: the modulos were divisions
 *  by multiplications and calls of the library of the compiler; here only out of -719 to 719)
 *  @param d Difference of 2 angles (degrees)
 */
static  __jo_force_inline int       jo_entity_angle_difference(int d)
{
    // (the difference of 2 angles of 0 to 359 and -179 to 180: -539 to 359, one addition)
    if (d >= 720 || d <= -720)
        d %= 360;
    else if (d >= 360)
        d -= 360;
    else if (d <= -360)
        d += 360;
    if (d >= 180)
        d -= 360;
    else if (d < -180)
        d += 360;
    return (d);
}

int                                 jo_entity_turn_to(jo_entity * const entity, const jo_fixed x, const jo_fixed z, const int max_step)
{
    int                             target;
    int                             d;

    if (x == entity->position.x && z == entity->position.z)
        return (0);
    // (sin yaw, cos yaw) = the direction: the heading of the vector (dz, dx)
    target = jo_atan2_degrees(x - entity->position.x, z - entity->position.z);
    d = jo_entity_angle_difference(target - entity->yaw);
    if (d > max_step)
        entity->yaw += max_step;
    else if (d < -max_step)
        entity->yaw -= max_step;
    else
        entity->yaw += d;
    // (0 to 359; the modulo only out of -719 to 719)
    if (entity->yaw >= 720 || entity->yaw <= -720)
        entity->yaw %= 360;
    else if (entity->yaw >= 360)
        entity->yaw -= 360;
    else if (entity->yaw <= -360)
        entity->yaw += 360;
    if (entity->yaw < 0)
        entity->yaw += 360;
    d = jo_entity_angle_difference(target - entity->yaw);
    return (JO_ABS(d));
}

/*
** Actors
*/

/** @brief The model of a mesh of the entities (JO_NULL: none) */
static jo_3d_model                  *__jo_entity_model_of(const jo_3d_mesh * const mesh)
{
    int                             k;

    for (k = 0; k < __jo_entity_model_count; ++k)
        if (__jo_entity_models[k].model != JO_NULL && __jo_entity_models[k].model->mesh == mesh)
            return (__jo_entity_models[k].model);
    return (JO_NULL);
}

void                                jo_actor_spawn(jo_entity * const entity)
{
    jo_actor                        *a;

    a = (jo_actor *)entity->data;
    if (a == JO_NULL)
        return;
    a->model = __jo_entity_model_of(entity->mesh);
    a->far_model = jo_entity_get_model(jo_entity_get_string(entity, "far_model", ""));
    if (a->model != JO_NULL && a->model->bone_count == 0)
        a->model = JO_NULL;
    if (a->far_model != JO_NULL && a->far_model->bone_count != a->model->bone_count)
        a->far_model = JO_NULL;
    a->animation = -1;
    a->previous = -1;
    jo_actor_play(entity, jo_entity_get_string(entity, "animation", "idle"), 0);
    // (the time of the animation: a crowd doesn't move together)
    a->time = (int)(entity - __jo_entities) * 337;
}

void                                jo_actor_update(jo_entity * const entity, const int elapsed)
{
    jo_actor                        *a;

    a = (jo_actor *)entity->data;
    if (a == JO_NULL)
        return;
    a->time += elapsed;
    a->previous_time += elapsed;
    if (a->blend > 0)
        a->blend -= elapsed;
    // (the time stays small: 1 hour)
    if (a->time > 3600000)
        a->time -= 3000000;
}

bool                                jo_actor_restart(jo_entity * const entity, const char * const name, const int blend)
{
    jo_actor                        *a;
    int                             animation;

    a = (jo_actor *)entity->data;
    if (a == JO_NULL || a->model == JO_NULL)
        return (false);
    animation = jo_3d_model_find_animation(a->model, name);
    if (animation < 0)
        return (false);
    a->previous = a->animation;
    a->previous_time = a->time;
    a->blend = a->previous >= 0 ? blend : 0;
    a->blend_duration = blend;
    a->animation = (short)animation;
    a->time = 0;
    return (true);
}

bool                                jo_actor_play(jo_entity * const entity, const char * const name, const int blend)
{
    const jo_actor                  *a;

    a = (const jo_actor *)entity->data;
    if (a == JO_NULL || a->model == JO_NULL)
        return (false);
    if (a->animation >= 0 && jo_actor_is_playing(entity, name))
        return (true);
    return (jo_actor_restart(entity, name, blend));
}

bool                                jo_actor_is_playing(const jo_entity * const entity, const char * const name)
{
    const jo_actor                  *a;

    a = (const jo_actor *)entity->data;
    return (a != JO_NULL && a->model != JO_NULL && a->animation >= 0 && jo_3d_model_is_animation(a->model, a->animation, name));
}

bool                                jo_actor_is_done(const jo_entity * const entity)
{
    const jo_actor                  *a;

    a = (const jo_actor *)entity->data;
    if (a == JO_NULL || a->model == JO_NULL || a->animation < 0 ||
        (a->model->animations[a->animation].flags & JO_3D_ANIMATION_LOOP))
        return (false);
    return (a->time * JO_3D_ANIMATION_TICKS_PER_SECOND / 1000 >= jo_3d_model_animation_duration(a->model, a->animation));
}

/** @brief Called after each bone: the callback of the actor with its entity */
static void                         __jo_actor_bone(const int bone, void * const param, const jo_fixed (* const matrix)[3])
{
    jo_entity                       *entity;

    entity = (jo_entity *)param;
    ((jo_actor *)entity->data)->bone_callback(entity, bone, matrix);
}

void                                jo_actor_draw(jo_entity * const entity)
{
    jo_actor                        *a;
    jo_3d_model                     *model;
    jo_3d_pose                      pose;
    jo_3d_pose                      previous;
    int                             distance;

    a = (jo_actor *)entity->data;
    if (a == JO_NULL || a->model == JO_NULL)
        return;
    distance = jo_entity_view_distance(entity);
    if (distance < 0)
        return;
    model = a->far_model != JO_NULL && distance > entity->far_distance ? a->far_model : a->model;
    jo_3d_model_get_pose(model, a->animation, a->time * JO_3D_ANIMATION_TICKS_PER_SECOND / 1000, &pose);
    if (a->blend > 0 && a->blend_duration > 0)
    {
        // The change of animation: from the pose of the previous animation (it goes on during the change)
        jo_3d_model_get_pose(model, a->previous, a->previous_time * JO_3D_ANIMATION_TICKS_PER_SECOND / 1000, &previous);
        jo_3d_pose_blend(model, &previous, &pose, 256 - jo_int_div(a->blend << 8, a->blend_duration));
        pose = previous;
    }
    jo_3d_push_matrix();
    {
        jo_3d_translate_matrix_fixed(entity->position.x, entity->position.y, entity->position.z);
        if (entity->yaw != 0)
            jo_3d_rotate_matrix_y(entity->yaw);
        jo_3d_model_draw_pose(model, &pose, a->bone_callback != JO_NULL ? __jo_actor_bone : JO_NULL, entity);
    }
    jo_3d_pop_matrix();
}

void                                jo_actor_draw_slave(jo_entity * const entity, const int far_distance, const unsigned int hidden_bones)
{
    jo_actor                        *a;
    jo_3d_model                     *model;
    jo_3d_pose                      pose;
    jo_3d_pose                      previous;

    a = (jo_actor *)entity->data;
    if (a == JO_NULL || a->model == JO_NULL || entity->view_distance < 0)
        return;
    model = a->far_model != JO_NULL && entity->view_distance > (far_distance < 0 ? entity->far_distance : far_distance) ? a->far_model : a->model;
    jo_3d_model_get_pose(model, a->animation, a->time * JO_3D_ANIMATION_TICKS_PER_SECOND / 1000, &pose);
    if (a->blend > 0 && a->blend_duration > 0)
    {
        jo_3d_model_get_pose(model, a->previous, a->previous_time * JO_3D_ANIMATION_TICKS_PER_SECOND / 1000, &previous);
        jo_3d_pose_blend(model, &previous, &pose, 256 - jo_int_div(a->blend << 8, a->blend_duration));
        pose = previous;
    }
    if (!jo_3d_slave_push_matrix())
        return;
    jo_3d_slave_translate_matrix_fixed(entity->position.x, entity->position.y, entity->position.z);
    if (entity->yaw != 0)
        jo_3d_slave_rotate_matrix(0, entity->yaw, 0);
    jo_3d_slave_model_draw_pose(model, &pose, a->bone_callback != JO_NULL ? __jo_actor_bone : JO_NULL, entity, hidden_bones);
    jo_3d_slave_pop_matrix();
}

static const jo_entity_class        __jo_entity_class_entity = {"entity", 0, JO_NULL, JO_NULL, JO_NULL, JO_NULL, JO_NULL, JO_NULL, JO_NULL};
static const jo_entity_class        __jo_entity_class_relay = {"relay", 0, JO_NULL, JO_NULL, JO_NULL, JO_NULL, JO_NULL, JO_NULL, JO_NULL};
static const jo_entity_class        __jo_entity_class_trigger = {"trigger", 0, JO_NULL, __jo_trigger_update, JO_NULL, JO_NULL, JO_NULL,
                                                                 JO_NULL, JO_NULL};
static const jo_entity_class        __jo_entity_class_spawn = {"spawn", 0, JO_NULL, JO_NULL, JO_NULL, JO_NULL, JO_NULL, JO_NULL, JO_NULL};
static const jo_entity_class        __jo_entity_class_zone_link = {"zone_link", 0, JO_NULL, __jo_zone_link_update, __jo_zone_link_action,
                                                                   JO_NULL, JO_NULL, JO_NULL, JO_NULL};
static const jo_entity_class        __jo_entity_class_explosive = {"explosive", 0, __jo_explosive_spawn, __jo_explosive_update,
                                                                   JO_NULL, __jo_explosive_death, JO_NULL, JO_NULL, JO_NULL};
/** @brief An actor: drawn by both CPUs (jo_actor_draw_slave() on the slave CPU) */
static void                         __jo_actor_draw_slave(jo_entity * const entity)
{
    jo_actor_draw_slave(entity, -1, 0);
}

static const jo_entity_class        __jo_entity_class_actor = {"actor", sizeof(jo_actor), jo_actor_spawn, jo_actor_update, JO_NULL,
                                                               JO_NULL, jo_actor_draw, __jo_actor_draw_slave, JO_NULL};

static void                         __jo_entity_register_engine_classes(void)
{
    // (a class of the game with the same name, registered before, stays)
    if (__jo_entity_find_class("entity") == JO_NULL)
        jo_entity_register_class(&__jo_entity_class_entity);
    if (__jo_entity_find_class("relay") == JO_NULL)
        jo_entity_register_class(&__jo_entity_class_relay);
    if (__jo_entity_find_class("trigger") == JO_NULL)
        jo_entity_register_class(&__jo_entity_class_trigger);
    if (__jo_entity_find_class("spawn") == JO_NULL)
        jo_entity_register_class(&__jo_entity_class_spawn);
    if (__jo_entity_find_class("zone_link") == JO_NULL)
        jo_entity_register_class(&__jo_entity_class_zone_link);
    if (__jo_entity_find_class("explosive") == JO_NULL)
        jo_entity_register_class(&__jo_entity_class_explosive);
    if (__jo_entity_find_class("actor") == JO_NULL)
        jo_entity_register_class(&__jo_entity_class_actor);
    if (__jo_entity_find_class("vehicle") == JO_NULL)
        jo_entity_register_class(&__jo_vehicle_class);
}

#endif /* !JO_COMPILE_WITH_LEVEL_SUPPORT && JO_COMPILE_WITH_3D_SUPPORT */

/*
** END OF FILE
*/
