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

/*
** INCLUDES
*/
#include <stdbool.h>
#include "jo/conf.h"
#include "jo/types.h"
#include "jo/sega_saturn.h"
#include "jo/smpc.h"
#include "jo/core.h"
#include "jo/audio.h"
#include "jo/tools.h"
#include "jo/malloc.h"
#include "jo/fs.h"
#include "jo/math.h"
#include "internal/sega_saturn.h"
#include "internal/fs.h"

#ifdef JO_COMPILE_WITH_AUDIO_SUPPORT

/*
** By default the sound CPU (68000) runs an idle loop: sounds are played by programming the SCSP slots directly from the
** SH-2. PCM data are copied in sound RAM.
**
** Options (makefile of the game):
**   JO_COMPILE_WITH_AUDIO_DRIVER_MODULE = 1   the 68000 runs a driver (sound_driver/driver.c): the SH-2 writes commands
**                                             in the sound RAM and the 68000 programs the slots (no wait on the SH-2
**                                             between a key off and a key on), the pitches glide (each millisecond),
**                                             the tones (FM, noise: jo_audio_play_tone())
**   JO_COMPILE_WITH_AUDIO_DSP_MODULE = 1      the DSP of the SCSP runs a reverberation (dsp/reverb.scsp):
**                                             jo_audio_set_reverb(), the reverb field of the sounds; its ring buffer
**                                             takes the last 16 KB of the sound RAM
*/

#ifdef JO_COMPILE_WITH_AUDIO_DRIVER_SUPPORT
# include "sound_driver/protocol.h"
# include "audio_driver_68k.h"
#endif
#ifdef JO_COMPILE_WITH_AUDIO_DSP_SUPPORT
# include "scsp_dsp_reverb.h"
#endif

/*
** INTERNAL MACROS
*/
#ifdef JO_COMPILE_WITH_AUDIO_DRIVER_SUPPORT
/** @brief The waveform of the tones: a sine of 1024 samples (16 bits), with 256 samples before and after (the FM moves
 *  the reads around the sample played) */
# define JO_AUDIO_TONE_SAMPLES              (1024)
# define JO_AUDIO_TONE_GUARD                (256)
# define JO_AUDIO_TONE_WAVE                 (JO_DRIVER_AREA_END)
/** @brief The first sample of the loop of the tones (offset in sound RAM) */
# define JO_AUDIO_TONE_START                (JO_AUDIO_TONE_WAVE + JO_AUDIO_TONE_GUARD * 2)
/** @brief Sound RAM area used for PCM data (the driver and the waveform of the tones are below) */
# define JO_AUDIO_PCM_AREA_START            (JO_AUDIO_TONE_WAVE + (JO_AUDIO_TONE_SAMPLES + 2 * JO_AUDIO_TONE_GUARD) * 2)
#else
/** @brief Sound RAM area used for PCM data (the 68000 program is below) */
# define JO_AUDIO_PCM_AREA_START            (0x01000)
#endif
#ifdef JO_COMPILE_WITH_AUDIO_DSP_SUPPORT
/** @brief The ring buffer of the DSP: the end of the sound RAM (8 KB aligned) */
# define JO_AUDIO_DSP_RING                  (0x80000 - JO_SCSP_DSP_REVERB_RING_WORDS * 2)
# define JO_AUDIO_PCM_AREA_END              (JO_AUDIO_DSP_RING)
#else
# define JO_AUDIO_PCM_AREA_END              (0x80000)
#endif
/** @brief The room of the sound RAM kept for the sounds copied when they play (not resident: jo_audio_make_resident()) */
# define JO_AUDIO_COPIED_ROOM               (0x8000)
/** @brief Sounds are copied in sound RAM by 32 bits words */
# define JO_AUDIO_ALIGN(SIZE)               (((SIZE) + 3) & ~3)
/** @brief Sounds kept in sound RAM (more sounds than entries: a sound played again is copied again, over the oldest one,
 *  maybe still playing: a loop (the engine of a vehicle) then played the new data, a strident noise) */
# define JO_AUDIO_CACHE_ENTRIES             (64)
/** @brief Maximum sample count of a sound (the loop end register is 16 bits) */
# define JO_AUDIO_MAX_SAMPLES               (0xFFFF)
/** @brief A loop plays until its channel is stopped (VBLANK count: about 6 months at 60 Hz) */
# define JO_AUDIO_LOOP_VBLANKS              (0x3FFFFFFF)
/** @brief CD audio volume (0 to 7) */
# define JO_AUDIO_CDDA_LEVEL                (JO_AUDIO_CD_VOLUME_MAX)
/** @brief CD audio endless repeat (ST-38-R1-121093 page 24) */
# define JO_AUDIO_CD_ENDLESS_REPEAT         (0x0f)
/** @brief CD audio: no repeat */
# define JO_AUDIO_CD_NO_REPEAT              (0x00)
/** @brief The 12 registers of a slot (0x00 to 0x16) */
# define JO_AUDIO_SLOT_REGISTERS            (12)

typedef struct
{
    const void                      *data;
    unsigned int                    size;
    unsigned int                    offset; // In sound RAM
}                                   jo_audio_cache_entry;

typedef struct
{
    unsigned int                    end_vblank; // Playback end (VBLANK count)
    unsigned int                    offset;     // Its sound in the sound RAM (an upload doesn't overwrite it while it plays)
    unsigned int                    size;
    bool                            active;
    bool                            stereo;
    bool                            tone;       // A tone (jo_audio_play_tone()): its second slot is the FM modulator
    bool                            loop;
    unsigned int                    rate;       // Sample rate played (see jo_audio_set_channel_sample_rate())
    const jo_sound                  *sound;     // The sound played (see jo_audio_stop_sound())
}                                   jo_audio_channel;

static jo_audio_cache_entry         __jo_audio_cache[JO_AUDIO_CACHE_ENTRIES];
static unsigned int                 __jo_audio_cache_next;
static unsigned int                 __jo_audio_alloc_next = JO_AUDIO_PCM_AREA_START;
/** @brief The resident sounds (jo_audio_make_resident()) fill the sound RAM from its end down to there: the sounds
 *  copied when they play stay before */
static unsigned int                 __jo_audio_resident_start = JO_AUDIO_PCM_AREA_END;
/** @brief The samples of the music streamed from the CD (0: not reserved yet, see jo_audio_play_music()) */
static unsigned int                 __jo_audio_music_sound_ram = 0;
static jo_audio_channel             __jo_audio_channels[JO_SOUND_MAX_CHANNEL];
/** @brief The direct level and pan of the slots 0 and 1 (their mixer register also sends the outputs of the DSP: the
 *  reverberation, see __jo_audio_effect_mix) */
static unsigned short               __jo_audio_direct_mix[2];
/** @brief The effect level and pan of the slots 0 and 1: the left and right outputs of the DSP (EFREG 0 and 1) */
static unsigned short               __jo_audio_effect_mix[2];
/** @brief Slots programmed together without the driver (the 2 slots of a stereo sound or of a tone) */
# define JO_AUDIO_PENDING_SLOTS             (2)
/** @brief The slots programmed and not started yet, without the driver (see jo_audio_start_slots()) */
static unsigned short               __jo_audio_pending_regs[JO_AUDIO_PENDING_SLOTS][JO_AUDIO_SLOT_REGISTERS];
static unsigned char                __jo_audio_pending_slot[JO_AUDIO_PENDING_SLOTS];
static unsigned int                 __jo_audio_pending_count;

#ifdef JO_COMPILE_WITH_AUDIO_DRIVER_SUPPORT

/** @brief The driver of the 68000 runs (otherwise the SH-2 programs the slots, as without the driver) */
static bool                         __jo_audio_driver_running;
/** @brief The next command written in the queue, not yet given to the 68000 (see jo_audio_driver_flush()) */
static unsigned int                 __jo_audio_driver_head;
/** @brief Reads of the tail before the queue is said to be stuck (the driver doesn't run any more: the SH-2 programs
 *  the slots) */
# define JO_AUDIO_DRIVER_TIMEOUT            (200000)

/** @brief The highest frequency of a tone (Hz) */
# define JO_AUDIO_TONE_MAX_FREQUENCY        (8000)

static void                         jo_audio_tone_init(void);

/** @brief A word of the sound RAM */
# define JO_AUDIO_RAM16(OFFSET)             (*(volatile unsigned short *)(JO_SOUND_RAM + (OFFSET)))

/** @brief Write a command in the queue of the driver (given to the 68000 by jo_audio_driver_flush(): the commands of a
 *  batch start together)
 *  @param words Words of the command (word 0: command << 8 | slot)
 *  @param count Word count (16 at most)
 */
static void                         jo_audio_driver_post(const unsigned short * const words, const int count)
{
    volatile unsigned short         *cmd;
    unsigned int                    next;
    int                             k;

    next = (__jo_audio_driver_head + 1) & (JO_DRIVER_QUEUE_LENGTH - 1);
    // (the queue full: the 68000 runs the commands given before; a batch longer than the queue is given in parts)
    if (next == JO_AUDIO_RAM16(JO_DRIVER_TAIL))
    {
        JO_AUDIO_RAM16(JO_DRIVER_HEAD) = (unsigned short)__jo_audio_driver_head;
        for (JO_ZERO(k); next == JO_AUDIO_RAM16(JO_DRIVER_TAIL); ++k)
            if (k > JO_AUDIO_DRIVER_TIMEOUT)
            {
                __jo_audio_driver_running = false;
                return;
            }
    }
    cmd = (volatile unsigned short *)(JO_SOUND_RAM + JO_DRIVER_QUEUE + __jo_audio_driver_head * JO_DRIVER_COMMAND_SIZE);
    for (JO_ZERO(k); k < count; ++k)
        cmd[k] = words[k];
    __jo_audio_driver_head = next;
}

/** @brief The commands written by jo_audio_driver_post() given to the 68000 */
static  __jo_force_inline void      jo_audio_driver_flush(void)
{
    if (__jo_audio_driver_running)
        JO_AUDIO_RAM16(JO_DRIVER_HEAD) = (unsigned short)__jo_audio_driver_head;
}

/** @brief Load the driver in the sound RAM and start the 68000
 *  @return true if the driver runs
 */
static bool                         jo_audio_driver_start(void)
{
    register unsigned int           i;

    __jo_smpc_command(DisableSoundCPU);
    for (JO_ZERO(i); i < JO_DRIVER_AREA_END; i += 4)
        *(volatile unsigned int *)(JO_SOUND_RAM + i) = 0;
    for (JO_ZERO(i); i < sizeof(jo_audio_driver_68k); i += 2)
        JO_AUDIO_RAM16(i) = (unsigned short)((jo_audio_driver_68k[i] << 8) | jo_audio_driver_68k[i + 1]);
    JO_ZERO(__jo_audio_driver_head);
    __jo_smpc_command(EnableSoundCPU);
    // (the driver writes its state when it runs: a few microseconds)
    for (JO_ZERO(i); i < JO_AUDIO_DRIVER_TIMEOUT; ++i)
        if (JO_AUDIO_RAM16(JO_DRIVER_STATE) == JO_DRIVER_MAGIC)
            return (true);
    return (false);
}

#endif

/** @brief Write a register of a slot (the 68000 writes it when its driver runs)
 *  @param slot Slot (0 to 31)
 *  @param reg Register offset (JO_SCSP_SLOT_xxx, not JO_SCSP_SLOT_CONTROL)
 *  @param value Value
 */
static void                         jo_audio_slot_write(const unsigned int slot, const unsigned int reg, const unsigned short value)
{
#ifdef JO_COMPILE_WITH_AUDIO_DRIVER_SUPPORT
    unsigned short                  cmd[3];

    if (__jo_audio_driver_running)
    {
        cmd[0] = (unsigned short)((JO_DRIVER_WRITE << 8) | slot);
        cmd[1] = (unsigned short)reg;
        cmd[2] = value;
        jo_audio_driver_post(cmd, 3);
        jo_audio_driver_flush();
        return;
    }
#endif
    JO_SCSP_SLOT(slot, reg) = value;
}

#ifdef JO_COMPILE_WITH_FS_SUPPORT

/** @brief State of the CD audio (for jo_audio_pause_cd() and jo_audio_resume_cd()) */
static struct
{
    bool                            paused;
    unsigned int                    fad;
    int                             from_track;
    int                             to_track;
    unsigned char                   play_mode;
}                                   __jo_audio_cd;

void			    jo_audio_play_cd_track(const int from_track, const int to_track, const bool repeat_infinitely)
{
    __jo_audio_cd.paused = false;
    __jo_audio_cd.from_track = from_track;
    __jo_audio_cd.to_track = to_track;
    __jo_audio_cd.play_mode = repeat_infinitely ? JO_AUDIO_CD_ENDLESS_REPEAT : JO_AUDIO_CD_NO_REPEAT;
    __jo_cd_play_tracks(from_track, to_track, __jo_audio_cd.play_mode);
}

void		        jo_audio_stop_cd(void)
{
    __jo_audio_cd.paused = false;
    __jo_cd_pause();
}

void		        jo_audio_pause_cd(void)
{
    if (__jo_audio_cd.paused || __jo_cd_get_status(&__jo_audio_cd.fad) != JO_CD_DRIVE_PLAY)
        return ;
    __jo_cd_pause();
    __jo_audio_cd.paused = true;
}

void		        jo_audio_resume_cd(void)
{
    if (!__jo_audio_cd.paused)
        return ;
    __jo_audio_cd.paused = false;
    // (a repeat starts again at the pause position, not at the start of the tracks)
    __jo_cd_play_from_fad(__jo_audio_cd.fad, __jo_audio_cd.from_track, __jo_audio_cd.to_track, __jo_audio_cd.play_mode);
}

bool		        jo_audio_is_cd_paused(void)
{
    return (__jo_audio_cd.paused);
}

bool		        jo_audio_is_cd_playing(void)
{
    return (!__jo_audio_cd.paused && __jo_cd_get_status(JO_NULL) == JO_CD_DRIVE_PLAY);
}

#endif

