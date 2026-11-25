# Performance on the Saturn with the Jo Engine

The budget of a frame: 16.7 ms at 60 images per second (NTSC), 33.3 ms at 30. The master CPU, the slave CPU and the
VDP1 work at the same time: the slowest of the three sets the frame rate. Measure them (see `diagnose.md`) before
changing anything.

Costs of common calls on the CPU (measured in Mednafen, release build; a frame at 60 images per second is about 447 000
cycles of a CPU at 26.8 MHz, 894 000 at 30):

| Call | Cycles (about) |
|---|---|
| `jo_sprite_draw3D()` (no rotation) | 400 |
| `jo_sprite_draw3D_and_rotate()` | 600 |
| `jo_printf()` of a line of 35 characters with 4 numbers | 2 000 (the formatting: 1 500) |
| `jo_3d_draw_billboard()`, `jo_3d_draw_standing_sprite()` | 600 |
| `jo_3d_draw_sprite()` | 1 600 |
| `jo_3d_push_matrix()`, translate, rotate, `jo_3d_pop_matrix()` | 700 |
| `jo_3d_mesh_draw()` of a cube (6 quads, 24 points) | 6 300 |
| the same cube as 6 `jo_3d_draw()` (`jo_3d_draw_array()`) | 10 800 |

The VDP1 draws in parallel: its time is the pixels of the sprites and polygons (below).

## The SH-2 CPUs

About 28 MHz, one instruction per cycle when the data is in the cache. No FPU, no barrel shifter, 16 registers.

| Operation | Cost | Instead |
|---|---|---|
| `float` / `double` operation | tens to hundreds of cycles (libgcc emulation) | `jo_fixed`, `jo_angle`; the link warns about each function that uses float |
| `a / b`, `a % b` with `b` variable | a routine of tens of cycles | an inverse computed once; `jo_fixed_div()`; `jo_fixed_div_start()` + `jo_fixed_div_result()`: the divider unit computes while the CPU works |
| `x << n`, `x >> n` with `n` variable | a routine call | a constant shift, a table, a multiplication, `x += x` in a loop |
| `x >> k` on a signed value (k > 2, not 8 or 16) | often a routine call | `JO_DIV_BY_xxx()` on a value known positive, `jo_shift_right8()`, a multiplication |
| `(long long)a * b` | one instruction (`dmuls.l`) | the way to multiply two 16.16 numbers without overflow |
| 64 bits division | a long routine | avoid it; rescale the operands |
| Function call with more than 4 arguments | the others go through the stack | pass a pointer to a structure, or inline the function |
| Read from the slow work RAM | about twice a read from the fast one | the data read at each frame in the fast work RAM |
| Read through the cache-through address | a bus access each time | read through the cache when the data doesn't change during the read |

Other rules:

- **The cache is 4 KB per CPU** (lines of 16 bytes). A loop and its data that fit in it run much faster: small tables,
  structures of arrays for the data of a loop, no big lookup table read at random.
- **Writes are write-through without allocation**: each write goes to the bus. Writing a structure on the stack and
  reading it back right away costs cache misses; keep the hot values in local variables (registers).
- **Code size matters**: a big function inlined in several places fills the cache. The engine is compiled with `-O2` and
  link time optimization; don't force inlining of big functions.
- **16.16 overflows**: `x * y` of two 16.16 values overflows 32 bits from about 180 x 180 (units); squares of distances
  from about 180 too. Do these products in 64 bits (`(long long)`), or in smaller units.
- **The compiler can bring back a slow routine by itself** (a division it thinks equivalent, a multiplication changed into
  a shift by a variable). Look at the warnings of the link and run `python3 tools/slow_calls.py game.elf`: the functions
  that call `__sdivsi3`, `__udivsi3`, `__ashrsi3`, `__ashiftrt_r4_*`, `__divdi3`... each one is a slow path.
  `JO_OPAQUE()` (`math.h`) stops the compiler from changing a value it doesn't need to change.

