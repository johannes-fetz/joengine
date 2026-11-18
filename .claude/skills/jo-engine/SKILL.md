---
name: jo-engine
description: Writing, porting or optimizing a Sega Saturn game in C with the Jo Engine. Use the engine API instead of rewriting functions, follow the performance rules of the Saturn (SH-2 without FPU, VDP1, VDP2, 2 CPUs), build 3D models without glitches, convert the assets with the engine tools, and find what slows a game down.
---

# Jo Engine (Sega Saturn)

The Saturn: 2 SH-2 CPUs at 28 MHz without FPU and without barrel shifter (4 KB of cache each), 1 MB of fast work RAM
and 1 MB of slow work RAM, the VDP1 (sprites and polygons, no z-buffer, its own 512 KB of VRAM) and the VDP2 (up to 5
background layers, 512 KB of VRAM). A game written as for a PC is often 10 to 100 times too slow on it. The engine hides
most of the hardware; use it, and follow its rules.

## Workflow

1. **Find before writing.** Search `Documentation/api_index.md` (every function, macro and type of the engine, one line
   each) and the header in `jo_engine/jo/` for what you need. Read the comment of the function in its header: it gives
   the units, the limits and the cost. Look at the sample of the feature in `Samples/`.
2. **Write the game code with the rules below** (fixed point, no allocation per frame, the work shared by the 2 CPUs).
3. **Convert the assets with the tools of the engine** (`references/assets.md`), never by hand.
4. **Build and fix every warning**, including the warnings of the link (a float, a slow routine of libgcc).
5. **Run it in Mednafen** (timings close to a console) and **measure** with the profiler before and after a change
   (`references/diagnose.md`).
6. A feature that any game could use belongs in the engine (`jo_engine/`, documented in its header), not in the game.

## Rules that decide the frame rate

- **No `float` or `double`.** Emulated by libgcc: tens to hundreds of cycles per operation. Use `jo_fixed` (16.16) and
  `jo_angle` (65536 = one turn). Constants: `JO_FLOAT_TO_FIXED(0.5f)`, `JO_DEG_TO_ANGLE(90)` (computed by the compiler).
  The link warns about each function that still uses float: fix them all.
- **No division or modulo by a variable in a loop.** Compute an inverse once, divide by a constant power of 2, or use
  the divider unit: `jo_fixed_div()`, `jo_fixed_div_start()` then `jo_fixed_div_result()` (it computes in parallel).
- **No shift by a variable count** (`x << n`, `1 << k`): a routine call. Constants, tables, multiplications.
- **Fixed point products**: `jo_fixed_mult()` or `(long long)a * b` (one instruction); a 32 bits product of two 16.16
  numbers overflows from about 180 x 180.
- **No `jo_malloc()` / `jo_free()` in the frame loop**: allocate when a level loads, free when it is left. Check
  `jo_memory_usage_percent()` and `jo_memory_fragmentation()` after a few loads.
- **Data read at each frame in the fast work RAM** (`jo_malloc()` gives it first); `jo_3d_model_use_fast_memory()` for a
  model drawn often. Big data read rarely can go to the slow work RAM.
- **Both CPUs**: the master CPU runs the game, `jo_core_exec_on_slave()` gives a job to the slave CPU (draw half of the
  scene, compute the physics), `jo_core_wait_for_slave()` waits for it. A job reads the shared data without the cache of
  the other CPU: see the header (`core.h`). The slave stack is small: no big local arrays in its jobs.
- **One mesh per object**: `jo_3d_mesh_draw()` or a JOM model, not quads drawn one by one with `jo_3d_draw()` or
  `jo_3d_draw_array()` (about 1.7 times slower: each call transforms and prepares on its own). A sprite facing the
  camera in 3D: `jo_3d_draw_billboard()` or `jo_3d_draw_standing_sprite()`, about a third of `jo_3d_draw_sprite()`.
- **The VDP1 time is the pixels it walks**, transparent and off-screen pixels included: small sprites and polygons,
  reduced textures far away (`jo_3d_set_texture_mipmaps()`), levels of detail, a draw distance, no half transparency on
  big surfaces (screen doors instead).
- **The VDP2 draws the backgrounds for free**: scrolling planes, bitmaps, the 3D planes of `background.h`, a sky. Never a
  full screen background made of sprites.
