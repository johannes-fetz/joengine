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
#include "jo/vdp2.h"
#include "jo/math.h"
#include "jo/palette.h"
#include "jo/vdp2_malloc.h"

/*
** MACROS
*/

#define CELL_WIDTH              (8)
#define CELL_HEIGHT             (8)
#define CELL_SIZE               (CELL_WIDTH * CELL_HEIGHT)
#define CELL_COUNT_PER_ROW      (64)
#define MAP_LENGTH              (512 * 8)

#define RBG0_OVER_MODE_REPEAT   (0)
#define RBG0_OVER_MODE_SINGLE   (3)

void                            jo_vdp2_malloc_init(void);

#ifdef JO_COMPILE_WITH_PRINTF_SUPPORT
static void                     jo_init_nbg0_printf(void)
{
    slCharNbg0(COL_TYPE_256, CHAR_SIZE_1x1);
    jo_set_printf_palette_color(JO_COLOR_INDEX_White, JO_COLOR_White);
    jo_set_printf_palette_color(JO_COLOR_INDEX_Black, JO_COLOR_Black);
    jo_set_printf_palette_color(JO_COLOR_INDEX_Red, JO_COLOR_Red);
    jo_set_printf_palette_color(JO_COLOR_INDEX_Green, JO_COLOR_Green);
    jo_set_printf_palette_color(JO_COLOR_INDEX_Yellow, JO_COLOR_Yellow);
    jo_set_printf_palette_color(JO_COLOR_INDEX_Blue, JO_COLOR_Blue);
    jo_set_printf_palette_color(JO_COLOR_INDEX_Purple, JO_COLOR_Purple);
    jo_set_printf_color_index(0);
}
#endif

//NBG0
static unsigned short               *nbg0_map = JO_NULL;
static unsigned char                *nbg0_cell = JO_NULL;
static int                          *nbg0_scroll_table = JO_NULL;
//NBG1
static jo_color                     *nbg1_bitmap = JO_NULL;
static unsigned short               *nbg1_map = JO_NULL;
static unsigned char                *nbg1_cell = JO_NULL;
static int                          *nbg1_scroll_table = JO_NULL;
//RBG0
static unsigned char                *rbg0_cell_a = JO_NULL;
static unsigned char                *rbg0_cell_b = JO_NULL;
static unsigned short               *rbg0_map_a = JO_NULL;
static unsigned short               *rbg0_map_b = JO_NULL;
static void                         *rbg0_ktable = JO_NULL;
static void                         *rbg0_rtable = JO_NULL;
//OTHER
static unsigned int                 screen_flags = 0;
static void                         *back_color = JO_NULL;

void                            jo_set_default_background_color(const jo_color background_color)
{
    if (back_color == JO_NULL)
        back_color = jo_vdp2_malloc_autosize(JO_VDP2_RAM_BACK_COLOR);
    slBack1ColSet(back_color, background_color);
}

void                            __jo_init_vdp2(const jo_color background_color)
{
    jo_vdp2_malloc_init();
    //NBG0
#ifdef JO_COMPILE_WITH_PRINTF_SUPPORT
    jo_init_nbg0_printf();
    JO_SET_FLAGS(screen_flags, NBG0ON | NBG1ON);
#else
    JO_SET_FLAGS(screen_flags, NBG1ON);
#endif
    nbg1_bitmap = jo_vdp2_malloc(JO_VDP2_RAM_BITMAP, JO_VDP2_WIDTH * JO_VDP2_HEIGHT * sizeof(*nbg1_bitmap));
    slBitMapNbg1(COL_TYPE_32768, JO_VDP2_SIZE, nbg1_bitmap);
    jo_set_default_background_color(background_color);
    slScrAutoDisp(screen_flags);
}

