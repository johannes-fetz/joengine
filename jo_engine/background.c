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
#include "jo/sgl_prototypes.h"
#include "jo/conf.h"
#include "jo/types.h"
#include "jo/sega_saturn.h"
#include "jo/smpc.h"
#include "jo/core.h"
#include "jo/tools.h"
#include "jo/malloc.h"
#include "jo/fs.h"
#include "jo/image.h"
#include "jo/sprites.h"
#include "jo/background.h"
#include "jo/math.h"
#include "jo/palette.h"

/*
** MACROS
*/
#define RBG0RB_MAP_ADR			(VDP2_VRAM_B0)
#define RBG0RA_MAP_ADR			(VDP2_VRAM_B0 + 8192)

#define RBG0_CEL_ADR			(VDP2_VRAM_A0)

#define RBG0_KTB_ADR			(VDP2_VRAM_A1)
#define RBG0_PRA_ADR			(VDP2_VRAM_A1 + 0x1fe00)
#define RBG0_PRB_ADR			(RBG0_PRA_ADR + 0x00080)
#define BACK_COL_ADR			(VDP2_VRAM_A1 + 0x1fffe)

#define CELL_WIDTH              (8)
#define CELL_HEIGHT             (8)
#define CELL_SIZE               (CELL_WIDTH * CELL_HEIGHT)
#define CELL_COUNT_PER_ROW      (64)
#define PLANE_LENGTH            (512)
#define MAP_LENGTH              (512 * 8)

#define RBG0_OVER_MODE_REPEAT   (0)
#define RBG0_OVER_MODE_SINGLE   (3)

#define LINE_TBL_ADR            (VDP2_VRAM_A1)

static unsigned char            *__jo_cell_adr = (unsigned char *)RBG0_CEL_ADR;
static int                      __jo_cell_mapoff = 0;

static void                     jo_set_printf_palette(void)
{
    jo_set_printf_palette_color(JO_COLOR_INDEX_White, JO_COLOR_White);
    jo_set_printf_palette_color(JO_COLOR_INDEX_Black, JO_COLOR_Black);
    jo_set_printf_palette_color(JO_COLOR_INDEX_Red, JO_COLOR_Red);
    jo_set_printf_palette_color(JO_COLOR_INDEX_Green, JO_COLOR_Green);
    jo_set_printf_palette_color(JO_COLOR_INDEX_Yellow, JO_COLOR_Yellow);
    jo_set_printf_palette_color(JO_COLOR_INDEX_Blue, JO_COLOR_Blue);
    jo_set_printf_palette_color(JO_COLOR_INDEX_Purple, JO_COLOR_Purple);
}

/*
** Default Jo Engine screen priority : NBG0 > SPR0 > SPR1 > RBG0 > NBG1 > NBG2 > NBG3
**                                        7      6      5      4      3      2      1
*/
void                            __jo_init_vdp2(const jo_color back_color)
{
    //NBG0 (printf)
    slCharNbg0(COL_TYPE_256, CHAR_SIZE_1x1);
    jo_set_printf_palette();
    jo_set_printf_color_index(0);
    //NBG1
    slBitMapNbg1(COL_TYPE_32768, JO_VDP2_SIZE, (void *)VDP2_VRAM_A0);

    slBack1ColSet((void *)BACK_CRAM, back_color);
    slScrAutoDisp(NBG0ON | NBG1ON);
}

void                            jo_img_to_vdp2_cells(const jo_img_8bits * const img, bool vertical_flip)
{
    register int                x;
    register int                y;
    register int                cell_x;
    register int                cell_y;
    register int                line_y;
    register int                i;

#ifdef JO_DEBUG
        if (img == JO_NULL)
        {
            jo_core_error("img is null");
            return ;
        }
        if (JO_MOD_POW2(img->width, 8) != 0)
        {
            jo_core_error("%s: Image width must be multiple of 8");
            return ;
        }
        if (JO_MOD_POW2(img->height, 8) != 0)
        {
            jo_core_error("%s: Image height must be multiple of 8");
            return ;
        }
#endif
    for (JO_ZERO(x); x < img->width; x += CELL_WIDTH)
    {
        for (JO_ZERO(y), JO_ZERO(line_y); y < img->height; y += CELL_HEIGHT, line_y += JO_MULT_BY_8(img->width))
        {
            cell_x = x;
            cell_y = line_y;
            for (JO_ZERO(i); i < CELL_SIZE; ++i, ++__jo_cell_adr, cell_y += img->width)
            {
                if (i != 0 && JO_MOD_POW2(i, CELL_WIDTH) == 0)
                {
                     ++cell_x;
                     cell_y = line_y;
                }
                if (!vertical_flip)
                    *__jo_cell_adr = img->data[(img->width - cell_x - 1) + cell_y];
                else
                    *__jo_cell_adr = img->data[cell_x + cell_y];
            }
        }
    }
}