void		        jo_audio_set_cd_volume(const unsigned char level)
{
    unsigned short                  effect_level;

    // The level of the EXTS inputs of the slots that take the CD audio (EFSDL: 0 off, 7 full, -6 dB per step)
    effect_level = JO_SCSP_EFFECT_LEVEL(level > JO_AUDIO_CD_VOLUME_MAX ? JO_AUDIO_CD_VOLUME_MAX : level);
    jo_audio_slot_write(JO_SCSP_CDDA_LEFT_SLOT, JO_SCSP_SLOT_MIXER, effect_level | JO_SCSP_EFFECT_PAN(JO_SCSP_PAN_LEFT));
    jo_audio_slot_write(JO_SCSP_CDDA_RIGHT_SLOT, JO_SCSP_SLOT_MIXER, effect_level | JO_SCSP_EFFECT_PAN(JO_SCSP_PAN_RIGHT));
}

/** @brief The register 0x400 of the SCSP: the sound RAM of 4 Mbits and the master volume
 *  @param volume Master volume (0 to 15)
 *  @remarks Written by bytes: Yabause 0.9.15 takes the memory size of a write of the high byte only (with a write of the
 *  whole register, the sounds above 256 KB stayed silent); not read back (it reads 0 there)
 */
static void                         jo_audio_set_memory_and_volume(const unsigned int volume)
{
    *(volatile unsigned char *)&JO_SCSP_COMMON(JO_SCSP_COMMON_MASTER_VOLUME) = (unsigned char)(JO_SCSP_MEMORY_4MBITS >> 8);
    *((volatile unsigned char *)&JO_SCSP_COMMON(JO_SCSP_COMMON_MASTER_VOLUME) + 1) = (unsigned char)(volume & JO_SCSP_MASTER_VOLUME_MASK);
}

void		        jo_audio_set_volume(const unsigned char vol)
{
    // 0-127 volume to 0-15 master volume
    jo_audio_set_memory_and_volume(vol >> 3);
}

#ifdef JO_COMPILE_WITH_AUDIO_DSP_SUPPORT

/** @brief Write a coefficient of the DSP
 *  @param index Coefficient (JO_SCSP_DSP_REVERB_COEF_xxx)
 *  @param value 13 bits signed (4096 = 1)
 */
static  __jo_force_inline void      jo_audio_dsp_coef(const unsigned int index, const int value)
{
    *(volatile unsigned short *)(JO_SCSP_DSP_AREA + JO_SCSP_DSP_COEF + index * 2) = (unsigned short)((value & 0x1FFF) << 3);
}

/** @brief Load the reverberation in the DSP (silent: its levels at 0, see jo_audio_set_reverb()) */
static void                         jo_audio_dsp_init(void)
{
    register unsigned int           i;

    // The ring buffer filled with 0 in the float format of the DSP (0x0000 is +0.5)
    for (JO_ZERO(i); i < JO_SCSP_DSP_REVERB_RING_WORDS * 2; i += 2)
        *(volatile unsigned short *)(JO_SOUND_RAM + JO_AUDIO_DSP_RING + i) = JO_SCSP_DSP_FLOAT_ZERO;
    JO_SCSP_COMMON(JO_SCSP_COMMON_RING_BUFFER) = JO_SCSP_RING_BUFFER(JO_SCSP_DSP_REVERB_RING_WORDS, JO_AUDIO_DSP_RING);
    for (JO_ZERO(i); i < sizeof(jo_scsp_dsp_reverb_addresses) / sizeof(jo_scsp_dsp_reverb_addresses[0]); ++i)
        *(volatile unsigned short *)(JO_SCSP_DSP_AREA + JO_SCSP_DSP_ADDRESS + i * 2) = jo_scsp_dsp_reverb_addresses[i];
    for (JO_ZERO(i); i < JO_SCSP_DSP_REVERB_COEF_COUNT; ++i)
        jo_audio_dsp_coef(i, jo_scsp_dsp_reverb_coefs[i]);
    for (JO_ZERO(i); i < JO_SCSP_DSP_REVERB_STEPS * 4; ++i)
        *(volatile unsigned short *)(JO_SCSP_DSP_AREA + JO_SCSP_DSP_PROGRAM + i * 2) = jo_scsp_dsp_reverb_program[i];
}

#endif

void		                    jo_audio_init(void)
{
    register unsigned int       i;
#ifndef JO_COMPILE_WITH_AUDIO_DRIVER_SUPPORT
    // 68000 program: reset vectors then an infinite loop
    static const unsigned char  idle_program[] =
    {
        0x00, 0x00, 0x10, 0x00,     // Initial stack pointer (0x1000)
        0x00, 0x00, 0x00, 0x08,     // Initial program counter (0x0008)
        0x60, 0xFE                  // 0x0008: bra.s * (branch to itself)
    };

    // Load the idle loop (the sound CPU must be stopped only while loading)
    __jo_smpc_command(DisableSoundCPU);
    for (JO_ZERO(i); i < JO_AUDIO_PCM_AREA_START; i += 4)
        *(volatile unsigned int *)(JO_SOUND_RAM + i) = 0;
    jo_memcpy((void *)JO_SOUND_RAM, idle_program, sizeof(idle_program));
    __jo_smpc_command(EnableSoundCPU);
#else
    // (the driver is loaded at the end: the SH-2 programs the SCSP alone until then)
    __jo_audio_driver_running = false;
#endif
    // 4 Mbits sound RAM (the bit was missing: the SCSP of Yabause and Kronos addressed 256 KB, the sounds above were
    // silent), 16 bits DAC, master volume max
    jo_audio_set_memory_and_volume(JO_SCSP_MASTER_VOLUME_MAX);
    // The DSP of the SCSP stopped: the BIOS leaves the program of the echo of its startup sound running, its ring buffer
    // (16 to 128 KB of the sound RAM) written all the time: it wrote over the sounds there (noise, glitches). Its program
    // cleared (no step writes the ring buffer), its coefficients and addresses cleared, the ring buffer at 0, 16 KB
    for (JO_ZERO(i); i < JO_SCSP_DSP_AREA_SIZE; i += 2)
        *(volatile unsigned short *)(JO_SCSP_DSP_AREA + i) = 0;
    JO_SCSP_COMMON(JO_SCSP_COMMON_RING_BUFFER) = 0;
    // Key off all slots
    for (JO_ZERO(i); i < JO_SCSP_SLOT_COUNT; ++i)
    {
        JO_SCSP_SLOT(i, JO_SCSP_SLOT_CONTROL) = 0;
        JO_SCSP_SLOT(i, JO_SCSP_SLOT_ENVELOPE_RELEASE) = JO_SCSP_RELEASE_RATE_MAX;
        JO_SCSP_SLOT(i, JO_SCSP_SLOT_MIXER) = 0;
        JO_SCSP_SLOT(i, JO_SCSP_SLOT_DSP_INPUT) = 0;
    }
    JO_SCSP_SLOT(0, JO_SCSP_SLOT_CONTROL) = JO_SCSP_KEY_ON_EXECUTE;
    for (JO_ZERO(i); i < JO_AUDIO_CACHE_ENTRIES; ++i)
        __jo_audio_cache[i].data = JO_NULL;
    for (JO_ZERO(i); i < JO_SOUND_MAX_CHANNEL; ++i)
        __jo_audio_channels[i].active = false;
    JO_ZERO(__jo_audio_cache_next);
    JO_ZERO(__jo_audio_pending_count);
    __jo_audio_alloc_next = JO_AUDIO_PCM_AREA_START;
    __jo_audio_resident_start = JO_AUDIO_PCM_AREA_END;
    JO_ZERO(__jo_audio_music_sound_ram);
    JO_ZERO(__jo_audio_direct_mix[0]);
    JO_ZERO(__jo_audio_direct_mix[1]);
    JO_ZERO(__jo_audio_effect_mix[0]);
    JO_ZERO(__jo_audio_effect_mix[1]);
    // CD audio enters the mixer through the EXTS inputs of the slots 16 (left) and 17 (right)
    JO_SCSP_SLOT(JO_SCSP_CDDA_LEFT_SLOT, JO_SCSP_SLOT_MIXER) = JO_SCSP_EFFECT_LEVEL(JO_AUDIO_CDDA_LEVEL) | JO_SCSP_EFFECT_PAN(JO_SCSP_PAN_LEFT);
    JO_SCSP_SLOT(JO_SCSP_CDDA_RIGHT_SLOT, JO_SCSP_SLOT_MIXER) = JO_SCSP_EFFECT_LEVEL(JO_AUDIO_CDDA_LEVEL) | JO_SCSP_EFFECT_PAN(JO_SCSP_PAN_RIGHT);
#ifdef JO_COMPILE_WITH_AUDIO_DSP_SUPPORT
    jo_audio_dsp_init();
#endif
#ifdef JO_COMPILE_WITH_AUDIO_DRIVER_SUPPORT
    __jo_audio_driver_running = jo_audio_driver_start();
    jo_audio_tone_init();
#endif
}

/** @brief Copy a sound in sound RAM (if it is not already there)
 *  @param data Sound data
 *  @param size Sound size in bytes
 *  @return Offset of the sound in sound RAM
 *  @remarks The last 16 sounds are kept in a cache. The oldest sounds are overwritten when the sound RAM is full
 */
/** @brief Forget a sound in the sound RAM cache
 *  @param data Sound data
 *  @remarks Must be called when the sound data is freed because the memory block can be reused by another sound
 */
static void                         jo_audio_forget_pcm(const void *data)
{
    register unsigned int           i;

    for (JO_ZERO(i); i < JO_AUDIO_CACHE_ENTRIES; ++i)
        if (__jo_audio_cache[i].data == data)
            __jo_audio_cache[i].data = JO_NULL;
}

static bool                         jo_audio_channel_status(const unsigned int channel);

/** @brief The place of a new sound in the sound RAM: after the last one, not over a sound that plays (it would play
 *  the new samples: a shrill noise)
 *  @return Offset (the next place if all the places overlap a sound that plays)
 */
static unsigned int                 jo_audio_place_pcm(const unsigned int size)
{
    unsigned int                    offset;
    unsigned int                    c;
    int                             tries;
    bool                            moved;

    offset = __jo_audio_alloc_next;
    for (tries = 0; tries < 2 * JO_SOUND_MAX_CHANNEL + 2; ++tries)
    {
        if (offset + size > __jo_audio_resident_start)
            offset = JO_AUDIO_PCM_AREA_START;
        moved = false;
        for (JO_ZERO(c); c < JO_SOUND_MAX_CHANNEL; ++c)
            if (jo_audio_channel_status(c) && __jo_audio_channels[c].offset < offset + size &&
                __jo_audio_channels[c].offset + __jo_audio_channels[c].size > offset)
            {
                offset = __jo_audio_channels[c].offset + __jo_audio_channels[c].size;
                moved = true;
                break;
            }
        if (!moved && offset + size <= JO_AUDIO_PCM_AREA_END)
            return (offset);
    }
    // (before the resident sounds and the music, like the places tried above: the copy overwrote them)
    return (__jo_audio_alloc_next + size > __jo_audio_resident_start ? JO_AUDIO_PCM_AREA_START : __jo_audio_alloc_next);
}

static unsigned int                 jo_audio_upload_pcm(const void *data, unsigned int size)
{
    register unsigned int           i;
    unsigned int                    offset;
    unsigned int                    end;

    // The cache stores the aligned size
    size = JO_AUDIO_ALIGN(size);
    if (size > __jo_audio_resident_start - JO_AUDIO_PCM_AREA_START)
        size = __jo_audio_resident_start - JO_AUDIO_PCM_AREA_START;
    for (JO_ZERO(i); i < JO_AUDIO_CACHE_ENTRIES; ++i)
        if (__jo_audio_cache[i].data == data && __jo_audio_cache[i].size == size)
            return (__jo_audio_cache[i].offset);
    offset = jo_audio_place_pcm(size);
    end = offset + size;
    __jo_audio_alloc_next = end;
    // Forget the cached sounds we overwrite
    for (JO_ZERO(i); i < JO_AUDIO_CACHE_ENTRIES; ++i)
        if (__jo_audio_cache[i].data != JO_NULL && __jo_audio_cache[i].offset < end && __jo_audio_cache[i].offset + __jo_audio_cache[i].size > offset)
            __jo_audio_cache[i].data = JO_NULL;
    jo_dma_copy(data, (void *)(JO_SOUND_RAM + offset), size);
    __jo_audio_cache[__jo_audio_cache_next].data = data;
    __jo_audio_cache[__jo_audio_cache_next].size = size;
    __jo_audio_cache[__jo_audio_cache_next].offset = offset;
    __jo_audio_cache_next = (__jo_audio_cache_next + 1) % JO_AUDIO_CACHE_ENTRIES;
    return (offset);
}

/** @brief Most reads of the SCSP between a key off and a key on (about 4 samples in Mednafen: the wait before, when an
 *  emulator doesn't set the bit of each sample, see jo_audio_wait_samples()) */
#define JO_AUDIO_KEY_OFF_WAIT              (96)

/** @brief Wait for the start of 2 samples of the SCSP (between 1 and 2 samples: 23 to 45 microseconds)
 *  @remarks The bit of each sample in MCIPD (not an interrupt: the main CPU doesn't enable it). The 96 reads of the SCSP
 *  done before (about 4 samples, 200 microseconds) at most: an emulator that doesn't set the bit
 */
static void                         jo_audio_wait_samples(void)
{
    unsigned int                    k;
    unsigned int                    n;

    JO_ZERO(k);
    for (JO_ZERO(n); n < 2; ++n)
    {
        JO_SCSP_COMMON(JO_SCSP_COMMON_MAIN_INTERRUPT_RESET) = JO_SCSP_INTERRUPT_SAMPLE;
        while (!(JO_SCSP_COMMON(JO_SCSP_COMMON_MAIN_INTERRUPT_PENDING) & JO_SCSP_INTERRUPT_SAMPLE) && ++k < JO_AUDIO_KEY_OFF_WAIT)
            ;
    }
}

/** @brief Start the slots programmed by jo_audio_program_slot() (without the driver)
 *  @remarks Key off first: a slot that played a sound to its end is still keyed on, and a key on of a keyed on slot is
 *  ignored (the second sound played on the same channel would be silent). The SCSP runs the keys at its next sample: the
 *  key on written at once would replace the key off (the slot stays keyed on, the sound is not started again: shots were
 *  missing). The slots keyed off together, one wait, then keyed on by the same KYONEX: the 2 slots of a stereo sound start
 *  at the same sample (the right one started 10 samples after the left one: recorded in Mednafen)
 */