static  __jo_force_inline void  __jo_switch_to_bitmap_mode(void)
{
    if (nbg0_cell != JO_NULL)
    {
        jo_vdp2_free(nbg0_cell);
        nbg0_cell = JO_NULL;
    }
    if (nbg0_map != JO_NULL)
    {
        jo_vdp2_free(nbg0_map);
        nbg0_map = JO_NULL;
    }
    if (nbg1_cell != JO_NULL)
    {
        jo_vdp2_free(nbg1_cell);
        nbg1_cell = JO_NULL;
    }
    if (nbg1_map != JO_NULL)
    {
        jo_vdp2_free(nbg1_map);
        nbg1_map = JO_NULL;
    }
    if (rbg0_cell_b != JO_NULL)
    {
        jo_vdp2_free(rbg0_cell_b);
        rbg0_cell_b = JO_NULL;
    }
    if (rbg0_cell_a != JO_NULL)
    {
        jo_vdp2_free(rbg0_cell_a);
        rbg0_cell_a = JO_NULL;
    }
    if (rbg0_map_a != JO_NULL)
    {
        jo_vdp2_free(rbg0_map_a);
        rbg0_map_a = JO_NULL;
    }
    if (rbg0_map_b != JO_NULL)
    {
        jo_vdp2_free(rbg0_map_b);
        rbg0_map_b = JO_NULL;
    }
}

static  __jo_force_inline void  __jo_switch_to_8bits_mode(void)
{
    if (nbg1_bitmap != JO_NULL)
    {
        jo_vdp2_free(nbg1_bitmap);
        nbg1_bitmap = JO_NULL;
    }
}

void                            jo_img_to_vdp2_cells(const jo_img_8bits * const img, bool vertical_flip, unsigned char *cell)
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
            jo_core_error("Image width must be multiple of 8");
            return ;
        }
        if (JO_MOD_POW2(img->height, 8) != 0)
        {
            jo_core_error("Image height must be multiple of 8");
            return ;
        }
#endif
    for (JO_ZERO(x); x < img->width; x += CELL_WIDTH)
    {
        for (JO_ZERO(y), JO_ZERO(line_y); y < img->height; y += CELL_HEIGHT, line_y += JO_MULT_BY_8(img->width))
        {
            cell_x = x;
            cell_y = line_y;
            for (JO_ZERO(i); i < CELL_SIZE; ++i, ++cell, cell_y += img->width)
            {
                if (i != 0 && JO_MOD_POW2(i, CELL_WIDTH) == 0)
                {
                     ++cell_x;
                     cell_y = line_y;
                }
                if (!vertical_flip)
                    *cell = img->data[(img->width - cell_x - 1) + cell_y];
                else
                    *cell = img->data[cell_x + cell_y];
            }
        }
    }
}

static void                     __jo_create_map(const jo_img_8bits * const img, unsigned short *map, unsigned short palnum, int map_offset)
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
        *map = (JO_MULT_BY_2(y2 + x2 * y) | paloff) + map_offset;
            ++map;
        ++y2;
        if (y2 >= y)
            JO_ZERO(y2);
    }
}

void                            jo_vdp2_set_rbg0_plane_a_8bits_image(jo_img_8bits *img, int palette_id, bool repeat, bool vertical_flip)
{
    slPlaneRA(PL_SIZE_1x1);
    if (rbg0_map_a == JO_NULL)
        rbg0_map_a = (unsigned short *)jo_vdp2_malloc(JO_VDP2_RAM_MAP, JO_VDP2_MAP_SIZE);
    sl1MapRA(rbg0_map_a);
	slOverRA(repeat ? RBG0_OVER_MODE_REPEAT : RBG0_OVER_MODE_SINGLE);
	slKtableRA(rbg0_ktable, K_FIX | K_DOT | K_2WORD | K_ON | K_LINECOL);
    if (rbg0_cell_b != JO_NULL)
        jo_vdp2_free(rbg0_cell_a);
    else
    {
        rbg0_cell_a = (unsigned char *)jo_vdp2_malloc(JO_VDP2_RAM_CELL, img->width * img->height);
        if (rbg0_cell_b == JO_NULL)
            slPageRbg0(rbg0_cell_a, 0, PNB_1WORD | CN_12BIT);
    }
    jo_img_to_vdp2_cells(img, vertical_flip, rbg0_cell_a);
	__jo_create_map(img, rbg0_map_a, palette_id, JO_VDP2_CELL_TO_MAP_OFFSET(rbg0_cell_a));
}

