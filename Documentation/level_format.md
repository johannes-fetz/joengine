# Jo Engine level file (JOL) {#level_format}

*This file is made by `Compiler/COMMON/jo_level/gen.py` from `Compiler/COMMON/jo_level/schema.py`: change the schema, not this file.*

A level (`.JOL`) holds the data of a level of a game: terrain, models and their collisions, plants, decals... The engine loads it with `jo_level_load()` (module `JO_COMPILE_WITH_LEVEL_MODULE = 1`); the tools of `Compiler/COMMON/jo_level` write it (Python), check it and print it.

## Rules of the format

1. The file is big endian (the SH-2 reads it directly). Every value is aligned on its size, the sections on 4 bytes.
2. The file starts with a header (FILE_HEADER) and a directory: one entry (DIRECTORY_ENTRY) per section.
3. A section has an identifier of 4 characters, a version, flags, a header (fields), records (fields, all the same size)
   and arrays (referenced by offsets in its header, from the start of the section).
4. The sizes of the header and of a record are written in the directory: a reader reads the fields it knows and skips
   the rest; a field that is not in the file (older file) gets its default value.
5. A new field is always added at the end of a header or of a record, with a default value and the version of the
   section where it appeared (the version of the section goes up).
6. A field, a section identifier or a value of an enumeration is never removed, moved or used again for something else.
   A field that is no longer used stays in the format (its documentation says so).
7. A reader ignores the sections it doesn't know, unless the flag REQUIRED is set: then it refuses the file with the
   identifier of the section in the error message.
8. The identifiers in uppercase belong to the engine; the identifiers with a lowercase letter belong to the game (the
   engine loads them and gives them to the game, see jo_level_find_section()).
9. The version of the file (FORMAT_VERSION) only changes if the header or the directory change.


## Types

| Type | Size | Description |
|---|---|---|
| u8, i8, u16, i16, u32, i32 | 1, 2, 4 | Integers (big endian) |
| fixed | 4 | Q16.16 fixed point number (jo_fixed) |
| string | 4 | Offset of a string in the STRS section, 0xFFFFFFFF: no string |
| offset | 4 | Offset of an array from the start of the section (0: empty) |
| index | 2 | Index of a record of another section, 0xFFFF: none |

## File header

| Offset | Field | Type | Default | Since | Description |
|---|---|---|---|---|---|
| 0 | `magic` | u32 | 0 | v1 | 'JOLV' |
| 4 | `version` | u16 | 0 | v1 | Version of the file format (FORMAT_VERSION: header and directory) |
| 6 | `flags` | u16 | 0 | v1 | Reserved (0) |
| 8 | `size` | u32 | 0 | v1 | Size of the file (bytes) |
| 12 | `section_count` | u32 | 0 | v1 | Number of entries of the directory (after this header) |

Size: 16 bytes.

## Directory entry

One entry per section, after the file header.

| Offset | Field | Type | Default | Since | Description |
|---|---|---|---|---|---|
| 0 | `id` | u32 | 0 | v1 | Identifier: 4 characters |
| 4 | `version` | u16 | 0 | v1 | Version of the section |
| 6 | `flags` | u16 | 0 | v1 | FLAG_xxx (REQUIRED: 1) |
| 8 | `offset` | u32 | 0 | v1 | Offset of the section from the start of the file (multiple of 4) |
| 12 | `size` | u32 | 0 | v1 | Size of the section (bytes, header, records and arrays) |
| 16 | `header_size` | u16 | 0 | v1 | Size of the header of the section (bytes) |
| 18 | `record_size` | u16 | 0 | v1 | Size of a record (bytes) |
| 20 | `record_count` | u32 | 0 | v1 | Number of records (after the header) |

Size: 24 bytes.

## Sections

### INFO: Level (version 1, required)

The level: its name. The first section of the file.

Header:

| Offset | Field | Type | Default | Since | Description |
|---|---|---|---|---|---|
| 0 | `name` | string | 0xFFFFFFFF | v1 | Name of the level |
| 4 | `flags` | u32 | 0 | v1 | Reserved (0) |

Size: 8 bytes.

### STRS: Strings (version 1)

Strings of the level (names of files, of classes...): ASCII, each one ended by a zero. A field of type string is the offset of the string in this section.

### TERR: Terrain (version 1)