static void                     __jo_create_map(const jo_img_8bits * const img, unsigned short *map, unsigned short palnum)
{
    register int                x;
    register int                y;
    register int                x2;
    register int                y2;
    register int                i;
    unsigned short              paloff;

    paloff = JO_MULT_BY_4096(palnum);
    y = JO_DIV_BY_8(img->height);
    x = JO_DIV_BY_8(img->width);
    JO_ZERO(y2);
    JO_ZERO(x2);
    for (JO_ZERO(i); i < MAP_LENGTH; ++i)
    {
        if (i != 0 && JO_MOD_POW2(i, CELL_COUNT_PER_ROW) == 0)
            ++x2;
        if (x2 >= x)
            JO_ZERO(x2);
        *map = (JO_MULT_BY_2(y2 + x2 * y) | paloff) + __jo_cell_mapoff;
            ++map;
        ++y2;
        if (y2 >= y)
            JO_ZERO(y2);
    }
    __jo_cell_mapoff += JO_DIV_BY_32(__jo_cell_adr - ((unsigned char *)RBG0_CEL_ADR));
}

void                            jo_background_3d_plane_begin_setup_a(bool repeat)
{
    slPlaneRA(PL_SIZE_1x1);
	sl1MapRA((unsigned short *)RBG0RA_MAP_ADR);
	slOverRA(repeat ? RBG0_OVER_MODE_REPEAT : RBG0_OVER_MODE_SINGLE);
	slKtableRA((void *)RBG0_KTB_ADR, K_FIX | K_DOT | K_2WORD | K_ON | K_LINECOL);
}

void                            jo_background_3d_plane_a_img(jo_img_8bits *img, int palette_id, bool repeat, bool vertical_flip)
{
    jo_background_3d_plane_begin_setup_a(repeat);
    jo_img_to_vdp2_cells(img, vertical_flip);
	__jo_create_map(img, (unsigned short *)RBG0RA_MAP_ADR, palette_id);
}

void                            jo_background_3d_plane_begin_setup_b(bool repeat)
{
    slPlaneRB(PL_SIZE_1x1);
	sl1MapRB((void *)RBG0RB_MAP_ADR);
	slOverRB(repeat ? RBG0_OVER_MODE_REPEAT : RBG0_OVER_MODE_SINGLE);
	slKtableRB((void *)RBG0_KTB_ADR, K_FIX | K_DOT | K_2WORD | K_ON | K_LINECOL);
}

void                            jo_background_3d_plane_b_img(jo_img_8bits *img, int palette_id, bool repeat, bool vertical_flip)
{
    jo_background_3d_plane_begin_setup_b(repeat);
    jo_img_to_vdp2_cells(img, vertical_flip);
	__jo_create_map(img, (unsigned short *)RBG0RB_MAP_ADR, palette_id);
}

void                            jo_reset_background_3d_plane(void)
{
    __jo_cell_adr = (unsigned char *)RBG0_CEL_ADR;
}

void                            jo_enable_background_3d_plane(jo_color background_color)
{
    jo_reset_background_3d_plane();
    slRparaInitSet((void *)RBG0_PRA_ADR);
    slMakeKtable((void *)RBG0_KTB_ADR);
    slCharRbg0(COL_TYPE_256, CHAR_SIZE_1x1);
    slPageRbg0(__jo_cell_adr, 0, PNB_1WORD | CN_12BIT);
    slRparaMode(K_CHANGE);
    slBack1ColSet((void *)BACK_COL_ADR, background_color);
    slScrAutoDisp(NBG0ON | NBG1ON | RBG0ON);
}

