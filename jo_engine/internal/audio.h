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
/** @file internal/audio.h
 *  @author Johannes Fetz
 *
 *  @brief Audio: internal functions of the engine (see jo/audio.h)
 *  @warning Not part of the API: only for the source files of the engine (included after jo/audio.h). The games
 *  include jo/jo.h, which doesn't include this file
 *  @bug No known bugs.
 */

#ifndef __JO_INTERNAL_AUDIO_H__
# define __JO_INTERNAL_AUDIO_H__

# if defined(JO_COMPILE_WITH_AUDIO_SUPPORT) && defined(JO_COMPILE_WITH_VIDEO_SUPPORT)

/** @brief Reserve the place of the samples of a movie in the sound RAM and the last channel (internal engine usage: the
 *  sound of jo_video_play() uses the SCSP slots of the channel JO_SOUND_MAX_CHANNEL - 1)
 *  @param size Size (bytes)
 *  @return Offset in the sound RAM (the same one until __jo_audio_release_movie()), 0: no room (resident sounds)
 *  @remarks The sounds copied there are copied again before when they play, the sounds of the channel are stopped, and
 *  the channel stays busy until __jo_audio_release_movie()
 */
unsigned int    __jo_audio_reserve_movie(const unsigned int size);

/** @brief Give back the place and the channel of __jo_audio_reserve_movie() (internal engine usage, the movie stopped) */
void            __jo_audio_release_movie(void);

# endif

#endif /* !__JO_INTERNAL_AUDIO_H__ */

/*
** END OF FILE
*/
