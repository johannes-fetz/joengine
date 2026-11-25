# Working with the Jo Engine (instructions for coding assistants)

The Jo Engine is a C engine for the Sega Saturn: 2 SH-2 CPUs at 28 MHz **without FPU**, no barrel shifter, 4 KB of cache
each, 2 MB of work RAM, a VDP1 that draws the sprites and polygons (no z-buffer) and a VDP2 for the backgrounds. Code
that is fine on a PC is often 10 to 100 times too slow there. Read this page before writing or porting a game with the
engine. For Claude Code, the skill `.claude/skills/jo-engine` has the details.

## 1. Use the engine, don't write it again

- **Before writing any function** (math, lists, collisions, sprites, 3D, files, sound, input, memory, strings), search
  `Documentation/api_index.md` (one line per function, macro and type of the engine, generated from the headers) and the
  header in `jo_engine/jo/`. The engine's version uses the hardware (divider unit, tables, DMA, the 2 CPUs) and is tested.
- The usual needs:

  | Need | Engine |
  |---|---|
  | Numbers with a fraction | `jo_fixed` (16.16): `jo_fixed_mult()`, `jo_fixed_div()`, `jo_int2fixed()`, `JO_FIXED_1`, `JO_FLOAT_TO_FIXED()` for constants |
  | Angles, sin, cos | `jo_angle` (65536 = 360 degrees): `jo_sin_angle()`, `jo_cos_angle()`, `jo_sincos_angle()`, `jo_atan2_angle()`; degrees: `jo_sin()`, `jo_cos()` |
  | Square root, vectors | `jo_sqrt()`, `jo_fixed_sqrt()`, `jo_vector_fixed_length()`, `jo_vector_fixed_normalize()`, `jo_vector_fixed_dot()`, `jo_vector_fixed_cross()` |
  | Min, max, clamp, interpolation | `JO_MIN()`, `JO_MAX()`, `JO_CLAMP()`, `JO_ABS()`, `jo_lerp()` |
  | Random | `jo_random()`, `jo_random_seeded()` |
  | Memory | `jo_malloc()`, `jo_free()`, `jo_memory_usage_percent()`; copies: `jo_memcpy()`, `jo_memset()`, `jo_dma_copy()`, `jo_dma_copy_async()` |
  | Lists | `jo_list_init()`, `jo_list_add()`, `jo_list_foreach()`... |
  | Main loop | `jo_core_init()`, `jo_core_add_callback()`, `jo_core_run()`; errors: `jo_core_error()` (`JO_DEBUG`) |
  | Pad | `jo_is_pad1_key_pressed()`, `jo_is_pad1_key_down()` |
  | Sprites, 2D | `jo_sprite_add_tga()`, `jo_sprite_draw3D()`; collisions: `jo_hitbox_detection()`; a platform game: `physics.h` |
  | Backgrounds | the VDP2 planes and bitmaps (`vdp2.h`), not big sprites |
  | 3D | `jo_3d_load_model()` (`.JOM`), `jo_3d_model_draw()`, the camera (`jo_3d_camera_*`), the matrices (`jo_3d_push_matrix()`...) |
  | Files | `jo_fs_read_file()` (reads the files compressed by `jo_lz.py` as they are) |
  | Sound | `jo_audio_play_sound()`, the CD tracks: `jo_audio_play_cd_track()` |
  | The second CPU | `jo_core_exec_on_slave()`, `jo_core_wait_for_slave()` |
  | Measures | `JO_COMPILE_WITH_PROFILER_MODULE = 1`, `jo_profile_print()` (`profile.h`) |

- **The assets go through the tools of the engine**, never a file format written by hand:
  `Compiler/COMMON/jo_model.py` (OBJ + MTL → `.JOM` models, `--check-sort` for the glitches),
  `Compiler/COMMON/jo_mesh.py` (models built by a Python program), `Compiler/COMMON/jo_level` (levels `.JOL`),
  `Compiler/COMMON/jo_lz.py` (compression). The formats: `Documentation/model_format.md`, `Documentation/level_format.md`.
- A feature that the engine doesn't have yet but that any game could use belongs in the engine (`jo_engine/`), with its
  documentation in the header, not copied in each game.

## 2. Performance rules