static void                         jo_audio_start_slots(void)
{
    unsigned int                    i;
    unsigned int                    k;
    unsigned int                    slot;
    unsigned int                    last;

    if (__jo_audio_pending_count == 0)
        return;
    last = __jo_audio_pending_count - 1;
    for (JO_ZERO(i); i <= last; ++i)
    {
        slot = __jo_audio_pending_slot[i];
        JO_SCSP_SLOT(slot, JO_SCSP_SLOT_CONTROL) = (JO_SCSP_SLOT(slot, JO_SCSP_SLOT_CONTROL) & JO_SCSP_KEY_MASK) |
                                                   (i == last ? JO_SCSP_KEY_ON_EXECUTE : 0);
    }
    jo_audio_wait_samples();
    for (JO_ZERO(i); i <= last; ++i)
    {
        slot = __jo_audio_pending_slot[i];
        JO_SCSP_SLOT(slot, JO_SCSP_SLOT_CONTROL) = __jo_audio_pending_regs[i][0] & JO_SCSP_KEY_MASK;
        for (k = 1; k < JO_AUDIO_SLOT_REGISTERS; ++k)
            JO_SCSP_SLOT(slot, k * 2) = __jo_audio_pending_regs[i][k];
        JO_SCSP_SLOT(slot, JO_SCSP_SLOT_CONTROL) = (__jo_audio_pending_regs[i][0] & JO_SCSP_KEY_MASK) | JO_SCSP_KEY_ON |
                                                   (i == last ? JO_SCSP_KEY_ON_EXECUTE : 0);
    }
    JO_ZERO(__jo_audio_pending_count);
}

/** @brief Start the slots programmed (jo_audio_program_slot()) and the commands written for the driver of the 68000 */
static  __jo_force_inline void      jo_audio_flush(void)
{
#ifdef JO_COMPILE_WITH_AUDIO_DRIVER_SUPPORT
    if (__jo_audio_driver_running)
    {
        jo_audio_driver_flush();
        return;
    }
#endif
    jo_audio_start_slots();
}

/** @brief Program a slot to key it on
 *  @param slot Slot (0 to 31)
 *  @param regs The 12 registers of the slot (0x00 to 0x16; the key bits of the register 0x00 are ignored)
 *  @remarks Started by jo_audio_flush() (with the driver of the 68000: written in its queue)
 */
static void                         jo_audio_program_slot(const unsigned int slot, const unsigned short * const regs)
{
    int                             k;

#ifdef JO_COMPILE_WITH_AUDIO_DRIVER_SUPPORT
    unsigned short                  cmd[1 + JO_AUDIO_SLOT_REGISTERS];

    if (__jo_audio_driver_running)
    {
        cmd[0] = (unsigned short)((JO_DRIVER_KEY_ON << 8) | slot);
        for (JO_ZERO(k); k < JO_AUDIO_SLOT_REGISTERS; ++k)
            cmd[k + 1] = regs[k];
        jo_audio_driver_post(cmd, 1 + JO_AUDIO_SLOT_REGISTERS);
        return;
    }
#endif
    if (__jo_audio_pending_count == JO_AUDIO_PENDING_SLOTS)
        jo_audio_start_slots();
    __jo_audio_pending_slot[__jo_audio_pending_count] = (unsigned char)slot;
    for (JO_ZERO(k); k < JO_AUDIO_SLOT_REGISTERS; ++k)
        __jo_audio_pending_regs[__jo_audio_pending_count][k] = regs[k];
    ++__jo_audio_pending_count;
}

/** @brief The mixer register of a slot: the direct level and pan, and for the slots 0 and 1 the outputs of the DSP
 *  @param slot Slot (0 to 31)
 *  @param direct Direct level and pan (JO_SCSP_DIRECT_LEVEL() | JO_SCSP_DIRECT_PAN())
 */
static unsigned short               jo_audio_mixer(const unsigned int slot, const unsigned short direct)
{
    if (slot >= 2)
        return (direct);
    __jo_audio_direct_mix[slot] = direct;
    return (direct | __jo_audio_effect_mix[slot]);
}

/** @brief Start a sound on a SCSP slot
 *  @param slot Slot (0 to 31)
 *  @param offset Sound offset in sound RAM
 *  @param samples Sample count
 *  @param bits8 true for 8 bits samples
 *  @param pitch Pitch register value (JO_SCSP_PITCH())
 *  @param volume Volume (0 to 127)
 *  @param pan Pan register value (JO_SCSP_PAN_xxx)
 *  @param loop The sound plays again and again
 *  @param reverb Its send to the reverberation of the DSP (0: none, 1 to 7: -36 to 0 dB)
 */
static void                         jo_audio_key_on(const unsigned int slot, const unsigned int offset, const unsigned int samples, const bool bits8,
                                                    const unsigned short pitch, const unsigned int volume, const unsigned int pan,
                                                    const bool loop, const unsigned int reverb)
{
    unsigned short                  regs[JO_AUDIO_SLOT_REGISTERS];

    regs[0] = (unsigned short)((bits8 ? JO_SCSP_PCM_8BITS : 0) | (loop ? JO_SCSP_LOOP_NORMAL : 0) | JO_SCSP_START_ADDRESS_HIGH(offset));
    regs[1] = (unsigned short)JO_SCSP_START_ADDRESS_LOW(offset);
    regs[2] = 0;
    // (the SCSP plays the samples before the loop end: a loop ends after its last sample (a looped sound skipped its last
    // sample at each turn: a click; the music a sample at each turn of its ring: a click every 0.37 s); a sound played once
    // stops before its last sample: the interpolation of the SCSP doesn't read the data after it)
    regs[3] = (unsigned short)(loop ? samples : samples - 1);
    // No attack, no decay
    regs[4] = JO_SCSP_ATTACK_RATE_MAX;
    regs[5] = JO_SCSP_RELEASE_RATE_MAX;
    regs[6] = (unsigned short)JO_SCSP_TOTAL_LEVEL(volume);
    regs[7] = 0;
    regs[8] = pitch;
    regs[9] = 0;
    regs[10] = (unsigned short)JO_SCSP_DSP_SEND(0, reverb);
    regs[11] = jo_audio_mixer(slot, (unsigned short)(JO_SCSP_DIRECT_LEVEL(JO_SCSP_LEVEL_MAX) | JO_SCSP_DIRECT_PAN(pan)));
    jo_audio_program_slot(slot, regs);
}

/** @brief Stop a SCSP slot
 *  @param slot Slot (0 to 31)
 */
static void                         jo_audio_key_off(const unsigned int slot)
{
#ifdef JO_COMPILE_WITH_AUDIO_DRIVER_SUPPORT
    unsigned short                  cmd;

    if (__jo_audio_driver_running)
    {
        cmd = (unsigned short)((JO_DRIVER_KEY_OFF << 8) | slot);
        jo_audio_driver_post(&cmd, 1);
        return;
    }
#endif
    JO_SCSP_SLOT(slot, JO_SCSP_SLOT_CONTROL) = (JO_SCSP_SLOT(slot, JO_SCSP_SLOT_CONTROL) & JO_SCSP_KEY_MASK) | JO_SCSP_KEY_ON_EXECUTE;
}

/** @brief Convert a Jo Engine pan to a SCSP pan
 *  @param pan -128 (left) to 127 (right)
 *  @return SCSP pan (JO_SCSP_PAN_xxx)
 */
static unsigned int                 jo_audio_pan(const char pan)
{
    int                             p;

    p = pan;
    if (p == 0)
        return (JO_SCSP_PAN_CENTER);
    // -128 to 127 => attenuation of the other side (0 to 15)
    if (p < 0)
        return (JO_SCSP_PAN_LEFT_SIDE | ((-p) >> 3));
    return (p >> 3);
}

/** @brief The first SCSP slot of a channel (2 slots per channel: left and right of a stereo sound). The slots 16 and
 *  17 take the CD audio (EXTS): the channels 8 and more use the slots from 20
 *  @param channel Channel (0 to JO_SOUND_MAX_CHANNEL - 1)
 */
static  __jo_force_inline unsigned int jo_audio_channel_slot(const unsigned int channel)
{
    return (channel < 8 ? JO_MULT_BY_2(channel) : JO_MULT_BY_2(channel) + 4);
}

/** @brief Check if a channel is playing
 *  @param channel Channel (0 to JO_SOUND_MAX_CHANNEL - 1)
 *  @return true if the sound is not finished
 *  @remarks The end of a sound is computed from its length (the SCSP is not read)
 */
static bool                         jo_audio_channel_status(const unsigned int channel)
{
    jo_audio_channel                *c;

    c = &__jo_audio_channels[channel];
    if (!c->active)
        return (false);
    if ((int)(__jo_core_vblank_count - c->end_vblank) >= 0)
    {
        c->active = false;
        return (false);
    }
    return (true);
}

/** @brief Play a sound on a channel
 *  @param channel Channel (0 to JO_SOUND_MAX_CHANNEL - 1)
 *  @param sound Sound
 *  @remarks A channel uses 2 slots (left and right for a stereo sound)
 */
static void                         jo_audio_channel_on(const unsigned int channel, const jo_sound * const sound, const bool loop)
{
    unsigned int                    slot;
    unsigned int                    offset;
    unsigned int                    samples;
    unsigned int                    rate;
    unsigned int                    bytes_per_channel;
    unsigned int                    length;
    unsigned short                  pitch;
    bool                            bits8;
    bool                            stereo;

    if (sound->data_length == 0 || (sound->data == JO_NULL && sound->resident == 0))
        return;
    slot = jo_audio_channel_slot(channel);
    bits8 = (sound->mode & JoSoundMono8Bit) == JoSoundMono8Bit;
    stereo = (sound->mode & JoSoundStereo16Bit) == JoSoundStereo16Bit;
    rate = sound->sample_rate ? sound->sample_rate : JO_SCSP_BASE_SAMPLING_RATE;
    pitch = __jo_scsp_pitch(rate);
    offset = sound->resident != 0 ? sound->resident - 1 : jo_audio_upload_pcm(sound->data, sound->data_length);
    // (the bytes copied: less than the sound when the resident sounds take the sound RAM, the rest played the next data)
    length = sound->data_length;
    if (sound->resident == 0 && length > __jo_audio_resident_start - JO_AUDIO_PCM_AREA_START)
    {
        length = __jo_audio_resident_start - JO_AUDIO_PCM_AREA_START;
        // (a stereo sound cut: its right channel was not copied, the left one only)
        stereo = false;
    }
    // Stereo data: left channel samples, then right channel samples
    bytes_per_channel = stereo ? JO_DIV_BY_2(length) : length;
    samples = bits8 ? bytes_per_channel : JO_DIV_BY_2(bytes_per_channel);
    if (samples > JO_AUDIO_MAX_SAMPLES)
        samples = JO_AUDIO_MAX_SAMPLES;
    if (stereo)
    {
        jo_audio_key_on(slot, offset, samples, bits8, pitch, sound->volume, JO_SCSP_PAN_LEFT, loop, sound->reverb & 7);
        jo_audio_key_on(slot + 1, offset + bytes_per_channel, samples, bits8, pitch, sound->volume, JO_SCSP_PAN_RIGHT, loop,
                        sound->reverb & 7);
    }
    else
        jo_audio_key_on(slot, offset, samples, bits8, pitch, sound->volume, jo_audio_pan(sound->pan), loop, sound->reverb & 7);
    // (the 2 slots of a stereo sound start together)
    jo_audio_flush();
    __jo_audio_channels[channel].active = true;
    __jo_audio_channels[channel].stereo = stereo;
    __jo_audio_channels[channel].tone = false;
    __jo_audio_channels[channel].sound = sound;
    __jo_audio_channels[channel].offset = offset;
    __jo_audio_channels[channel].size = JO_AUDIO_ALIGN(sound->data_length);
    __jo_audio_channels[channel].loop = loop;
    __jo_audio_channels[channel].rate = rate;
    // A loop plays until the channel is stopped
    __jo_audio_channels[channel].end_vblank = __jo_core_vblank_count +
                                              (loop ? JO_AUDIO_LOOP_VBLANKS : ((samples * jo_core_get_refresh_rate()) / rate) + 1);
}

/** @brief Stop a channel
 *  @param channel Channel (0 to JO_SOUND_MAX_CHANNEL - 1)
 */
static void                         jo_audio_channel_off(const unsigned int channel)
{
    jo_audio_key_off(jo_audio_channel_slot(channel));
    if (__jo_audio_channels[channel].stereo || __jo_audio_channels[channel].tone)
        jo_audio_key_off(jo_audio_channel_slot(channel) + 1);
    jo_audio_flush();
    __jo_audio_channels[channel].active = false;
}

/** @brief Stop the channels that play a sound copied in an area of the sound RAM taken now by a resident sound or by the
 *  music (the copy is overwritten: a loop, the engine of a vehicle, played the new data, a strident noise)
 *  @param start Start of the area (offset in sound RAM)
 *  @param end End of the area (excluded)
 */
static void                         jo_audio_stop_channels_in(const unsigned int start, const unsigned int end)
{
    register unsigned int           c;

    for (JO_ZERO(c); c < JO_SOUND_MAX_CHANNEL; ++c)
        if (jo_audio_channel_status(c) && __jo_audio_channels[c].offset < end &&
            __jo_audio_channels[c].offset + __jo_audio_channels[c].size > start)
            jo_audio_channel_off(c);
}

bool                jo_audio_is_channel_playing(const unsigned char channel)
{
#ifdef JO_DEBUG
    if (channel >= JO_SOUND_MAX_CHANNEL)
    {
        jo_core_error("channel (%d) is too high (max=%d)", channel, JO_SOUND_MAX_CHANNEL - 1);
        return false;
    }
#endif

    // Also checked in the release builds (without JO_DEBUG)
    if (channel >= JO_SOUND_MAX_CHANNEL)
    {
        return false;
    }

    return (jo_audio_channel_status(channel));
}

