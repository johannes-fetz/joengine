/*
** Jo Sega Saturn Engine: level file format (JOL)
**
** Made by Compiler/COMMON/jo_level/gen.py from Compiler/COMMON/jo_level/schema.py: change the schema, not this file.
** Documentation: Documentation/level_format.md
*/

#ifndef __JO_LEVEL_FORMAT_H__
# define __JO_LEVEL_FORMAT_H__

/** @brief File magic */
# define JO_LEVEL_MAGIC                     (0x4A4F4C56)
/** @brief Version of the file format (header and directory) */
# define JO_LEVEL_FORMAT_VERSION            (1)
/** @brief A reader that doesn't know this section must refuse the file */
# define JO_LEVEL_FLAG_REQUIRED             (0x0001)
/** @brief No string (field of type string) */
# define JO_LEVEL_NO_STRING                 (0xFFFFFFFFU)
/** @brief No record (field of type index) */
# define JO_LEVEL_NO_INDEX                  (0xFFFF)

/** @brief Identifier of a section from its 4 characters */
# define JO_LEVEL_ID(A, B, C, D)            (((unsigned int)(A) << 24) | ((unsigned int)(B) << 16) | ((unsigned int)(C) << 8) | (unsigned int)(D))

/** @brief File header (16 bytes) */
typedef struct
{
    unsigned int                magic;               // v1: 'JOLV'
    unsigned short              version;             // v1: Version of the file format (FORMAT_VERSION: header and directory)
    unsigned short              flags;               // v1: Reserved (0)
    unsigned int                size;                // v1: Size of the file (bytes)
    unsigned int                section_count;       // v1: Number of entries of the directory (after this header)
}                               jo_level_file_header;

/** @brief Directory entry (one per section, after the file header) (24 bytes) */
typedef struct
{
    unsigned int                id;                  // v1: Identifier: 4 characters
    unsigned short              version;             // v1: Version of the section
    unsigned short              flags;               // v1: FLAG_xxx (REQUIRED: 1)
    unsigned int                offset;              // v1: Offset of the section from the start of the file (multiple of 4)
    unsigned int                size;                // v1: Size of the section (bytes, header, records and arrays)
    unsigned short              header_size;         // v1: Size of the header of the section (bytes)
    unsigned short              record_size;         // v1: Size of a record (bytes)
    unsigned int                record_count;        // v1: Number of records (after the header)
}                               jo_level_directory_entry;

/*
** INFO: Level (version 1)
*/
# define JO_LEVEL_SECTION_INFO               JO_LEVEL_ID('I', 'N', 'F', 'O')
# define JO_LEVEL_INFO_VERSION               (1)

/** @brief Level: header (8 bytes) */
typedef struct
{
    unsigned int                name;                // v1: Name of the level
    unsigned int                flags;               // v1: Reserved (0)
}                               jo_level_info_header;

/** @brief Default values (the fields missing in an older file, see jo_level_get_records()) */
# define JO_LEVEL_INFO_HEADER_DEFAULTS      {.name = 0xFFFFFFFF}

/*
** STRS: Strings (version 1)
*/
# define JO_LEVEL_SECTION_STRS               JO_LEVEL_ID('S', 'T', 'R', 'S')
# define JO_LEVEL_STRS_VERSION               (1)

/*
** TERR: Terrain (version 1)
*/
# define JO_LEVEL_SECTION_TERR               JO_LEVEL_ID('T', 'E', 'R', 'R')
# define JO_LEVEL_TERR_VERSION               (1)

/** @brief Terrain: header (28 bytes) */
typedef struct
{
    unsigned short              n;                   // v1: Vertices per side
    unsigned short              cell;                // v1: Distance between 2 vertices (world units)
    int                         origin_x;            // v1: Center of the height map (world units)
    int                         origin_z;            // v1: Center of the height map (world units)
    unsigned int                heights;             // v1: Heights: n x n i16, row after row (j), Y up (world units)
    unsigned int                colors;              // v1: Colors of the vertices: n x n RGB 555 of the Saturn (the light of the sun is baked)
    unsigned int                hole_count;          // v1: Number of entries of holes
    unsigned int                holes;               // v1: Vertices under a building: u16 j x n + i. A quad with its 4 vertices in this list is not drawn. With the flag 0x8000 the quad is drawn, sorted behind the building
}                               jo_level_terr_header;

