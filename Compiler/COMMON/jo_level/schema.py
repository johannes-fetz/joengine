"""Jo Engine level file (JOL): the schema of the format, the single source of truth.

Everything else is made from this file:
    - the documentation (Documentation/level_format.md, see gen.py)
    - the C header of the engine (jo_engine/jo/level_format.h, see gen.py)
    - the writer, the reader and the validator of the Python tools (level.py)

Rules to change the format (see FORMAT_RULES below): add a section (new identifier) or add fields at the end of a header
or of a record (with a default value and the version of the section where it appeared). Never remove, move or reuse a
field or an identifier.
"""

# File
MAGIC = b'JOLV'
FORMAT_VERSION = 1

# Section flags
FLAG_REQUIRED = 0x0001          # A reader that doesn't know this section (or this version of it) must refuse the file

# Field types: struct format (big endian), size, C type
TYPES = {
    'u8':     ('B', 1, 'unsigned char'),
    'i8':     ('b', 1, 'signed char'),
    'u16':    ('H', 2, 'unsigned short'),
    'i16':    ('h', 2, 'short'),
    'u32':    ('I', 4, 'unsigned int'),
    'i32':    ('i', 4, 'int'),
    'fixed':  ('i', 4, 'jo_fixed'),            # Q16.16 fixed point
    'string': ('I', 4, 'unsigned int'),         # Offset of a string in the STRS section (0xFFFFFFFF: no string)
    'offset': ('I', 4, 'unsigned int'),         # Offset of an array from the start of the section
    'index':  ('H', 2, 'unsigned short'),       # Index of a record of another section (0xFFFF: none)
}

NO_STRING = 0xFFFFFFFF
NO_INDEX = 0xFFFF

FORMAT_RULES = """\
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
"""


class Field:
    def __init__(self, name, type, doc, default=0, since=1, unit=None, count=1):
        assert type in TYPES, type
        self.name = name
        self.type = type
        self.doc = doc
        self.default = default
        self.since = since
        self.unit = unit
        self.count = count      # Fixed array in the header or the record (for instance a vector: 3)

    @property
    def size(self):
        return TYPES[self.type][1] * self.count


class Array:
    """Array of a section: its offset and its element count are fields of the header"""

    def __init__(self, name, type, doc, offset_field, count_field=None, count_expr=None, since=1):
        assert type in TYPES, type
        self.name = name
        self.type = type
        self.doc = doc
        self.offset_field = offset_field
        self.count_field = count_field      # Field of the header with the element count
        self.count_expr = count_expr        # Or an expression of the fields of the header (for instance 'n * n')
        self.since = since


class Section:
    def __init__(self, id, version, title, doc, header=(), record=(), arrays=(), flags=0, raw=False):
        assert len(id) == 4
        self.id = id
        self.version = version
        self.title = title
        self.doc = doc
        self.header = list(header)
        self.record = list(record)
        self.arrays = list(arrays)
        self.flags = flags
        self.raw = raw          # The data is a block of bytes (no header, no records): the string table

    @staticmethod
    def _size(fields):
        size = 0
        for f in fields:
            size = (size + TYPES[f.type][1] - 1) // TYPES[f.type][1] * TYPES[f.type][1]
            size += f.size
        return (size + 3) // 4 * 4 if fields else 0

    @property
    def header_size(self):
        return self._size(self.header)

    @property
    def record_size(self):
        return self._size(self.record)


class Enum:
    """Values of a field (u16): the C header gets JO_LEVEL_<NAME>_<VALUE> defines, the documentation a table"""

    def __init__(self, name, doc, values):
        self.name = name
        self.doc = doc
        self.values = list(values)      # (NAME, doc): the value is the position in the list (never reorder it)


