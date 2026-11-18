# Diagnose: too slow, crashes, glitches

Measure, then change one thing, then measure again. A change that is not measured is a guess, and on the Saturn most
guesses about speed are wrong.

## The game is too slow

1. **Read the warnings of the build**: `float or double: emulated in software` at the link (each one names the function:
   fix them all), and the compiler warnings.
2. **Find the calls of the slow routines of libgcc**: `python3 tools/slow_calls.py game.elf` lists the functions that call
   them (float or double, a division or a shift by a variable count, a 64 bits division). The engine calls a few when it
   starts or loads; the others are in the game: fix those of the functions called at each frame (the profiler tells
   which ones take time).
3. **Turn the profiler on**: `JO_COMPILE_WITH_PROFILER_MODULE = 1` in the makefile, `jo_profile_print(x, y)` in the
   drawing callback. It shows per frame (average and maximum):
   - the master CPU (callbacks), the time it waits for the slave CPU, the slave CPU busy, the wait for the VBLANK;
   - the VDP1: drawing time, commands, polygons drawn and tested, vertices, an estimate of the pixels walked;
   - your own measures: `jo_profile_begin(JO_PROFILE_USER(0))` / `jo_profile_end(...)`, named with
     `jo_profile_set_name()`.
   The profiler costs some CPU itself: compare measures made with it, turn it off for the release.
4. **Find who limits the frame**:
   - **the master CPU is full and the slave waits**: move work to the slave CPU (`jo_core_exec_on_slave()`), remove
     float, divisions and allocations from the loops, draw fewer polygons (culling, levels of detail, draw distance);
   - **the master waits for the slave**: give less to the slave, or split its job better;
   - **the CPUs wait for the VDP1** (`JO_PROFILE_VDP1_WAIT`, a long VDP1 draw): fewer pixels: smaller sprites and
     polygons, no half transparency on big surfaces, reduced textures far away (`jo_3d_set_texture_mipmaps()`), nothing
     drawn out of the screen, backgrounds on the VDP2;
   - **the master waits for the VBLANK most of the frame**: there is time left; the frame rate is set by `JO_FRAMERATE`.
5. **Count before optimizing**: polygons per frame by category (scenery, characters, effects), sprites, the late frames
   (`jo_core_get_late_frame_count()`). Optimize the biggest one.
6. **Check in Mednafen** (timings close to the console). A real console also pays for the VDP1 pixels walked out of the
   screen: keep a margin.

## The game crashes or shows an error

- **Build with `JO_DEBUG = 1`**: the engine checks its arguments and shows an error screen with the function and the
  message (`jo_core_error()`), traps the writes through a NULL pointer of the master CPU, and shows the CPU exceptions
  (the address of the instruction, the registers) instead of freezing.
- **Out of memory** (`jo_malloc()` returns NULL, "Out of memory" on the screen): print `jo_memory_usage_percent()` at
  each loading; free what the previous level used; smaller data; `jo_3d_model_use_fast_memory()` only for what is drawn
  often.
- **"Too many sprites"**: `JO_MAX_SPRITE` in the makefile (each texture, each set of gouraud tables of a model is a
  sprite).
- **A crash later after a loading**: a buffer written past its end, a stack too small on the slave CPU (big local
  arrays in a job), data written in the low work RAM used by the engine (see `JO_LOW_WORK_RAM_FREE`).
- **A freeze**: an infinite wait (a CD read that never ends, a loop on a flag written by the other CPU read through the
  cache: read it with `JO_CACHE_THROUGH()`).

## Glitches

- 3D: see `3d.md` (the sorts, the orientation of the faces, `jo_model.py --check-sort`).
- **Wrong colors on a texture of 16 colors**: its color table overwritten in the VDP1 VRAM (a game that writes gouraud
  tables or colors by itself, see `jo_3d_set_unique_gouraud_tables()`), or its index 15 used (an end code).
- **A layer of the VDP2 not shown**: its VRAM bank taken by another layer (the remarks of the function in `vdp2.h`),
  its priority under another layer, or the layer not enabled.
- **A sprite cut or squeezed**: its width not a multiple of 8, or a texture bigger than the VDP1 allows.
- Look at the data before the code: the VDP1 command table of a save state at the place of the glitch tells which
  polygon is drawn, with which texture and where.

## Before shipping a change

- Build without warning, `JO_DEBUG = 1` without error screen, then `JO_DEBUG = 0`.
- The heaviest scene at the target frame rate, without late frames.
- Memory after several loadings back at its value after the first one.