/** @brief Default values (the fields missing in an older file, see jo_level_get_records()) */
# define JO_LEVEL_TERR_HEADER_DEFAULTS      {}

/*
** PLNT: Plants (version 1)
*/
# define JO_LEVEL_SECTION_PLNT               JO_LEVEL_ID('P', 'L', 'N', 'T')
# define JO_LEVEL_PLNT_VERSION               (1)

/** @brief Plants: record (20 bytes) */
typedef struct
{
    int                         x;                   // v1: Position (world units)
    int                         y;                   // v1: Height of the foot, Y up (world units)
    int                         z;                   // v1: Position (world units)
    unsigned short              kind;                // v1: Kind: 0 palm tree, 1 bush, 2 fern, 3 rock, 4 tree of the inland
    short                       angle;               // v1: Rotation around Y (degrees)
    unsigned short              scale;               // v1: Scale x 256 (256: 1)
    unsigned char               __pad0[2];
}                               jo_level_plnt_record;

/** @brief Default values (the fields missing in an older file, see jo_level_get_records()) */
# define JO_LEVEL_PLNT_RECORD_DEFAULTS      {}

/*
** MODL: Models (version 1)
*/
# define JO_LEVEL_SECTION_MODL               JO_LEVEL_ID('M', 'O', 'D', 'L')
# define JO_LEVEL_MODL_VERSION               (1)

/** @brief Models: record (20 bytes) */
typedef struct
{
    unsigned int                file;                // v1: JOM file (in the root directory of the CD)
    unsigned short              first_box;           // v1: First collision box (MBOX)
    unsigned short              box_count;           // v1: Number of collision boxes
    unsigned short              has_floor;           // v1: 1: the model has a floor the player can walk on
    short                       floor[5];            // v1: Floor: x0, z0, x1, z1 (model space), height (Y up) (world units)
}                               jo_level_modl_record;

/** @brief Default values (the fields missing in an older file, see jo_level_get_records()) */
# define JO_LEVEL_MODL_RECORD_DEFAULTS      {.file = 0xFFFFFFFF}

/*
** MBOX: Collision boxes (version 1)
*/
# define JO_LEVEL_SECTION_MBOX               JO_LEVEL_ID('M', 'B', 'O', 'X')
# define JO_LEVEL_MBOX_VERSION               (1)

/** @brief Collision boxes: record (8 bytes) */
typedef struct
{
    short                       x;                   // v1: Center (world units)
    short                       z;                   // v1: Center (world units)
    short                       half_x;              // v1: Half size (world units)
    short                       half_z;              // v1: Half size (world units)
}                               jo_level_mbox_record;

/** @brief Default values (the fields missing in an older file, see jo_level_get_records()) */
# define JO_LEVEL_MBOX_RECORD_DEFAULTS      {}

/*
** INST: Instances (version 1)
*/
# define JO_LEVEL_SECTION_INST               JO_LEVEL_ID('I', 'N', 'S', 'T')
# define JO_LEVEL_INST_VERSION               (1)

/** @brief Instances: record (24 bytes) */
typedef struct
{
    unsigned short              model;               // v1: Model seen from far away (MODL)
    unsigned short              near_model;          // v1: Model seen from near (MODL), NO_INDEX: the same
    unsigned short              inside_model;        // v1: Model of the inside, drawn from near (MODL), NO_INDEX: none
    short                       yaw;                 // v1: Rotation around Y (degrees)
    int                         x;                   // v1: Position (world units)
    int                         y;                   // v1: Position, Saturn Y (down) (world units)
    int                         z;                   // v1: Position (world units)
    short                       half_x;              // v1: Collision box when the model has none (model space), 0: no collision (world units)
    short                       half_z;              // v1: Collision box when the model has none (world units)
}                               jo_level_inst_record;

/** @brief Default values (the fields missing in an older file, see jo_level_get_records()) */
# define JO_LEVEL_INST_RECORD_DEFAULTS      {.model = 0xFFFF, .near_model = 0xFFFF, .inside_model = 0xFFFF}