ENUMS = [
    Enum('EVENT', 'Events of an entity (LINK.event): what happened to the source', [
        ('TRIGGER', 'The entity received the action TRIGGER or USE (a relay, a switch that is used)'),
        ('ENTER', 'The player entered the entity (a trigger zone)'),
        ('EXIT', 'The player left the entity (a trigger zone)'),
        ('DAMAGE', 'The entity received damage and is still alive (param: the damage)'),
        ('DEATH', 'The health of the entity reached 0 (an explosive explodes)'),
    ]),
    Enum('ACTION', 'Actions sent to an entity (LINK.action): what the target does', [
        ('TRIGGER', 'Fire the event TRIGGER of the target (a relay passes it on)'),
        ('DAMAGE', 'Damage the target (param: the damage)'),
        ('KILL', 'The health of the target goes to 0 (event DEATH)'),
        ('ENABLE', 'The target is active again (drawn, solid, it reacts)'),
        ('DISABLE', 'The target is not active (not drawn, not solid, it ignores the actions but ENABLE)'),
        ('TOGGLE', 'ENABLE or DISABLE'),
        ('USE', 'The target is used (like the player does: a switch, a door). The default: TRIGGER'),
        ('OPEN', 'Open the target (a door, a mover)'),
        ('CLOSE', 'Close the target (a door, a mover)'),
    ]),
    Enum('CELL_SLOPE', 'Floors of the cells (CELL.slope)', [
        ('FLAT', 'The floor is flat (floor0)'),
        ('X', 'The floor goes from floor0 at x0 to floor1 at x1 (stairs along X)'),
        ('Z', 'The floor goes from floor0 at z0 to floor1 at z1 (stairs along Z)'),
    ]),
    Enum('PORTAL', 'Kinds of portals (PRTL.kind)', [
        ('DOOR', 'An opening: the other cell is seen through it'),
        ('MIRROR', 'A mirror: the cell (and the cells next to it) is drawn again, reflected by the plane of the portal, '
                   'clipped to its rectangle on the screen (see jo_cells_draw_mirror()). The wall has a hole there'),
        ('WINDOW', 'An opening seen through from the cell only (an embrasure, a window too high): from the other side, '
                   'the cell is not seen through it (the hole is covered, for instance by a dark polygon)'),
    ]),
    Enum('PROPERTY', 'Types of the properties of the entities (PROP.type)', [
        ('INT', 'value is an integer (i32)'),
        ('FIXED', 'value is a Q16.16 fixed point number'),
        ('STRING', 'value is the offset of a string in STRS'),
    ]),
]

ENUM_BY_NAME = {e.name: e for e in ENUMS}

