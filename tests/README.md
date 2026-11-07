# Jo Engine regression tests

The tests build the samples and a test ROM, run them in Mednafen and compare what they show, play and measure with
references kept in this directory (`tests/golden`). Everything runs on your computer: no server, no Python package.

## Requirements

- Python 3.6 or later (`py -3` or `python` in the `PATH`), with nothing else to install
- Mednafen 1.29 in `Emulators/mednafen` (`mednafen.exe`) with the Saturn BIOS in `Emulators/mednafen/firmware`
  (the files you already use with `run_with_mednafen.bat`). Another place: `--mednafen <exe>` or `MEDNAFEN=<exe>`
- The compiler of the repository (`Compiler/WINDOWS`), like `compile.bat`

The emulator runs with its own settings in `tests/_work/mednafen_home`: your `mednafen.cfg` is not changed.

## Usage

| Command | When | What |
|---|---|---|
| `run_tests.bat` | before a commit (about 1 minute) | quick: layers 0, 1 and 2 |
| `run_tests.bat --full` | on demand, after a big change (30 minutes or more) | all the layers |
| `run_tests.bat --approve` | after an intended change | runs everything and records the results as the new references |

Other options: `--only fighters rom` (names that contain one of the words), `--rom-only`, `--no-build` (the builds of the
last run), `--no-audio` (the samples run without sound, so faster than real time), `--jobs N` (parallel builds).

The exit code is 0 when everything passed. The report is `tests/_work/report.html`: failures first, with the reference
picture, the new picture and their difference. `tests/_work/results.json` has the same results for scripts.

On Linux and macOS: `python3 tests/run_tests.py` (with `--compiler-dir` if needed). Without a display, the harness
starts `Xvfb`.

## Layers

| Layer | Quick | Full | Checks |
|---|---|---|---|
| 0 build | every sample, the test ROM 320x224 | + the variants (`release`: `JO_DEBUG=0`), the test ROM 704x480 | the build succeeds; the warnings of the test ROM are shown |
| 1 logic | yes | yes | math (fixed point, square roots, trigonometry, matrices), strings, `jo_sprintf`, lists, `jo_malloc` |
| 2 state | yes | yes | after the API calls: screen modes, VDP2 registers (windows, blend, shadow, mosaic, special colors, sprite priorities, color offset), palettes, sprites, the VDP1 commands of the colored shapes (`jo_sprite_draw_rectangle()`: corners, draw mode, effects, depth key), VDP2 memory, files of the CD |
| 3 images | | yes | the scenes of the test ROM, frame by frame, against `golden/rom/<variant>/*.png` |
| 4 sound | | yes | the loudness of each sample (blocks of 100 ms) against `golden/samples/<sample>/audio.json` |
| 5 budgets | sizes: warning only | yes | CPU and VDP1 time of the measures of the test ROM, memory used, sizes of each program (code, data, RAM), against `golden/budgets.json`; the absolute limits of `samples.max_bin_size` (`demo - minimal`, built with `JO_MINIMAL = 1`: a failure in both modes) |
| 6 samples | | yes | each sample boots in Mednafen; its frames against `golden/samples/<sample>/frame_*.png` |
| 7 checks | with `--only <sample>` | yes | builds of a sample with defines that run a scripted scene and judge it (`samples.checks`, see below) |
| 3 glitch | 9 views | 217 views | the anti-glitch views of the 3D (`rom/t_glitch.c`, `harness/glitch.py`): absolute checks, no reference (see below) |

Layers 1 and 2 have two kinds of checks:

- **assertions** written in the test ROM (`JT_EQ`, `JT_TRUE`, `JT_NEAR`): known results (`jo_sqrt(144) == 12`, the SDCTL
  bit of NBG1 after `jo_vdp2_enable_shadow(JO_NBG1_SCREEN)`). A failure is a bug or a wrong test: never approved
- **characterizations** (`jt_value`, `jt_blob`): the current result (a hash of `jo_sin` from 0 to 359 degrees, the VDP2
  registers changed by an API call) compared with `golden/rom_values.json`. A difference is shown with the names of the
  VDP2 registers; `--approve` records the new value when the change is intended

## References

Mednafen starts each run with the same clock (2000-01-01 12:00, see `harness/emulator.py`): `jo_core_init()` seeds
`jo_random()` with it, the samples give the same frames at each run. When a sample shows the same picture (or sound) a
few frames sooner or later, the result is a warning (the timing changed), not a failure.

