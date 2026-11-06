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
/** @file audio.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine Function prototypes for audio
 *  @bug No known bugs.
 */

#ifndef __JO_AUDIO_H__
# define __JO_AUDIO_H__

#ifdef JO_COMPILE_WITH_AUDIO_SUPPORT

/** @brief Channels of sounds (2 slots of the SCSP each; the slots 16 and 17 take the CD audio) */
# define JO_SOUND_MAX_CHANNEL       (14)

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
    unsigned short  sample_rate; // Set sampling rate from 8000 to 44100 kHz
    unsigned char   volume; // Change volume: 0 (JO_MIN_AUDIO_VOLUME) -> 127 (JO_MAX_AUDIO_VOLUME)
    char            pan; // Adjust stereo: -128 (left speaker) -> 127 (right speaker)
    unsigned int    resident; // Kept in the sound RAM (jo_audio_make_resident()): its offset there + 1, 0: not resident
    unsigned char   reverb; // Its send to the reverberation (JO_COMPILE_WITH_AUDIO_DSP_MODULE): 0 (none), 1 to 7 (-36 to 0 dB, 6 dB steps)
}                   jo_sound;

/** @brief Default audio volume */
# define JO_DEFAULT_AUDIO_VOLUME    (127)
/** @brief Max audio volume available for jo_audio_set_volume()  */
# define JO_MAX_AUDIO_VOLUME        (127)
/** @brief Min audio volume available for jo_audio_set_volume()  */
# define JO_MIN_AUDIO_VOLUME        (0)

#ifdef JO_COMPILE_WITH_FS_SUPPORT

/** @brief Play a track from the CD
 *  @param from_track First track index to play
 *  @param to_track Last track index to play
 *  @param repeat_infinitely Play tracks infinitely or not
 *  @warning Requires JO_COMPILE_WITH_FS_MODULE
 */
void	jo_audio_play_cd_track(const int from_track, const int to_track, const bool repeat_infinitely);

/** @brief Stop playing audio track
 *  @warning Requires JO_COMPILE_WITH_FS_MODULE
 */
void	jo_audio_stop_cd(void);

/** @brief Pause the audio track played by jo_audio_play_cd_track()
 *  @remarks Nothing happens if the CD is not playing or already paused
 *  @remarks You can load files while the CD audio is paused
 *  @warning Requires JO_COMPILE_WITH_FS_MODULE
 */
void	jo_audio_pause_cd(void);

/** @brief Resume the audio track paused by jo_audio_pause_cd()
 *  @remarks The playback continues from the pause position until the end of the last track given to jo_audio_play_cd_track()
 *  @remarks Nothing happens if the CD audio is not paused
 *  @warning Requires JO_COMPILE_WITH_FS_MODULE
 */
void	jo_audio_resume_cd(void);

/** @brief Check if the CD audio is paused (see jo_audio_pause_cd())
 *  @return true if the CD audio is paused
 *  @warning Requires JO_COMPILE_WITH_FS_MODULE
 */
bool	jo_audio_is_cd_paused(void);

/** @brief Check if the CD drive is playing
 *  @return true if the CD audio is playing
 *  @remarks Useful to know when the last track is finished (without repeat)
 *  @warning Requires JO_COMPILE_WITH_FS_MODULE
 *  @warning The CD drive is also in "play" status when a file is loading
 */
bool	jo_audio_is_cd_playing(void);

#endif

/** @brief Set the audio volume
 *  @param vol Volume level from 0 to 127
 */
void	jo_audio_set_volume(const unsigned char vol);

/** @brief Maximum level of the CD audio (see jo_audio_set_cd_volume()), the default */
# define JO_AUDIO_CD_VOLUME_MAX     (7)

/** @brief Set the level of the CD audio in the mix (the music tracks of the CD, see jo_audio_play_cd_track())
 *  @param level 0 (silent) to JO_AUDIO_CD_VOLUME_MAX (7, the default): each step halves the volume (-6 dB)
 *  @remarks The sounds are not changed: a music too loud for the sounds of the game is lowered with it
 */
void	jo_audio_set_cd_volume(const unsigned char level);

 /** @brief Checks whether a given audio channel is currently playing a sound
  *  @param channel Channel to check (0 to JO_SOUND_MAX_CHANNEL - 1)
  *  @return true if the channel is playing, false otherwise
  */
bool    jo_audio_is_channel_playing(const unsigned char channel);

/** @brief Play a sound on a specific channel
 *  @param sound Sound definition
 *  @param channel Channel (0 to JO_SOUND_MAX_CHANNEL - 1)
 *  @remarks Nothing is played if the channel is still playing a sound (see jo_audio_stop_channel())
 */
void	jo_audio_play_sound_on_channel(jo_sound * const sound, const unsigned char channel);

/** @brief Play a sound again and again on a specific channel, until the channel is stopped (an engine for instance)
 *  @param sound Sound definition
 *  @param channel Channel (0 to JO_SOUND_MAX_CHANNEL - 1)
 *  @remarks Nothing is played if the channel is still playing a sound (see jo_audio_stop_channel())
 *  @remarks See jo_audio_set_channel_sample_rate() and jo_audio_set_channel_volume() to change it while it plays
 */