# Classes of entities of the engine (jo_engine/entity.c) and their properties: (name, type, default, description).
# A game adds its own classes (jo_entity_register_class()): their properties are documented by the game
ENTITY_CLASSES = [
    ('all the classes', 'Properties of all the entities (the classes of the game too)', [
        ('far_model', 'STRING', '', 'JOM file drawn farther than far_distance (fewer polygons), "": the model'),
        ('far_distance', 'INT', 600, 'The far model is drawn farther (world units, on the ground)'),
        ('draw_distance', 'INT', 2000, 'The model is not drawn farther (world units, on the ground): each polygon costs CPU '
                                       'time, even small far away'),
        ('box_x', 'INT', 0, 'With box_z: the collisions of the player and the vehicles use a box turned with the entity '
                            '(half sizes, world units) instead of its cylinder (a partition, a counter). The bullets keep the '
                            'cylinder'),
        ('box_z', 'INT', 0, 'See box_x'),
    ]),
    ('entity', 'The default class: its model at its position. An entity with an unknown class is an entity', []),
    ('relay', 'Passes the action TRIGGER (or USE) on: its links TRIGGER send their actions, with their delays (a sequence '
              'of events: a fuse, a chain of doors). Flag HIDDEN', []),
    ('trigger', 'A zone: its cylinder (radius, height above the foot). Fires ENTER when the player goes in, EXIT when he '
                'goes out. Flag HIDDEN', [
        ('once', 'INT', 0, '1: the zone is disabled after the first ENTER'),
    ]),
    ('spawn', 'A place where the player appears (the start of a level, the end of a zone_link): its position and its yaw. '
              'Flag HIDDEN', []),
    ('zone_link', 'A passage to another level (stairs, a door, an elevator): when the player goes in its cylinder (or when it '
                  'receives TRIGGER or USE), the game is asked to load the level and to put the player on the spawn '
                  '(jo_entities_get_zone_request()). The player must go out of it before (he may start in it). Flag HIDDEN', [
        ('level', 'STRING', '', 'JOL file of the level'),
        ('spawn', 'STRING', '', 'Name of the entity (class spawn) of the level where the player appears'),
    ]),
    ('explosive', 'Explodes when its health reaches 0 (bullets, an explosion near it, the action KILL): fireballs and '
                  'smoke (sprites NAME1.BIN, NAME2.BIN... opaque with end codes), flash of the screen, shake of the '
                  'camera (module JO_COMPILE_WITH_EFFECTS_MODULE = 1), sound, then its dead model. After the delay, it damages what is around it (the other '
                  'explosives: chain reactions) and the player (the game gets the damage, see '
                  'jo_entities_set_area_damage_callback()). Flag SOLID: the bullets hit it', [
        ('damage', 'INT', 100, 'Damage at the center (less farther, 0 at the radius)'),
        ('radius', 'INT', 200, 'Radius of the damage (world units)'),
        ('delay', 'INT', 150, 'Delay of the damage after the explosion (ms)'),
        ('size', 'INT', 100, 'Size of the fireball (world units)'),
        ('fire', 'STRING', 'FIRE', 'Base name of the frames of the fire (FIRE1.BIN to FIRE4.BIN)'),
        ('fire_frames', 'INT', 4, 'Number of frames of the fire (1 to 4)'),
        ('smoke', 'STRING', 'SMOKE', 'Base name of the frames of the smoke'),
        ('smoke_frames', 'INT', 2, 'Number of frames of the smoke (0: no smoke, up to 4)'),
        ('sound', 'STRING', '', 'PCM file (8 bits mono), "": none'),
        ('dead_model', 'STRING', '', 'JOM file drawn after the explosion, "": nothing (and not solid any more)'),
        ('flash', 'INT', 120, 'Brightness of the flash of the screen (0 to 255), less far from the player'),
        ('shake', 'INT', 6, 'Shake of the camera (world units), less far from the player'),
    ]),
    ('actor', 'An animated model (a skeleton, see jo_3d_model_draw_pose()): plays an animation (a soldier who waits, a '
              'flag). The classes of the game (enemies) are actors too (see jo_actor_spawn()). The far model has the same '
              'skeleton', [
        ('animation', 'STRING', 'idle', 'Name of the animation of the model played at the start'),
    ]),
    ('vehicle', 'A vehicle driven by the player (see jo/vehicle.h): its model has a skeleton with the bones wheel_fl, '
                'wheel_fr, wheel_rl, wheel_rr (they turn), steer_fl, steer_fr (they steer) and steering (the steering '
                'wheel). The 4 wheels follow the ground (the game gives it, see jo_vehicles_set_ground_callback()), the '
                'solid entities stop it (a hit at speed damages them). Flag SOLID: the player walks around it', [
        ('cockpit_model', 'STRING', '', 'JOM file drawn when the player drives (what the driver sees, the same '
                                        'skeleton), "": the model'),
        ('wheelbase', 'INT', 106, 'Distance between the front and the rear axles (world units)'),
        ('track', 'INT', 78, 'Distance between the left and the right wheels (world units)'),
        ('wheel_radius', 'INT', 19, 'Radius of the wheels (world units)'),
        ('max_steer', 'INT', 30, 'Angle of the front wheels at most (degrees)'),
        ('max_speed', 'INT', 900, 'Top speed (world units per second; backward: a third)'),
        ('acceleration', 'INT', 280, 'Acceleration of the engine (world units per second per second)'),
        ('braking', 'INT', 800, 'Deceleration of the brakes (world units per second per second)'),
        ('impact', 'INT', 150, 'Damage of a hit at the top speed'),
    ]),
]

ENTITY_FLAGS = [
    ('DISABLED', 0x0001, 'Not active at the start (see the action ENABLE)'),
    ('SOLID', 0x0002, 'The player and the bullets collide with its cylinder (radius, height)'),
    ('HIDDEN', 0x0004, 'Its model is not drawn (a trigger zone, a relay)'),
]


# The header of the file and an entry of the directory (they don't change with the sections)
FILE_HEADER = [
    Field('magic', 'u32', "'JOLV'"),
    Field('version', 'u16', 'Version of the file format (FORMAT_VERSION: header and directory)'),
    Field('flags', 'u16', 'Reserved (0)'),
    Field('size', 'u32', 'Size of the file (bytes)'),
    Field('section_count', 'u32', 'Number of entries of the directory (after this header)'),
]

DIRECTORY_ENTRY = [
    Field('id', 'u32', 'Identifier: 4 characters'),
    Field('version', 'u16', 'Version of the section'),
    Field('flags', 'u16', 'FLAG_xxx (REQUIRED: 1)'),
    Field('offset', 'u32', 'Offset of the section from the start of the file (multiple of 4)'),
    Field('size', 'u32', 'Size of the section (bytes, header, records and arrays)'),
    Field('header_size', 'u16', 'Size of the header of the section (bytes)'),
    Field('record_size', 'u16', 'Size of a record (bytes)'),
    Field('record_count', 'u32', 'Number of records (after the header)'),
]