void                            jo_vdp2_set_rbg0_plane_b_8bits_image(jo_img_8bits *img, int palette_id, bool repeat, bool vertical_flip)
{
    slPlaneRB(PL_SIZE_1x1);
    if (rbg0_map_b == JO_NULL)
        rbg0_map_b = (unsigned short *)jo_vdp2_malloc(JO_VDP2_RAM_MAP, JO_VDP2_MAP_SIZE);
    sl1MapRB(rbg0_map_b);
	slOverRB(repeat ? RBG0_OVER_MODE_REPEAT : RBG0_OVER_MODE_SINGLE);
	slKtableRB(rbg0_ktable, K_FIX | K_DOT | K_2WORD | K_ON | K_LINECOL);
    if (rbg0_cell_b != JO_NULL)
        jo_vdp2_free(rbg0_cell_b);
    else
    {
        rbg0_cell_b = (unsigned char *)jo_vdp2_malloc(JO_VDP2_RAM_CELL, img->width * img->height);
        if (rbg0_cell_a == JO_NULL)
            slPageRbg0(rbg0_cell_b, 0, PNB_1WORD | CN_12BIT);
    }
    jo_img_to_vdp2_cells(img, vertical_flip, rbg0_cell_b);
	__jo_create_map(img, rbg0_map_b, palette_id, JO_VDP2_CELL_TO_MAP_OFFSET(rbg0_cell_b));
}

void                            jo_vdp2_enable_rbg0(void)
{
    __jo_switch_to_8bits_mode();
    if (rbg0_rtable == JO_NULL)
        rbg0_rtable = jo_vdp2_malloc_autosize(JO_VDP2_RAM_RTABLE);
    slRparaInitSet(rbg0_rtable);
    if (rbg0_ktable == JO_NULL)
        rbg0_ktable = jo_vdp2_malloc_autosize(JO_VDP2_RAM_KTABLE);
    slMakeKtable(rbg0_ktable);
    slCharRbg0(COL_TYPE_256, CHAR_SIZE_1x1);
    slRparaMode(K_CHANGE);
    JO_ADD_FLAG(screen_flags, RBG0ON);
    slScrAutoDisp(screen_flags);
}

void                            jo_vdp2_disable_rbg0(void)
{
    JO_REMOVE_FLAG(screen_flags, RBG0ON);
    slScrAutoDisp(screen_flags);
}

void                            jo_vdp2_draw_bitmap_nbg1_line(int x0, int y0, int x1, int y1, const jo_color color)
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
        jo_vdp2_put_pixel_bitmap_nbg1(x0, y0, color);
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

void                            jo_vdp2_clear_bitmap_nbg1(const jo_color color)
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

void			                jo_vdp2_set_nbg1_8bits_image(jo_img_8bits *img, int palette_id, bool vertical_flip)
{
    __jo_switch_to_8bits_mode();
    if (nbg1_cell != JO_NULL)
        jo_vdp2_free(nbg1_cell);
    slPlaneNbg1(PL_SIZE_1x1);
    slCharNbg1(COL_TYPE_256, CHAR_SIZE_1x1);
    if (nbg1_map == JO_NULL)
        nbg1_map = (unsigned short *)jo_vdp2_malloc(JO_VDP2_RAM_MAP, JO_VDP2_MAP_SIZE);
    slMapNbg1(nbg1_map, nbg1_map, nbg1_map, nbg1_map);
    nbg1_cell = (unsigned char *)jo_vdp2_malloc(JO_VDP2_RAM_CELL, img->width * img->height);
	slPageNbg1(nbg1_cell, 0, PNB_1WORD | CN_12BIT);
    jo_img_to_vdp2_cells(img, vertical_flip, nbg1_cell);
	__jo_create_map(img, nbg1_map, palette_id, JO_VDP2_CELL_TO_MAP_OFFSET(nbg1_cell));
}

void			                jo_set_nbg0_8bits_image(jo_img_8bits *img, int palette_id, bool vertical_flip)
{
    __jo_switch_to_8bits_mode();
    if (nbg0_cell != JO_NULL)
        jo_vdp2_free(nbg0_cell);
    slPlaneNbg0(PL_SIZE_1x1);
    slCharNbg0(COL_TYPE_256, CHAR_SIZE_1x1);
    if (nbg0_map == JO_NULL)
        nbg0_map = (unsigned short *)jo_vdp2_malloc(JO_VDP2_RAM_MAP2, JO_VDP2_MAP_SIZE);
    slMapNbg0(nbg0_map, nbg0_map, nbg0_map, nbg0_map);
    nbg0_cell = (unsigned char *)jo_vdp2_malloc(JO_VDP2_RAM_CELL2, img->width * img->height);
	slPageNbg0(nbg0_cell, 0, PNB_1WORD | CN_12BIT);
    jo_img_to_vdp2_cells(img, vertical_flip, nbg0_cell);
	__jo_create_map(img, nbg0_map, palette_id, JO_VDP2_CELL_TO_MAP_OFFSET(nbg0_cell));
	JO_ADD_FLAG(screen_flags, NBG0ON);
    slScrAutoDisp(screen_flags);
}