void	            jo_audio_play_sound_on_channel(jo_sound * const sound, const unsigned char channel)
{
#ifdef JO_DEBUG
    if (sound == JO_NULL)
    {
        jo_core_error("sound is null");
        return;
    }
    if (channel >= JO_SOUND_MAX_CHANNEL)
    {
        jo_core_error("channel (%d) is too high (max=%d)", channel, JO_SOUND_MAX_CHANNEL);
        return;
    }
    if ((sound->data == JO_NULL && sound->resident == 0) || sound->data_length == 0)
    {
        jo_core_error("sound is empty");
        return;
    }
#endif
    // (checked in all the builds: the slot of a channel after the last one is the common registers of the SCSP)
    if (channel >= JO_SOUND_MAX_CHANNEL || jo_audio_channel_status(channel))
        return ;
    sound->current_playing_channel = channel;
    jo_audio_channel_on(channel, sound, false);
}

void	            jo_audio_play_sound_loop_on_channel(jo_sound * const sound, const unsigned char channel)
{
#ifdef JO_DEBUG
    if (sound == JO_NULL || channel >= JO_SOUND_MAX_CHANNEL || (sound->data == JO_NULL && sound->resident == 0) || sound->data_length == 0)
    {
        jo_core_error("Invalid sound or channel (%d)", channel);
        return;
    }
#endif
    if (channel >= JO_SOUND_MAX_CHANNEL || jo_audio_channel_status(channel))
        return ;
    sound->current_playing_channel = channel;
    jo_audio_channel_on(channel, sound, true);
}

void	            jo_audio_set_channel_sample_rate(const unsigned char channel, const unsigned short sample_rate)
{
    unsigned short  pitch;

    if (channel >= JO_SOUND_MAX_CHANNEL || !jo_audio_channel_status(channel))
        return;
    jo_audio_channel                *c;
    int                             left;

    pitch = __jo_scsp_pitch(sample_rate);
    jo_audio_slot_write(jo_audio_channel_slot(channel), JO_SCSP_SLOT_PITCH, pitch);
    c = &__jo_audio_channels[channel];
    if (c->stereo)
        jo_audio_slot_write(jo_audio_channel_slot(channel) + 1, JO_SCSP_SLOT_PITCH, pitch);
    // (the end of a sound played once: at another rate, the channel was free while its sound played, a new sound
    // overwrote it, or busy after it)
    if (c->tone || c->loop || c->rate == 0 || sample_rate == 0)
        return;
    left = (int)(c->end_vblank - __jo_core_vblank_count);
    if (left > 0)
        c->end_vblank = __jo_core_vblank_count + (unsigned int)left * c->rate / sample_rate + 1;
    c->rate = sample_rate;
}

void	            jo_audio_set_channel_volume(const unsigned char channel, const unsigned char volume)
{
    if (channel >= JO_SOUND_MAX_CHANNEL || !jo_audio_channel_status(channel))
        return;
    jo_audio_slot_write(jo_audio_channel_slot(channel), JO_SCSP_SLOT_TOTAL_LEVEL, (unsigned short)JO_SCSP_TOTAL_LEVEL(volume));
    if (__jo_audio_channels[channel].stereo)
        jo_audio_slot_write(jo_audio_channel_slot(channel) + 1, JO_SCSP_SLOT_TOTAL_LEVEL, (unsigned short)JO_SCSP_TOTAL_LEVEL(volume));
}

void	            jo_audio_set_channel_reverb(const unsigned char channel, const unsigned char level)
{
    if (channel >= JO_SOUND_MAX_CHANNEL || !jo_audio_channel_status(channel))
        return;
    jo_audio_slot_write(jo_audio_channel_slot(channel), JO_SCSP_SLOT_DSP_INPUT, (unsigned short)JO_SCSP_DSP_SEND(0, level & 7));
    if (__jo_audio_channels[channel].stereo)
        jo_audio_slot_write(jo_audio_channel_slot(channel) + 1, JO_SCSP_SLOT_DSP_INPUT, (unsigned short)JO_SCSP_DSP_SEND(0, level & 7));
}

#ifdef JO_COMPILE_WITH_AUDIO_DSP_SUPPORT

/** @brief 2^(-x / 1024) (1 = 4096)
 *  @param x 0 to 32767
 */
static int                          jo_audio_exp2_neg(const unsigned int x)
{
    // 2^(-k / 16), k = 0 to 16 (32768 = 1)
    static const unsigned short     table[17] =
    {
        32768, 31379, 30048, 28774, 27554, 26386, 25268, 24196, 23170, 22188, 21247, 20347, 19484, 18658, 17867, 17110, 16384
    };
    unsigned int                    k;
    unsigned int                    f;
    int                             v;

    k = (x & 1023) >> 6;
    f = x & 63;
    v = table[k] - (int)(((table[k] - table[k + 1]) * f) >> 6);
    return ((v >> (x >> 10)) >> 3);
}

void                                jo_audio_set_reverb(const jo_audio_reverb * const reverb)
{
    unsigned int                    decay;
    int                             feedback;
    int                             damp;
    int                             early;

    if (reverb == JO_NULL)
    {
        JO_ZERO(__jo_audio_effect_mix[0]);
        JO_ZERO(__jo_audio_effect_mix[1]);
    }
    else
    {
        // The feedback of the combs: their output is 60 dB lower after the decay time (their mean length: 1234 samples
        // at 44.1 kHz): 10^(-3 * 1234 / (44100 * decay)) = 2^(-278.9 / decay (ms))
        decay = JO_MAX(50, JO_MIN(10000, reverb->decay_ms));
        feedback = jo_audio_exp2_neg(285614 / decay);
        // (the low pass filter of the combs: up to 0.6)
        damp = (reverb->damping & 127) * 2458 / 127;
        early = reverb->early & 127;
        jo_audio_dsp_coef(JO_SCSP_DSP_REVERB_COEF_DAMP, damp);
        jo_audio_dsp_coef(JO_SCSP_DSP_REVERB_COEF_DAMP1, 4096 - damp);
        jo_audio_dsp_coef(JO_SCSP_DSP_REVERB_COEF_FBD1, (feedback * (4096 - damp)) >> 12);
        jo_audio_dsp_coef(JO_SCSP_DSP_REVERB_COEF_FBD, (feedback * damp) >> 12);
        jo_audio_dsp_coef(JO_SCSP_DSP_REVERB_COEF_ER1, early * 1843 / 127);
        jo_audio_dsp_coef(JO_SCSP_DSP_REVERB_COEF_ER2, early * 1638 / 127);
        jo_audio_dsp_coef(JO_SCSP_DSP_REVERB_COEF_ER3, early * 1229 / 127);
        jo_audio_dsp_coef(JO_SCSP_DSP_REVERB_COEF_ER4, early * 1147 / 127);
        jo_audio_dsp_coef(JO_SCSP_DSP_REVERB_COEF_WET, (reverb->tail & 127) * 4095 / 127);
        // The outputs of the DSP: left (EFREG 0) through the slot 0, right (EFREG 1) through the slot 1
        __jo_audio_effect_mix[0] = JO_SCSP_EFFECT_LEVEL(JO_SCSP_LEVEL_MAX) | JO_SCSP_EFFECT_PAN(JO_SCSP_PAN_LEFT);
        __jo_audio_effect_mix[1] = JO_SCSP_EFFECT_LEVEL(JO_SCSP_LEVEL_MAX) | JO_SCSP_EFFECT_PAN(JO_SCSP_PAN_RIGHT);
    }
    jo_audio_slot_write(0, JO_SCSP_SLOT_MIXER, __jo_audio_direct_mix[0] | __jo_audio_effect_mix[0]);
    jo_audio_slot_write(1, JO_SCSP_SLOT_MIXER, __jo_audio_direct_mix[1] | __jo_audio_effect_mix[1]);
}

#endif

#ifdef JO_COMPILE_WITH_AUDIO_DRIVER_SUPPORT

/** @brief The waveform of the tones: a sine (16 bits) */
static void                         jo_audio_tone_init(void)
{
    // (a sine by its recurrence: s(n + 1) = 2 cos(w) s(n) - s(n - 1), Q30; 2 cos(2 pi / 1024) = 1.99996235)
    const long long                 c = 2147443143LL;
    long long                       s0;
    long long                       s1;
    long long                       s2;
    int                             n;
    int                             v;
    volatile short                  *wave;

    wave = (volatile short *)(JO_SOUND_RAM + JO_AUDIO_TONE_WAVE);
    s0 = 0;
    // sin(2 pi / 1024) in Q30
    s1 = 6588356LL;
    for (n = 0; n < JO_AUDIO_TONE_SAMPLES; ++n)
    {
        // (32000: a little lower than the maximum, the interpolation of the SCSP doesn't overflow)
        v = (int)((s0 * 32000) >> 30);
        wave[JO_AUDIO_TONE_GUARD + n] = (short)v;
        if (n < JO_AUDIO_TONE_GUARD)
            wave[JO_AUDIO_TONE_GUARD + JO_AUDIO_TONE_SAMPLES + n] = (short)v;
        if (n >= JO_AUDIO_TONE_SAMPLES - JO_AUDIO_TONE_GUARD)
            wave[n - (JO_AUDIO_TONE_SAMPLES - JO_AUDIO_TONE_GUARD)] = (short)v;
        s2 = ((c * s1) >> 30) - s0;
        s0 = s1;
        s1 = s2;
    }
}

/** @brief The pitch register of a tone (the waveform: 1024 samples per period)
 *  @param frequency Hz (the sine played 1024 x frequency samples per second)
 */
static unsigned short               jo_audio_tone_pitch(unsigned int frequency)
{
    int                             oct;
    unsigned int                    base;

    // (44100 x 2^oct x (1 + fns / 1024) = 1024 x frequency: base = 44100 / 1024 x 2^oct; the frequency in 1/256 Hz)
    frequency = JO_MAX(20, JO_MIN(JO_AUDIO_TONE_MAX_FREQUENCY, frequency)) << 8;
    base = (JO_SCSP_BASE_SAMPLING_RATE << 8) / JO_AUDIO_TONE_SAMPLES;
    oct = 0;
    while (frequency < base && oct > -8)
    {
        base >>= 1;
        --oct;
    }
    while (frequency >= (base << 1) && oct < 7)
    {
        base <<= 1;
        ++oct;
    }
    return ((unsigned short)JO_SCSP_PITCH(oct, JO_MIN(JO_SCSP_FNS_MAX, ((frequency - base) << 10) / base)));
}

/** @brief The decay rate of the envelope of the SCSP (D1R) for a decay time
 *  @param ms Time (ms) to fall by 60 dB
 *  @return Rate (1 to 31)
 */
static unsigned int                 jo_audio_decay_rate(const unsigned int ms)
{
    // The time (ms) of a fall by 96 dB (the whole range of the envelope) at each rate (measured on the envelope generator
    // of Mednafen: about 2 times faster every 2 rates)
    static const unsigned int       times[31] =
    {
        126595, 94970, 63298, 47485, 31649, 23742, 15824, 11871, 7912, 5936, 3956, 2968, 1978, 1484, 989, 742, 495, 371,
        247, 186, 124, 93, 62, 46, 31, 23, 15, 12, 8, 6, 6
    };
    unsigned int                    full;
    unsigned int                    r;

    full = ms * 8 / 5;
    for (JO_ZERO(r); r < 30 && times[r + 1] >= full; ++r)
        ;
    // (the closer of the 2 rates)
    if (r < 30 && times[r] - full > full - times[r + 1])
        ++r;
    return (r + 1);
}

/** @brief The attenuation (TL: 0.375 dB steps) of an amplitude
 *  @param amplitude 1 to 32767 (32767: 0 dB)
 */
static unsigned int                 jo_audio_attenuation(unsigned int amplitude)
{
    unsigned int                    steps;

    // (16 steps per 6 dB: 16 x log2(32768 / amplitude), the fraction by the 4 bits below the highest bit)
    if (amplitude == 0)
        return (255);
    JO_ZERO(steps);
    while (amplitude < 16384)
    {
        amplitude <<= 1;
        steps += 16;
    }
    // amplitude: 16384 to 32767; 16 x log2(32768 / amplitude) ~ 16 x (32768 - amplitude) / 16384 x 0.9
    steps += ((32768 - amplitude) * 15) >> 14;
    return (JO_MIN(255, steps));
}

