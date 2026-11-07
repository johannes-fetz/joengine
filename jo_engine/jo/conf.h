/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2026, Johannes Fetz (johannesfetz@gmail.com)
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of the Johannes Fetz nor the
**       names of its contributors may be used to endorse or promote products
**       derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL Johannes Fetz BE LIABLE FOR ANY
** DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/** @file conf.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine Internal configuration
 *  @bug No known bugs.
 */

#ifndef __JO_CONF_H__
# define __JO_CONF_H__

/*
      _             _ _     _                   _       _   _     _        __ _ _
     | |           ( ) |   | |                 | |     | | | |   (_)      / _(_) |
   __| | ___  _ __ |/| |_  | |_ ___  _   _  ___| |__   | |_| |__  _ ___  | |_ _| | ___
  / _` |/ _ \| '_ \  | __| | __/ _ \| | | |/ __| '_ \  | __| '_ \| / __| |  _| | |/ _ \
 | (_| | (_) | | | | | |_  | || (_) | |_| | (__| | | | | |_| | | | \__ \ | | | | |  __/
  \__,_|\___/|_| |_|  \__|  \__\___/ \__,_|\___|_| |_|  \__|_| |_|_|___/ |_| |_|_|\___|

  All options are now available on your Makefile.

  makefile sample:

    JO_COMPILE_WITH_VIDEO_MODULE = 0
    JO_COMPILE_WITH_BACKUP_MODULE = 1
    JO_COMPILE_WITH_TGA_MODULE = 1
    JO_COMPILE_WITH_AUDIO_MODULE = 1
    JO_COMPILE_WITH_AUDIO_DRIVER_MODULE = 0      (the 68000 runs a sound driver: glides, tones; needs the audio module)
    JO_COMPILE_WITH_AUDIO_DSP_MODULE = 0         (the DSP of the SCSP makes a reverberation; needs the audio module)
    JO_COMPILE_WITH_3D_MODULE = 1
    JO_COMPILE_WITH_PSEUDO_MODE7_MODULE = 1
    JO_COMPILE_WITH_EFFECTS_MODULE = 1
    JO_COMPILE_WITH_DUAL_CPU_MODULE = 1
    JO_COMPILE_WITH_PRINTF_MODULE = 1
    JO_COMPILE_WITH_RAM_CARD_MODULE = 1
    JO_COMPILE_WITH_STORYBOARD_MODULE = 1
    JO_COMPILE_WITH_SPRITE_HASHTABLE = 0
    JO_COMPILE_WITH_SOFTWARE_RENDERER_MODULE = 1
    JO_GLOBAL_MEMORY_SIZE_FOR_MALLOC = 524288
    JO_PSEUDO_SATURN_KAI_SUPPORT = 1
    JO_MAX_FS_BACKGROUND_JOBS = 4
    JO_MAX_SPRITE = 255
    JO_MAP_MAX_LAYER = 8
    JO_MAX_SPRITE_ANIM = 16
    JO_MAX_FILE_IN_IMAGE_PACK = 32
    JO_DEBUG = 1
    JO_NTSC = 0
    JO_SCREEN_WIDTH = 320                        (320, 352, 640 or 704; optional)
    JO_SCREEN_HEIGHT = 240                       (224, 240, 256 in PAL; 448, 480, 512 in PAL: interlace; optional)
    SRCS=main.c
    include ../Compiler/COMMON/jo_engine_makefile

  The smallest engine: JO_MINIMAL = 1

    JO_MINIMAL = 1
    SRCS=main.c
    include ../Compiler/COMMON/jo_engine_makefile

  Every module is off unless the makefile turns it on, even the 5 modules on by default: the file system (FS: on with
  the TGA, AUDIO, VIDEO and LEVEL modules, they read files), jo_printf() (PRINTF), the RAM cartridge (RAM_CARD), the
  slave CPU (DUAL_CPU) and jo_sprite_name2id() (SPRITE_HASHTABLE). What stays: jo_core_init() and the main loop, the
  VDP1 (sprites added from images in the program, jo_sprite_draw_rectangle()...), the VDP2 (the back color, the NBG1
  bitmap, the palettes), the inputs, jo_malloc(), the math, the lists, the time. Without jo_printf() an error of the
  engine (JO_DEBUG = 1) shows a red screen, press START to go on. A module can still be chosen one by one:

    JO_MINIMAL = 1
    JO_COMPILE_WITH_PRINTF_MODULE = 1            (jo_printf() and the messages of the errors)
    JO_DEBUG = 0                                 (no check, no error message: the smallest program)

  The helpers that are always compiled (maps, sprite animations, physics, sprite fonts, images, palettes) cost no byte
  in a program that does not call them: the link time optimization keeps only the functions used. Sizes of
  Samples/demo - minimal (game.raw): 29052 bytes with the default engine, 20940 with JO_MINIMAL = 1, 17616 with
  JO_MINIMAL = 1 and JO_DEBUG = 0
*/