void			                jo_vdp2_set_nbg1_image(const jo_img *const img, const unsigned short left, const unsigned short top)
{
    register int                y;
    register jo_color           *vram_ptr;
    register jo_color           *img_ptr;

    __jo_switch_to_bitmap_mode();
    if (nbg1_bitmap == JO_NULL)
        nbg1_bitmap = jo_vdp2_malloc(JO_VDP2_RAM_BITMAP, JO_VDP2_WIDTH * JO_VDP2_HEIGHT * sizeof(*nbg1_bitmap));
    slBitMapNbg1(COL_TYPE_32768, JO_VDP2_SIZE, nbg1_bitmap);
    if (top)
        vram_ptr = nbg1_bitmap + (JO_VDP2_WIDTH * top);
    else
        vram_ptr = nbg1_bitmap;
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

void                            jo_vdp2_disable_nbg1_line_scroll(void)
{
    if (nbg1_scroll_table != JO_NULL)
    {
        jo_vdp2_free(nbg1_scroll_table);
        nbg1_scroll_table = JO_NULL;
    }
}

void                            jo_vdp2_compute_nbg1_line_scroll(unsigned short offset)
{
    slLineScrollTable1(nbg1_scroll_table + offset);
}

int                             *jo_vdp2_enable_nbg1_line_scroll(void)
{
    __jo_switch_to_8bits_mode();
    if (nbg0_scroll_table == JO_NULL)
        nbg0_scroll_table = jo_vdp2_malloc(JO_VDP2_RAM_LINE_SCROLL, JO_NBG1_SCROLL_TABLE_SIZE * sizeof(int));
    slLineScrollModeNbg1(lineSZ1 | lineHScroll);
    slLineScrollTable1(nbg0_scroll_table);
    return (nbg0_scroll_table);
}

void                            jo_vdp2_disable_nbg0_line_scroll(void)
{
    if (nbg0_scroll_table != JO_NULL)
    {
        jo_vdp2_free(nbg0_scroll_table);
        nbg0_scroll_table = JO_NULL;
    }
}

void                            jo_vdp2_compute_nbg0_line_scroll(unsigned short offset)
{
    slLineScrollTable0(nbg0_scroll_table + offset);
}

int                             *jo_vdp2_enable_nbg0_line_scroll(void)
{
    __jo_switch_to_8bits_mode();
    if (nbg0_scroll_table == JO_NULL)
        nbg0_scroll_table = jo_vdp2_malloc(JO_VDP2_RAM_LINE_SCROLL, JO_NBG0_SCROLL_TABLE_SIZE * sizeof(int));
    slLineScrollModeNbg0(lineSZ1 | lineHScroll);
    slLineScrollTable0(nbg0_scroll_table);
    return (nbg0_scroll_table);
}

void      jo_disable_all_screens_transparency(void)
{
    slColorCalc(CC_RATE| CC_TOP);
    slColorCalcOn(0);
    slScrTransparent(0);
}

void      jo_enable_screen_transparency(const jo_scroll_screen screen, const unsigned short transparency_level)
{
    slColorCalc(CC_RATE| CC_TOP | screen);
    slColorCalcOn(screen);
    switch (screen)
    {
        case JO_NBG0_SCREEN: slColRateNbg0(transparency_level); break;
        case JO_NBG1_SCREEN: slColRateNbg1(transparency_level); break;
        case JO_NBG2_SCREEN: slColRateNbg2(transparency_level); break;
        case JO_NBG3_SCREEN: slColRateNbg3(transparency_level); break;
        case JO_RBG0_SCREEN: slColRateRbg0(transparency_level); break;
        case JO_BACK_SCREEN: slColRateBACK(transparency_level); break;
        case JO_SPRITE_SCREEN:
            slColRateSpr0(transparency_level);
            slColRateSpr1(transparency_level);
            slColRateSpr2(transparency_level);
            slColRateSpr3(transparency_level);
            slColRateSpr4(transparency_level);
            slColRateSpr5(transparency_level);
            slColRateSpr6(transparency_level);
            slColRateSpr7(transparency_level);
            break;
        default: break;
    }
    slScrTransparent(screen);
}

/*
** END OF FILE
*/
