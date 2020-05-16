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
/** @file video.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine Video Player (Sega FILM/CPK File Format)
 *  @todo handle all kind of movie (bpp, audio channel, etc)
 *  @todo Custom implementation
 *  @todo Reduce memory consumption
 *  @bug No known bugs.
 */

#ifndef __JO_VIDEO_H__
# define __JO_VIDEO_H__

#ifdef JO_COMPILE_WITH_VIDEO_SUPPORT

/*
** TYPEDEFS
*/
/** @brief Function prototype for jo_video_play() */
typedef void	(*jo_video_onstop_callback)(void);

/** @brief Error handling (Internal usage)
 *  @warning MC Hammer: don't touch this
 */
extern int  __jo_video_sprite_id;

/** @brief Open a Sega FILM / Cinepak
 *  @param filename Filename (upper case and shorter as possible like "A.CPK")
 *  @return true if succeeded
 */
bool					        jo_video_open_file(const char *const filename);

/** @brief Play the movie previously loaded
 *  @return true if succeeded
 */
bool                            jo_video_play(const jo_video_onstop_callback onstop);

/** @brief Pause the movie (must be called after jo_video_play())
 *  @warning To play the movie again, you must reopen the movie (jo_video_open_file())
 *  @return true if succeeded
 */
bool                            jo_video_pause(void);

/** @brief Stop the movie (must be called after jo_video_play())
 *  @return true if succeeded
 *  @warning To play the movie again, you must reopen the movie (jo_video_open_file())
 */
void                            jo_video_stop(void);

/** @brief Get video sprite Id
 *  @return sprite Id
 */
static  __jo_force_inline int   jo_get_video_sprite(void)
{
    return (__jo_video_sprite_id);
}

#endif /* !JO_COMPILE_WITH_VIDEO_SUPPORT */

#endif /* !__JO_VIDEO_H__ */

/*
** END OF FILE
*/