/*
** DECL: Decals (version 1)
*/
# define JO_LEVEL_SECTION_DECL               JO_LEVEL_ID('D', 'E', 'C', 'L')
# define JO_LEVEL_DECL_VERSION               (1)

/** @brief Decals: record (16 bytes) */
typedef struct
{
    unsigned short              kind;                // v1: Kind: 0 coral, 1 brain coral, 2 starfish, 3 rocks, 4 sea grass, 5 anchor, 6 barrel
    short                       angle;               // v1: Rotation around Y (degrees)
    unsigned short              size;                // v1: Size (world units)
    unsigned char               __pad0[2];
    int                         x;                   // v1: Position (world units)
    int                         z;                   // v1: Position (world units)
}                               jo_level_decl_record;

/** @brief Default values (the fields missing in an older file, see jo_level_get_records()) */
# define JO_LEVEL_DECL_RECORD_DEFAULTS      {}

/*
** ENTI: Entities (version 1)
*/
# define JO_LEVEL_SECTION_ENTI               JO_LEVEL_ID('E', 'N', 'T', 'I')
# define JO_LEVEL_ENTI_VERSION               (1)

/** @brief Entities: record (44 bytes) */
typedef struct
{
    unsigned int                name;                // v1: Name (unique in the level: the links and the game find the entity by it)
    unsigned int                cls;                 // v1: Class: a class of the engine (explosive, relay, trigger...) or of the game
    unsigned int                model;               // v1: JOM file drawn at its position, NO_STRING: none
    int                         x;                   // v1: Position (world units)
    int                         y;                   // v1: Position, Saturn Y (down): the foot of the entity (world units)
    int                         z;                   // v1: Position (world units)
    short                       yaw;                 // v1: Rotation around Y (degrees)
    short                       health;              // v1: Health at the start, 0: it can't be damaged
    unsigned short              flags;               // v1: ENTITY_FLAG_xxx: DISABLED 1, SOLID 2, HIDDEN 4
    unsigned short              radius;              // v1: Cylinder (collisions, bullets, zone of a trigger): radius (world units)
    unsigned short              height;              // v1: Cylinder: height above the foot (world units)
    unsigned short              first_prop;          // v1: First property (PROP)
    unsigned short              prop_count;          // v1: Number of properties
    unsigned short              first_link;          // v1: First link of which it is the source (LINK)
    unsigned short              link_count;          // v1: Number of links
    unsigned char               __pad0[2];
}                               jo_level_enti_record;

/** @brief Default values (the fields missing in an older file, see jo_level_get_records()) */
# define JO_LEVEL_ENTI_RECORD_DEFAULTS      {.name = 0xFFFFFFFF, .cls = 0xFFFFFFFF, .model = 0xFFFFFFFF}

/*
** PROP: Properties (version 1)
*/
# define JO_LEVEL_SECTION_PROP               JO_LEVEL_ID('P', 'R', 'O', 'P')
# define JO_LEVEL_PROP_VERSION               (1)

/** @brief Properties: record (12 bytes) */
typedef struct
{
    unsigned int                key;                 // v1: Name
    unsigned short              type;                // v1: PROPERTY_xxx: INT 0, FIXED 1, STRING 2
    unsigned char               __pad0[2];
    int                         value;               // v1: Value (the offset of the string for a STRING)
}                               jo_level_prop_record;

/** @brief Default values (the fields missing in an older file, see jo_level_get_records()) */
# define JO_LEVEL_PROP_RECORD_DEFAULTS      {.key = 0xFFFFFFFF}

/*
** LINK: Links (version 1)
*/
# define JO_LEVEL_SECTION_LINK               JO_LEVEL_ID('L', 'I', 'N', 'K')
# define JO_LEVEL_LINK_VERSION               (1)

/** @brief Links: record (12 bytes) */
typedef struct
{
    unsigned short              event;               // v1: EVENT_xxx of the source
    unsigned short              action;              // v1: ACTION_xxx sent to the target
    unsigned short              target;              // v1: Target (ENTI), NO_INDEX: the source itself
    unsigned short              delay;               // v1: Delay (ms)
    int                         param;               // v1: Parameter of the action (the damage of DAMAGE), -1: the parameter of the event
}                               jo_level_link_record;

