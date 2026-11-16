# VDP1 and memory: the settings on by default {#performance_guide}

Two settings of the 3D make the VDP1 draw the same scenes in less time. They are **on by default** since November 2026.
A third one, for the memory, is **off by default** and must be turned on in the makefile. This page says what each one
does, what it changes in the picture, and when to turn it off.

## High speed shrink: `jo_3d_set_high_speed_shrink()`

When a texture is drawn smaller than it is on a line (far away, or a wall seen at an angle), the VDP1 reads every texel
of the line. With the high speed shrink (bit 12 of the draw mode, `JO_VDP1_HIGH_SPEED_SHRINK`), it reads every other
texel: the line takes about half the time on a console. A texture drawn at its size or bigger is not changed.

- **On by default** for every textured polygon of the 3D (models, meshes, levels): `jo_3d_set_high_speed_shrink(true)`.
- **The cost:** far away, the textures are a little less sharp. A detail of one texel (a thin line, a small text) can
  flicker or vanish between two smaller copies of the texture (`jo_3d_set_texture_mipmaps()`).
- **Turn it off:** `jo_3d_set_high_speed_shrink(false)`, at any time (for a scene, a view). Each polygon then uses its own
  setting: `jo_3d_set_mesh_high_speed_shrink()`, or the material flag `JO_3D_MATERIAL_HIGH_SPEED_SHRINK` (MTL:
  `jo_high_speed_shrink 1`, see `Compiler/COMMON/jo_model.py`).
- **Measured:** Mednafen draws the picture of the high speed shrink but not its gain. With a Mednafen changed to count
  a shrunken line without it twice (as the Saturn does), the VDP1 time of the rally demo goes from 24.0 to 20.6 ms per
  frame (p95: 28.0 to 24.2 ms).

## Flat gouraud drawn without gouraud: `jo_3d_set_uniform_gouraud_skipped()`

A polygon whose 4 gouraud colors are the same is drawn without gouraud, the light put in its color. The picture is the same (checked frame by frame in
Mednafen); the VDP1 draws a polygon without gouraud faster.

- **On by default**, for the models created or loaded from now on (`jo_3d_load_model()`, `jo_3d_create_model...()`,
  the pools of models). The polygons whose light leaves the texture as it is (16, 16, 16) lose their gouraud too.
- **What is changed:**
  - an untextured polygon: its RGB color gets the light;
  - a texture of 16 colors (color lookup table): the polygon uses a copy of the table with the light (32 bytes of VRAM,
    shared by the polygons with the same texture and the same light);
  - the RGB and palette textures and the polygons lit by the engine (`JO_3D_MATERIAL_LIGHT`) keep their gouraud.
- **Not changed:** the models of the entities (drawn with a tint), of the cells of a level (lit by the point lights), and
  the models loaded with `jo_3d_set_unique_gouraud_tables(true)`.
- **Turn it off for a model** that your game:
  - tints with `jo_3d_set_tint()`;
  - adds to the point lights with `jo_3d_point_lights_add_mesh()`;
  - whose gouraud tables or polygon colors it writes again.

  Those polygons would not get the tint or the light (they have no table any more), and a color written again would
  lose the light that was put in it. Load such a model between `jo_3d_set_uniform_gouraud_skipped(false)` and
  `jo_3d_set_uniform_gouraud_skipped(true)` (see `Samples/demo - first person shooter/weapon.c`).
- `jo_3d_set_neutral_gouraud_skipped(true)` keeps the old, smaller setting: only the neutral polygons lose their gouraud.
- **Measured:** about −0.1 ms of VDP1 per frame in Mednafen, which counts little for the gouraud. The gain on a console
  is not measured yet.

## Functions read rarely in the low work RAM: `JO_COMPILE_WITH_LOW_WORK_RAM_CODE` (off by default)

The functions of the engine that are read rarely (the loading and the creation of the models, the textures, the sounds,
the backup library) and those of your game marked `JO_LOW_WORK_RAM_CODE` (its loading, its menus) can run from the low
work RAM. The high work RAM they leave goes to `jo_malloc()`: the data read at each frame (a level, the models) are
read faster there (a read from the low work RAM takes about twice the time).

- **Turn it on** in the makefile of your game:

  ```c
  JO_COMPILE_WITH_LOW_WORK_RAM_CODE = 1
  ```

  and mark the functions of your game that are not called at each frame:

  ```
  JO_LOW_WORK_RAM_CODE void load_level(int k) { ... }
  ```

- **The low work RAM free** starts at `JO_LOW_WORK_RAM_FREE`, not at `JO_WORK_RAM_LOW` (0x00200000).
  `jo_add_memory_zone()` skips the functions there by itself. A game that writes in the low work RAM by itself (a
  buffer at a fixed address) must start at `JO_LOW_WORK_RAM_FREE`, or it writes over the code of the engine: the game
  crashes later, without an error message. This is why the option is off by default.
- **Not for a function called at each frame:** its instructions are read from the low work RAM too.