/*
** PARAMS
*/

/** @brief Width of the screen at the start (makefile: JO_SCREEN_WIDTH = 320, 352, 640 or 704; see
 *  jo_core_set_screen_resolution() to change it while the game runs)
 */
#if defined(JO_SCREEN_WIDTH)
    # define JO_TV_WIDTH                        (JO_SCREEN_WIDTH)
#else
    # define JO_TV_WIDTH                        (320)
#endif
/** @brief Height of the screen at the start (makefile: JO_SCREEN_HEIGHT = 224 or 240, 256 in PAL; 448, 480 or 512: double
 *  density interlace)
 */
#if defined(JO_SCREEN_HEIGHT)
    # define JO_TV_HEIGHT                       (JO_SCREEN_HEIGHT)
#elif defined(JO_NTSC_VERSION)
    # define JO_TV_HEIGHT                       (240)
#else
    # define JO_TV_HEIGHT                       (256)
#endif
/** @brief Screen width divided by 2 (math optimization purpose) */
# define JO_TV_WIDTH_2                          (JO_TV_WIDTH / 2)
/** @brief Screen width divided by 4 (math optimization purpose) */
# define JO_TV_WIDTH_4                          (JO_TV_WIDTH / 4)
/** @brief Screen height divided by 2 (math optimization purpose) */
# define JO_TV_HEIGHT_2                         (JO_TV_HEIGHT / 2)
/** @brief Screen height divided by 4 (math optimization purpose) */
# define JO_TV_HEIGHT_4                         (JO_TV_HEIGHT / 4)

#if JO_TV_WIDTH != 320 && JO_TV_WIDTH != 352 && JO_TV_WIDTH != 640 && JO_TV_WIDTH != 704
# error "JO_SCREEN_WIDTH must be 320, 352, 640 or 704"
#endif
#if JO_TV_HEIGHT != 224 && JO_TV_HEIGHT != 240 && JO_TV_HEIGHT != 448 && JO_TV_HEIGHT != 480 && \
    (defined(JO_NTSC_VERSION) || (JO_TV_HEIGHT != 256 && JO_TV_HEIGHT != 512))
# error "JO_SCREEN_HEIGHT must be 224, 240, 448 or 480 (256 and 512 in PAL)"
#endif

/** @brief VDP2 Background bitmap size */
# define JO_VDP2_SIZE                           (0x02) /* 512x256 */
/** @brief VDP2 Background bitmap width */
# define JO_VDP2_WIDTH                          (512)
/** @brief VDP2 Background bitmap height */
# define JO_VDP2_HEIGHT                         (256)

/** @brief Max character in available in jo_printf() */
# define JO_PRINTF_BUF_SIZE				    (64)
/** @brief Max filename length (DOS) <=> GFS_FNAME_LEN + 1 */
# define JO_MAX_FILENAME_LENGTH             (13)
/** @brief Max file available in the entire file system (on the CD of course) */
# define JO_FS_MAX_FILES					(256)

#if JO_FRAMERATE < 1
# error "JO_FRAMERATE must be greater than zero"
#endif

#endif /* !__JO_CONF_H__ */

/*
** END OF FILE
*/