SECTIONS = [
    Section('INFO', 1, 'Level', "The level: its name. The first section of the file", flags=FLAG_REQUIRED, header=[
        Field('name', 'string', 'Name of the level', default=NO_STRING),
        Field('flags', 'u32', 'Reserved (0)'),
    ]),

    Section('STRS', 1, 'Strings', "Strings of the level (names of files, of classes...): ASCII, each one ended by a "
            "zero. A field of type string is the offset of the string in this section", raw=True),

    Section('TERR', 1, 'Terrain', "Height map of the ground: n x n vertices, one every cell units. The vertex (i, j) is "
            "at x = origin_x + (i - (n - 1) / 2) x cell, z = origin_z + (j - (n - 1) / 2) x cell. The meshes of the "
            "ground are built from it when the level is loaded", header=[
        Field('n', 'u16', 'Vertices per side'),
        Field('cell', 'u16', 'Distance between 2 vertices', unit='world units'),
        Field('origin_x', 'i32', 'Center of the height map', unit='world units'),
        Field('origin_z', 'i32', 'Center of the height map', unit='world units'),
        Field('heights', 'offset', 'Heights: n x n i16, row after row (j), Y up', unit='world units'),
        Field('colors', 'offset', 'Colors of the vertices: n x n RGB 555 of the Saturn (the light of the sun is baked)'),
        Field('hole_count', 'u32', 'Number of entries of holes'),
        Field('holes', 'offset', 'Vertices under a building: u16 j x n + i. A quad with its 4 vertices in this list is '
              'not drawn. With the flag 0x8000 the quad is drawn, sorted behind the building'),
    ], arrays=[
        Array('heights', 'i16', 'Heights', 'heights', count_expr='n * n'),
        Array('colors', 'u16', 'Colors', 'colors', count_expr='n * n'),
        Array('holes', 'u16', 'Holes', 'holes', count_field='hole_count'),
    ]),

    Section('PLNT', 1, 'Plants', "Plants and rocks placed on the ground", record=[
        Field('x', 'i32', 'Position', unit='world units'),
        Field('y', 'i32', 'Height of the foot, Y up', unit='world units'),
        Field('z', 'i32', 'Position', unit='world units'),
        Field('kind', 'u16', 'Kind: 0 palm tree, 1 bush, 2 fern, 3 rock, 4 tree of the inland'),
        Field('angle', 'i16', 'Rotation around Y', unit='degrees'),
        Field('scale', 'u16', 'Scale x 256 (256: 1)'),
    ]),

    Section('MODL', 1, 'Models', "3D models of the level (JOM files) and their collisions", record=[
        Field('file', 'string', 'JOM file (in the root directory of the CD)', default=NO_STRING),
        Field('first_box', 'u16', 'First collision box (MBOX)'),
        Field('box_count', 'u16', 'Number of collision boxes'),
        Field('has_floor', 'u16', '1: the model has a floor the player can walk on'),
        Field('floor', 'i16', 'Floor: x0, z0, x1, z1 (model space), height (Y up)', count=5, unit='world units'),
    ]),

    Section('MBOX', 1, 'Collision boxes', "Collision boxes of the models (vertical boxes, model space)", record=[
        Field('x', 'i16', 'Center', unit='world units'),
        Field('z', 'i16', 'Center', unit='world units'),
        Field('half_x', 'i16', 'Half size', unit='world units'),
        Field('half_z', 'i16', 'Half size', unit='world units'),
    ]),

    Section('INST', 1, 'Instances', "Models placed in the level (buildings)", record=[
        Field('model', 'index', 'Model seen from far away (MODL)', default=NO_INDEX),
        Field('near_model', 'index', 'Model seen from near (MODL), NO_INDEX: the same', default=NO_INDEX),
        Field('inside_model', 'index', 'Model of the inside, drawn from near (MODL), NO_INDEX: none', default=NO_INDEX),
        Field('yaw', 'i16', 'Rotation around Y', unit='degrees'),
        Field('x', 'i32', 'Position', unit='world units'),
        Field('y', 'i32', 'Position, Saturn Y (down)', unit='world units'),
        Field('z', 'i32', 'Position', unit='world units'),
        Field('half_x', 'i16', 'Collision box when the model has none (model space), 0: no collision', unit='world units'),
        Field('half_z', 'i16', 'Collision box when the model has none', unit='world units'),
    ]),

    Section('DECL', 1, 'Decals', "Flat decals lying at the sea level on the bottom of the lagoon", record=[
        Field('kind', 'u16', 'Kind: 0 coral, 1 brain coral, 2 starfish, 3 rocks, 4 sea grass, 5 anchor, 6 barrel'),
        Field('angle', 'i16', 'Rotation around Y', unit='degrees'),
        Field('size', 'u16', 'Size', unit='world units'),
        Field('x', 'i32', 'Position', unit='world units'),
        Field('z', 'i32', 'Position', unit='world units'),
    ]),

    Section('ENTI', 1, 'Entities', "The things of the level that live: an entity has a class (its behavior, given by the "
            "engine or by the game), a position, a health, properties (PROP) and links to other entities (LINK). See "
            "jo_engine/jo/entity.h and the classes of the engine in Documentation/level_format.md", record=[
        Field('name', 'string', 'Name (unique in the level: the links and the game find the entity by it)', default=NO_STRING),
        Field('cls', 'string', 'Class: a class of the engine (explosive, relay, trigger...) or of the game', default=NO_STRING),
        Field('model', 'string', 'JOM file drawn at its position, NO_STRING: none', default=NO_STRING),
        Field('x', 'i32', 'Position', unit='world units'),
        Field('y', 'i32', 'Position, Saturn Y (down): the foot of the entity', unit='world units'),
        Field('z', 'i32', 'Position', unit='world units'),
        Field('yaw', 'i16', 'Rotation around Y', unit='degrees'),
        Field('health', 'i16', 'Health at the start, 0: it can\'t be damaged'),
        Field('flags', 'u16', 'ENTITY_FLAG_xxx: DISABLED 1, SOLID 2, HIDDEN 4'),
        Field('radius', 'u16', 'Cylinder (collisions, bullets, zone of a trigger): radius', unit='world units'),
        Field('height', 'u16', 'Cylinder: height above the foot', unit='world units'),
        Field('first_prop', 'u16', 'First property (PROP)'),
        Field('prop_count', 'u16', 'Number of properties'),
        Field('first_link', 'u16', 'First link of which it is the source (LINK)'),
        Field('link_count', 'u16', 'Number of links'),
    ]),

    Section('PROP', 1, 'Properties', "Properties of the entities: a name and a value (PROPERTY_xxx type). The classes read "
            "them by name, with a default value when the property is missing", record=[
        Field('key', 'string', 'Name', default=NO_STRING),
        Field('type', 'u16', 'PROPERTY_xxx: INT 0, FIXED 1, STRING 2'),
        Field('value', 'i32', 'Value (the offset of the string for a STRING)'),
    ]),

    Section('LINK', 1, 'Links', "Links between the entities: when the event of the source happens, the action is sent to the "
            "target after the delay. The links of an entity follow each other (ENTI.first_link)", record=[
        Field('event', 'u16', 'EVENT_xxx of the source'),
        Field('action', 'u16', 'ACTION_xxx sent to the target'),
        Field('target', 'index', 'Target (ENTI), NO_INDEX: the source itself', default=NO_INDEX),
        Field('delay', 'u16', 'Delay', unit='ms'),
        Field('param', 'i32', 'Parameter of the action (the damage of DAMAGE), -1: the parameter of the event', default=-1),
    ]),

    Section('CELL', 3, 'Cells', "The rooms of an inside level (a base, a building): each cell has its model (walls, floor, "
            "ceiling, in world coordinates) and a box where the player can walk, with its floor (flat or a slope: the "
            "stairs). The cells overlap at the doors: the player walks from one to the other. The engine draws the cell "
            "of the camera and the cells seen through its portals (PRTL), see jo_engine/jo/cell.h", record=[
        Field('model', 'string', 'JOM file of the cell (world coordinates)', default=NO_STRING),
        Field('x0', 'i32', 'Box where the player can walk: smallest X', unit='world units'),
        Field('z0', 'i32', 'Smallest Z', unit='world units'),
        Field('x1', 'i32', 'Biggest X', unit='world units'),
        Field('z1', 'i32', 'Biggest Z', unit='world units'),
        Field('floor0', 'i32', 'Floor, Saturn Y (down): at x0 (slope X), at z0 (slope Z) or everywhere (flat)',
              unit='world units'),
        Field('floor1', 'i32', 'Floor at x1 (slope X) or at z1 (slope Z)', unit='world units'),
        Field('slope', 'u16', 'CELL_SLOPE_xxx: FLAT 0, X 1, Z 2'),
        Field('first_portal', 'u16', 'First portal of the cell (PRTL)'),
        Field('portal_count', 'u16', 'Number of portals'),
        Field('yaw', 'i16', 'Rotation of the box around its center (Y axis, the same as the yaw of INST): a building turned '
              'on the terrain', unit='degrees', since=2),
        Field('reflection', 'string', 'JOM file drawn in the mirrors of the cell instead of its model (a simpler model: '
              'bigger quads, no floor), NO_STRING: its model and the cells next to it', default=NO_STRING, since=3),
    ]),

    Section('PRTL', 1, 'Portals', "Openings of the cells (doors, windows, stairs): a quad through which another cell is "
            "seen. The portals of a cell follow each other (CELL.first_portal)", record=[
        Field('cell', 'index', 'Cell seen through the portal (CELL), NO_INDEX: the outside (the terrain around a building, '
              'see jo_cells_outside_view())', default=NO_INDEX),
        Field('kind', 'u16', 'PORTAL_xxx: DOOR 0 (an opening), MIRROR 1 (a mirror of the cell), WINDOW 2 (seen through from the cell only)'),
        Field('points', 'i32', 'The 4 corners: x, y (Saturn Y), z of each corner', count=12, unit='world units'),
    ]),

    Section('WEAP', 1, 'Weapons', "Definitions of the weapons of the player (and later of the enemies): a file of definitions "
            "shared by the levels (for instance DEFS.JOL). The engine runs them (ammo, fire rate, reload, change of weapon, "
            "see jo_engine/jo/weapon.h), the game draws them. The times are frames of the game", record=[
        Field('name', 'string', 'Name', default=NO_STRING),
        Field('model', 'string', 'JOM file of the weapon and the hands (first person view)', default=NO_STRING),
        Field('x', 'fixed', 'Position of the model in front of the camera: right', unit='world units, Q16.16'),
        Field('y', 'fixed', 'Down', unit='world units, Q16.16'),
        Field('z', 'fixed', 'Forward', unit='world units, Q16.16'),
        Field('muzzle_y', 'fixed', 'The muzzle in the model: height (the axis of the barrel)', unit='world units, Q16.16'),
        Field('muzzle_z', 'fixed', 'The muzzle in the model: forward', unit='world units, Q16.16'),
        Field('convergence', 'u16', 'The axis of the barrel crosses the line of sight at this distance (the crosshair)',
              default=600, unit='world units'),
        Field('automatic', 'u16', '1: it fires while the button is held, 0: one shot per press'),
        Field('fire_delay', 'u16', 'Frames between 2 shots', default=4),
        Field('damage', 'u16', 'Damage of a bullet', default=25),
        Field('spread', 'u16', 'Spread of the bullets: the offset at 1000 units', unit='world units'),
        Field('recoil', 'u16', 'The model goes back (frames of recoil)', default=10),
        Field('kick', 'u16', 'The view goes up at each shot (degrees)', default=2),
        Field('magazine', 'u16', 'Bullets of a magazine', default=30),
        Field('ammo_type', 'u16', 'Type of ammunition (the weapons with the same type share their reserve)'),
        Field('ammo_start', 'u16', 'Reserve at the start (not in the magazine)'),
        Field('ammo_max', 'u16', 'Reserve at most', default=999),
        Field('reload_time', 'u16', 'Frames of the reload', default=50),
        Field('switch_time', 'u16', 'Frames to take this weapon (it comes up), the other one goes down in the same time',
              default=12),
        Field('flash', 'string', 'Base name of the muzzle flashes (FLASH: FLASH1.BIN, FLASH2.BIN, FLASH3.BIN)', default=NO_STRING),
        Field('flash_size', 'u16', 'Size of the muzzle flash (pixels)', default=22),
        Field('casing', 'u16', '1: the casings are ejected'),
        Field('sound_shot', 'string', 'PCM file of the shot', default=NO_STRING),
        Field('sound_reload', 'string', 'PCM file of the reload', default=NO_STRING),
        Field('shot_volume', 'u16', 'Volume of the shot (0 to 127)', default=120),
    ]),
]

# Identifiers reserved for the next versions of the engine (see the design notes): they must not be used by a game
RESERVED_IDS = {
    'ENVI': 'Environment: sky, sea, sun, palette, music',
    'MATL': 'Materials: effects and sounds of the impacts, steps, walkable, blocking, grip',
    'COLL': 'Collision geometry (static, dynamic)',
    'NAVI': 'Navigation grid',
    'PATH': 'Paths: patrols, roads',
    'FX  ': 'Effects',
}

SECTION_BY_ID = {s.id: s for s in SECTIONS}