## The two CPUs

- The master CPU runs the game and the engine; the slave CPU waits for jobs: `jo_core_exec_on_slave(callback)` gives it
  one, `jo_core_wait_for_slave()` waits for all of them. Give it about half of the work of a frame (draw half of the
  models, the physics of the objects, the particles), measured, not guessed: the gain comes from the balance.
- **Each CPU has its own cache**: the data written by one CPU is not seen by the other through its cache. The engine
  purges the cache of the slave CPU at the start of its jobs, and the cache of the master CPU in
  `jo_core_wait_for_slave()`: the results of a job are read after it. A value shared while both CPUs run (a flag, a
  counter) is read through its cache-through address (`JO_CACHE_THROUGH()`, `sega_saturn.h`). Read the comments of
  `core.h` before sharing data.
- The slave CPU has a small stack: no big local arrays in its jobs.
- The 3D of the engine can draw on the slave CPU (the `jo_3d_slave_*()` functions): the commands of both CPUs are sorted
  together.

## Memory

| Memory | Size | Use |
|---|---|---|
| Fast work RAM (high, 0x06000000) | 1 MB | the program, the engine, `jo_malloc()` first: the data read at each frame |
| Slow work RAM (low, 0x00200000) | 1 MB | `jo_malloc()` when the fast one is full: data read rarely, loading buffers |
| VDP1 VRAM | 512 KB | the textures of the sprites and polygons, the gouraud tables, the command table |
| VDP2 VRAM | 512 KB | the backgrounds (cells, maps, bitmaps) |
| Sound RAM | 512 KB | the sounds kept resident (`jo_audio_make_resident()`), the sound program |

- `jo_malloc()` gives the fast work RAM first. A model drawn at each frame and loaded late can land in the slow RAM:
  `jo_3d_model_use_fast_memory()` moves its points and polygons.
- No allocation in the frame loop; load a level, allocate everything, free everything when it is left, in the reverse
  order. Check that the free memory comes back after a few loads (`jo_memory_usage_percent()`,
  `jo_memory_fragmentation()`).
- The sprites and textures are a stack: `jo_sprite_free_from()` frees from an id. Load what changes (the level) last.
- `JO_COMPILE_WITH_LOW_WORK_RAM_CODE = 1` and `JO_LOW_WORK_RAM_CODE` on the functions called rarely (loading, menus)
  give their fast RAM to `jo_malloc()` (see `Documentation/performance.md`).
- **The VRAM is slow for the CPU** while the VDPs draw: don't read it back (keep a copy in RAM), group the writes, write
  the VDP2 VRAM in the VBLANK or only what changed.

## Loadings

- **Reading the CD is most of a loading**: measured in Mednafen, about 216 KB per second through `fs.h`, 91 % of the
  loading of a level of 600 KB. Compress the data files (`python3 Compiler/COMMON/jo_lz.py -f IN OUT`, or
  `jo_lz.compress_file()` at the end of the build of the assets): `jo_fs_read_file()` and `jo_fs_open()` read them
  decompressed, the CPU decompressing about 1 MB per second. Measured: textures of 16 colors 33 to 47 % of their size,
  tables of a terrain 28 %, sparse layers of the VDP2 12 %; the level loaded in 1.9 s instead of 3.1 s. PCM sounds don't
  compress (104 %): 8 bits samples halve them.
- Measure before and after: `jo_fs_get_read_stats()` (the time spent reading, the bytes, the files) and
  `jo_core_get_vblank_count()` around the whole loading (the decompression and the conversions are outside the reads).
- The files on the disc in the order of their reading (`cd.sort`). A pack needed later in the level (its boss) read with
  the level and kept in the slow RAM (`jo_malloc_set_slow_memory_first()`): no read during the game.

## The VDP1 (sprites and polygons)

