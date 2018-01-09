/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2017, Johannes Fetz (johannesfetz@gmail.com)
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
/** @file audio.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine Function prototypes for audio
 *  @bug No known bugs.
 */

#ifndef __JO_AUDIO_H__
# define __JO_AUDIO_H__

#ifdef JO_COMPILE_WITH_AUDIO_SUPPORT

# define JO_SOUND_MAX_CHANNEL       (6)

typedef enum
{
    JoSoundMono8Bit = (0x00 | 0x10),
    JoSoundMono16Bit = (0x00 | 0x00),
    JoSoundStereo8Bit = (0x80 | 0x10),
    JoSoundStereo16Bit = (0x80 | 0x00)
}                   jo_sound_mode;

typedef struct
{
    char            *data;
    unsigned int    data_length;
    jo_sound_mode   mode;
    unsigned char   current_playing_channel;
}                   jo_sound;

/** @brief Default audio volume */
# define JO_DEFAULT_AUDIO_VOLUME    (127)
/** @brief Max audio volume available for jo_audio_set_volume()  */
# define JO_MAX_AUDIO_VOLUME        (127)
/** @brief Min audio volume available for jo_audio_set_volume()  */
# define JO_MIN_AUDIO_VOLUME        (0)

/** @brief Play a track from the CD
 *  @param fromTrack First track index to play
 *  @param totrack Last track index to play
 *  @param loop 1 to enable repeat mode or 0
 */
void	jo_audio_play_cd_track(const int fromTrack, const int totrack, const int loop);

/** @brief Set the audio volume
 *  @param vol Volume level from 0 to 127
 */
void	jo_audio_set_volume(const unsigned char vol);

/** @brief Stop playing audio track
 */
void	jo_audio_stop_cd(void);

/** @brief Play a sound on a specific channel
 *  @param sound Sound definition
 *  @param channel Channel (0 to 6)
 */
void	jo_audio_play_sound_on_channel(jo_sound * const sound, const unsigned char channel);

/** @brief Play a sound on first available channel
 *  @param sound Sound definition
 */
void	jo_audio_play_sound(jo_sound * const sound);

/** @brief Stop a sound
 *  @param sound Sound definition
 */
void	jo_audio_stop_sound(const jo_sound * const sound);

/** @brief Load PCM file from CD
 *  @param filename Pcm filename (example: A.PCM)
 *  @param mode PCM audio mode (JoSoundMono16Bit, etc)
 *  @param sound Sound definition (output)
 *  @remarks To convert any audio file to PCM under Linux or Windows => http://ffmpeg.org
 *  @remarks ffmpeg -i A.WAV -f s8 -ar 44100 A.PCM
 *  @remarks ffmpeg -i A.MP3 -f s8 -ac 1 -ar 8000 A.PCM
 *  @return true if succeed otherwise false
 */
bool     jo_audio_load_pcm(const char * const filename, const jo_sound_mode mode, jo_sound *sound);

/** @brief Free pcm sound loaded with jo_audio_load_pcm()
 *  @param sound Sound definition
 */
void     jo_audio_free_pcm(jo_sound *sound);

/** @brief Get audio channel usage
 *  @return Audio channel usage percent
 *  @remarks jo_printf(0, 0, "Audio channel usage: %d%%  ", jo_audio_usage_percent());
 */
int                         jo_audio_usage_percent(void);

#endif /* !JO_COMPILE_WITH_AUDIO_SUPPORT */

#endif /* !__JO_AUDIO_H__ */

/*
** END OF FILE
*/