The references are made by `--approve` on the computer that runs the tests: the pictures, the timings and the sizes
depend on the compiler and on the version of Mednafen. Mednafen gives the same frames at each run (same BIOS, same
settings), so a picture is compared pixel by pixel (tolerances in `config.json`).

After an intended change (a new effect, a faster function):

1. `run_tests.bat --full`, then check the differences in the report
2. `run_tests.bat --approve`
3. commit `tests/golden` with the change

A result without reference is `NEW` (not a failure): a new scene, a new sample, a new value.

## The test ROM

`tests/rom` is a Saturn program (its makefile is written by the harness from `config.json`). It runs steps:

1. the checks of layers 1 and 2 (`t_logic.c`, `t_more.c`, `t_state.c`, and `t_bugs.c`: one check per bug fixed, that
   failed before its fix), then a report
   - `t_leaks.c`: each resource (models, meshes, sprites, tints, gouraud tables, fonts, files, sounds, maps, levels...)
     created, used and freed during 8 cycles: the free memory (work RAM, VDP1, sprites, VDP2, color RAM, file handles)
     must be exactly the same after them
   - `t_fuzz.c`: the parsers of files (JOM, TGA, WAV, MAP, JOL) fed with damaged data (in memory, and the files of
     `rom/cd/FUZZ`, made by `rom/gen_fuzz.py` from `rom/fuzz_seeds`) and random sequences of the APIs (malloc, VDP2 malloc,
     sprites, lists); after each input: no crash, the heap walk, no write after the VDP1 textures, no leak. Short in the
     other variants, long in the variants `fuzz` and `fuzz_release` (`T_FUZZ_LONG`; `rom.variants` keys `T_*` are defines
     of the test ROM). A failed check gives the iteration: `T_FUZZ_FIRST` / `T_FUZZ_COUNT` replay it alone
2. the measures (`scenes.c`, `JT_STEP_BENCH`: the averages of the profiler over 32 frames), then a report
3. the scenes (`JT_STEP_SCENE`): a marker, then a static picture during 24 frames

The ROM sends its results as a barcode at the top of the screen (blocks of 4 x 4 pixels, with a CRC): the harness reads
the frames of the video recorded by Mednafen (`-qtrecord`, PNG codec) while it is written. See the top of `rom/jt.c`.

To add a check: a function in `t_logic.c` or `t_state.c` with `jt_group("name")` and `JT_EQ(actual, expected)`. To add a
scene or a measure: a line in `t_scene_steps` (`scenes.c`), then `--approve`. A scene must show the same picture at each
frame (no time, no random without a fixed seed), in the 320 and 704 pixels modes (the 640 and 704 pixels modes show only
the palette sprites and colors).

An error of the engine (`jo_core_error()`) does not stop the ROM: the test ROM sets `jo_core_set_error_handler()`, the
error is a failure of the report and the next checks run. A check of an error path counts the errors between
`jt_expect_errors(true)` and `jt_expect_errors(false)`. A ROM that stops (a crash) fails with its last frame in the
report.

## Configuration (`config.json`)

- `rom.modules`, `rom.sources`, `rom.variants`: the makefile of the test ROM and its variants (`JO_SCREEN_WIDTH`, etc); the keys
  `T_*` are defines of the test ROM (`T_FUZZ_LONG`)
- `samples.skip`: samples not built; `samples.full_variants`: the other builds of the full mode
- `samples.max_bin_size`: the most bytes of the program (`game.raw`) of a sample in each build (`demo - minimal`: the
  engine reduced by `JO_MINIMAL = 1` must stay much smaller than the default engine, 29 KB for the same program)
- `samples.boot.frames`: the frames of each sample compared (the BIOS takes about 700 frames);
  `samples.boot.per_sample`: other frames or `"audio": false` for a sample; `samples.boot.skip`: samples built but not run
- `samples.checks`: the checks of a sample (layer 7): `name`, `what` (shown in the report), `variables` (of make, e.g.
  `_CCFLAGS` with the defines of the check), `first_frame` (the verdict is looked for from this frame), `max_frames`
- `tolerances`: `image_threshold` (difference of a color component, 0 to 255), `image_max_pixels_percent`, `perf_percent`,
  `ram_percent`, `size_percent`, `audio_db`, `audio_max_blocks_percent`

## The checks of the samples

A check builds a sample with the defines of `samples.checks` (a scripted scene, no input) and runs it until the sample
shows its verdict: the whole screen green (PASS) or red (FAIL), with its counts as text (in the report). They run in the
full mode, and in the quick mode when `--only` names the sample (`run_tests.bat --only "first person"`, about 2 minutes).