Height map of the ground: n x n vertices, one every cell units. The vertex (i, j) is at x = origin_x + (i - (n - 1) / 2) x cell, z = origin_z + (j - (n - 1) / 2) x cell. The meshes of the ground are built from it when the level is loaded.

Header:

| Offset | Field | Type | Default | Since | Description |
|---|---|---|---|---|---|
| 0 | `n` | u16 | 0 | v1 | Vertices per side |
| 2 | `cell` | u16 | 0 | v1 | Distance between 2 vertices (world units) |
| 4 | `origin_x` | i32 | 0 | v1 | Center of the height map (world units) |
| 8 | `origin_z` | i32 | 0 | v1 | Center of the height map (world units) |
| 12 | `heights` | offset | 0 | v1 | Heights: n x n i16, row after row (j), Y up (world units) |
| 16 | `colors` | offset | 0 | v1 | Colors of the vertices: n x n RGB 555 of the Saturn (the light of the sun is baked) |
| 20 | `hole_count` | u32 | 0 | v1 | Number of entries of holes |
| 24 | `holes` | offset | 0 | v1 | Vertices under a building: u16 j x n + i. A quad with its 4 vertices in this list is not drawn. With the flag 0x8000 the quad is drawn, sorted behind the building |

Size: 28 bytes.

Arrays (after the header and the records, aligned on 4 bytes):

- `heights`: i16, n * n elements (offset: `heights`). Heights.
- `colors`: u16, n * n elements (offset: `colors`). Colors.
- `holes`: u16, hole_count elements (offset: `holes`). Holes.

### PLNT: Plants (version 1)

Plants and rocks placed on the ground.

Record:

| Offset | Field | Type | Default | Since | Description |
|---|---|---|---|---|---|
| 0 | `x` | i32 | 0 | v1 | Position (world units) |
| 4 | `y` | i32 | 0 | v1 | Height of the foot, Y up (world units) |
| 8 | `z` | i32 | 0 | v1 | Position (world units) |
| 12 | `kind` | u16 | 0 | v1 | Kind: 0 palm tree, 1 bush, 2 fern, 3 rock, 4 tree of the inland |
| 14 | `angle` | i16 | 0 | v1 | Rotation around Y (degrees) |
| 16 | `scale` | u16 | 0 | v1 | Scale x 256 (256: 1) |

Size: 20 bytes.

### MODL: Models (version 1)

3D models of the level (JOM files) and their collisions.

Record:

| Offset | Field | Type | Default | Since | Description |
|---|---|---|---|---|---|
| 0 | `file` | string | 0xFFFFFFFF | v1 | JOM file (in the root directory of the CD) |
| 4 | `first_box` | u16 | 0 | v1 | First collision box (MBOX) |
| 6 | `box_count` | u16 | 0 | v1 | Number of collision boxes |
| 8 | `has_floor` | u16 | 0 | v1 | 1: the model has a floor the player can walk on |
| 10 | `floor` | i16[5] | 0 | v1 | Floor: x0, z0, x1, z1 (model space), height (Y up) (world units) |

Size: 20 bytes.

### MBOX: Collision boxes (version 1)

Collision boxes of the models (vertical boxes, model space).

Record:

| Offset | Field | Type | Default | Since | Description |
|---|---|---|---|---|---|
| 0 | `x` | i16 | 0 | v1 | Center (world units) |
| 2 | `z` | i16 | 0 | v1 | Center (world units) |
| 4 | `half_x` | i16 | 0 | v1 | Half size (world units) |
| 6 | `half_z` | i16 | 0 | v1 | Half size (world units) |

Size: 8 bytes.

### INST: Instances (version 1)

Models placed in the level (buildings).

Record:

| Offset | Field | Type | Default | Since | Description |
|---|---|---|---|---|---|
| 0 | `model` | index | 0xFFFF | v1 | Model seen from far away (MODL) |
| 2 | `near_model` | index | 0xFFFF | v1 | Model seen from near (MODL), NO_INDEX: the same |
| 4 | `inside_model` | index | 0xFFFF | v1 | Model of the inside, drawn from near (MODL), NO_INDEX: none |
| 6 | `yaw` | i16 | 0 | v1 | Rotation around Y (degrees) |
| 8 | `x` | i32 | 0 | v1 | Position (world units) |
| 12 | `y` | i32 | 0 | v1 | Position, Saturn Y (down) (world units) |
| 16 | `z` | i32 | 0 | v1 | Position (world units) |
| 20 | `half_x` | i16 | 0 | v1 | Collision box when the model has none (model space), 0: no collision (world units) |
| 22 | `half_z` | i16 | 0 | v1 | Collision box when the model has none (world units) |

