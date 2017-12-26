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

static int      first_gem_sprite_id = -1;

static jo_tile Gems[] =
{
    {0, 0, 32, 32},
    {128, 128, 32, 32},
    {0, 96, 32, 32}
};

void			my_draw(void)
{
    jo_printf(0, 0, "Dynamic memory usage: %d%%  ", jo_memory_usage_percent());

	jo_sprite_draw3D(first_gem_sprite_id, -32, 0, 500);
    jo_sprite_draw3D(first_gem_sprite_id + 1, 0, 0, 500);
    jo_sprite_draw3D(first_gem_sprite_id + 2, 32, 0, 500);
}

void			jo_main(void)
{
	jo_core_init(JO_COLOR_White);
    jo_set_printf_color_index(JO_COLOR_INDEX_Black);

    first_gem_sprite_id = jo_sprite_add_tga_tileset("TEX", "GEMS.TGA", JO_COLOR_Green, Gems, JO_TILE_COUNT(Gems));

	jo_core_add_callback(my_draw);
	jo_core_run();
}

/*
** END OF FILE
*/