/** @brief Default values (the fields missing in an older file, see jo_level_get_records()) */
# define JO_LEVEL_LINK_RECORD_DEFAULTS      {.target = 0xFFFF, .param = -1}

/*
** CELL: Cells (version 3)
*/
# define JO_LEVEL_SECTION_CELL               JO_LEVEL_ID('C', 'E', 'L', 'L')
# define JO_LEVEL_CELL_VERSION               (3)

/** @brief Cells: record (40 bytes) */
typedef struct
{
    unsigned int                model;               // v1: JOM file of the cell (world coordinates)
    int                         x0;                  // v1: Box where the player can walk: smallest X (world units)
    int                         z0;                  // v1: Smallest Z (world units)
    int                         x1;                  // v1: Biggest X (world units)
    int                         z1;                  // v1: Biggest Z (world units)
    int                         floor0;              // v1: Floor, Saturn Y (down): at x0 (slope X), at z0 (slope Z) or everywhere (flat) (world units)
    int                         floor1;              // v1: Floor at x1 (slope X) or at z1 (slope Z) (world units)
    unsigned short              slope;               // v1: CELL_SLOPE_xxx: FLAT 0, X 1, Z 2
    unsigned short              first_portal;        // v1: First portal of the cell (PRTL)
    unsigned short              portal_count;        // v1: Number of portals
    short                       yaw;                 // v2: Rotation of the box around its center (Y axis, the same as the yaw of INST): a building turned on the terrain (degrees)
    unsigned int                reflection;          // v3: JOM file drawn in the mirrors of the cell instead of its model (a simpler model: bigger quads, no floor), NO_STRING: its model and the cells next to it
}                               jo_level_cell_record;

/** @brief Default values (the fields missing in an older file, see jo_level_get_records()) */
# define JO_LEVEL_CELL_RECORD_DEFAULTS      {.model = 0xFFFFFFFF, .reflection = 0xFFFFFFFF}

/*
** PRTL: Portals (version 1)
*/
# define JO_LEVEL_SECTION_PRTL               JO_LEVEL_ID('P', 'R', 'T', 'L')
# define JO_LEVEL_PRTL_VERSION               (1)

/** @brief Portals: record (52 bytes) */
typedef struct
{
    unsigned short              cell;                // v1: Cell seen through the portal (CELL), NO_INDEX: the outside (the terrain around a building, see jo_cells_outside_view())
    unsigned short              kind;                // v1: PORTAL_xxx: DOOR 0 (an opening), MIRROR 1 (a mirror of the cell), WINDOW 2 (seen through from the cell only)
    int                         points[12];          // v1: The 4 corners: x, y (Saturn Y), z of each corner (world units)
}                               jo_level_prtl_record;

/** @brief Default values (the fields missing in an older file, see jo_level_get_records()) */
# define JO_LEVEL_PRTL_RECORD_DEFAULTS      {.cell = 0xFFFF}

/*
** WEAP: Weapons (version 1)
*/
# define JO_LEVEL_SECTION_WEAP               JO_LEVEL_ID('W', 'E', 'A', 'P')
# define JO_LEVEL_WEAP_VERSION               (1)