- **No `float` or `double`**: they are emulated in software by libgcc. The link prints
  `warning: float or double: emulated in software...` with the function that uses them: fix each one with `jo_fixed`.
  Constants: `JO_FLOAT_TO_FIXED(1.5f)` is computed by the compiler.
- **No division or modulo by a variable in a loop** (a call of a slow routine): compute the inverse once, or use
  `jo_fixed_div()` / `jo_fixed_div_start()` + `jo_fixed_div_result()` (the divider unit, in parallel). A power of 2: a
  constant shift.
- **No shift by a variable count** (`x << n`): the SH-2 shifts by 1, 2, 8 or 16 bits only, the compiler calls a routine.
  Use a constant, a table or a multiplication.
- **Products of fixed point numbers**: `jo_fixed_mult()`, or `(long long)a * b` (one instruction): a 32 bits product
  overflows quickly.
- **No allocation in the frame loop**: allocate when a level is loaded, free when it is left.
- **Loadings**: reading the CD is most of their time: compress the data files with `jo_lz.py -f` (read decompressed by
  `jo_fs_read_file()` and `jo_fs_open()`), measure with `jo_fs_get_read_stats()`.
- **The 2 CPUs**: give the slave CPU about half of the work of a frame (`jo_core_exec_on_slave()`). Its stack is small:
  no big local arrays in its jobs.
- **One mesh per object** in 3D: `jo_3d_mesh_draw()` or a `.JOM` model, not quads drawn one by one with `jo_3d_draw()`
  or `jo_3d_draw_array()` (about 1.7 times slower: each call transforms and prepares on its own). A sprite facing the
  camera: `jo_3d_draw_billboard()` or `jo_3d_draw_standing_sprite()`, about a third of `jo_3d_draw_sprite()`.
- **The VDP1 time is the pixels it walks**, the transparent and off-screen ones too: fewer and smaller polygons and
  sprites, no half transparency on big surfaces (the screen doors instead: `jo_3d_set_mesh_screen_doors()`, MTL
  `jo_screen_doors 1`), reduced copies of the far textures (`jo_3d_set_texture_mipmaps()`), levels of detail and a draw
  distance.
- **Static backgrounds on the VDP2** (planes, bitmaps, the 3D planes of `background.h`), not with sprites.
- **`JO_FRAMERATE`** in the makefile: the game runs every n VBLANK (1: 60 images per second in NTSC, 2: 30, 3: 20).
  Choose the one the game holds everywhere; `jo_core_get_fps()` and `jo_core_get_late_frame_count()` tell if it does.
- **Measure before optimizing**: the profiler gives the time of each CPU and of the VDP1 per frame, the polygons and the
  pixels drawn (`JO_COMPILE_WITH_PROFILER_MODULE = 1`, `jo_profile_print()`); a guess is often wrong.

## 3. 3D without glitches

The VDP1 has no z-buffer: the engine sorts the polygons by one depth each (the painter's algorithm) and draws the whole
texture on each quad (no UV coordinates). Build the models for it:

- quads, not triangles (a triangle squeezes its texture); no polygon laid on another one (same depth: they flicker);
  nothing hidden inside a closed volume; no twisted quads;
- the order of the corners gives the orientation of the texture and the front face (clockwise on the screen);
- big faces next to small objects: cut them, or sort the small object on its nearest point (`jo_sort min` in the MTL;
  without `jo_sort`, `jo_model.py` chooses the sort of each polygon by itself);
- the gouraud doesn't saturate (a channel over 31 wraps around to dark): keep it at most 1.0 on light textures;
- check each model with `python3 Compiler/COMMON/jo_model.py --check-sort MODEL.JOM`.

## 4. Build, run, check

- Build with `compile.bat` / `compile.sh` (or `make`) in the folder of the game, the makefile includes
  `Compiler/COMMON/jo_engine_makefile`. **Fix every warning**, the link warnings too.
- `python3 tools/slow_calls.py game.elf`: the functions that call the slow routines of libgcc (float or double, a
  division or a shift by a variable count). Fix those of the game called at each frame.
- `JO_DEBUG = 1` while developing (an error screen tells what is wrong), `JO_DEBUG = 0` for the release.
- Test in an emulator (`run_with_*.bat`): Mednafen has the timings closest to the console (CPU, VDP1), the other
  emulators are often faster than a Saturn.
- One game per folder in `Projects/` (copy a sample of `Samples/`: one sample per feature of the engine).
