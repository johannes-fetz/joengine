/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2013, Johannes Fetz (johannesfetz@gmail.com)
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

static jo_sound     blop;

void			my_draw(void)
{
    jo_printf(0, 0, jo_get_last_error());
}

void			    my_gamepad(void)
{
    static bool     is_cd_playing = false;

    if (!jo_is_pad1_available())
        return ;
    if (jo_is_pad1_key_down(JO_KEY_A))
        jo_audio_play_sound_on_channel(&blop, 0);
    if (jo_is_pad1_key_down(JO_KEY_B))
    {
        if (!is_cd_playing)
        {
            /* The first two tracks in this demo are reserved so the first audio track is 3.
               For more details you can open "game.cue" file with a notepad */
            jo_audio_play_cd_track(3, 3, true);
            is_cd_playing = true;
        }
        else
        {
            jo_audio_stop_cd();
            is_cd_playing = false;
        }
    }
}

void            load_blop_sound(void)
{
	/*
		To convert any audio file to PCM under Linux or Windows => http://ffmpeg.org

		Just some restriction due to the Sega Saturn hardware:

		Compatible Samplerate : 8000 to 44100 Hz (-ar option)
		Compatible PCM format : s8 (-f option)
		Compatible mode: 8/16 bit mono/stereo

		Some command line:

		ffmpeg -i A.WAV -f s16be -ar 44100 A.PCM => JoSoundMono16Bit
		ffmpeg -i A.MP3 -f s8 -ac 1 -ar 8000 A.PCM => JoSoundStereo8Bit
	*/
    jo_audio_load_pcm("A.PCM", JoSoundMono16Bit, &blop);
}

void			jo_main(void)
{
	jo_core_init(JO_COLOR_DarkGray);
	load_blop_sound();
    jo_printf(0, 1, "Press A to play Blop sound");
    jo_printf(0, 2, "Press B to play CD music");
	jo_core_add_callback(my_draw);
	jo_core_add_callback(my_gamepad);
	jo_core_run();
}

/*
** END OF FILE
*/