/** @brief Weapons: record (76 bytes) */
typedef struct
{
    unsigned int                name;                // v1: Name
    unsigned int                model;               // v1: JOM file of the weapon and the hands (first person view)
    jo_fixed                    x;                   // v1: Position of the model in front of the camera: right (world units, Q16.16)
    jo_fixed                    y;                   // v1: Down (world units, Q16.16)
    jo_fixed                    z;                   // v1: Forward (world units, Q16.16)
    jo_fixed                    muzzle_y;            // v1: The muzzle in the model: height (the axis of the barrel) (world units, Q16.16)
    jo_fixed                    muzzle_z;            // v1: The muzzle in the model: forward (world units, Q16.16)
    unsigned short              convergence;         // v1: The axis of the barrel crosses the line of sight at this distance (the crosshair) (world units)
    unsigned short              automatic;           // v1: 1: it fires while the button is held, 0: one shot per press
    unsigned short              fire_delay;          // v1: Frames between 2 shots
    unsigned short              damage;              // v1: Damage of a bullet
    unsigned short              spread;              // v1: Spread of the bullets: the offset at 1000 units (world units)
    unsigned short              recoil;              // v1: The model goes back (frames of recoil)
    unsigned short              kick;                // v1: The view goes up at each shot (degrees)
    unsigned short              magazine;            // v1: Bullets of a magazine
    unsigned short              ammo_type;           // v1: Type of ammunition (the weapons with the same type share their reserve)
    unsigned short              ammo_start;          // v1: Reserve at the start (not in the magazine)
    unsigned short              ammo_max;            // v1: Reserve at most
    unsigned short              reload_time;         // v1: Frames of the reload
    unsigned short              switch_time;         // v1: Frames to take this weapon (it comes up), the other one goes down in the same time
    unsigned char               __pad0[2];
    unsigned int                flash;               // v1: Base name of the muzzle flashes (FLASH: FLASH1.BIN, FLASH2.BIN, FLASH3.BIN)
    unsigned short              flash_size;          // v1: Size of the muzzle flash (pixels)
    unsigned short              casing;              // v1: 1: the casings are ejected
    unsigned int                sound_shot;          // v1: PCM file of the shot
    unsigned int                sound_reload;        // v1: PCM file of the reload
    unsigned short              shot_volume;         // v1: Volume of the shot (0 to 127)
    unsigned char               __pad1[2];
}                               jo_level_weap_record;

/** @brief Default values (the fields missing in an older file, see jo_level_get_records()) */
# define JO_LEVEL_WEAP_RECORD_DEFAULTS      {.name = 0xFFFFFFFF, .model = 0xFFFFFFFF, .convergence = 0x258, .fire_delay = 4, .damage = 0x19, .recoil = 0xA, .kick = 2, .magazine = 0x1E, .ammo_max = 0x3E7, .reload_time = 0x32, .switch_time = 0xC, .flash = 0xFFFFFFFF, .flash_size = 0x16, .sound_shot = 0xFFFFFFFF, .sound_reload = 0xFFFFFFFF, .shot_volume = 0x78}

/** @brief Events of an entity (LINK.event): what happened to the source */
# define JO_LEVEL_EVENT_TRIGGER             (0)
# define JO_LEVEL_EVENT_ENTER               (1)
# define JO_LEVEL_EVENT_EXIT                (2)
# define JO_LEVEL_EVENT_DAMAGE              (3)
# define JO_LEVEL_EVENT_DEATH               (4)

/** @brief Actions sent to an entity (LINK.action): what the target does */
# define JO_LEVEL_ACTION_TRIGGER            (0)
# define JO_LEVEL_ACTION_DAMAGE             (1)
# define JO_LEVEL_ACTION_KILL               (2)
# define JO_LEVEL_ACTION_ENABLE             (3)
# define JO_LEVEL_ACTION_DISABLE            (4)
# define JO_LEVEL_ACTION_TOGGLE             (5)
# define JO_LEVEL_ACTION_USE                (6)
# define JO_LEVEL_ACTION_OPEN               (7)
# define JO_LEVEL_ACTION_CLOSE              (8)

/** @brief Floors of the cells (CELL.slope) */
# define JO_LEVEL_CELL_SLOPE_FLAT           (0)
# define JO_LEVEL_CELL_SLOPE_X              (1)
# define JO_LEVEL_CELL_SLOPE_Z              (2)

/** @brief Kinds of portals (PRTL.kind) */
# define JO_LEVEL_PORTAL_DOOR               (0)
# define JO_LEVEL_PORTAL_MIRROR             (1)
# define JO_LEVEL_PORTAL_WINDOW             (2)

/** @brief Types of the properties of the entities (PROP.type) */
# define JO_LEVEL_PROPERTY_INT              (0)
# define JO_LEVEL_PROPERTY_FIXED            (1)
# define JO_LEVEL_PROPERTY_STRING           (2)

/** @brief Flags of the entities (ENTI.flags) */
# define JO_LEVEL_ENTITY_FLAG_DISABLED      (0x0001)
# define JO_LEVEL_ENTITY_FLAG_SOLID         (0x0002)
# define JO_LEVEL_ENTITY_FLAG_HIDDEN        (0x0004)

#endif /* !__JO_LEVEL_FORMAT_H__ */

/*
** END OF FILE
*/