- **Its time is the pixels it walks**: the whole quad, its transparent pixels, the pixels hidden by the next polygons and
  the pixels out of the screen. A sprite or a polygon twice as big costs 4 times more.
- A polygon is drawn in lines from its edge A-D to its edge B-C: a thin long quad, or a quad seen on its edge, costs
  about the square of its length.
- **Half transparency reads the frame buffer**: slow, and stripes where lines overlap. For big surfaces, use the screen
  doors (mesh: every other pixel) or the transparency layer of the engine (`transparency.h`).
- **Textures far away**: `jo_3d_set_texture_mipmaps()` (reduced copies), `jo_3d_set_high_speed_shrink()` (on by default
  for the 3D). Fewer texels read = faster.
- **Gouraud costs a little**: a polygon with 4 equal gouraud colors is drawn without it by default
  (`jo_3d_set_uniform_gouraud_skipped()`).
- The command table has a size (`JO_VDP1_MAX_COMMANDS`): a scene with too many polygons loses the last ones.
- **A turning effect** (a swirl, a rotor, a needle): one sprite turned by the VDP1 (`jo_sprite_draw3D_and_rotate()`,
  `jo_sprite_draw_4p_fixed()`) costs one texture. A frame per angle only when the pixel art must stay sharp under a light
  that doesn't turn with it. An object with a symmetry repeats: a gear of N teeth with N / k holes or spokes looks the same
  after k teeth, so k times the steps of a tooth are enough frames, and the gears of the same tooth step that mesh share
  the frame index (each one turns by the same part of a tooth per frame).
- Mednafen counts the VDP1 time well for the pixels on the screen; a real console also pays for the pixels walked out of
  the screen and for the texture lines skipped: keep a margin.

## The VDP2 (backgrounds)

- Up to 4 scrolling planes (NBG0 to NBG3) and 1 or 2 rotating planes (RBG0, the floor or the sky of a 3D view), with
  their priorities, color calculation (transparency between layers) and windows: **free for the CPU and the VDP1**.
- Use them for every background, the HUD made of text or tiles, a sky, a floor seen in perspective (`background.h`).
- The VRAM banks are shared by the layers: a layer that needs a bank already taken is not shown. The engine allocates
  them (`vdp2_malloc.h`): read the remarks of the functions when a layer doesn't show.
- **Sprites mixed by the VDP2**: palette sprites (`jo_sprite_set_palette_code()`) with a priority of their own, mixed
  alone (`jo_vdp2_blend_sprites_of_priority()`, `jo_vdp2_set_sprite_blend_ratios()`): one mix per pixel when the image is
  shown, free, where the half transparency of the VDP1 reads the frame buffer.
- **The extended color calculation** (`JO_VDP2_BLEND_EXTENDED`) doesn't mix the third image when it is a palette image
  in color RAM mode 1 or 2 (the ratios 4:0:0 of the VDP2 manual): the 2 images above it are mixed without it, so an
  effect between them looks flat and opaque. Don't draw the effect where it covers the other mixed layer (a mask
  computed when the level loads).
- Some calls display their layer when they set it (`jo_vdp2_set_nbg1_8bits_bitmap()`; `jo_vdp2_set_nbg2_8bits_image()`
  and `jo_vdp2_set_nbg3_8bits_image()` with `enabled` true): to load behind a loading screen, set the layers hidden
  (`enabled` false, `jo_set_displayed_screens()` after the bitmap) and show them when everything is ready.

## Frame rate

- `JO_FRAMERATE = n` in the makefile: the game runs every n VBLANK. A frame that takes longer delays the next one: aim at
  a frame rate the game holds in its heaviest scene. `JO_FRAMERATE_DYNAMIC = 1`: as soon as a frame is ready.
- `jo_core_get_fps()`, `jo_core_get_late_frame_count()` on the screen while developing.
- A regulator of the level of detail driven by the frame time hides the gains and the losses: freeze it to measure.