void                                jo_audio_play_tone(const jo_audio_tone * const tone, const unsigned char channel)
{
    unsigned short                  regs[JO_AUDIO_SLOT_REGISTERS];
    unsigned short                  cmd[4];
    unsigned short                  pitch;
    unsigned short                  modulator_pitch;
    unsigned int                    slot;
    unsigned int                    rate;
    unsigned int                    offset;
    bool                            fm;

#ifdef JO_DEBUG
    if (tone == JO_NULL || channel >= JO_SOUND_MAX_CHANNEL)
    {
        jo_core_error("Invalid tone or channel (%d)", channel);
        return;
    }
#endif
    if (channel >= JO_SOUND_MAX_CHANNEL || jo_audio_channel_status(channel))
        return;
    slot = jo_audio_channel_slot(channel);
    fm = tone->modulation != 0 && tone->waveform == JoToneSine;
    pitch = jo_audio_tone_pitch(tone->frequency);
    modulator_pitch = jo_audio_tone_pitch((tone->frequency * JO_MAX(1, tone->modulator_ratio)) >> 2);
    rate = tone->decay_ms ? jo_audio_decay_rate(tone->decay_ms) : 0;
    offset = JO_AUDIO_TONE_START;
    // The modulator (the second slot): not heard, its output modulates the carrier (the first slot reads it in the stack
    // of the outputs of the slots, one sample before: MDXSL = MDYSL = 32 + 1). The modulation in samples: 2 x its output
    // / 2^(16 - MDL); MDL 10: the output / 32. An index of 1 radian: 1024 / (2 pi) = 163 samples, an output of 5215
    if (fm)
    {
        regs[0] = (unsigned short)(JO_SCSP_LOOP_NORMAL | JO_SCSP_START_ADDRESS_HIGH(offset));
        regs[1] = (unsigned short)JO_SCSP_START_ADDRESS_LOW(offset);
        regs[2] = 0;
        regs[3] = JO_AUDIO_TONE_SAMPLES;
        regs[4] = (unsigned short)JO_SCSP_ENVELOPE_ATTACK(JO_SCSP_ATTACK_RATE_MAX, rate, rate);
        regs[5] = (unsigned short)JO_SCSP_ENVELOPE_RELEASE(JO_SCSP_RELEASE_RATE_MAX);
        regs[6] = (unsigned short)jo_audio_attenuation(JO_MIN(32767, tone->modulation * 326));
        regs[7] = 0;
        regs[8] = modulator_pitch;
        regs[9] = 0;
        regs[10] = 0;
        regs[11] = 0;
        jo_audio_program_slot(slot + 1, regs);
    }
    // The carrier
    regs[0] = (unsigned short)((tone->waveform == JoToneNoise ? JO_SCSP_SOURCE_NOISE : 0) | JO_SCSP_LOOP_NORMAL |
                               JO_SCSP_START_ADDRESS_HIGH(offset));
    regs[1] = (unsigned short)JO_SCSP_START_ADDRESS_LOW(offset);
    regs[2] = 0;
    regs[3] = JO_AUDIO_TONE_SAMPLES;
    regs[4] = (unsigned short)JO_SCSP_ENVELOPE_ATTACK(JO_SCSP_ATTACK_RATE_MAX, rate, rate);
    regs[5] = (unsigned short)JO_SCSP_ENVELOPE_RELEASE(JO_SCSP_RELEASE_RATE_MAX);
    regs[6] = (unsigned short)JO_SCSP_TOTAL_LEVEL(tone->volume);
    regs[7] = (unsigned short)(fm ? JO_SCSP_MODULATION(10, 33, 33) : 0);
    regs[8] = pitch;
    regs[9] = (unsigned short)(tone->tremolo ? JO_SCSP_LFO(tone->tremolo_rate, JO_SCSP_LFO_TRIANGLE, tone->tremolo) : 0);
    regs[10] = (unsigned short)JO_SCSP_DSP_SEND(0, tone->reverb & 7);
    regs[11] = jo_audio_mixer(slot, (unsigned short)(JO_SCSP_DIRECT_LEVEL(JO_SCSP_LEVEL_MAX) | JO_SCSP_DIRECT_PAN(jo_audio_pan(tone->pan))));
    jo_audio_program_slot(slot, regs);
    // The glides (the modulator glides by the same ratio)
    if (__jo_audio_driver_running && tone->end_frequency != 0 && tone->end_frequency != tone->frequency)
    {
        cmd[0] = (unsigned short)((JO_DRIVER_GLIDE << 8) | slot);
        cmd[1] = (unsigned short)JO_DRIVER_GLIDE_PITCH(jo_audio_tone_pitch(tone->end_frequency));
        cmd[2] = (unsigned short)(32768 / JO_MAX(2, tone->glide_ms));
        cmd[3] = (unsigned short)JO_DRIVER_GLIDE_PITCH(pitch);
        jo_audio_driver_post(cmd, 4);
        if (fm)
        {
            cmd[0] = (unsigned short)((JO_DRIVER_GLIDE << 8) | (slot + 1));
            cmd[1] = (unsigned short)JO_DRIVER_GLIDE_PITCH(jo_audio_tone_pitch((tone->end_frequency * JO_MAX(1, tone->modulator_ratio)) >> 2));
            cmd[3] = (unsigned short)JO_DRIVER_GLIDE_PITCH(modulator_pitch);
            jo_audio_driver_post(cmd, 4);
        }
    }
    jo_audio_flush();
    __jo_audio_channels[channel].active = true;
    __jo_audio_channels[channel].stereo = false;
    __jo_audio_channels[channel].tone = fm;
    __jo_audio_channels[channel].sound = JO_NULL;
    // (no sound in the sound RAM to keep)
    __jo_audio_channels[channel].offset = JO_AUDIO_PCM_AREA_END;
    JO_ZERO(__jo_audio_channels[channel].size);
    __jo_audio_channels[channel].end_vblank = __jo_core_vblank_count +
                                              (tone->decay_ms ? (tone->decay_ms * 8 / 5 * jo_core_get_refresh_rate()) / 1000 + 1 : JO_AUDIO_LOOP_VBLANKS);
}

#endif

/*
 Special thanks for VBT for sharing his experience with multiple PCMs.
 https://segaxtreme.net/threads/another-sonic-homebrew-for-saturn.23846/page-2
*/
void	                    jo_audio_play_sound(jo_sound * const sound)
{
    register unsigned int   i;

#ifdef JO_DEBUG
    if (sound == JO_NULL)
    {
        jo_core_error("sound is null");
        return;
    }
#endif
    if (sound->volume == JO_MIN_AUDIO_VOLUME)
        return ;
    for (JO_ZERO(i); i < JO_SOUND_MAX_CHANNEL; ++i)
    {
        if (!jo_audio_channel_status(i))
        {
            jo_audio_play_sound_on_channel(sound, i);
            return ;
        }
    }
}

int                         jo_audio_usage_percent(void)
{
    register unsigned int   i;
    int                     free_channel;

    JO_ZERO(free_channel);
    for (JO_ZERO(i); i < JO_SOUND_MAX_CHANNEL; ++i)
    {
        if (!jo_audio_channel_status(i))
            ++free_channel;
    }
    return (JO_PERCENT_USED(JO_SOUND_MAX_CHANNEL, free_channel));
}

void                jo_audio_free_pcm(jo_sound *sound)
{
#ifdef JO_DEBUG
    if (sound == JO_NULL)
    {
        jo_core_error("sound is null");
        return;
    }
#endif
    if (sound->data != JO_NULL)
    {
        jo_audio_forget_pcm(sound->data);
        jo_free(sound->data);
    }
    sound->data = JO_NULL;
    // (a resident sound stays in the sound RAM until jo_audio_free_resident())
    JO_ZERO(sound->resident);
}

bool                jo_audio_make_resident(jo_sound *sound)
{
    unsigned int    size;
    unsigned int    start;
    register unsigned int i;

#ifdef JO_DEBUG
    if (sound == JO_NULL)
    {
        jo_core_error("sound is null");
        return (false);
    }
#endif
    if (sound->resident != 0)
        return (true);
    if (sound->data == JO_NULL || sound->data_length == 0)
        return (false);
    size = JO_AUDIO_ALIGN(sound->data_length);
    // (a room for the copied sounds stays before)
    if (__jo_audio_resident_start < JO_AUDIO_PCM_AREA_START + JO_AUDIO_COPIED_ROOM + size)
        return (false);
    start = (__jo_audio_resident_start - size) & ~0x1F;
    // The copied sounds that were there are forgotten (they are copied again before when they play)
    for (JO_ZERO(i); i < JO_AUDIO_CACHE_ENTRIES; ++i)
        if (__jo_audio_cache[i].data != JO_NULL && __jo_audio_cache[i].offset + __jo_audio_cache[i].size > start)
            __jo_audio_cache[i].data = JO_NULL;
    if (__jo_audio_alloc_next > start)
        __jo_audio_alloc_next = JO_AUDIO_PCM_AREA_START;
    jo_audio_stop_channels_in(start, __jo_audio_resident_start);
    __jo_audio_resident_start = start;
    jo_dma_copy(sound->data, (void *)(JO_SOUND_RAM + start), size);
    jo_audio_forget_pcm(sound->data);
    jo_free(sound->data);
    sound->data = JO_NULL;
    sound->resident = start + 1;
    return (true);
}

void                jo_audio_free_resident(void)
{
    __jo_audio_resident_start = __jo_audio_music_sound_ram != 0 ? __jo_audio_music_sound_ram : JO_AUDIO_PCM_AREA_END;
}

void	            jo_audio_stop_sound(const jo_sound * const sound)
{
#ifdef JO_DEBUG
    if (sound == JO_NULL)
    {
        jo_core_error("sound is null");
        return;
    }
    if (sound->current_playing_channel >= JO_SOUND_MAX_CHANNEL)
    {
        jo_core_error("channel (%d) is too high (max=%d)", sound->current_playing_channel, JO_SOUND_MAX_CHANNEL);
        return;
    }
#endif
    // (only if the channel still plays this sound: another sound may play on it now)
    if (sound->current_playing_channel >= JO_SOUND_MAX_CHANNEL || !jo_audio_channel_status(sound->current_playing_channel) ||
        __jo_audio_channels[sound->current_playing_channel].sound != sound)
        return ;
    jo_audio_channel_off(sound->current_playing_channel);
}

void	            jo_audio_stop_channel(const unsigned char channel)
{
#ifdef JO_DEBUG
    if (channel >= JO_SOUND_MAX_CHANNEL)
    {
        jo_core_error("channel (%d) is too high (max=%d)", channel, JO_SOUND_MAX_CHANNEL - 1);
        return;
    }
#endif
    if (channel >= JO_SOUND_MAX_CHANNEL || !jo_audio_channel_status(channel))
        return ;
    jo_audio_channel_off(channel);
}

#ifdef JO_COMPILE_WITH_FS_SUPPORT

static void         __jo_audio_load_pcm_async_callback(char *contents, int length, int optional_token)
{
    jo_sound        *sound;

    sound = (jo_sound *)optional_token;
    sound->data_length = length;
    sound->data = contents;
    sound->volume = JO_MAX_AUDIO_VOLUME;
    // (a new sound: not the resident copy of the sound loaded before in this structure, see jo_audio_load_pcm())
    JO_ZERO(sound->resident);
}

bool                jo_audio_load_pcm_async(const char * const filename, const jo_sound_mode mode, jo_sound *sound)
{
#ifdef JO_DEBUG
    if (sound == JO_NULL)
    {
        jo_core_error("sound is null");
        return (false);
    }
#endif
    sound->mode = mode;
    if (!jo_fs_read_file_async(filename, __jo_audio_load_pcm_async_callback, (int)sound))
        return (false);
    return true;
}

bool                jo_audio_load_pcm(const char * const filename, const jo_sound_mode mode, jo_sound *sound)
{
    char            *pcm;
    int             len;

#ifdef JO_DEBUG
    if (sound == JO_NULL)
    {
        jo_core_error("sound is null");
        return (false);
    }
#endif
    if ((pcm = jo_fs_read_file(filename, &len)) == JO_NULL)
        return (false);
    sound->mode = mode;
    sound->data_length = len;
    sound->data = pcm;
    sound->volume = JO_MAX_AUDIO_VOLUME;
    JO_ZERO(sound->resident);
    return (true);
}

/** @brief A little endian value of a WAV file */
static  __jo_force_inline unsigned int  jo_audio_wav_value(const unsigned char * const p, const int bytes)
{
    return (bytes == 2 ? (unsigned int)(p[0] | (p[1] << 8)) : (unsigned int)(p[0] | (p[1] << 8) | (p[2] << 16) | ((unsigned int)p[3] << 24)));
}

/** @brief The format of the samples of a WAV file (1: PCM, 0x11: IMA ADPCM)
 *  @param p The chunk "fmt " (24 bytes in the file at least)
 *  @param end The end of the bytes read
 *  @remarks WAVE_FORMAT_EXTENSIBLE (0xFFFE): the format is the first 2 bytes of its sub format (written by the encoders
 *  for the samples of more than 16 bits or more than 2 channels, by some for any file: such a 16 bits file was refused)
 */
static unsigned int                 jo_audio_wav_format(const unsigned char * const p, const unsigned char * const end)
{
    unsigned int                    format;

    format = jo_audio_wav_value(p + 8, 2);
    if (format == 0xFFFE && jo_audio_wav_value(p + 4, 4) >= 40 && p + 34 <= end)
        format = jo_audio_wav_value(p + 32, 2);
    return (format);
}

bool                jo_audio_load_wav(const char * const filename, jo_sound *sound)
{
    unsigned char   *file;
    unsigned char   *p;
    unsigned char   *end;
    unsigned char   *data;
    unsigned int    size;
    unsigned int    channels;
    unsigned int    rate;
    unsigned int    bits;
    unsigned int    frames;
    unsigned int    k;
    unsigned int    w;
    int             len;
    unsigned char   c;

#ifdef JO_DEBUG
    if (sound == JO_NULL)
    {
        jo_core_error("sound is null");
        return (false);
    }
#endif
    if ((file = (unsigned char *)jo_fs_read_file(filename, &len)) == JO_NULL)
        return (false);
    // RIFF "WAVE", then the chunks: "fmt " (PCM: format 1), "data"
    channels = rate = bits = size = 0;
    data = JO_NULL;
    end = file + len;
    if (len < 12 || file[0] != 'R' || file[1] != 'I' || file[8] != 'W' || file[9] != 'A')
    {
        jo_free(file);
        return (false);
    }
    for (p = file + 12; end - p >= 8; p += 8 + ((size + 1) & ~1U))
    {
        size = jo_audio_wav_value(p + 4, 4);
        if (p[0] == 'f' && p[1] == 'm' && p[2] == 't' && p + 24 <= end && jo_audio_wav_format(p, end) == 1)
        {
            channels = jo_audio_wav_value(p + 10, 2);
            rate = jo_audio_wav_value(p + 12, 4);
            bits = jo_audio_wav_value(p + 22, 2);
        }
        else if (p[0] == 'd' && p[1] == 'a' && p[2] == 't' && p[3] == 'a')
        {
            data = p + 8;
            // (compared as sizes: a size of 0xFFFFFFFF, written by the encoders of streams, wrapped the pointer around)
            if (size > (unsigned int)(end - data))
                size = (unsigned int)(end - data);
            break;
        }
        // (a chunk after the end of the file: the end, the next chunk would be out of the file or before it)
        if (size > (unsigned int)(end - p) - 8)
            break;
    }
    if (data == JO_NULL || (bits != 8 && bits != 16) || (channels != 1 && channels != 2) || rate == 0)
    {
#ifdef JO_DEBUG
        jo_core_error("%s: not a PCM WAV", filename);
#endif
        jo_free(file);
        return (false);
    }
    // The samples at the start of the block (it is freed by its address), for the SCSP: 16 bits big endian, 8 bits signed
    frames = size / (channels * (bits >> 3));
    size = frames * channels * (bits >> 3);
    if (channels == 1)
        jo_memcpy(file, data, size);
    else
    {
        // (stereo: the left samples, then the right ones; the right ones in a copy)
        p = (unsigned char *)jo_malloc(size >> 1);
        if (p == JO_NULL)
        {
            jo_free(file);
            return (false);
        }
        for (JO_ZERO(k); k < frames; ++k)
        {
            if (bits == 16)
            {
                file[k * 2] = data[k * 4];
                file[k * 2 + 1] = data[k * 4 + 1];
                p[k * 2] = data[k * 4 + 2];
                p[k * 2 + 1] = data[k * 4 + 3];
            }
            else
            {
                file[k] = data[k * 2];
                p[k] = data[k * 2 + 1];
            }
        }
        jo_memcpy(file + (size >> 1), p, size >> 1);
        jo_free(p);
    }
    // (4 bytes at once: file is the start of a block of jo_malloc(), aligned on 4 bytes; byte by byte, 1.4 % of the loading
    // of the rally demo)
    if (bits == 16)
    {
        for (JO_ZERO(k); k + 3 < size; k += 4)
        {
            w = *(unsigned int *)(file + k);
            *(unsigned int *)(file + k) = ((w >> 8) & 0x00FF00FFu) | ((w & 0x00FF00FFu) << 8);
        }
        for (; k + 1 < size; k += 2)
        {
            c = file[k];
            file[k] = file[k + 1];
            file[k + 1] = c;
        }
    }
    else
    {
        for (JO_ZERO(k); k + 3 < size; k += 4)
            *(unsigned int *)(file + k) ^= 0x80808080u;
        for (; k < size; ++k)
            file[k] ^= 0x80;
    }
    sound->mode = channels == 2 ? (bits == 8 ? JoSoundStereo8Bit : JoSoundStereo16Bit) : (bits == 8 ? JoSoundMono8Bit : JoSoundMono16Bit);
    sound->data = (char *)file;
    sound->data_length = size;
    sound->sample_rate = (unsigned short)rate;
    sound->volume = JO_MAX_AUDIO_VOLUME;
    JO_ZERO(sound->pan);
    JO_ZERO(sound->resident);
    return (true);
}