void                            jo_disable_background_3d_plane(jo_color background_color)
{
    jo_clear_background(background_color);
    slScrAutoDisp(NBG0ON | NBG1ON);
}

void                            jo_draw_background_line(int x0, int y0, int x1, int y1, const jo_color color)
{
    int                         dx;
    int                         sx;
    int                         dy;
    int                         sy;
    int                         err;
    int                         e2;

    dx = JO_ABS(x1 - x0);
    sx = x0 < x1 ? 1 : -1;
    dy = JO_ABS(y1 - y0);
    sy = y0 < y1 ? 1 : -1;
    if (dx == dy)
        JO_ZERO(err);
    else
        err = JO_DIV_BY_2(dx > dy ? dx : -dy);
    for (;;)
    {
        jo_put_pixel_in_background(x0, y0, color);
        if (x0 == x1 && y0 == y1)
            break;
        e2 = err;
        if (e2 > -dx)
        {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dy)
        {
            err += dx;
            y0 += sy;
        }
    }
}

void                            jo_clear_background(const jo_color color)
{
    register jo_color           *vram_ptr;
    register int                i;
    jo_color                    buf[JO_VDP2_WIDTH];

    for (JO_ZERO(i); i < JO_VDP2_WIDTH; ++i)
        buf[i] = color;
    vram_ptr = (jo_color *)VDP2_VRAM_A0;
    for (JO_ZERO(i); i < JO_VDP2_HEIGHT; ++i)
    {
        jo_dma_copy(buf, vram_ptr, JO_VDP2_WIDTH * sizeof(color));
        vram_ptr += JO_VDP2_WIDTH;
    }
}

void			                jo_set_background_8bits_sprite(jo_img_8bits *img, int palette_id, bool vertical_flip)
{
    __jo_cell_adr = (unsigned char *)JO_VDP2_NBG1_CEL_ADR;
	slPageNbg1(__jo_cell_adr, 0, PNB_1WORD | CN_12BIT);
    slCharNbg1(COL_TYPE_256, CHAR_SIZE_1x1);
    jo_img_to_vdp2_cells(img, vertical_flip);
	__jo_create_map(img, (unsigned short *)JO_VDP2_NBG1_MAP_ADR, palette_id);
}

void			                jo_set_background_sprite(const jo_img *const img, const unsigned short left, const unsigned short top)
{
    register int                y;
    register jo_color           *vram_ptr;
    register jo_color           *img_ptr;

    slBitMapNbg1(COL_TYPE_32768, JO_VDP2_SIZE, (void *)VDP2_VRAM_A0);
    if (top)
        vram_ptr = ((jo_color *)VDP2_VRAM_A0) + (JO_VDP2_WIDTH * top);
    else
        vram_ptr = (jo_color *)VDP2_VRAM_A0;
    img_ptr = img->data;
    for (JO_ZERO(y); y < img->height; ++y)
    {
        if (left)
            vram_ptr += left;
        jo_dma_copy(img_ptr, vram_ptr, img->width * sizeof(*img_ptr));
        if (left)
            vram_ptr += JO_VDP2_WIDTH - left;
        else
            vram_ptr += JO_VDP2_WIDTH;
        img_ptr += img->width;
    }
}

void                            jo_disable_background_horizontal_line_scroll(void)
{
    jo_memset((int *)LINE_TBL_ADR, 0, 2048);
}

void                            jo_compute_background_horizontal_line_scroll(unsigned short offset)
{
    int                         *scroll_table;

    scroll_table = ((int *)LINE_TBL_ADR) + offset;
    slLineScrollTable1(scroll_table);
}

int                             *jo_enable_background_horizontal_line_scroll(void)
{
    int                         *scroll_table;

    scroll_table = (int *)LINE_TBL_ADR;
    slLineScrollModeNbg1(lineSZ1 | lineHScroll);
    slLineScrollTable1(scroll_table);
    return (scroll_table);
}

/*
** END OF FILE
*/