- **A frame rate the game holds everywhere**: `JO_FRAMERATE` in the makefile (1 = 60 images per second NTSC, 2 = 30, 3 =
  20). Check with `jo_core_get_fps()` and `jo_core_get_late_frame_count()`.

Details and the costs: `references/performance.md`.

## Use the engine for

| Need | Engine (search the API index for the others) |
|---|---|
| Fixed point math | `jo_fixed`, `jo_fixed_mult()`, `jo_fixed_div()`, `jo_int2fixed()`, `jo_fixed2int()`, `JO_FIXED_1` |
| Trigonometry | `jo_sin_angle()`, `jo_cos_angle()`, `jo_sincos_angle()`, `jo_atan2_angle()` (tables); degrees: `jo_sin()`, `jo_cos()` |
| Roots, vectors | `jo_sqrt()`, `jo_fixed_sqrt()`, `jo_vector_fixed_length()`, `jo_vector_fixed_normalize()`, `jo_vector_fixed_dot()`, `jo_vector_fixed_cross()` |
| Small helpers | `JO_MIN()`, `JO_MAX()`, `JO_CLAMP()`, `JO_ABS()`, `jo_lerp()`, `jo_random()`, `jo_random_seeded()` |
| Memory | `jo_malloc()`, `jo_free()`, `jo_memcpy()`, `jo_memset()`, `jo_dma_copy()`, `jo_dma_copy_async()` |
| Lists | `jo_list_init()`, `jo_list_add()`, `jo_list_remove()`, `jo_list_foreach()` |
| Loop, errors | `jo_core_init()`, `jo_core_add_callback()`, `jo_core_run()`, `jo_core_error()` (with `JO_DEBUG`) |
| Input | `jo_is_pad1_key_pressed()`, `jo_is_pad1_key_down()`, `input.h` for the other ports and devices |
| 2D | `jo_sprite_add_tga()`, `jo_sprite_draw3D()`, `jo_sprite_draw3D_and_rotate()`, `jo_hitbox_detection()`, `map.h`, `sprite_animator.h`, `physics.h` (a platform game, in `jo_fixed`) |
| Backgrounds | `vdp2.h`, `background.h`, `palette.h`, `tga.h` |
| 3D | `jo_3d_load_model()`, `jo_3d_model_draw()`, `jo_3d_camera_*()`, `jo_3d_push_matrix()`, `jo_3d_translate_matrix_fixed()`, `jo_3d_rotate_matrix()`, `jo_3d_set_depth_bias()` |
| Levels, entities | `level.h`, `cell.h` (rooms and portals), `entity.h`, `water.h`, `light.h` |
| Files | `jo_fs_read_file()` (reads the files compressed by `jo_lz.py`), `jo_fs_open()` for streaming |
| Sound | `jo_audio_load_pcm()`, `jo_audio_play_sound()`, `jo_audio_play_cd_track()` |
| Save | `backup.h` |
| Profiling | `JO_COMPILE_WITH_PROFILER_MODULE = 1`, `jo_profile_begin()`, `jo_profile_end()`, `jo_profile_print()` |

## 3D without glitches (short)

The VDP1 has no z-buffer: the engine draws the polygons from the farthest to the nearest, one depth per polygon, and
maps the whole texture on each quad. So: quads rather than triangles, no polygon laid on another one, nothing hidden
inside a volume, the corners in order (they orient the texture and the face), the right sort for the small parts, a
gouraud that never goes over 1.0 on light textures. Check each model with `jo_model.py --check-sort`.
Details: `references/3d.md`.

## References

- `references/performance.md`: the costs of the CPUs, of the memories, of the VDP1 and of the VDP2, and how to share the
  work between the 2 CPUs.
- `references/3d.md`: models, textures, sorts, levels of detail, terrains, transparency, checks.
- `references/assets.md`: the tools of the engine (models, levels, images, sounds, compression) and the CD.
- `references/porting.md`: porting a game from another platform (C, C++, a PC engine) to the Jo Engine.
- `references/diagnose.md`: a game too slow, a crash, a glitch: what to measure and where to look.

## Keeping this skill right

This skill is in the repository of the engine (`.claude/skills/jo-engine`). A change of the engine that changes a rule
here (a new function to use, a new limit, a new warning) updates this skill in the same change. Keep it generic: rules
that apply to any game, not the history of how they were found.
