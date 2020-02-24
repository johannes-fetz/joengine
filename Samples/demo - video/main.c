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
#include <jo/jo.h>

void			    my_draw(void)
{
    // Yes, it consume a lot of memory during playing :)
    jo_printf(0, 0, "Dynamic memory usage: %d%%  ", jo_memory_usage_percent());

    jo_sprite_draw3D(jo_get_video_sprite(), 0, 0, 170);
}

void                my_video_stopped(void)
{
    jo_printf(0, 1, "Done.");
}

/*
** HOW TO CREATE CPK FILE (Work In Progress)
**
** 1) Install ffmpeg: https://www.ffmpeg.org
** 2) Use the following command line:
**    ffmpeg -i SOURCE.AVI -c:a libavformat -f film_cpk -s 320x224 -pix_fmt rgb24 SAMPLE.CPK
**
** Note: The video glitch on SSF and crash Yabause.
**       There must still be values ​​to set on ffmpeg to show the video properly (like tbn and tbc).
*/

void			    jo_main(void)
{
	jo_core_init(JO_COLOR_Black);
	if (jo_video_open_file("SAMPLE.CPK"))
        jo_video_play(my_video_stopped);
	jo_core_add_callback(my_draw);
	jo_core_run();
}

/*
** END OF FILE
*/