Size: 24 bytes.

### DECL: Decals (version 1)

Flat decals lying at the sea level on the bottom of the lagoon.

Record:

| Offset | Field | Type | Default | Since | Description |
|---|---|---|---|---|---|
| 0 | `kind` | u16 | 0 | v1 | Kind: 0 coral, 1 brain coral, 2 starfish, 3 rocks, 4 sea grass, 5 anchor, 6 barrel |
| 2 | `angle` | i16 | 0 | v1 | Rotation around Y (degrees) |
| 4 | `size` | u16 | 0 | v1 | Size (world units) |
| 8 | `x` | i32 | 0 | v1 | Position (world units) |
| 12 | `z` | i32 | 0 | v1 | Position (world units) |

Size: 16 bytes.

### ENTI: Entities (version 1)

The things of the level that live: an entity has a class (its behavior, given by the engine or by the game), a position, a health, properties (PROP) and links to other entities (LINK). See jo_engine/jo/entity.h and the classes of the engine in Documentation/level_format.md.

Record:

| Offset | Field | Type | Default | Since | Description |
|---|---|---|---|---|---|
| 0 | `name` | string | 0xFFFFFFFF | v1 | Name (unique in the level: the links and the game find the entity by it) |
| 4 | `cls` | string | 0xFFFFFFFF | v1 | Class: a class of the engine (explosive, relay, trigger...) or of the game |
| 8 | `model` | string | 0xFFFFFFFF | v1 | JOM file drawn at its position, NO_STRING: none |
| 12 | `x` | i32 | 0 | v1 | Position (world units) |
| 16 | `y` | i32 | 0 | v1 | Position, Saturn Y (down): the foot of the entity (world units) |
| 20 | `z` | i32 | 0 | v1 | Position (world units) |
| 24 | `yaw` | i16 | 0 | v1 | Rotation around Y (degrees) |
| 26 | `health` | i16 | 0 | v1 | Health at the start, 0: it can't be damaged |
| 28 | `flags` | u16 | 0 | v1 | ENTITY_FLAG_xxx: DISABLED 1, SOLID 2, HIDDEN 4 |
| 30 | `radius` | u16 | 0 | v1 | Cylinder (collisions, bullets, zone of a trigger): radius (world units) |
| 32 | `height` | u16 | 0 | v1 | Cylinder: height above the foot (world units) |
| 34 | `first_prop` | u16 | 0 | v1 | First property (PROP) |
| 36 | `prop_count` | u16 | 0 | v1 | Number of properties |
| 38 | `first_link` | u16 | 0 | v1 | First link of which it is the source (LINK) |
| 40 | `link_count` | u16 | 0 | v1 | Number of links |

Size: 44 bytes.

### PROP: Properties (version 1)

Properties of the entities: a name and a value (PROPERTY_xxx type). The classes read them by name, with a default value when the property is missing.

Record:

| Offset | Field | Type | Default | Since | Description |
|---|---|---|---|---|---|
| 0 | `key` | string | 0xFFFFFFFF | v1 | Name |
| 4 | `type` | u16 | 0 | v1 | PROPERTY_xxx: INT 0, FIXED 1, STRING 2 |
| 8 | `value` | i32 | 0 | v1 | Value (the offset of the string for a STRING) |

Size: 12 bytes.

### LINK: Links (version 1)

Links between the entities: when the event of the source happens, the action is sent to the target after the delay. The links of an entity follow each other (ENTI.first_link).

Record:

| Offset | Field | Type | Default | Since | Description |
|---|---|---|---|---|---|
| 0 | `event` | u16 | 0 | v1 | EVENT_xxx of the source |
| 2 | `action` | u16 | 0 | v1 | ACTION_xxx sent to the target |
| 4 | `target` | index | 0xFFFF | v1 | Target (ENTI), NO_INDEX: the source itself |
| 6 | `delay` | u16 | 0 | v1 | Delay (ms) |
| 8 | `param` | i32 | -1 | v1 | Parameter of the action (the damage of DAMAGE), -1: the parameter of the event |

Size: 12 bytes.

### CELL: Cells (version 3)

