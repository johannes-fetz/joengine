![Jo Engine logo](https://www.jo-engine.org/assets/img/logo-compact.svg)
Welcome to the official Jo-Engine repository !
-------------
**Jo Engine is an open source 2D and 3D game engine and SDK for the Sega Saturn, written in C under MIT license.**
It allows to develop game easily without extensive knowledge in embedded development.
And... games run on emulator and also on the original system !

Development on Windows, Linux and macOS (for macOS, see [Compiler/MACOS/README.txt](Compiler/MACOS/README.txt)).

Website: https://jo-engine.org/

Documentation: https://www.jo-engine.org/doxygen/

## Features

- Drives the Saturn hardware directly: 2 SH-2 CPUs, SCU, VDP1, VDP2, SMPC, sound, CD-ROM.
- 2D: sprites and animations, TGA images, tilesets, maps, VDP2 backgrounds and planes, line scroll, mode 7, transparency, 2D physics and collisions.
- 3D: models and levels in the engine's binary formats (`.JOM`, `.JOL`) converted from OBJ, camera, matrices, gouraud shading, lights, environment mapping, water, vertex animation, levels of detail.
- Fixed point math (`jo_fixed`), angles, vectors, random, lists, memory allocator, DMA, the slave CPU.
- Input: pad, analog pad, wheel, mission stick, light gun, mouse, keyboard, multitap; backup memory, serial link, CD audio tracks, video.
- Tools: model, level and compression converters (`Compiler/COMMON/`), profiler, regression tests (`tests/`), [Jo Map Editor](JoMapEditor).

## Getting started

1. Download the [latest release](https://github.com/johannes-fetz/joengine/releases/latest) or clone this repository.
2. Copy a sample of [Samples/](Samples) (one sample per feature: `demo - minimal`, `demo - 3D model`, `demo - shooter`...) into `Projects/`.
3. Build it with `compile.bat` (Windows) or `compile.sh` (Linux, macOS).
4. Run it in an emulator with one of the `run_with_*` scripts, or burn it on a CD and play it on a Saturn.

The [FAQ](FAQ.TXT) answers the usual questions (memory, resolution, frame rate, CD audio, porting from SGL).

## Coding with an AI assistant

[AGENTS.md](AGENTS.md) (the rules to use the engine well, read by most coding assistants),
the skill [.claude/skills/jo-engine](.claude/skills/jo-engine/SKILL.md) (Claude Code), the index of the API
[Documentation/api_index.md](Documentation/api_index.md) (one line per function of the engine, `tools/make_api_index.py`)
and [llms.txt](llms.txt) (a summary of the project for the language models).

## Games made with Jo Engine

Jo Engine was used for Saturn ports of X-Men (arcade, 1992), Celeste, and for homebrew games like
Bridget Bishop and Vigilante Paradise. More on https://jo-engine.org/#games

## Community

Main forum: https://segaxtreme.net/forums/saturn-dev.34/

Questions: https://github.com/johannes-fetz/joengine/discussions

Bugs: https://github.com/johannes-fetz/joengine/issues

X (aka Twitter): https://twitter.com/johannes_fetz

Wikipedia: https://en.wikipedia.org/wiki/Homebrew_(video_games)#Sega_Saturn

## Publications

* Homebrew Game Development and The Extra Lives of Consoles by Robin Wilde (ISBN: 9781399072656, 139907265X) Page 61-62, 78
* Programación de Videojuegos Para SEGA Saturn by Rolando Fernández Benavidez (ISBN: 9801509687)

Online articles:

* Entretien avec Johannes Fetz à propos de jo-engine : https://linuxfr.org/news/entretien-avec-johannes-fetz-a-propos-de-jo-engine
* Jo Sega Saturn Engine - pour développer des jeux sans crise de nerfs https://www.rom-game.fr/news/2150-Jo+Sega+Saturn+Engine+-+pour+developper+des+jeux+sans+crise+de+nerfs.html
* EnginesDatabase: https://enginesdatabase.com/engine/jo_engine/