/*
** Music streamed from the CD
*/

/** @brief The slots of the music (free between the slots of the CD audio and the slots of the channels 8 and more) */
# define JO_AUDIO_MUSIC_LEFT_SLOT           (18)
# define JO_AUDIO_MUSIC_RIGHT_SLOT          (19)
/** @brief The samples in sound RAM: a loop of 4 blocks of 4096 samples (16 bits) per side, the SCSP tells the block it plays */
# define JO_AUDIO_MUSIC_BLOCK_SAMPLES       (4096)
# define JO_AUDIO_MUSIC_RING_SAMPLES        (4 * JO_AUDIO_MUSIC_BLOCK_SAMPLES)
# define JO_AUDIO_MUSIC_RING_MASK           (JO_AUDIO_MUSIC_RING_SAMPLES - 1)
# define JO_AUDIO_MUSIC_AHEAD               (3 * JO_AUDIO_MUSIC_BLOCK_SAMPLES)
/** @brief The bytes of a side in sound RAM: its ring, then a copy of its first sample (the SCSP interpolates the last sample
 *  of the loop with the next one in the memory, at a rate other than 44.1 kHz) and 2 bytes (4 bytes aligned) */
# define JO_AUDIO_MUSIC_SIDE_SIZE           (JO_AUDIO_MUSIC_RING_SAMPLES * 2 + 4)
/** @brief The sound RAM taken by the music (left then right side) */
# define JO_AUDIO_MUSIC_SOUND_RAM_SIZE      (2 * JO_AUDIO_MUSIC_SIDE_SIZE)
/** @brief Samples decoded per side before they are written in the sound RAM (by 32 bits, see jo_audio_music_flush()) */
# define JO_AUDIO_MUSIC_STAGE_SAMPLES       (256)
/** @brief Bytes of the file read in advance in work RAM (a power of 2) */
#ifndef JO_AUDIO_MUSIC_BUFFER_SIZE
# define JO_AUDIO_MUSIC_BUFFER_SIZE         (65536)
#endif
_Static_assert(JO_AUDIO_MUSIC_BUFFER_SIZE >= 16384 && (JO_AUDIO_MUSIC_BUFFER_SIZE & (JO_AUDIO_MUSIC_BUFFER_SIZE - 1)) == 0,
               "JO_AUDIO_MUSIC_BUFFER_SIZE: a power of 2, 16384 or more");
# define JO_AUDIO_MUSIC_BUFFER_MASK         (JO_AUDIO_MUSIC_BUFFER_SIZE - 1)
# define JO_AUDIO_MUSIC_SECTOR_SIZE         (2048)
/** @brief Biggest block of an IMA ADPCM file */
# define JO_AUDIO_MUSIC_MAX_BLOCK           (4096)
/** @brief Tries of the reads done while a file is loaded (the buffer of the music below a quarter) */
# define JO_AUDIO_MUSIC_URGENT_TIMEOUT      (0x200000)
/** @brief No limit to the samples decoded (see jo_audio_music_decode()) */
# define JO_AUDIO_MUSIC_NO_QUOTA            (0xFFFFFFFF)

typedef enum
{
    JoAudioMusicPcm8,
    JoAudioMusicPcm16,
    JoAudioMusicImaAdpcm
}                                   jo_audio_music_format;

static struct
{
    volatile bool                   playing;
    bool                            paused;
    bool                            loop;
    volatile bool                   end_of_data;
    jo_audio_music_format           format;
    unsigned int                    channels;
    unsigned int                    rate;
    unsigned int                    block_align;
    unsigned int                    samples_per_block;
    unsigned int                    frame_size;     // Bytes of a sample of all the channels (PCM)
    unsigned int                    quota;          // Samples decoded at most at each VBLANK (twice the samples played)
    unsigned int                    fad;
    unsigned int                    data_start;
    unsigned int                    data_end;
    unsigned int                    cursor;
    unsigned int                    reading_sector;
    unsigned char                   *buffer;
    volatile unsigned int           in;
    volatile unsigned int           out;
    unsigned int                    written;        // Samples written in the sound RAM (the total count)
    unsigned int                    staged;         // Samples decoded, not written yet (see jo_audio_music_flush())
    unsigned int                    ima_left;       // Samples of the IMA ADPCM block being decoded (0: the next block)
    const unsigned char             *ima_data;      // The next bytes of this block
    int                             ima_sample[2];
    unsigned int                    ima_index[2];   // Step index x 64 (see __jo_audio_ima_table)
    unsigned int                    played_blocks;
    unsigned int                    block;
    unsigned int                    block_vblank;   // The VBLANK when the block played was seen
    unsigned int                    frame_samples;  // Samples played per frame (rounded down)
    unsigned int                    end_written;
    unsigned int                    underruns;
    volatile bool                   ended;
    unsigned char                   volume;
}                                   __jo_audio_music = { .volume = JO_DEFAULT_AUDIO_VOLUME };

static unsigned int                 __jo_audio_music_sector[JO_AUDIO_MUSIC_SECTOR_SIZE / 4];
static unsigned int                 __jo_audio_music_block[JO_AUDIO_MUSIC_MAX_BLOCK / 4];
static short                        __jo_audio_music_stage[2][JO_AUDIO_MUSIC_STAGE_SAMPLES] __attribute__((aligned(4)));

extern void                         (*__jo_core_music_vblank)(void);
extern void                         (*__jo_core_music_update)(void);