The rooms of an inside level (a base, a building): each cell has its model (walls, floor, ceiling, in world coordinates) and a box where the player can walk, with its floor (flat or a slope: the stairs). The cells overlap at the doors: the player walks from one to the other. The engine draws the cell of the camera and the cells seen through its portals (PRTL), see jo_engine/jo/cell.h.

Record:

| Offset | Field | Type | Default | Since | Description |
|---|---|---|---|---|---|
| 0 | `model` | string | 0xFFFFFFFF | v1 | JOM file of the cell (world coordinates) |
| 4 | `x0` | i32 | 0 | v1 | Box where the player can walk: smallest X (world units) |
| 8 | `z0` | i32 | 0 | v1 | Smallest Z (world units) |
| 12 | `x1` | i32 | 0 | v1 | Biggest X (world units) |
| 16 | `z1` | i32 | 0 | v1 | Biggest Z (world units) |
| 20 | `floor0` | i32 | 0 | v1 | Floor, Saturn Y (down): at x0 (slope X), at z0 (slope Z) or everywhere (flat) (world units) |
| 24 | `floor1` | i32 | 0 | v1 | Floor at x1 (slope X) or at z1 (slope Z) (world units) |
| 28 | `slope` | u16 | 0 | v1 | CELL_SLOPE_xxx: FLAT 0, X 1, Z 2 |
| 30 | `first_portal` | u16 | 0 | v1 | First portal of the cell (PRTL) |
| 32 | `portal_count` | u16 | 0 | v1 | Number of portals |
| 34 | `yaw` | i16 | 0 | v2 | Rotation of the box around its center (Y axis, the same as the yaw of INST): a building turned on the terrain (degrees) |
| 36 | `reflection` | string | 0xFFFFFFFF | v3 | JOM file drawn in the mirrors of the cell instead of its model (a simpler model: bigger quads, no floor), NO_STRING: its model and the cells next to it |

Size: 40 bytes.

### PRTL: Portals (version 1)

Openings of the cells (doors, windows, stairs): a quad through which another cell is seen. The portals of a cell follow each other (CELL.first_portal).

Record:

| Offset | Field | Type | Default | Since | Description |
|---|---|---|---|---|---|
| 0 | `cell` | index | 0xFFFF | v1 | Cell seen through the portal (CELL), NO_INDEX: the outside (the terrain around a building, see jo_cells_outside_view()) |
| 2 | `kind` | u16 | 0 | v1 | PORTAL_xxx: DOOR 0 (an opening), MIRROR 1 (a mirror of the cell), WINDOW 2 (seen through from the cell only) |
| 4 | `points` | i32[12] | 0 | v1 | The 4 corners: x, y (Saturn Y), z of each corner (world units) |

Size: 52 bytes.

### WEAP: Weapons (version 1)

Definitions of the weapons of the player (and later of the enemies): a file of definitions shared by the levels (for instance DEFS.JOL). The engine runs them (ammo, fire rate, reload, change of weapon, see jo_engine/jo/weapon.h), the game draws them. The times are frames of the game.

Record:

