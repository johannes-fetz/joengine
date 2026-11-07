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
| 2 state | yes | yes | after the API calls: screen modes, VDP2 registers (windows, blend, shadow, mosaic, special colors, sprite priorities, color offset), palettes, sprites, VDP2 memory, files of the CD |
| 3 images | | yes | the scenes of the test ROM, frame by frame, against `golden/rom/<variant>/*.png` |
| 4 sound | | yes | the loudness of each sample (blocks of 100 ms) against `golden/samples/<sample>/audio.json` |
| 5 budgets | sizes: warning only | yes | CPU and VDP1 time of the measures of the test ROM, memory used, sizes of each program (code, data, RAM), against `golden/budgets.json` |
| 6 samples | | yes | each sample boots in Mednafen; its frames against `golden/samples/<sample>/frame_*.png` |

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

- `rom.modules`, `rom.sources`, `rom.variants`: the makefile of the test ROM and its variants (`JO_SCREEN_WIDTH`, etc)
- `samples.skip`: samples not built; `samples.full_variants`: the other builds of the full mode
- `samples.boot.frames`: the frames of each sample compared (the BIOS takes about 700 frames);
  `samples.boot.per_sample`: other frames or `"audio": false` for a sample; `samples.boot.skip`: samples built but not run
- `tolerances`: `image_threshold` (difference of a color component, 0 to 255), `image_max_pixels_percent`, `perf_percent`,
  `ram_percent`, `size_percent`, `audio_db`, `audio_max_blocks_percent`
