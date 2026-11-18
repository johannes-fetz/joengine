# Porting a game to the Jo Engine

A game from a PC, from another console or from an engine of C or C++ code: keep its rules and its content, rewrite its
platform layer on the engine, and change its numbers and its data for the Saturn.

## Plan

1. **Read the game first**: its main loop, its update and draw functions, its data (levels, sprites, models, sounds),
   its math (float everywhere?), its memory (allocations per frame?), its resolution and frame rate.
2. **Map each platform function to the engine** (search `Documentation/api_index.md`):
   - the window and the loop: `jo_core_init()`, `jo_core_add_callback()` (called once per frame), `jo_core_run()`;
   - the input: `jo_is_pad1_key_pressed()` / `jo_is_pad1_key_down()` (the keyboard, the mouse and the analog pads:
     `input.h`, `keyboard.h`);
   - the drawing: sprites (`sprites.h`), backgrounds (`vdp2.h`, `background.h`), text (`jo_printf()`, `font.h`), 3D
     (`3d.h`, `model.h`);
   - the files: `jo_fs_read_file()`; the sound: `audio.h`; the save: `backup.h`;
   - the time: the frame is the unit (`JO_FRAMERATE`); `time.h` for the ticks.
3. **Convert the data offline** with the tools (`assets.md`): models to JOM, images to TGA (8 bits with a palette for
   the big ones), sounds to WAV of a lower rate, levels to JOL or to a compact binary format of the game, compressed with
   `jo_lz.py`. Nothing is parsed at run time that could be converted before (no JSON, no text formats, no PNG).
4. **Make it run, then make it fast**: measure (`diagnose.md`) before optimizing.

## The numbers

- **Replace float and double by `jo_fixed`** (16.16, `math.h`), the angles by `jo_angle`, the trigonometry by the tables
  of the engine. Do it everywhere: the link prints a warning for each function that still uses float.
- Choose the units so that the values fit: 16.16 holds -32768 to 32767 with a precision of 1/65536. World coordinates
  in meters with a scale where the levels stay under a few thousand units; products in 64 bits (`(long long)`) when they
  can exceed 32767.
- Divisions: by a constant, or once per object (an inverse), or the divider unit (`jo_fixed_div()`).
- Random: `jo_random()` (or `jo_random_seeded()` for a sequence of its own, the same at each run).

## The memory

- 2 MB of work RAM (half of it slow), 512 KB of VRAM per VDP: measure what the game uses (`jo_memory_usage_percent()`).
- Allocate at loading, not per frame; pools of objects (bullets, particles) of a fixed size.
- Data of a PC game are often too big: smaller textures (16 colors when possible), fewer animation frames, sounds at 11
  or 22 kHz, levels loaded one at a time.

## The screen

- 320 x 240 (NTSC) or 320 x 256 (PAL) by default (`JO_SCREEN_WIDTH`, `JO_SCREEN_HEIGHT` in the makefile: 352, 224,
  the high resolutions 640 / 704 and the interlaced heights); a high resolution costs VDP1 time.
- Sprites have a width multiple of 8; scaling and rotation are free for the CPU (`jo_sprite_draw3D_and_rotate()`,
  `jo_sprite_draw_rotated_fixed()`...) but cost VDP1 time with their size on the screen.
- A HUD of text: `jo_printf()` (a VDP2 layer), not hundreds of sprites.

## The structure

- Keep the logic of the game separate from the drawing: the update at a fixed step (one call per frame), the drawing
  from the state.
- Put the independent work on the slave CPU (`jo_core_exec_on_slave()`): half of the 3D, the physics of many objects.
- A function that any game needs and that the engine doesn't have: add it to the engine (`jo_engine/`, documented in its
  header) rather than in the game.