| Offset | Field | Type | Default | Since | Description |
|---|---|---|---|---|---|
| 0 | `name` | string | 0xFFFFFFFF | v1 | Name |
| 4 | `model` | string | 0xFFFFFFFF | v1 | JOM file of the weapon and the hands (first person view) |
| 8 | `x` | fixed | 0 | v1 | Position of the model in front of the camera: right (world units, Q16.16) |
| 12 | `y` | fixed | 0 | v1 | Down (world units, Q16.16) |
| 16 | `z` | fixed | 0 | v1 | Forward (world units, Q16.16) |
| 20 | `muzzle_y` | fixed | 0 | v1 | The muzzle in the model: height (the axis of the barrel) (world units, Q16.16) |
| 24 | `muzzle_z` | fixed | 0 | v1 | The muzzle in the model: forward (world units, Q16.16) |
| 28 | `convergence` | u16 | 0x258 | v1 | The axis of the barrel crosses the line of sight at this distance (the crosshair) (world units) |
| 30 | `automatic` | u16 | 0 | v1 | 1: it fires while the button is held, 0: one shot per press |
| 32 | `fire_delay` | u16 | 4 | v1 | Frames between 2 shots |
| 34 | `damage` | u16 | 0x19 | v1 | Damage of a bullet |
| 36 | `spread` | u16 | 0 | v1 | Spread of the bullets: the offset at 1000 units (world units) |
| 38 | `recoil` | u16 | 0xA | v1 | The model goes back (frames of recoil) |
| 40 | `kick` | u16 | 2 | v1 | The view goes up at each shot (degrees) |
| 42 | `magazine` | u16 | 0x1E | v1 | Bullets of a magazine |
| 44 | `ammo_type` | u16 | 0 | v1 | Type of ammunition (the weapons with the same type share their reserve) |
| 46 | `ammo_start` | u16 | 0 | v1 | Reserve at the start (not in the magazine) |
| 48 | `ammo_max` | u16 | 0x3E7 | v1 | Reserve at most |
| 50 | `reload_time` | u16 | 0x32 | v1 | Frames of the reload |
| 52 | `switch_time` | u16 | 0xC | v1 | Frames to take this weapon (it comes up), the other one goes down in the same time |
| 56 | `flash` | string | 0xFFFFFFFF | v1 | Base name of the muzzle flashes (FLASH: FLASH1.BIN, FLASH2.BIN, FLASH3.BIN) |
| 60 | `flash_size` | u16 | 0x16 | v1 | Size of the muzzle flash (pixels) |
| 62 | `casing` | u16 | 0 | v1 | 1: the casings are ejected |
| 64 | `sound_shot` | string | 0xFFFFFFFF | v1 | PCM file of the shot |
| 68 | `sound_reload` | string | 0xFFFFFFFF | v1 | PCM file of the reload |
| 72 | `shot_volume` | u16 | 0x78 | v1 | Volume of the shot (0 to 127) |

Size: 76 bytes.

## Enumerations

### EVENT

Events of an entity (LINK.event): what happened to the source.

| Value | Name | Description |
|---|---|---|
| 0 | `EVENT_TRIGGER` | The entity received the action TRIGGER or USE (a relay, a switch that is used) |
| 1 | `EVENT_ENTER` | The player entered the entity (a trigger zone) |
| 2 | `EVENT_EXIT` | The player left the entity (a trigger zone) |
| 3 | `EVENT_DAMAGE` | The entity received damage and is still alive (param: the damage) |
| 4 | `EVENT_DEATH` | The health of the entity reached 0 (an explosive explodes) |

### ACTION

Actions sent to an entity (LINK.action): what the target does.

| Value | Name | Description |
|---|---|---|
| 0 | `ACTION_TRIGGER` | Fire the event TRIGGER of the target (a relay passes it on) |
| 1 | `ACTION_DAMAGE` | Damage the target (param: the damage) |
| 2 | `ACTION_KILL` | The health of the target goes to 0 (event DEATH) |
| 3 | `ACTION_ENABLE` | The target is active again (drawn, solid, it reacts) |
| 4 | `ACTION_DISABLE` | The target is not active (not drawn, not solid, it ignores the actions but ENABLE) |
| 5 | `ACTION_TOGGLE` | ENABLE or DISABLE |
| 6 | `ACTION_USE` | The target is used (like the player does: a switch, a door). The default: TRIGGER |
| 7 | `ACTION_OPEN` | Open the target (a door, a mover) |
| 8 | `ACTION_CLOSE` | Close the target (a door, a mover) |

### CELL_SLOPE

Floors of the cells (CELL.slope).

| Value | Name | Description |
|---|---|---|
| 0 | `CELL_SLOPE_FLAT` | The floor is flat (floor0) |
| 1 | `CELL_SLOPE_X` | The floor goes from floor0 at x0 to floor1 at x1 (stairs along X) |
| 2 | `CELL_SLOPE_Z` | The floor goes from floor0 at z0 to floor1 at z1 (stairs along Z) |

### PORTAL

Kinds of portals (PRTL.kind).

| Value | Name | Description |
|---|---|---|
| 0 | `PORTAL_DOOR` | An opening: the other cell is seen through it |
| 1 | `PORTAL_MIRROR` | A mirror: the cell (and the cells next to it) is drawn again, reflected by the plane of the portal, clipped to its rectangle on the screen (see jo_cells_draw_mirror()). The wall has a hole there |
| 2 | `PORTAL_WINDOW` | An opening seen through from the cell only (an embrasure, a window too high): from the other side, the cell is not seen through it (the hole is covered, for instance by a dark polygon) |

### PROPERTY