void	jo_audio_play_sound_loop_on_channel(jo_sound * const sound, const unsigned char channel);

/** @brief Change the sampling rate of the sound played on a channel (its pitch: a faster engine for instance)
 *  @param channel Channel (0 to JO_SOUND_MAX_CHANNEL - 1)
 *  @param sample_rate Sampling rate (Hz, 1378 to 88200: 44100 / 32 to 44100 x 2)
 */
void	jo_audio_set_channel_sample_rate(const unsigned char channel, const unsigned short sample_rate);

/** @brief Change the volume of the sound played on a channel
 *  @param channel Channel (0 to JO_SOUND_MAX_CHANNEL - 1)
 *  @param volume Volume: 0 (JO_MIN_AUDIO_VOLUME) to 127 (JO_MAX_AUDIO_VOLUME)
 */
void	jo_audio_set_channel_volume(const unsigned char channel, const unsigned char volume);

/** @brief Change the send of the sound played on a channel to the reverberation (JO_COMPILE_WITH_AUDIO_DSP_MODULE)
 *  @param channel Channel (0 to JO_SOUND_MAX_CHANNEL - 1)
 *  @param level 0 (none), 1 to 7 (-36 to 0 dB, 6 dB steps)
 */
void	jo_audio_set_channel_reverb(const unsigned char channel, const unsigned char level);

#ifdef JO_COMPILE_WITH_AUDIO_DSP_SUPPORT

/** @brief The reverberation of a room (the DSP of the SCSP: jo_audio_set_reverb()) */
typedef struct
{
    unsigned short  decay_ms;   // The tail falls by 60 dB in this time (the low frequencies): 50 to 10000 ms
    unsigned char   damping;    // The high frequencies die faster: 0 (as the low ones) to 127 (much faster: a soft room)
    unsigned char   early;      // The level of the early reflections (the walls: 9 to 23 ms): 0 to 127
    unsigned char   tail;       // The level of the tail: 0 to 127
}                   jo_audio_reverb;

/** @brief Set the reverberation of the DSP of the SCSP (a room: the sounds whose field reverb is not 0 go there)
 *  @param reverb The room, JO_NULL: no reverberation
 *  @remarks Requires JO_COMPILE_WITH_AUDIO_DSP_MODULE = 1 in the makefile: its ring buffer takes the last 16 KB of the
 *  sound RAM. The sounds are mixed in mono into it, the reverberation is stereo
 *  @remarks Sounds without their own echo take less sound RAM: the DSP makes the echo of the room where they play
 */
void    jo_audio_set_reverb(const jo_audio_reverb * const reverb);

#endif

#ifdef JO_COMPILE_WITH_AUDIO_DRIVER_SUPPORT

/** @brief Waveform of a tone */
typedef enum
{
    JoToneSine = 0,
    JoToneNoise = 1
}                   jo_tone_waveform;

/** @brief A tone made by the SCSP (no sample: a sine, FM, or the noise generator), see jo_audio_play_tone() */
typedef struct
{
    unsigned short      frequency;          // Hz (20 to 8000)
    unsigned short      end_frequency;      // The pitch glides to this frequency (Hz, 0: no glide)
    unsigned short      glide_ms;           // The time of the glide: 63 % of the way in this time
    unsigned short      decay_ms;           // The tone falls by 60 dB in this time (0: it plays until the channel is stopped)
    unsigned char       volume;             // 0 (JO_MIN_AUDIO_VOLUME) -> 127 (JO_MAX_AUDIO_VOLUME)
    char                pan;                // -128 (left speaker) -> 127 (right speaker)
    unsigned char       reverb;             // Send to the reverberation (JO_COMPILE_WITH_AUDIO_DSP_MODULE): 0 to 7
    unsigned char       modulation;         // FM: the index x 16 (16: 1 radian, 0: no FM); the second slot of the channel is the modulator
    unsigned char       modulator_ratio;    // FM: the frequency of the modulator / the frequency x 4 (4: the same frequency, 8: twice)
    unsigned char       tremolo;            // The depth of the tremolo (the amplitude LFO): 0 (none) to 7
    unsigned char       tremolo_rate;       // Its frequency: 0 to 31 (LFOF of the SCSP: 0.17 Hz to 96 Hz; 28: 38 Hz)
    jo_tone_waveform    waveform;
}                       jo_audio_tone;

/** @brief Play a tone made by the SCSP on a channel (FM, glide, tremolo, noise: no sample in the sound RAM)
 *  @param tone The tone
 *  @param channel Channel (0 to JO_SOUND_MAX_CHANNEL - 1): its 2 slots (the carrier and the FM modulator)
 *  @remarks Requires JO_COMPILE_WITH_AUDIO_DRIVER_MODULE = 1 in the makefile: the 68000 makes the glides (it changes
 *  the pitch each millisecond)
 *  @remarks Nothing is played if the channel is still playing a sound (see jo_audio_stop_channel())
 */
void    jo_audio_play_tone(const jo_audio_tone * const tone, const unsigned char channel);

#endif

/** @brief Play a sound on first available channel
 *  @param sound Sound definition
 */
