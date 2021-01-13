/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2020, Johannes Fetz (johannesfetz@gmail.com)
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
#include <SDDRVS.DAT>
#include "jo/sgl_prototypes.h"
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

#ifdef JO_COMPILE_WITH_AUDIO_SUPPORT

# define CDC_PLY_PMODE_ENDLESS_REPEAT   (0x0f)

static PCM             __jo_internal_pcm[JO_SOUND_MAX_CHANNEL] =
{
    {(_Stereo | _PCM16Bit), 0, JO_DEFAULT_AUDIO_VOLUME, 0, 0x0, 0, 0, 0, 0},
    {(_Stereo | _PCM16Bit), 2, JO_DEFAULT_AUDIO_VOLUME, 0, 0x0, 0, 0, 0, 0},
    {(_Stereo | _PCM16Bit), 4, JO_DEFAULT_AUDIO_VOLUME, 0, 0x0, 0, 0, 0, 0},
    {(_Stereo | _PCM16Bit), 6, JO_DEFAULT_AUDIO_VOLUME, 0, 0x0, 0, 0, 0, 0},
    {(_Stereo | _PCM16Bit), 8, JO_DEFAULT_AUDIO_VOLUME, 0, 0x0, 0, 0, 0, 0},
    {(_Stereo | _PCM16Bit), 10, JO_DEFAULT_AUDIO_VOLUME, 0, 0x0, 0, 0, 0, 0},
};

void			    jo_audio_play_cd_track(const int from_track, const int to_track, const bool repeat_infinitely)
{
    CdcPly		    plywk;

    CDC_PLY_STYPE(&plywk) = CDC_PTYPE_TNO;
    CDC_PLY_STNO(&plywk) = from_track;
    CDC_PLY_SIDX(&plywk) = 1;

    CDC_PLY_ETYPE(&plywk) = CDC_PTYPE_TNO;
    CDC_PLY_ETNO(&plywk) = to_track;
    CDC_PLY_EIDX(&plywk) = 1;

    /*
        ST-38-R1-121093 page 24:

        0x00       No repeat (plays only once)
        0x01~0x0e  Maximum repeats (1~14 times)
        0x0f       Endless repeat
        0x7f       No change to the repeat mode
    */
    CDC_PLY_PMODE(&plywk) = repeat_infinitely ? CDC_PLY_PMODE_ENDLESS_REPEAT : CDC_PM_DFL;

    CDC_CdPlay(&plywk);
}

void		        jo_audio_set_volume(const unsigned char vol)
{
#if JO_COMPILE_USING_SGL
    slSndVolume(vol);
#else
    /*TODO slSndVolume replacement */
    JO_UNUSED_ARG(vol);
#endif
}

void		        jo_audio_stop_cd(void)
{
    CdcPos	        poswk;

    CDC_POS_PTYPE(&poswk) = CDC_PTYPE_DFL;
    CDC_CdSeek(&poswk);
}

void		        jo_audio_init(void)
{
#if JO_COMPILE_USING_SGL
    const char	map[] = {0xff, 0xff, 0xff, 0xff};

    slInitSound((unsigned char *)sddrvstsk, sizeof(sddrvstsk), (unsigned char *)map, sizeof(map));
    *(volatile unsigned char *)(0x25a004e1) = 0x0;
    CDC_CdInit(0x00, 0x00, 0x05, 0x0f);
    slCDDAOn(127, 127, 0, 0);
#else
    /*TODO slInitSound replacement */
#endif
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
#endif
    if (slPCMStat(&__jo_internal_pcm[(int)channel]))
        return ;
    slSndFlush();
    sound->current_playing_channel = channel;
    __jo_internal_pcm[(int)channel].level = (Uint8)sound->volume;
    __jo_internal_pcm[(int)channel].mode = (Uint8)sound->mode;
    __jo_internal_pcm[(int)channel].pitch = (Uint16)sound->sample_rate;
    __jo_internal_pcm[(int)channel].pan = (Sint8)sound->pan;
    slPCMOn(&__jo_internal_pcm[(int)channel], sound->data, sound->data_length);
}

/*
 Special thanks for VBT for sharing his experience with multiple PCMs.
 https://segaxtreme.net/threads/another-sonic-homebrew-for-saturn.23846/page-2
*/
void	                    jo_audio_play_sound(jo_sound * const sound)
{
    register unsigned int   i;

    if (sound->volume == JO_MIN_AUDIO_VOLUME)
        return ;
#ifdef JO_DEBUG
    if (sound == JO_NULL)
    {
        jo_core_error("sound is null");
        return;
    }
#endif
    if (slSndPCMNum(sound->mode) < 0)
    {
#ifdef JO_DEBUG
        jo_core_error("No available channel");
#endif
        return ;
    }
    for (JO_ZERO(i); i < JO_SOUND_MAX_CHANNEL; ++i)
    {
        if (!slPCMStat(&__jo_internal_pcm[i]))
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
        if (!slPCMStat(&__jo_internal_pcm[i]))
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
    jo_free(sound->data);
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
    if (!slPCMStat(&__jo_internal_pcm[(int)sound->current_playing_channel]))
        return ;
    slPCMOff(&__jo_internal_pcm[(int)sound->current_playing_channel]);
}

#ifdef JO_COMPILE_WITH_FS_SUPPORT

static void         __jo_audio_load_pcm_async_callback(char *contents, int length, int optional_token)
{
    jo_sound        *sound;

    sound = (jo_sound *)optional_token;
    sound->data_length = length;
    sound->data = contents;
    sound->volume = JO_MAX_AUDIO_VOLUME;
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
    return (true);
}

#endif

#endif /* !JO_COMPILE_WITH_AUDIO_SUPPORT */

/*
** END OF FILE
*/
