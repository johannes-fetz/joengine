# Assets: the tools of the engine and the CD

Every file of the CD is written by a tool, from sources that are kept (OBJ, PNG, WAV, a generator in Python). Never edit
a generated file by hand, never write a file format of the engine by hand: the tools already know its rules
(alignment, endianness, limits of the hardware).

## 3D models (`.JOM`)

- **From a 3D software**: export Wavefront OBJ + MTL, convert with
  `python3 Compiler/COMMON/jo_model.py MODEL.OBJ -o cd/MODEL.JOM [options]`, load with `jo_3d_load_model()`.
  - `--saturn-axes` when the OBJ is already in the axes of the Saturn (X right, Y down, Z forward); else the converter
    turns it (Y up OBJ, as Blender exports it).
  - `--scale S`, `--embed-textures` (the textures in the JOM file), `--skeleton FILE.json` (a rigid skeleton and its
    animations), `--morph FILE.json` (vertex animations), `--info FILE.JOM` (what a file holds).
  - The materials: the MTL keys `jo_*` (`jo_dual 1`, `jo_sort min|max|center`, `jo_screen_doors 1`, `jo_light 1`,
    `jo_lut 1` for a texture in 16 colors, `jo_value N` for a value read by the game): see the help of the converter
    (`python3 Compiler/COMMON/jo_model.py --help`).
  - `--check-sort FILE.JOM`: the views all around the model drawn like the VDP1, the glitches and the holes.
  - The format: `Documentation/model_format.md`; the API: `model.h`.
- **Built by a program in Python** (generated scenery, tracks, cars): `Compiler/COMMON/jo_mesh.py` (`Mesh`, `Material`,
  `Texture`, `write()`): it orders the corners, quantizes the textures in 16 colors, limits the gouraud, writes the JOM.
- **Built in C at run time**: the mesh functions of `3d.h` (`jo_3d_create_mesh()`...).

## Levels (`.JOL`)

- The data of a level (terrain, models, plants, entities, cells and portals, the patterns of the water, the sprites of
  the sun...): `Compiler/COMMON/jo_level` (Python), read by `level.h`. `python3 Compiler/COMMON/jo_level check FILE.JOL`
  validates a file. The format: `Documentation/level_format.md`.

## Images

- Sprites and backgrounds: TGA (`jo_sprite_add_tga()`, `tga.h`): 8 bits with a palette (the palette set first, see
  `tga.h`), 24 or 32 bits (converted to the 16 bits colors of the Saturn), raw or RLE. The width of a sprite is a
  multiple of 8.
- 16 colors for the textures of the 3D when possible (half the VRAM of 8 bits, a quarter of 16 bits): `jo_lut 1` in the
  MTL, or `jo_mesh.Texture`.
- Big backgrounds go to the VDP2 (`vdp2.h`, `background.h`), with their palette (`palette.h`).

## Sounds and music

- Sound effects: WAV (PCM 8 or 16 bits, mono or stereo) with `jo_audio_load_wav()`, or raw PCM with
  `jo_audio_load_pcm()`; `jo_audio_play_sound()`. Keep the short ones resident in the sound RAM
  (`jo_audio_make_resident()`).
- Music: the audio tracks of the CD (`jo_audio_play_cd_track()`: the WAV files of the folder of `JO_CD_AUDIO_DIR`,
  written in the CUE sheet by the build), or a WAV streamed from the CD (`jo_audio_play_music()`).

## Compression and the CD

- `python3 Compiler/COMMON/jo_lz.py -f IN OUT`: a compressed file of the CD; `jo_fs_read_file()` reads it decompressed
  (fewer sectors to read: faster loading). `jo_lz.compress()` for the data kept compressed in RAM.
- The files of the CD: the folder `cd/` of the game, 8.3 names in capitals. `cd.sort` next to the makefile orders the
  files on the disc (the files read one after the other side by side: no seek).
- Read big files once at loading; for streaming, `jo_fs_open()` and the background reads of `fs.h`.