Types of the properties of the entities (PROP.type).

| Value | Name | Description |
|---|---|---|
| 0 | `PROPERTY_INT` | value is an integer (i32) |
| 1 | `PROPERTY_FIXED` | value is a Q16.16 fixed point number |
| 2 | `PROPERTY_STRING` | value is the offset of a string in STRS |

### ENTITY_FLAG

Flags of the entities (ENTI.flags).

| Value | Name | Description |
|---|---|---|
| 0x0001 | `ENTITY_FLAG_DISABLED` | Not active at the start (see the action ENABLE) |
| 0x0002 | `ENTITY_FLAG_SOLID` | The player and the bullets collide with its cylinder (radius, height) |
| 0x0004 | `ENTITY_FLAG_HIDDEN` | Its model is not drawn (a trigger zone, a relay) |

## Classes of entities of the engine

The value of `ENTI.cls`. A game registers its own classes with `jo_entity_register_class()` (a class with the name of a class of the engine replaces it). A property missing in the level gets its default value.

### all the classes

Properties of all the entities (the classes of the game too).

| Property | Type | Default | Description |
|---|---|---|---|
| `far_model` | STRING | "" | JOM file drawn farther than far_distance (fewer polygons), "": the model |
| `far_distance` | INT | 600 | The far model is drawn farther (world units, on the ground) |
| `draw_distance` | INT | 2000 | The model is not drawn farther (world units, on the ground): each polygon costs CPU time, even small far away |
| `box_x` | INT | 0 | With box_z: the collisions of the player and the vehicles use a box turned with the entity (half sizes, world units) instead of its cylinder (a partition, a counter). The bullets keep the cylinder |
| `box_z` | INT | 0 | See box_x |

### entity

The default class: its model at its position. An entity with an unknown class is an entity.

### relay

Passes the action TRIGGER (or USE) on: its links TRIGGER send their actions, with their delays (a sequence of events: a fuse, a chain of doors). Flag HIDDEN.

### trigger

A zone: its cylinder (radius, height above the foot). Fires ENTER when the player goes in, EXIT when he goes out. Flag HIDDEN.

| Property | Type | Default | Description |
|---|---|---|---|
| `once` | INT | 0 | 1: the zone is disabled after the first ENTER |

### spawn

A place where the player appears (the start of a level, the end of a zone_link): its position and its yaw. Flag HIDDEN.

### zone_link

A passage to another level (stairs, a door, an elevator): when the player goes in its cylinder (or when it receives TRIGGER or USE), the game is asked to load the level and to put the player on the spawn (jo_entities_get_zone_request()). The player must go out of it before (he may start in it). Flag HIDDEN.

| Property | Type | Default | Description |
|---|---|---|---|
| `level` | STRING | "" | JOL file of the level |
| `spawn` | STRING | "" | Name of the entity (class spawn) of the level where the player appears |

### explosive

Explodes when its health reaches 0 (bullets, an explosion near it, the action KILL): fireballs and smoke (sprites NAME1.BIN, NAME2.BIN... opaque with end codes), flash of the screen, shake of the camera (module JO_COMPILE_WITH_EFFECTS_MODULE = 1), sound, then its dead model. After the delay, it damages what is around it (the other explosives: chain reactions) and the player (the game gets the damage, see jo_entities_set_area_damage_callback()). Flag SOLID: the bullets hit it.

| Property | Type | Default | Description |
|---|---|---|---|
| `damage` | INT | 100 | Damage at the center (less farther, 0 at the radius) |
| `radius` | INT | 200 | Radius of the damage (world units) |
| `delay` | INT | 150 | Delay of the damage after the explosion (ms) |
| `size` | INT | 100 | Size of the fireball (world units) |
| `fire` | STRING | "FIRE" | Base name of the frames of the fire (FIRE1.BIN to FIRE4.BIN) |
| `fire_frames` | INT | 4 | Number of frames of the fire (1 to 4) |
| `smoke` | STRING | "SMOKE" | Base name of the frames of the smoke |
| `smoke_frames` | INT | 2 | Number of frames of the smoke (0: no smoke, up to 4) |
| `sound` | STRING | "" | PCM file (8 bits mono), "": none |
| `dead_model` | STRING | "" | JOM file drawn after the explosion, "": nothing (and not solid any more) |
| `flash` | INT | 120 | Brightness of the flash of the screen (0 to 255), less far from the player |
| `shake` | INT | 6 | Shake of the camera (world units), less far from the player |