| Sample | Check | Defines | Rule |
|---|---|---|---|
| `demo - first person shooter` | persons drawn on the walkway | `DEMO_BENCH DEMO_BENCH_VIEW=6 DEMO_BENCH_CHECK` | each person in the view of a frame is drawn in it, over 192 frames (the guard of the walkway vanished and came back: the persons out of the view in the bottling room below took the VDP1 budget of the persons, 83 missed before the fix of `plan_draw()`, 0 after) |
| `demo - first person shooter` | persons drawn in the laboratory | `DEMO_BENCH DEMO_BENCH_VIEW=1 DEMO_BENCH_CHECK` | the same rule with 2 scientists and 2 guards |

To add a check to a sample: a mode of the sample (a define) that plays a scene without input and counts what must hold
in each frame, then shows the verdict with `jo_set_default_background_color(JO_COLOR_Green)` or `JO_COLOR_Red` over the
whole screen (the 3D no longer drawn: `jo_3d_set_frame_clear(true)`), and an entry in `samples.checks`. Prove it: it must
fail on the code with the bug.

## The anti-glitch views of the 3D

`rom/t_glitch.c` draws known scenes from known cameras over a magenta background that no texture uses (the NBG1 bitmap
behind the sprites), each view a scene step of 9 frames (the last 3 black: the harness takes the last picture of the
view before them, a heavy view is shown during a few frames only). `harness/glitch.py` computes what each picture must
show with the projection of the engine (projection distance 160 pixels, center (160, 120), near plane at 20 units:
checked by the ROM) and reports, with the parameters of the view, the counts and zoomed crops (`report.html`):

| Check | Scenes | Rule |
|---|---|---|
| holes | the room seen from inside (6 faces of 8 x 8 quads sharing their vertices, or 6 big quads; textured with subdivision (the big quads and the single quads with the finer tiles, `jo_3d_set_texture_subdivision_fine()`), without, flat), the ground (24 x 24 quads), cameras from the center to 0.5 unit of a wall, an edge, a corner, 0.75 unit over the ground | a ray per pixel: a pixel whose ray hits the geometry in front of the near plane must be drawn (2 pixels from the edges of that region not checked). Nearer than the near plane, drawing is allowed (the engine draws the parts through the near plane whole) |
| spurious | the ground | nothing drawn where no ray hits the geometry |
| deformation, holes | one quad (facing, oblique, edge on, through the near plane, behind the camera, 4000 units wide, 2000 units away) | the quad clipped by the near plane must be drawn; nothing drawn more than 2.5 pixels out of the quad (the edges grown by 1 pixel, the rounding of the VDP1) in front of the camera |
| hull | a twisted quad, a quad seen exactly edge on | nothing drawn out of the convex hull of the corners |
| mapping | the textured quads, the room and the ground with subdivision | the texture is a gradient (red = column, green = line, blue = 31 - column): the texel of each pixel against the perspective correct texel; good within 1.5 texels + 10 pixels (the distortion of 8 pixels allowed by the subdivision + 2); 95 % of the pixels; in the room, not the pixels 2 pixels or less from an edge between 2 faces (the polygons are grown by 1 pixel) |
| mipmaps | the ground with `jo_3d_set_texture_mipmaps()`, also `FLIP_H` | each quad drawn as one command is mapped bilinearly by the VDP1 from its corners: column (flipped or not) and line within 3.5 texels + 1 pixel; 0.2 % of the pixels at most |
| colors | all | every pixel drawn is a color of the scene (a texel of the gradient or its averages, the flat colors) |
| slave | the room and the ground drawn by the slave CPU | the same picture as the master CPU |

Views flagged `DEFECT` draw a known defect (a quad missing, a crack of 2 units, a corner moved by 8 units, the ground not
flipped) and views flagged `CONTROL` a known bad case (a textured quad through the near plane without subdivision): their
checks must fail, else the check is broken (FAIL). Glitches of the engine known and not fixed are listed in `KNOWN`
(`harness/glitch.py`): a WARN with the reason and the most pixels of each problem (more: a FAIL). Remove an entry when
the engine is fixed.

The views are in `rom/t_glitch_views.h`, written by `python3 tests/harness/glitch.py --write-views` from `make_views()`
(edit the sweeps there, then rebuild the ROM: the checks read the same file). The views need the 320 or 352 pixels modes
(RGB textures): none in the 704x480 variant.