static const short                  __jo_audio_ima_steps[89] =
{
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060,
    1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132,
    7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

static const signed char            __jo_audio_ima_index[8] = {-1, -1, -1, -1, 2, 4, 6, 8};

/** @brief IMA ADPCM, for each step index and each code (16): the difference added to the sample (bits 31-16, signed),
 *  computed as the decoder of the format does (step / 8 + the step and its halves selected by the code: the same samples),
 *  and the next step index x 64 (bits 15-0: the offset of its entries in bytes) */
static unsigned int                 __jo_audio_ima_table[89 * 16];

/** @brief A sample of the music in sound RAM */
# define JO_AUDIO_MUSIC_RING(SIDE)          ((volatile short *)(JO_SOUND_RAM + __jo_audio_music_sound_ram + (SIDE) * JO_AUDIO_MUSIC_SIDE_SIZE))
# define JO_AUDIO_MUSIC_SAMPLE(SIDE, INDEX) (JO_AUDIO_MUSIC_RING(SIDE)[(INDEX) & JO_AUDIO_MUSIC_RING_MASK])

static void                         jo_audio_ima_tables(void)
{
    unsigned int                    i;
    unsigned int                    n;
    int                             step;
    int                             diff;
    int                             next;

    if (__jo_audio_ima_table[88 * 16 + 15] != 0)
        return;
    for (JO_ZERO(i); i < 89; ++i)
        for (JO_ZERO(n); n < 16; ++n)
        {
            step = __jo_audio_ima_steps[i];
            diff = (step >> 3) + ((n & 4) ? step : 0) + ((n & 2) ? step >> 1 : 0) + ((n & 1) ? step >> 2 : 0);
            next = JO_MAX(0, JO_MIN(88, (int)i + __jo_audio_ima_index[n & 7]));
            __jo_audio_ima_table[i * 16 + n] = ((unsigned int)((n & 8) ? -diff : diff) << 16) | (unsigned int)(next * 64);
        }
}

/** @brief Decode a code of IMA ADPCM
 *  @param CODE4 Code x 4 (bits 5-2, the other bits ignored)
 *  @param SAMPLE Last sample (updated)
 *  @param INDEX Step index x 64 (updated)
 */
# define JO_AUDIO_IMA_DECODE(CODE4, SAMPLE, INDEX) \
    do \
    { \
        unsigned int                _e = *(const unsigned int *)((const unsigned char *)__jo_audio_ima_table + (INDEX) + ((CODE4) & 0x3C)); \
        (INDEX) = _e & 0xFFFF; \
        (SAMPLE) += (short)(_e >> 16); \
        if ((SAMPLE) > 32767) \
            (SAMPLE) = 32767; \
        else if ((SAMPLE) < -32768) \
            (SAMPLE) = -32768; \
    } while (0)

/** @brief Write the samples decoded (__jo_audio_music_stage) in the rings of the sound RAM, after the samples written
 *  @remarks By 32 bits (the 16 bits writes on the B bus cost a sixth more) */
static void                         jo_audio_music_flush(void)
{
    volatile short                  *ring;
    volatile unsigned int           *ring32;
    const short                     *src;
    unsigned int                    count;
    unsigned int                    c;
    unsigned int                    i;
    unsigned int                    n;
    unsigned int                    w;

    if ((count = __jo_audio_music.staged) == 0)
        return;
    for (JO_ZERO(c); c < __jo_audio_music.channels; ++c)
    {
        ring = JO_AUDIO_MUSIC_RING(c);
        src = __jo_audio_music_stage[c];
        w = __jo_audio_music.written & JO_AUDIO_MUSIC_RING_MASK;
        // (the copy of the first sample of the ring after its end)
        i = (JO_AUDIO_MUSIC_RING_SAMPLES - w) & JO_AUDIO_MUSIC_RING_MASK;
        if (i < count)
            ring[JO_AUDIO_MUSIC_RING_SAMPLES] = src[i];
        JO_ZERO(i);
        while (i < count)
        {
            n = JO_MIN(count - i, JO_AUDIO_MUSIC_RING_SAMPLES - w);
            if (w & 1)
            {
                ring[w++] = src[i++];
                --n;
            }
            ring32 = (volatile unsigned int *)(ring + w);
            for (; n >= 2; n -= 2, i += 2, w += 2)
                *ring32++ = ((unsigned int)(unsigned short)src[i] << 16) | (unsigned short)src[i + 1];
            if (n != 0)
                ring[w++] = src[i++];
            w &= JO_AUDIO_MUSIC_RING_MASK;
        }
    }
    __jo_audio_music.written += count;
    JO_ZERO(__jo_audio_music.staged);
}

/** @brief Decode PCM samples of the buffer
 *  @param frames Samples of all the channels (in the buffer)
 */
static void                         jo_audio_music_decode_pcm(unsigned int frames)
{
    const unsigned char             *p;
    short                           *left;
    short                           *right;
    unsigned int                    position;
    unsigned int                    value;
    unsigned int                    n;
    unsigned int                    k;

    while (frames != 0)
    {
        position = __jo_audio_music.out & JO_AUDIO_MUSIC_BUFFER_MASK;
        // (a sample is never cut by the end of the buffer: the data start at its start, the buffer is a multiple of 4)
        n = JO_MIN(frames, JO_AUDIO_MUSIC_STAGE_SAMPLES - __jo_audio_music.staged);
        n = JO_MIN(n, (JO_AUDIO_MUSIC_BUFFER_SIZE - position) / __jo_audio_music.frame_size);
        p = __jo_audio_music.buffer + position;
        left = __jo_audio_music_stage[0] + __jo_audio_music.staged;
        right = __jo_audio_music_stage[1] + __jo_audio_music.staged;
        if (__jo_audio_music.format == JoAudioMusicPcm16)
        {
            if (__jo_audio_music.channels == 2)
                // (a word per sample: the buffer and its samples are 4 bytes aligned)
                for (JO_ZERO(k); k < n; ++k, p += 4)
                {
                    value = *(const unsigned int *)p;
                    left[k] = (short)__builtin_bswap16((unsigned short)(value >> 16));
                    right[k] = (short)__builtin_bswap16((unsigned short)value);
                }
            else
                for (JO_ZERO(k); k < n; ++k, p += 2)
                    left[k] = (short)(p[0] | (p[1] << 8));
        }
        else if (__jo_audio_music.channels == 2)
            for (JO_ZERO(k); k < n; ++k, p += 2)
            {
                left[k] = (short)((p[0] - 128) << 8);
                right[k] = (short)((p[1] - 128) << 8);
            }
        else
            for (JO_ZERO(k); k < n; ++k, ++p)
                left[k] = (short)((p[0] - 128) << 8);
        __jo_audio_music.staged += n;
        __asm__ volatile ("" : : : "memory");
        __jo_audio_music.out += n * __jo_audio_music.frame_size;
        frames -= n;
        if (__jo_audio_music.staged == JO_AUDIO_MUSIC_STAGE_SAMPLES)
            jo_audio_music_flush();
    }
}

/** @brief Decode IMA ADPCM samples of the buffer (a block in several calls: the decoding is shared by the VBLANKs)
 *  @param count Samples wanted (a few more: by 8 for a stereo file)
 *  @return Samples decoded (0: the next block isn't read yet)
 */
static unsigned int                 jo_audio_music_decode_ima(const unsigned int count)
{
    const unsigned char             *p;
    unsigned char                   *block;
    unsigned int                    decoded;
    unsigned int                    position;
    unsigned int                    value;
    unsigned int                    index;
    unsigned int                    c;
    unsigned int                    i;
    unsigned int                    n;
    int                             sample;
    short                           *dst;

    JO_ZERO(decoded);
    while (decoded < count)
    {
        if (__jo_audio_music.staged + 8 > JO_AUDIO_MUSIC_STAGE_SAMPLES)
            jo_audio_music_flush();
        if (__jo_audio_music.ima_left == 0)
        {
            // The next block: its header (the first sample and the step index of each channel)
            if (__jo_audio_music.in - __jo_audio_music.out < __jo_audio_music.block_align)
                break;
            position = __jo_audio_music.out & JO_AUDIO_MUSIC_BUFFER_MASK;
            if (position + __jo_audio_music.block_align <= JO_AUDIO_MUSIC_BUFFER_SIZE)
                block = __jo_audio_music.buffer + position;
            else
            {
                block = (unsigned char *)__jo_audio_music_block;
                for (JO_ZERO(i); i < __jo_audio_music.block_align; ++i)
                    block[i] = __jo_audio_music.buffer[(__jo_audio_music.out + i) & JO_AUDIO_MUSIC_BUFFER_MASK];
            }
            for (JO_ZERO(c); c < __jo_audio_music.channels; ++c)
            {
                __jo_audio_music.ima_sample[c] = (short)(block[c * 4] | (block[c * 4 + 1] << 8));
                __jo_audio_music.ima_index[c] = (block[c * 4 + 2] > 88 ? 88 : block[c * 4 + 2]) * 64;
                __jo_audio_music_stage[c][__jo_audio_music.staged] = (short)__jo_audio_music.ima_sample[c];
            }
            ++__jo_audio_music.staged;
            ++decoded;
            __jo_audio_music.ima_data = block + 4 * __jo_audio_music.channels;
            __jo_audio_music.ima_left = __jo_audio_music.samples_per_block - 1;
            continue;
        }
        p = __jo_audio_music.ima_data;
        if (__jo_audio_music.channels == 1)
        {
            // 2 samples per byte, the low code first
            n = JO_MIN(__jo_audio_music.ima_left, JO_AUDIO_MUSIC_STAGE_SAMPLES - __jo_audio_music.staged) & ~1U;
            n = JO_MIN(n, (count - decoded + 1) & ~1U);
            sample = __jo_audio_music.ima_sample[0];
            index = __jo_audio_music.ima_index[0];
            dst = __jo_audio_music_stage[0] + __jo_audio_music.staged;
            for (i = n; i != 0; i -= 2, ++p)
            {
                value = *p;
                JO_AUDIO_IMA_DECODE(value << 2, sample, index);
                *dst++ = (short)sample;
                JO_AUDIO_IMA_DECODE(value >> 2, sample, index);
                *dst++ = (short)sample;
            }
            __jo_audio_music.ima_sample[0] = sample;
            __jo_audio_music.ima_index[0] = index;
        }
        else
        {
            // 8 samples of the left channel (4 bytes), then 8 of the right one; the last group can be cut
            n = JO_MIN(__jo_audio_music.ima_left, 8);
            for (JO_ZERO(c); c < 2; ++c, p += 4)
            {
                // (the codes x 4: the low code of each byte first)
                value = (unsigned int)p[0] << 2 | (unsigned int)p[1] << 10 | (unsigned int)p[2] << 18 | (unsigned int)p[3] << 26;
                sample = __jo_audio_music.ima_sample[c];
                index = __jo_audio_music.ima_index[c];
                dst = __jo_audio_music_stage[c] + __jo_audio_music.staged;
                for (JO_ZERO(i); i < 7; ++i, value >>= 4)
                {
                    JO_AUDIO_IMA_DECODE(value, sample, index);
                    dst[i] = (short)sample;
                }
                // (the last code: its 2 high bits were shifted out)
                JO_AUDIO_IMA_DECODE((unsigned int)p[3] >> 2, sample, index);
                dst[7] = (short)sample;
                __jo_audio_music.ima_sample[c] = sample;
                __jo_audio_music.ima_index[c] = index;
            }
        }
        __jo_audio_music.ima_data = p;
        __jo_audio_music.staged += n;
        decoded += n;
        __jo_audio_music.ima_left -= n;
        if (__jo_audio_music.ima_left == 0)
        {
            __asm__ volatile ("" : : : "memory");
            __jo_audio_music.out += __jo_audio_music.block_align;
        }
    }
    return (decoded);
}

/** @brief Silence in sound RAM up to a sample
 *  @param end Sample (total count)
 */
static void                         jo_audio_music_silence(const unsigned int end)
{
    unsigned int                    n;
    unsigned int                    c;
    unsigned int                    i;

    while ((int)(end - (__jo_audio_music.written + __jo_audio_music.staged)) > 0)
    {
        n = JO_MIN(end - (__jo_audio_music.written + __jo_audio_music.staged), JO_AUDIO_MUSIC_STAGE_SAMPLES - __jo_audio_music.staged);
        for (JO_ZERO(c); c < __jo_audio_music.channels; ++c)
            for (i = __jo_audio_music.staged; i < __jo_audio_music.staged + n; ++i)
                __jo_audio_music_stage[c][i] = 0;
        __jo_audio_music.staged += n;
        jo_audio_music_flush();
    }
}

/** @brief Decode the bytes read up to a sample in sound RAM
 *  @param limit Last sample (excluded, total count)
 *  @param quota Most samples decoded (the decoding of a block of the ring shared by the VBLANKs: a whole block of 4096
 *  samples at once took up to 1.4 frame in the VBLANK interrupt with an IMA ADPCM file)
 */
static void                         jo_audio_music_decode(const unsigned int limit, unsigned int quota)
{
    unsigned int                    want;
    unsigned int                    n;
    unsigned int                    safe;

    while ((int)(limit - (__jo_audio_music.written + __jo_audio_music.staged)) > 0 && quota != 0)
    {
        want = JO_MIN(limit - (__jo_audio_music.written + __jo_audio_music.staged), quota);
        if (__jo_audio_music.format == JoAudioMusicImaAdpcm)
            n = jo_audio_music_decode_ima(want);
        else
        {
            n = JO_MIN(want, (__jo_audio_music.in - __jo_audio_music.out) / __jo_audio_music.frame_size);
            jo_audio_music_decode_pcm(n);
        }
        if (n == 0)
            break;
        quota -= n > quota ? quota : n;
    }
    jo_audio_music_flush();
    if ((int)(limit - __jo_audio_music.written) <= 0)
        return;
    // (the samples the SCSP plays soon: written, or silence)
    safe = __jo_audio_music.played_blocks * JO_AUDIO_MUSIC_BLOCK_SAMPLES + JO_AUDIO_MUSIC_BLOCK_SAMPLES * 2;
    if (quota == 0 && (int)(safe - __jo_audio_music.written) <= 0)
        return;
    if (__jo_audio_music.end_of_data && __jo_audio_music.ima_left == 0)
    {
        if (__jo_audio_music.end_written == 0)
            __jo_audio_music.end_written = __jo_audio_music.written;
        jo_audio_music_silence(__jo_audio_music.written + JO_MIN(limit - __jo_audio_music.written, quota));
        return;
    }
    if ((int)(safe - __jo_audio_music.written) > 0)
    {
        ++__jo_audio_music.underruns;
        jo_audio_music_silence(safe);
    }
}

static void                         jo_audio_music_key_off(void)
{
    jo_audio_key_off(JO_AUDIO_MUSIC_LEFT_SLOT);
    jo_audio_key_off(JO_AUDIO_MUSIC_RIGHT_SLOT);
    jo_audio_flush();
}

static void                         jo_audio_music_key_on(void)
{
    unsigned short                  pitch;

    pitch = __jo_scsp_pitch(__jo_audio_music.rate);
    if (__jo_audio_music.channels == 1)
        jo_audio_key_on(JO_AUDIO_MUSIC_LEFT_SLOT, __jo_audio_music_sound_ram, JO_AUDIO_MUSIC_RING_SAMPLES, false, pitch,
                        __jo_audio_music.volume, JO_SCSP_PAN_CENTER, true, 0);
    else
    {
        jo_audio_key_on(JO_AUDIO_MUSIC_LEFT_SLOT, __jo_audio_music_sound_ram, JO_AUDIO_MUSIC_RING_SAMPLES, false, pitch,
                        __jo_audio_music.volume, JO_SCSP_PAN_LEFT, true, 0);
        jo_audio_key_on(JO_AUDIO_MUSIC_RIGHT_SLOT, __jo_audio_music_sound_ram + JO_AUDIO_MUSIC_SIDE_SIZE,
                        JO_AUDIO_MUSIC_RING_SAMPLES, false, pitch, __jo_audio_music.volume, JO_SCSP_PAN_RIGHT, true, 0);
    }
    jo_audio_flush();
    JO_ZERO(__jo_audio_music.played_blocks);
    JO_ZERO(__jo_audio_music.block);
    __jo_audio_music.block_vblank = __jo_core_vblank_count;
}

/** @brief The samples of the music at each VBLANK (interrupt) */
static void                         jo_audio_music_vblank(void)
{
    unsigned int                    block;
    unsigned int                    played;

    if (!__jo_audio_music.playing || __jo_audio_music.paused || __jo_audio_music.ended)
        return;
    JO_SCSP_COMMON(JO_SCSP_COMMON_MONITOR) = JO_SCSP_MONITOR_SLOT(JO_AUDIO_MUSIC_LEFT_SLOT);
    block = JO_SCSP_MONITOR_BLOCK(JO_SCSP_COMMON(JO_SCSP_COMMON_MONITOR)) & 3;
    if (block != __jo_audio_music.block)
    {
        __jo_audio_music.played_blocks += (block - __jo_audio_music.block) & 3;
        __jo_audio_music.block = block;
        __jo_audio_music.block_vblank = __jo_core_vblank_count;
    }
    if (__jo_audio_music.end_written != 0 &&
        (int)(__jo_audio_music.played_blocks * JO_AUDIO_MUSIC_BLOCK_SAMPLES - __jo_audio_music.end_written) >= 0)
    {
        __jo_audio_music.ended = true;
        return;
    }
    // The samples played: at least the frames since the block was seen (the SCSP tells only the block): the samples
    // decoded follow the samples played at each frame (by the block, 4096 samples at once, the quota was too short to catch
    // up a block: underruns)
    played = (__jo_core_vblank_count - __jo_audio_music.block_vblank) * __jo_audio_music.frame_samples;
    played = __jo_audio_music.played_blocks * JO_AUDIO_MUSIC_BLOCK_SAMPLES + JO_MIN(played, JO_AUDIO_MUSIC_BLOCK_SAMPLES - 1);
    jo_audio_music_decode(played + JO_AUDIO_MUSIC_AHEAD, __jo_audio_music.quota);
}

/** @brief Put the sectors of the file in the buffer
 *  @param urgent Wait for the drive until the buffer is three quarters full (a file is loaded: the drive goes back to it after)
 */
static void                         jo_audio_music_fill(const bool urgent)
{
    unsigned int                    sector;
    unsigned int                    last_sector;
    unsigned int                    free;
    unsigned int                    from;
    unsigned int                    to;
    unsigned int                    position;
    unsigned int                    timeout;
    unsigned int                    count;
    unsigned char                   *src;
    int                             n;

    JO_ZERO(timeout);
    for (;;)
    {
        if (__jo_audio_music.cursor >= __jo_audio_music.data_end)
        {
            if (!__jo_audio_music.loop)
            {
                __jo_audio_music.end_of_data = true;
                return;
            }
            __jo_audio_music.cursor = __jo_audio_music.data_start;
        }
        free = JO_AUDIO_MUSIC_BUFFER_SIZE - (__jo_audio_music.in - __jo_audio_music.out);
        if (free < JO_AUDIO_MUSIC_SECTOR_SIZE || (urgent && free < JO_AUDIO_MUSIC_BUFFER_SIZE / 4))
            return;
        sector = __jo_audio_music.cursor / JO_AUDIO_MUSIC_SECTOR_SIZE;
        if (!__jo_fs_stream_is_reading() || sector != __jo_audio_music.reading_sector)
        {
            if (!urgent && free < JO_AUDIO_MUSIC_BUFFER_SIZE / 2)
                return;
            last_sector = (__jo_audio_music.data_end + JO_AUDIO_MUSIC_SECTOR_SIZE - 1) / JO_AUDIO_MUSIC_SECTOR_SIZE;
            if (last_sector > sector + free / JO_AUDIO_MUSIC_SECTOR_SIZE)
                last_sector = sector + free / JO_AUDIO_MUSIC_SECTOR_SIZE;
            if (__jo_fs_stream_read(__jo_audio_music.fad + sector, last_sector - sector) < 0)
                return;
            __jo_audio_music.reading_sector = sector;
        }
        from = __jo_audio_music.cursor - sector * JO_AUDIO_MUSIC_SECTOR_SIZE;
        position = __jo_audio_music.in & JO_AUDIO_MUSIC_BUFFER_MASK;
        // Whole sectors of the samples: read in the buffer (no copy; several sectors by command of the drive)
        count = JO_MIN(free, JO_AUDIO_MUSIC_BUFFER_SIZE - position) / JO_AUDIO_MUSIC_SECTOR_SIZE;
        count = JO_MIN(count, (__jo_audio_music.data_end - __jo_audio_music.cursor) / JO_AUDIO_MUSIC_SECTOR_SIZE);
        if (from == 0 && (position & 3) == 0 && count != 0)
        {
            if ((n = __jo_fs_stream_take(__jo_audio_music.buffer + position, (int)count)) <= 0)
            {
                if (!urgent || ++timeout > JO_AUDIO_MUSIC_URGENT_TIMEOUT)
                    return;
                continue;
            }
            JO_ZERO(timeout);
            __asm__ volatile ("" : : : "memory");
            __jo_audio_music.in += (unsigned int)n * JO_AUDIO_MUSIC_SECTOR_SIZE;
            __jo_audio_music.cursor += (unsigned int)n * JO_AUDIO_MUSIC_SECTOR_SIZE;
            __jo_audio_music.reading_sector = sector + (unsigned int)n;
            continue;
        }
        if ((n = __jo_fs_stream_take(__jo_audio_music_sector, 1)) <= 0)
        {
            if (!urgent || ++timeout > JO_AUDIO_MUSIC_URGENT_TIMEOUT)
                return;
            continue;
        }
        JO_ZERO(timeout);
        to = __jo_audio_music.data_end - sector * JO_AUDIO_MUSIC_SECTOR_SIZE;
        if (to > JO_AUDIO_MUSIC_SECTOR_SIZE)
            to = JO_AUDIO_MUSIC_SECTOR_SIZE;
        src = (unsigned char *)__jo_audio_music_sector + from;
        position = __jo_audio_music.in & JO_AUDIO_MUSIC_BUFFER_MASK;
        if (position + (to - from) <= JO_AUDIO_MUSIC_BUFFER_SIZE)
            jo_memcpy(__jo_audio_music.buffer + position, src, to - from);
        else
        {
            jo_memcpy(__jo_audio_music.buffer + position, src, JO_AUDIO_MUSIC_BUFFER_SIZE - position);
            jo_memcpy(__jo_audio_music.buffer, src + JO_AUDIO_MUSIC_BUFFER_SIZE - position, to - from - (JO_AUDIO_MUSIC_BUFFER_SIZE - position));
        }
        __asm__ volatile ("" : : : "memory");
        __jo_audio_music.in += to - from;
        __jo_audio_music.cursor = sector * JO_AUDIO_MUSIC_SECTOR_SIZE + to;
        __jo_audio_music.reading_sector = sector + 1;
    }
}

/** @brief The reads of the music at each frame */
static void                         jo_audio_music_update(void)
{
    if (__jo_audio_music.ended)
        jo_audio_stop_music();
    else if (__jo_audio_music.playing && !__jo_audio_music.end_of_data)
        jo_audio_music_fill(false);
}

/** @brief The reads of the music while a file is loaded: only when its buffer is below a quarter */
static void                         jo_audio_music_service(void)
{
    if (__jo_audio_music.playing && !__jo_audio_music.paused && !__jo_audio_music.end_of_data &&
        __jo_audio_music.in - __jo_audio_music.out < JO_AUDIO_MUSIC_BUFFER_SIZE / 4)
        jo_audio_music_fill(true);
}

/** @brief Read the header of a WAV file: the format, the place of the samples
 *  @return true if the format can be played
 */
static bool                         jo_audio_music_header(const unsigned char * const header, const unsigned int size)
{
    const unsigned char             *p;
    const unsigned char             *end;
    unsigned int                    format;
    unsigned int                    bits;
    unsigned int                    chunk;

    if (size < 12 || header[0] != 'R' || header[1] != 'I' || header[8] != 'W' || header[9] != 'A')
        return (false);
    JO_ZERO(format);
    JO_ZERO(bits);
    end = header + size;
    for (p = header + 12; p + 8 <= end; p += 8 + ((chunk + 1) & ~1U))
    {
        chunk = jo_audio_wav_value(p + 4, 4);
        if (p[0] == 'f' && p[1] == 'm' && p[2] == 't' && p + 24 <= end)
        {
            format = jo_audio_wav_format(p, end);
            __jo_audio_music.channels = jo_audio_wav_value(p + 10, 2);
            __jo_audio_music.rate = jo_audio_wav_value(p + 12, 4);
            __jo_audio_music.block_align = jo_audio_wav_value(p + 20, 2);
            bits = jo_audio_wav_value(p + 22, 2);
        }
        else if (p[0] == 'd' && p[1] == 'a' && p[2] == 't' && p[3] == 'a')
        {
            __jo_audio_music.data_start = (unsigned int)(p + 8 - header);
            // (a size of 0xFFFFFFFF: to the end of the file, see jo_audio_play_music())
            __jo_audio_music.data_end = chunk > 0xFFFFFFFFu - __jo_audio_music.data_start ? 0xFFFFFFFFu :
                                        __jo_audio_music.data_start + chunk;
            break;
        }
        // (a chunk after the end of the header read: the end, not a pointer wrapped around)
        if (chunk > (unsigned int)(end - p) - 8)
            break;
    }
    if (__jo_audio_music.data_end == 0 || (__jo_audio_music.channels != 1 && __jo_audio_music.channels != 2) ||
        __jo_audio_music.rate == 0 || __jo_audio_music.rate > 2 * JO_SCSP_BASE_SAMPLING_RATE)
        return (false);
    if (format == 1 && (bits == 8 || bits == 16))
    {
        __jo_audio_music.format = bits == 8 ? JoAudioMusicPcm8 : JoAudioMusicPcm16;
        __jo_audio_music.block_align = __jo_audio_music.channels * (bits >> 3);
        __jo_audio_music.frame_size = __jo_audio_music.block_align;
    }
    else if (format == 0x11 && bits == 4 && __jo_audio_music.block_align > 4 * __jo_audio_music.channels &&
             __jo_audio_music.block_align <= JO_AUDIO_MUSIC_MAX_BLOCK && (__jo_audio_music.block_align & 3) == 0)
    {
        __jo_audio_music.format = JoAudioMusicImaAdpcm;
        __jo_audio_music.samples_per_block = (__jo_audio_music.block_align - 4 * __jo_audio_music.channels) * 2 / __jo_audio_music.channels + 1;
    }
    else
        return (false);
    __jo_audio_music.data_end -= (__jo_audio_music.data_end - __jo_audio_music.data_start) % __jo_audio_music.block_align;
    return (__jo_audio_music.data_end > __jo_audio_music.data_start);
}

bool                                jo_audio_play_music(const char * const filename, const bool loop)
{
    void                            *handle;
    int                             file_id;
    int                             len;
    unsigned int                    size;

#ifdef JO_DEBUG
    if (filename == JO_NULL)
    {
        jo_core_error("filename is null");
        return (false);
    }
#endif
    jo_audio_stop_music();
    if ((file_id = __jo_fs_get_file_id(filename)) < 0 || (handle = __jo_fs_open(file_id)) == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("%s: file not found", filename);
#endif
        return (false);
    }
    __jo_audio_music.fad = __jo_fs_get_fad(handle);
    size = (unsigned int)__jo_fs_get_file_size(handle);
    JO_ZERO(__jo_audio_music.data_start);
    JO_ZERO(__jo_audio_music.data_end);
    len = __jo_fs_read_sectors(handle, 1, __jo_audio_music_sector, JO_AUDIO_MUSIC_SECTOR_SIZE);
    __jo_fs_close(handle);
    if (len <= 0 || !jo_audio_music_header((unsigned char *)__jo_audio_music_sector, (unsigned int)len))
    {
#ifdef JO_DEBUG
        jo_core_error("%s: not a PCM or IMA ADPCM WAV file", filename);
#endif
        return (false);
    }
    if (__jo_audio_music.data_end > size)
        __jo_audio_music.data_end = size - (size - __jo_audio_music.data_start) % __jo_audio_music.block_align;
    if (__jo_audio_music_sound_ram == 0)
    {
        if (__jo_audio_resident_start < JO_AUDIO_PCM_AREA_START + JO_AUDIO_COPIED_ROOM + JO_AUDIO_MUSIC_SOUND_RAM_SIZE)
            return (false);
        __jo_audio_music_sound_ram = (__jo_audio_resident_start - JO_AUDIO_MUSIC_SOUND_RAM_SIZE) & ~0x1F;
        for (len = 0; len < JO_AUDIO_CACHE_ENTRIES; ++len)
            if (__jo_audio_cache[len].data != JO_NULL && __jo_audio_cache[len].offset + __jo_audio_cache[len].size > __jo_audio_music_sound_ram)
                __jo_audio_cache[len].data = JO_NULL;
        if (__jo_audio_alloc_next > __jo_audio_music_sound_ram)
            __jo_audio_alloc_next = JO_AUDIO_PCM_AREA_START;
        jo_audio_stop_channels_in(__jo_audio_music_sound_ram, __jo_audio_resident_start);
        __jo_audio_resident_start = __jo_audio_music_sound_ram;
    }
    if ((__jo_audio_music.buffer = (unsigned char *)jo_malloc(JO_AUDIO_MUSIC_BUFFER_SIZE)) == JO_NULL)
        return (false);
    __jo_audio_music.loop = loop;
    __jo_audio_music.paused = false;
    __jo_audio_music.ended = false;
    __jo_audio_music.end_of_data = false;
    __jo_audio_music.cursor = __jo_audio_music.data_start;
    __jo_audio_music.reading_sector = 0xFFFFFFFF;
    JO_ZERO(__jo_audio_music.in);
    JO_ZERO(__jo_audio_music.out);
    JO_ZERO(__jo_audio_music.written);
    JO_ZERO(__jo_audio_music.end_written);
    JO_ZERO(__jo_audio_music.underruns);
    JO_ZERO(__jo_audio_music.staged);
    JO_ZERO(__jo_audio_music.ima_left);
    // (twice the samples played per frame, by 8: the decoding catches up a late start in a few frames)
    __jo_audio_music.frame_samples = __jo_audio_music.rate / jo_core_get_refresh_rate();
    __jo_audio_music.quota = (2 * __jo_audio_music.frame_samples + 8) & ~7U;
    jo_audio_ima_tables();
    jo_audio_music_fill(true);
    JO_ZERO(__jo_audio_music.played_blocks);
    jo_audio_music_decode(JO_AUDIO_MUSIC_AHEAD, JO_AUDIO_MUSIC_NO_QUOTA);
    jo_audio_music_key_on();
    __jo_audio_music.playing = true;
    __jo_fs_stream_service = jo_audio_music_service;
    __jo_core_music_update = jo_audio_music_update;
    __jo_core_music_vblank = jo_audio_music_vblank;
    return (true);
}

void                                jo_audio_stop_music(void)
{
    __jo_core_music_vblank = JO_NULL;
    __jo_core_music_update = JO_NULL;
    __jo_fs_stream_service = JO_NULL;
    if (__jo_audio_music.buffer == JO_NULL)
        return;
    __jo_audio_music.playing = false;
    jo_audio_music_key_off();
    __jo_fs_stream_stop();
    jo_free(__jo_audio_music.buffer);
    __jo_audio_music.buffer = JO_NULL;
}

void                                jo_audio_pause_music(void)
{
    if (!__jo_audio_music.playing || __jo_audio_music.paused || __jo_audio_music.ended)
        return;
    __jo_audio_music.paused = true;
    jo_audio_music_key_off();
}

void                                jo_audio_resume_music(void)
{
    unsigned int                    start;
    unsigned int                    count;
    unsigned int                    i;
    unsigned int                    c;
    short                           *copy;

    if (!__jo_audio_music.playing || !__jo_audio_music.paused)
        return;
    start = __jo_audio_music.played_blocks * JO_AUDIO_MUSIC_BLOCK_SAMPLES;
    count = __jo_audio_music.written - start;
    if ((copy = (short *)jo_malloc(count * 2 + 4)) != JO_NULL)
    {
        for (JO_ZERO(c); c < __jo_audio_music.channels; ++c)
        {
            for (JO_ZERO(i); i < count; ++i)
                copy[i] = JO_AUDIO_MUSIC_SAMPLE(c, start + i);
            for (JO_ZERO(i); i < count; ++i)
                JO_AUDIO_MUSIC_SAMPLE(c, i) = copy[i];
            // (the copy of the first sample after the ring)
            if (count != 0)
                JO_AUDIO_MUSIC_RING(c)[JO_AUDIO_MUSIC_RING_SAMPLES] = copy[0];
        }
        jo_free(copy);
    }
    else
        JO_ZERO(count);
    if (__jo_audio_music.end_written != 0)
        __jo_audio_music.end_written = __jo_audio_music.end_written > start ? __jo_audio_music.end_written - start : 1;
    __jo_audio_music.written = count;
    // (the samples kept are at the start of the ring again: the blocks played count from 0, as jo_audio_music_key_on()
    // will set them, before the decoding (with the count of the pause, the decoding wrote silence for a long time)
    JO_ZERO(__jo_audio_music.played_blocks);
    jo_audio_music_decode(JO_AUDIO_MUSIC_AHEAD, JO_AUDIO_MUSIC_NO_QUOTA);
    jo_audio_music_key_on();
    __jo_audio_music.paused = false;
}

bool                                jo_audio_is_music_playing(void)
{
    return (__jo_audio_music.playing && !__jo_audio_music.ended);
}

bool                                jo_audio_is_music_paused(void)
{
    return (__jo_audio_music.playing && __jo_audio_music.paused);
}

void                                jo_audio_set_music_volume(const unsigned char volume)
{
    __jo_audio_music.volume = volume > JO_MAX_AUDIO_VOLUME ? JO_MAX_AUDIO_VOLUME : volume;
    if (!__jo_audio_music.playing)
        return;
    jo_audio_slot_write(JO_AUDIO_MUSIC_LEFT_SLOT, JO_SCSP_SLOT_TOTAL_LEVEL, (unsigned short)JO_SCSP_TOTAL_LEVEL(__jo_audio_music.volume));
    jo_audio_slot_write(JO_AUDIO_MUSIC_RIGHT_SLOT, JO_SCSP_SLOT_TOTAL_LEVEL, (unsigned short)JO_SCSP_TOTAL_LEVEL(__jo_audio_music.volume));
}

unsigned int                        jo_audio_get_music_underruns(void)
{
    return (__jo_audio_music.underruns);
}

#endif

#endif /* !JO_COMPILE_WITH_AUDIO_SUPPORT */

/*
** END OF FILE
*/