### actor

An animated model (a skeleton, see jo_3d_model_draw_pose()): plays an animation (a soldier who waits, a flag). The classes of the game (enemies) are actors too (see jo_actor_spawn()). The far model has the same skeleton.

| Property | Type | Default | Description |
|---|---|---|---|
| `animation` | STRING | "idle" | Name of the animation of the model played at the start |

### vehicle

A vehicle driven by the player (see jo/vehicle.h): its model has a skeleton with the bones wheel_fl, wheel_fr, wheel_rl, wheel_rr (they turn), steer_fl, steer_fr (they steer) and steering (the steering wheel). The 4 wheels follow the ground (the game gives it, see jo_vehicles_set_ground_callback()), the solid entities stop it (a hit at speed damages them). Flag SOLID: the player walks around it.

| Property | Type | Default | Description |
|---|---|---|---|
| `cockpit_model` | STRING | "" | JOM file drawn when the player drives (what the driver sees, the same skeleton), "": the model |
| `wheelbase` | INT | 106 | Distance between the front and the rear axles (world units) |
| `track` | INT | 78 | Distance between the left and the right wheels (world units) |
| `wheel_radius` | INT | 19 | Radius of the wheels (world units) |
| `max_steer` | INT | 30 | Angle of the front wheels at most (degrees) |
| `max_speed` | INT | 900 | Top speed (world units per second; backward: a third) |
| `acceleration` | INT | 280 | Acceleration of the engine (world units per second per second) |
| `braking` | INT | 800 | Deceleration of the brakes (world units per second per second) |
| `impact` | INT | 150 | Damage of a hit at the top speed |

## Reserved identifiers

For the next versions of the engine:

- `ENVI`: Environment: sky, sea, sun, palette, music
- `MATL`: Materials: effects and sounds of the impacts, steps, walkable, blocking, grip
- `COLL`: Collision geometry (static, dynamic)
- `NAVI`: Navigation grid
- `PATH`: Paths: patrols, roads
- `FX  `: Effects

## Changing the format

1. Edit `Compiler/COMMON/jo_level/schema.py`:
   - a new field: at the end of the header or of the record, with `default=` and `since=` (the new version of the section), then the version of the section + 1;
   - a new section: a new `Section` (identifier of 4 characters, version 1). `REQUIRED` only if an older engine would draw the level wrongly without it.
2. `python3 Compiler/COMMON/jo_level gen`: this documentation and `jo_engine/jo/level_format.h` are made again.
3. The engine reads the new fields (`jo_level_get_header()`, `jo_level_get_records()` give the default values of the fields missing in an older file).
4. The tools that write the levels fill the new fields.

An engine reads the levels written for an older engine (the missing fields get their default values) and, when no section is required, the levels written for a newer engine (it ignores what it doesn't know).

## Tools

```
python3 Compiler/COMMON/jo_level dump cd/LEVEL.JOL      # print the sections
python3 Compiler/COMMON/jo_level check cd/LEVEL.JOL     # check the file
python3 Compiler/COMMON/jo_level gen                    # make this file and jo_engine/jo/level_format.h
```

Writing a level in Python:

```python
import sys
sys.path.insert(0, 'Compiler/COMMON')
import jo_level
lv = jo_level.Level('My level')
lv.add('TERR', header=dict(n=3, cell=128, origin_x=0, origin_z=0),
       arrays=dict(heights=[0] * 9, colors=[0x8000] * 9, holes=[]))
lv.add('PLNT', records=[dict(x=0, y=0, z=0, kind=0, angle=0, scale=256)])
lv.add_entities([dict(name='barrel', cls='explosive', model='BARREL.JOM', x=0, y=0, z=200, health=40,
                      flags=['SOLID'], radius=14, height=40, props=dict(sound='EXPLODE.PCM'),
                      links=[dict(event='DEATH', target='fuse', action='TRIGGER', delay=400)]),
                 dict(name='fuse', cls='relay', flags=['HIDDEN'],
                      links=[dict(event='TRIGGER', target='barrel2', action='KILL')]),
                 dict(name='barrel2', cls='explosive', model='BARREL.JOM', x=300, z=200, health=40,
                      flags=['SOLID'], radius=14, height=40)])
lv.write('cd/LEVEL.JOL')
```
