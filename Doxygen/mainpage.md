Jo Engine documentation {#mainpage}
=======================

Jo Engine is an open source 2D and 3D game engine for the **Sega Saturn**, written in C under the MIT license.
It lets you develop games without extensive knowledge of embedded development, and the games run on emulators
as well as on the original console.

The engine talks directly to the hardware (VDP1, VDP2, SCSP, SMPC, CD block): it doesn't need any Sega library.

- Website: https://jo-engine.org/
- Source code: https://github.com/johannes-fetz/joengine
- Forum: https://segaxtreme.net/forums/saturn-dev.34/

Getting started
---------------

Development works on Windows, Linux and macOS. The fastest way to start is to copy a folder from `Samples`
(for instance `Samples/demo1`) and to modify it:

1. Write your game in `main.c` (and add your other source files to `SRCS` in the `makefile`).
2. Put your assets (images, sounds, maps, etc) in the `cd` folder: it becomes the file system of the CD.
3. Run `compile.bat` (Windows) or `compile.sh` (Linux and macOS): it builds `game.iso` and `game.cue`.
4. Run the game with one of the `run_with_*` scripts (Yabause, Mednafen, Kronos, SSF, etc) or burn it.

A minimal game, which displays a sprite loaded from the CD:

~~~{.c}
#include <jo/jo.h>

void            my_draw(void)
{
    jo_sprite_draw3D(0, 0, 0, 500);
}

void            jo_main(void)
{
    jo_core_init(JO_COLOR_Black);
    jo_sprite_add_tga("TEX", "SONIC.TGA", JO_COLOR_Transparent);
    jo_core_add_callback(my_draw);
    jo_core_run();
}
~~~

`jo_main()` is the entry point. jo_core_init() initializes the hardware, jo_core_add_callback() registers a
function called every frame and jo_core_run() starts the main loop (it never returns).

The project makefile
--------------------

Every module can be enabled or disabled in the `makefile` of your project to save memory:

| Option | Description |
|--------|-------------|
| `JO_COMPILE_WITH_3D_MODULE` | 3D meshes, matrices, camera, models, lights and glows (\ref 3d.h, \ref model.h, \ref light.h, \ref glow.h) |
| `JO_COMPILE_WITH_LEVEL_MODULE` | Levels (.JOL files): cells and portals, entities, weapons, vehicles (\ref level.h) |
| `JO_COMPILE_WITH_AUDIO_MODULE` | PCM sounds and CD audio tracks (\ref audio.h) |
| `JO_COMPILE_WITH_AUDIO_DRIVER_MODULE` | The 68000 runs a sound driver: glides, tones (needs the audio module) |
| `JO_COMPILE_WITH_AUDIO_DSP_MODULE` | The DSP of the SCSP adds a reverberation (needs the audio module) |
| `JO_COMPILE_WITH_BACKUP_MODULE` | Save data in the internal memory, on a cartridge or an external device (\ref backup.h) |
| `JO_COMPILE_WITH_TGA_MODULE` | TGA image loader (\ref tga.h) |
| `JO_COMPILE_WITH_VIDEO_MODULE` | Sega FILM (CPK) video player (\ref video.h) |
| `JO_COMPILE_WITH_PSEUDO_MODE7_MODULE` | Mode 7 floor (\ref mode7.h) |
| `JO_COMPILE_WITH_EFFECTS_MODULE` | Special effects: particles, flash, fade, shake (\ref effects.h) |
| `JO_COMPILE_WITH_PROFILER_MODULE` | Time spent by the CPUs and the VDP1 in each frame (\ref profile.h) |
| `JO_COMPILE_WITH_DUAL_CPU_MODULE` | Run code on the slave SH-2 |
| `JO_DEBUG` | Debug mode: error checks, error screen (press START to continue) and memory usage tools |
| `JO_NTSC` | 1: NTSC (60 Hz), 0: PAL (50 Hz) |
| `JO_480i` | 480 lines interlaced high resolution |
| `JO_FRAMERATE` | The game runs every n VBLANK (1: 60 FPS, 2: 30 FPS). `JO_FRAMERATE_DYNAMIC = 1`: as soon as a frame is done |
| `JO_GAME_TITLE`, `JO_GAME_VERSION`, `JO_GAME_RELEASE_DATE`, `JO_GAME_MAKER_ID`, `JO_GAME_PRODUCT_NUMBER`, `JO_GAME_PERIPHERALS` | Game information written in the IP.BIN (the boot header of the CD) |
| `JO_GLOBAL_MEMORY_SIZE_FOR_MALLOC` | Memory size used by jo_malloc() in the high work RAM |
| `JO_MAX_SPRITE`, `JO_VDP1_MAX_COMMANDS` | Maximum sprites (textures) and VDP1 commands per frame: smaller to save RAM |

`JO_DEBUG` should be disabled in the final version of your game: the checks are removed and the game is faster.

Modules
-------

### Core
| Header | Content |
|--------|---------|
| \ref jo.h | Main header: the only one to include |
| \ref core.h | Initialization, main loop, callbacks, error handler |
| \ref conf.h | Engine configuration (limits, sizes) |
| \ref types.h | Common types (positions, sizes, images, etc) |
| \ref malloc.h | Dynamic memory allocation (jo_malloc(), jo_free()) |
| \ref list.h | Linked list |
| \ref time.h | Ticks, elapsed time and delta time |
| \ref tools.h | Utilities (printf, strings, endianness, etc) |
| \ref math.h | Fixed point numbers, trigonometry, random numbers, vectors and matrices |
| \ref sega_saturn.h | Sega Saturn hardware: registers, addresses, memory map |
| \ref dsp.h | SCU DSP programs (the third processor) |

### Graphics
| Header | Content |
|--------|---------|
| \ref sprites.h | Sprites (VDP1): loading, drawing, scaling, rotation |
| \ref sprite_animator.h | Sprite animations |
| \ref image.h | Image definition and conversion tools |
| \ref tga.h | Truevision Targa (TGA) loader |
| \ref palette.h | 8 bits palettes |
| \ref colors.h | Standard colors |
| \ref vdp2.h | Backgrounds (VDP2): NBG0 to NBG3, RBG0 rotation, line scroll, mosaic, etc |
| \ref background.h | Background helpers |
| \ref vdp2_malloc.h | VDP2 memory allocation |
| \ref vdp1_command_pipeline.h | VDP1 command table, windows, wireframe |
| \ref map.h | Tile maps |
| \ref mode7.h | Mode 7 floor |
| \ref 3d.h | 3D meshes, camera, matrices, texture subdivision, slave CPU rendering |
| \ref model.h | 3D models (.JOM files made by `Compiler/COMMON/jo_model.py`), skeletons and poses |
| \ref light.h | Point lights baked in the gouraud tables of the meshes |
| \ref glow.h | Glows: the halo of light around a lamp (VDP2 layers) |
| \ref software_renderer.h | Triangle based software renderer |
| \ref voxel.h | Voxel landscape renderer |
| \ref effects.h | Special effects: particles, flash, fade, shake |
| \ref storyboard.h | Storyboard animations (movements, rotations) |
| \ref font.h | Bitmap fonts |
| \ref video.h | Video player (Sega FILM/CPK) |

### Input, sound and storage
| Header | Content |
|--------|---------|
| \ref input.h | Gamepads, analog pads, mouse, multitap |
| \ref keyboard.h | Keyboard |
| \ref smpc.h | System Management & Peripheral Control |
| \ref audio.h | PCM sounds and CD audio tracks |
| \ref fs.h | CD file system |
| \ref backup.h | Saves (internal memory, cartridge or external device) |
| \ref serial.h | Serial link (Taisen cable) |
| \ref vcd_card.h | VCD card (work in progress) |

### Levels (JO_COMPILE_WITH_LEVEL_MODULE)
| Header | Content |
|--------|---------|
| \ref level.h | Levels (.JOL files, see `Documentation/level_format.md`), written by `Compiler/COMMON/jo_level` |
| \ref level_format.h | The sections of a level file |
| \ref cell.h | Cells and portals: the rooms of an inside level, the visibility |
| \ref entity.h | Entities: doors, explosives, triggers, actors (animated models), enemies |
| \ref weapon.h | Weapons of the player |
| \ref vehicle.h | Vehicles driven by the player |

### Tools
| Header | Content |
|--------|---------|
| \ref console.h | Debug console |
| \ref hitbox.h | Hitboxes and collisions |
| \ref physics.h | Basic physics |
| \ref profile.h | Profiler: the time of the CPUs and of the VDP1 in each frame |

Samples
-------

The `Samples` folder contains about 50 demos (sprites, 3D, 3D models, environment mapping, VDP2 planes, mode 7,
audio, video, backup, gamepads, mouse, keyboard, a first person shooter, etc). They are the best way to learn how
to use each module.

Tools
-----

The Python scripts of `Compiler/COMMON` prepare the assets of a game:

- `jo_model.py`: converts a Wavefront OBJ model (and its textures) into a .JOM file (\ref model.h)
- `jo_level`: writes, reads and checks .JOL level files (\ref level.h)
- `jo_lz.py`: compresses a file for jo_lz_decompress() (\ref tools.h)
- `scudsp_asm.py` and `scsp_dsp_asm.py`: assemblers of the SCU DSP and of the SCSP DSP