void	jo_audio_play_sound(jo_sound * const sound);

/** @brief Stop a sound
 *  @param sound Sound definition
 */
void	jo_audio_stop_sound(const jo_sound * const sound);

/** @brief Stop the sound played on a channel
 *  @param channel Channel (0 to JO_SOUND_MAX_CHANNEL - 1)
 *  @remarks jo_audio_play_sound_on_channel() doesn't play on a busy channel: stop it first to restart a sound
 *  (for instance the shots of an automatic weapon, longer than the time between 2 shots)
 */
void	jo_audio_stop_channel(const unsigned char channel);

#ifdef JO_COMPILE_WITH_FS_SUPPORT

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

/** @brief Load PCM file from CD asynchronously
 *  @param filename Pcm filename (example: A.PCM)
 *  @param mode PCM audio mode (JoSoundMono16Bit, etc)
 *  @param sound Sound definition (output)
 *  @remarks To convert any audio file to PCM under Linux or Windows => http://ffmpeg.org
 *  @remarks ffmpeg -i A.WAV -f s8 -ar 44100 A.PCM
 *  @remarks ffmpeg -i A.MP3 -f s8 -ac 1 -ar 8000 A.PCM
 *  @return true if succeed otherwise false
 */
bool     jo_audio_load_pcm_async(const char * const filename, const jo_sound_mode mode, jo_sound *sound);

/** @brief Load a WAV file from CD (PCM, 8 or 16 bits, mono or stereo, any sample rate: the sound gets its mode and its
 *  sample rate from the file)
 *  @param filename WAV filename (example: SHOT.WAV)
 *  @param sound Sound definition (output)
 *  @remarks The samples are converted for the SCSP (16 bits: big endian; 8 bits: signed; stereo: the left samples then
 *  the right ones). A WAV file can be listened to on a computer: export them with any audio tool (16 bits recommended:
 *  8 bits samples hiss in the quiet parts, the end of an echo)
 *  @return true if succeed otherwise false
 */
bool     jo_audio_load_wav(const char * const filename, jo_sound *sound);

/** @brief Play a music streamed from a WAV file of the CD (only a few seconds of it in memory)
 *  @param filename WAV filename (example: MUSIC.WAV): PCM 8 or 16 bits, or IMA ADPCM (4 times smaller: recommended),
 *  mono or stereo, up to 44100 Hz
 *  @param loop The music starts again at its end
 *  @remarks The music goes on while the files are loaded (jo_fs_read_file()...): the drive reads it again when its buffer
 *  is low, then goes back to the file. It uses 64 KB of work RAM (JO_AUDIO_MUSIC_BUFFER_SIZE) and 64 KB of sound RAM
 *  @remarks Convert any audio file with ffmpeg (http://ffmpeg.org):
 *  @remarks ffmpeg -i MUSIC.MP3 -acodec adpcm_ima_wav -ar 32000 MUSIC.WAV
 *  @remarks ffmpeg -i MUSIC.MP3 -acodec pcm_s16le -ar 22050 MUSIC.WAV
 *  @remarks Not with the CD audio tracks (jo_audio_play_cd_track()): both use the drive
 *  @return true if succeed otherwise false
 */
bool     jo_audio_play_music(const char * const filename, const bool loop);

/** @brief Stop the music (jo_audio_play_music()) */
void     jo_audio_stop_music(void);

/** @brief Pause the music (jo_audio_play_music()) */
void     jo_audio_pause_music(void);

/** @brief Resume the music paused by jo_audio_pause_music() */
void     jo_audio_resume_music(void);

/** @brief Check if the music is playing (jo_audio_play_music())
 *  @return true if the music is playing or paused, false when it is over (no loop) or stopped
 */
bool     jo_audio_is_music_playing(void);

/** @brief Check if the music is paused (jo_audio_pause_music())
 *  @return true if the music is paused
 */
bool     jo_audio_is_music_paused(void);

/** @brief Set the volume of the music (jo_audio_play_music())
 *  @param volume Volume (JO_MIN_AUDIO_VOLUME to JO_MAX_AUDIO_VOLUME)
 */
void     jo_audio_set_music_volume(const unsigned char volume);

/** @brief Count the times the music had no data to play (the drive was too late: a silence of about 0.1 s)
 *  @return Count since jo_audio_play_music()
 */
unsigned int    jo_audio_get_music_underruns(void);

#endif

/** @brief Keep a sound in the sound RAM (512 KB) and free its copy in the work RAM (the jo_malloc() memory)
 *  @param sound Sound loaded (jo_audio_load_pcm(), jo_audio_load_wav())
 *  @remarks The resident sounds fill the sound RAM from its end; the other sounds are copied before them when they play.
 *  Many sounds in 16 bits fit there and not in the work RAM
 *  @return true if succeed, false if the sound RAM has no room (the sound stays in the work RAM)
 */
bool     jo_audio_make_resident(jo_sound *sound);

/** @brief Forget all the resident sounds (a new level): their room in the sound RAM is free again
 *  @remarks The jo_sound of the resident sounds must not be played any more
 */
void     jo_audio_free_resident(void);

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
