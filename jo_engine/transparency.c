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
#include "jo/jo.h"
#include "internal/vdp1_command_pipeline.h"
#include "internal/vdp2.h"
#include "internal/3d.h"
#include "internal/sprites.h"

#ifdef JO_COMPILE_WITH_3D_SUPPORT

/*
** Shared by the transparency layer and the render to image: they draw in the part of the frame buffer that is not
** displayed (its right) in the off screen window (JO_3D_LAYER_WINDOW) and copy it with the level 1 of the SCU DMA when
** the VDP1 has finished a frame. Only one of them runs at a time.
*/

/** @brief SCU DMA level that reads the frame buffer (the engine uses the level 0 for the VDP1 command tables) */
# define READ_LEVEL             (1)
/** @brief Width of a line of the frame buffer (pixels) */
# define FB_WIDTH               JO_VDP1_FB_WIDTH
/** @brief Depth of the polygon that clears the off screen part: behind everything (the far limit of its window) */
# define CLEAR_DEPTH            (0x7FFF)

/** @brief A transfer of an indirect table of the SCU DMA (a row of the off screen part) */
typedef struct
{
    unsigned int                count;          // Bytes
    unsigned int                write;          // Write address
    unsigned int                read;           // Read address (| JO_SCU_DMA_INDIRECT_END: the last transfer)
}                               dma_transfer;

/** @brief The render to image is running (see jo_3d_render_to_image_init()) */
static bool                     render_running = false;

static  __jo_force_inline bool  scu_dma_busy(const int level)
{
    return ((JO_SCU_DMA_STATUS & JO_SCU_DMA_STATUS_BUSY(level)) != 0);
}

static void                     scu_dma_start(const int level, const dma_transfer * const table, const unsigned int add,
                                              const unsigned int mode, const unsigned int enable)
{
    JO_SCU_DMA_ENABLE(level) = 0;
    JO_SCU_DMA_WRITE(level) = JO_SCU_ADDRESS(table);
    JO_SCU_DMA_ADD(level) = add;
    JO_SCU_DMA_MODE(level) = mode;
    JO_SCU_DMA_ENABLE(level) = enable;
}

/** @brief Arm the level 1: it runs the indirect table when the VDP1 finishes its next frame
 *  @param table Indirect table (frame buffer to work RAM)
 */
static  __jo_force_inline void  arm_sprite_end_read(const dma_transfer * const table)
{
    // (the status of the sprite draw end interrupt cleared: masked, the interrupt is never accepted and its status stays set.
    // The SCU may start the DMA only when the status changes: Ymir never copied the layer again after the first frame, unless
    // a handler of this interrupt was installed, the profiler. The other bits are kept: a 1 written leaves a bit as it is)
    JO_SCU_IST = ~(unsigned int)JO_SCU_INT_SPRITE_END;
    scu_dma_start(READ_LEVEL, table, JO_SCU_DMA_ADD_READ4_WRITE4, JO_SCU_DMA_MODE_INDIRECT_SPRITE_END, JO_SCU_DMA_ENABLE_ONLY);
}

/** @brief The alignment of an indirect table of the SCU DMA: its size rounded up to a power of 2
 *  @param table_bytes Size of the table
 *  @return Alignment (bytes)
 */
static unsigned int             dma_table_alignment(const unsigned int table_bytes)
{
    unsigned int                align;

    for (align = sizeof(dma_transfer); align < table_bytes; align <<= 1)
        ;
    return (align);
}

/** @brief Allocate in the high work RAM (the SCU DMA does not reach the low work RAM)
 *  @param size Bytes
 *  @return The block, JO_NULL if out of high work RAM
 */
static void                     *high_work_ram_alloc(const unsigned int size)
{
    void                        *block;

    block = jo_malloc(size);
    if (block != JO_NULL && (JO_SCU_ADDRESS(block) & ~(JO_WORK_RAM_SIZE - 1)) != JO_WORK_RAM_HIGH)
    {
        jo_free(block);
        block = JO_NULL;
    }
    return (block);
}

/** @brief Clear a rectangle of the current window (a polygon of the color 0, behind everything)
 *  @param left Left from the center of the window (pixels)
 *  @param top Top from the center of the window
 *  @param right Right from the center of the window
 *  @param bottom Bottom from the center of the window
 */
static void                     clear_window_rect(const int left, const int top, const int right, const int bottom)
{
    jo_pos2D_fixed              corners[4];

    corners[0].x = jo_int2fixed(left); corners[0].y = jo_int2fixed(top);
    corners[1].x = jo_int2fixed(right); corners[1].y = corners[0].y;
    corners[2].x = corners[1].x; corners[2].y = jo_int2fixed(bottom);
    corners[3].x = corners[0].x; corners[3].y = corners[2].y;
    jo_sprite_draw_color_4p_fixed(corners, JO_COLOR_Transparent, jo_int2fixed(CLEAR_DEPTH), true);
}

/*
** The render to image: see jo/render_to_image.h. The rectangle is at the right of the frame buffer (x left to 511, y 0
** to height - 1), drawn by the off screen window; the level 1 of the SCU DMA copies it into the image (high work RAM)
** when the VDP1 finishes a frame, armed again at each VBLANK; the VBLANK copies the image into the texture of its
** sprite (if any). The image is the frame drawn before (the copy is done when the VDP1 has finished it)
*/

/** @brief No frame (see render_cleared_frame) */
# define RENDER_NO_FRAME        (0xFFFFFFFF)

static jo_img                   render_image;
static int                      render_width = 0;
static int                      render_height = 0;
static int                      render_sprite = -1;
static int                      render_vblank_event = -1;
static void                     *render_block = JO_NULL;
static dma_transfer             *render_table;
static bool                     render_in_image = false;
static unsigned int             render_cleared_frame = RENDER_NO_FRAME;
static int                      render_saved_window;
static unsigned short           render_saved_sprite_clipping;

/** @brief The left of the rectangle in the frame buffer: up to its right edge */
# define RENDER_LEFT            (FB_WIDTH - render_width)

/** @brief VBLANK: the image copied into the texture, the copy of the next frame armed */
static void                     jo_3d_render_to_image_vblank(void)
{
    if (!render_running)
        return;
    while (scu_dma_busy(READ_LEVEL))
        ;
    if (render_sprite >= 0)
        __jo_sprite_replace(&render_image, render_sprite, true);
    arm_sprite_end_read(render_table);
}

bool                            jo_3d_render_to_image_init(const int width, const int height)
{
    const unsigned int          image_bytes = (unsigned int)(width * height) * sizeof(jo_color);
    const unsigned int          table_bytes = (unsigned int)height * sizeof(dma_transfer);
    unsigned int                align;
    int                         y;

    jo_3d_render_to_image_stop();
    if (jo_3d_transparency_is_running())
    {
#ifdef JO_DEBUG
        jo_core_error("Render to image: stop the transparency first");
#endif
        return (false);
    }
    // (the frame buffer of the 640 and 704 pixels modes has 8 bits per pixel; interlaced: the VDP1 draws one field of 2)
    if (__jo_screen.hires || __jo_screen.interlace || width <= 0 || height <= 0 || (width & 1) != 0 ||
        width > FB_WIDTH - jo_core_get_screen_width() || height > jo_core_get_screen_height())
    {
#ifdef JO_DEBUG
        jo_core_error("Render to image: %dx%d not possible", width, height);
#endif
        return (false);
    }
    align = dma_table_alignment(table_bytes);
    if ((render_block = high_work_ram_alloc(image_bytes + align - 1 + align)) == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Render to image: out of high work RAM");
#endif
        return (false);
    }
    render_width = width;
    render_height = height;
    render_image.width = width;
    render_image.height = height;
    render_image.data = (jo_color *)render_block;
    jo_memset(render_image.data, 0, image_bytes);
    render_table = (dma_transfer *)(((unsigned int)render_block + image_bytes + align - 1) & ~(align - 1));
    for (JO_ZERO(y); y < height; ++y)
    {
        render_table[y].count = width * sizeof(jo_color);
        render_table[y].write = JO_SCU_ADDRESS(&render_image.data[y * width]);
        render_table[y].read = JO_SCU_ADDRESS(JO_VDP1_FB + (y * FB_WIDTH + RENDER_LEFT) * sizeof(jo_color));
    }
    render_table[height - 1].read |= JO_SCU_DMA_INDIRECT_END;
    // The VDP1 draws up to the right of the frame buffer; the windows of the screen stay on the screen
    jo_sprite_set_system_clipping_area(FB_WIDTH, jo_core_get_screen_height());
    __jo_vdp1_set_off_screen_window(true);
    render_cleared_frame = RENDER_NO_FRAME;
    render_vblank_event = jo_core_add_vblank_callback(jo_3d_render_to_image_vblank);
    arm_sprite_end_read(render_table);
    render_running = true;
    return (true);
}

int                             jo_3d_render_to_image_add_sprite(void)
{
    if (!render_running)
        return (-1);
    if (render_sprite < 0)
        render_sprite = jo_sprite_add(&render_image);
    return (render_sprite);
}

const jo_img                    *jo_3d_render_to_image_get(void)
{
    return (render_running ? &render_image : JO_NULL);
}

void                            jo_3d_render_to_image_begin(void)
{
    const unsigned int          frame = __jo_vdp1_frame_tag();

    if (!render_running || render_in_image)
    {
#ifdef JO_DEBUG
        if (render_in_image)
            jo_core_error("jo_3d_render_to_image_end() missing");
#endif
        return;
    }
    render_in_image = true;
    render_saved_window = __jo_vdp1_current_window;
    jo_3d_select_window(JO_3D_LAYER_WINDOW);
    jo_3d_window(RENDER_LEFT, 0, FB_WIDTH - 1, render_height - 1, CLEAR_DEPTH, RENDER_LEFT + render_width / 2, render_height / 2);
    jo_3d_window_clip(true);
    render_saved_sprite_clipping = __jo_sprite_attributes.clipping;
    jo_sprite_enable_clipping(false);
    if (render_cleared_frame != frame)
    {
        render_cleared_frame = frame;
        clear_window_rect(-render_width / 2, -render_height / 2, render_width - render_width / 2, render_height - render_height / 2);
    }
}

void                            jo_3d_render_to_image_end(void)
{
    if (!render_in_image)
        return;
    render_in_image = false;
    __jo_sprite_attributes.clipping = render_saved_sprite_clipping;
    jo_3d_select_window(render_saved_window);
}

void                            jo_3d_render_to_image_stop(void)
{
    if (!render_running)
        return;
    jo_3d_render_to_image_end();
    jo_core_remove_vblank_callback(render_vblank_event);
    render_vblank_event = -1;
    JO_SCU_DMA_ENABLE(READ_LEVEL) = 0;
    while (scu_dma_busy(READ_LEVEL))
        ;
    jo_sprite_set_system_clipping_area(jo_core_get_screen_width(), jo_core_get_screen_height());
    __jo_vdp1_set_off_screen_window(false);
    jo_free(render_block);
    render_block = JO_NULL;
    render_sprite = -1;
    render_running = false;
}

/** @brief The layer needs the manual change of the frame buffers (see the end of this file: without it, no layer) */
#if defined(JO_FRAMERATE_DYNAMIC) || JO_FRAMERATE > 1

/*
** The transparency layer: see jo/transparency.h.
**
** The layer is a rectangle of the frame buffer outside of the screen: x left to 511 (left = 512 - width), y 0 to
** height - 1. The screen w x h is drawn there at the scale s = scale / 256 (projection distance x s, the centers
** matching: center of the layer = s x center of the screen); the screen of the layer (NBG0 or NBG1) shows a 32768 colors
** bitmap with the coordinate increment s: the pixel (x, y) of the screen shows the pixel (s x, s y) of the bitmap, with a
** scroll of a fraction of pixel (the center of the projection of the VDP1 is a whole pixel). The bitmap is 512 pixels
** wide and starts at a bank of the VRAM; its rows below the layer are not displayed (other data may follow).
**
** Frames: jo_3d_transparency_begin() marks the frame being built (its number: __jo_vdp1_frame_tag()) and clears the
** layer (a polygon of the color 0 behind everything) at its first call in the frame. At the VBLANK where a frame is
** displayed (__jo_vdp1_displayed_frame_changed()): the frame buffers change at the end of this VBLANK, the frame drawn by
** the VDP1 is displayed from the next field; the level 1 of the SCU DMA has copied its layer into the work RAM when the
** VDP1 finished it (start factor "sprite draw end", armed at the previous change: before the change of the frame
** buffers, the frame buffer read by the SCU is the one just drawn). The level 2 copies it into the bitmap during
** the display: row 0 first, faster than the beam (a row of the layer: 2 lines of the screen at the scale 1/2). A frame
** that did not use the layer: the level 2 copies a row of zeros (transparent) into each row of the bitmap, once.
*/

/** @brief SCU DMA level that writes the bitmap of the screen of the layer */
# define WRITE_LEVEL            (2)
/** @brief The biggest screen of the layer: the widths of 320 and 352 pixels (the frame buffer of the 640 and 704 pixels
 *  modes has 8 bits per pixel), not interlaced (more lines: the VDP1 draws one field of 2 per frame buffer) */
# define MAX_SCREEN_WIDTH       (352)
# define MAX_SCREEN_HEIGHT      (256)
/** @brief The narrowest layer (pixels): coarser than that, its edges would be steps of 5 pixels */
# define MIN_LAYER_WIDTH        (64)
/** @brief The scale of the layer: 8 bits of fraction (SCALE_ONE: the size of the screen) */
# define SCALE_SHIFT            (8)
# define SCALE_ONE              (1 << SCALE_SHIFT)
/** @brief Frames remembered (see frame_used): more than the frames between the building and the display (3 in
 *  Mednafen), a power of 2 */
# define FRAME_SLOTS            (8)
/** @brief No frame (see cleared_frame) */
# define NO_FRAME               (0xFFFFFFFF)
/** @brief jo_fixed: 16 bits of fraction (JO_FIXED_1) */
# define FIXED_SHIFT            (16)
/** @brief The left of the layer in the frame buffer: up to its right edge */
# define LAYER_LEFT             (FB_WIDTH - layer_width)

static bool                     running = false;
static bool                     in_layer = false;
static int                      vblank_event = -1;
/** @brief The system clipping of the VDP1 widened to the frame buffer by jo_3d_transparency_init() (back to the screen
 *  by jo_3d_transparency_stop()) */
static bool                     system_clip_widened = false;
/** @brief The screen of the layer (NBG0 or NBG1), its bitmap (JO_NULL: none) and its rows */
static jo_scroll_screen         layer_screen = JO_NBG1_SCREEN;
static void                     *layer_bitmap = JO_NULL;
static int                      bitmap_rows = 0;
/** @brief The layer: its size in the frame buffer, its scale, its center (a whole pixel: the VDP1) */
static int                      layer_width = 0;
static int                      layer_height = 0;
static int                      layer_scale = SCALE_ONE;
static int                      layer_center_x = 0;
static int                      layer_center_y = 0;
/** @brief Scroll of the screen: the centers of the layer are whole pixels, the exact centers (screen x scale / 2) are not */
static jo_fixed                 layer_scroll_x = 0;
static jo_fixed                 layer_scroll_y = 0;
/** @brief Width asked to jo_3d_transparency_init() (0: default), the resolution of the screen of the layer */
static int                      requested_width = 0;
static int                      screen_width = 0;
static int                      screen_height = 0;
/** @brief The state before jo_3d_transparency_begin(): the projection, the window, the clipping of the sprites */
static jo_fixed                 saved_projection_distance;
static int                      saved_window;
static unsigned short           saved_sprite_clipping;
/** @brief One block of high work RAM (the SCU DMA does not reach the low work RAM), allocated by
 *  jo_3d_transparency_init() for the size of the layer: the copy of the layer, a row of zeros (each row of the bitmap is
 *  emptied with a copy of it: a fill that reads the same word, read add 0, writes nothing in Mednafen), the indirect tables
 *  of the SCU DMA (aligned on their size rounded up to a power of 2) */
static unsigned char            *block = JO_NULL;
static unsigned int             block_size = 0;
static jo_color                 *layer_copy;
static jo_color                 *zero_row;
static dma_transfer             *read_table;
static dma_transfer             *write_table;
/** @brief The frames that use the layer: their numbers (+ 1, 0: none), by frame number modulo FRAME_SLOTS */
static volatile unsigned int    frame_used[FRAME_SLOTS];
/** @brief Number of the frame whose layer is cleared */
static unsigned int             cleared_frame = NO_FRAME;
/** @brief The bitmap is empty (filled with 0), the write table reads the row of zeros (it empties the bitmap) */
static bool                     bitmap_empty = true;
static bool                     table_empties = false;
/** @brief The masks (see jo_3d_transparency_mask_begin()): the attributes of the polygons (untextured, color 0), the
 *  polygon data drawn with them, the color lookup table of the 16 colors sprites (all its colors 0: a sprite of its own,
 *  added once) */
static jo_3d_polygon_attribute  *mask_attributes = JO_NULL;
static unsigned int             mask_capacity = 0;
static jo_3d_polygon_data       mask_data;
static int                      mask_lut_sprite = -1;
static bool                     in_masks = false;

static void                     scu_dma_stop(void)
{
    JO_SCU_DMA_ENABLE(READ_LEVEL) = 0;
    while (scu_dma_busy(READ_LEVEL) || scu_dma_busy(WRITE_LEVEL))
        ;
    JO_SCU_DMA_ENABLE(WRITE_LEVEL) = 0;
}

/** @brief Arm the level 1: it copies the layer when the VDP1 finishes its next frame */
static  __jo_force_inline void  arm_read(void)
{
    arm_sprite_end_read(read_table);
}

/** @brief The rows read by the write table: the copy of the layer, or the row of zeros (the bitmap emptied) */
static void                     set_write_source(const bool zeros)
{
    int                         y;

    for (JO_ZERO(y); y < layer_height; ++y)
        write_table[y].read = JO_SCU_ADDRESS(zeros ? zero_row : &layer_copy[y * layer_width]);
    write_table[layer_height - 1].read |= JO_SCU_DMA_INDIRECT_END;
    table_empties = zeros;
}

/** @brief VBLANK: the copy of the layer of the frame displayed from the next field into the bitmap */
static void                     jo_3d_transparency_vblank(void)
{
    unsigned int                tag;
    volatile unsigned int       *used;

    if (!running || !__jo_vdp1_displayed_frame_changed(&tag))
        return;
    used = &frame_used[tag & (FRAME_SLOTS - 1)];
    // (the level 2 must not start while the level 1 runs: the VDP1 has just finished, its copy ends in less than 1 ms)
    while (scu_dma_busy(READ_LEVEL))
        ;
    if (*used == tag + 1)
    {
        *used = 0;
        if (table_empties)
            set_write_source(false);
        scu_dma_start(WRITE_LEVEL, write_table, JO_SCU_DMA_ADD_READ4_WRITE2, JO_SCU_DMA_MODE_INDIRECT, JO_SCU_DMA_ENABLE_START);
        bitmap_empty = false;
    }
    else if (!bitmap_empty)
    {
        // (the bitmap emptied once: the write table reads the row of zeros; the level 2 ended long ago)
        set_write_source(true);
        scu_dma_start(WRITE_LEVEL, write_table, JO_SCU_DMA_ADD_READ4_WRITE2, JO_SCU_DMA_MODE_INDIRECT, JO_SCU_DMA_ENABLE_START);
        bitmap_empty = true;
    }
    arm_read();
}

/** @brief The size and the scale of the layer for the resolution of the screen
 *  @param width Width asked (0: default)
 *  @return false if the resolution of the screen is not compatible: 640 or 704 pixels, interlaced (see jo/transparency.h)
 */
static bool                     compute_layer(int width)
{
    const int                   w = jo_core_get_screen_width();
    const int                   h = jo_core_get_screen_height();

    screen_width = w;
    screen_height = h;
    // The 640 and 704 pixels modes: the frame buffer of the VDP1 has 8 bits per pixel (and NBG1 has half the width); the
    // interlaced modes (more than 256 lines): the VDP1 draws one field of 2 per frame buffer, the frame buffers change at
    // each field
    if (__jo_screen.hires || __jo_screen.interlace || w > MAX_SCREEN_WIDTH || h > MAX_SCREEN_HEIGHT)
        return (false);
    if (width <= 0)
        width = JO_3D_TRANSPARENCY_DEFAULT_WIDTH;
    // The free width of the frame buffer: 192 pixels beside a screen of 320, 160 beside 352
    width = JO_MIN(width, JO_MIN(JO_3D_TRANSPARENCY_MAX_WIDTH, FB_WIDTH - w));
    // (even: the rows of the copy at multiples of 4 bytes, for the SCU DMA)
    width &= ~1;
    if (width < MIN_LAYER_WIDTH)
        return (false);
    layer_width = width;
    layer_scale = (width * SCALE_ONE) / w;
    if (((h * layer_scale + SCALE_ONE - 1) >> SCALE_SHIFT) > JO_3D_TRANSPARENCY_MAX_HEIGHT)
        layer_scale = (JO_3D_TRANSPARENCY_MAX_HEIGHT * SCALE_ONE) / h;
    if (layer_scale <= 0)
        return (false);
    layer_height = (h * layer_scale + SCALE_ONE - 1) >> SCALE_SHIFT;
    // The center of the projection in the layer: a whole pixel (the VDP1); the screen scrolled by the difference with the exact
    // center (in 352 x 240 at the scale 116 / 256: 79.75 x 54.375): the pixel (x, y) of the screen shows the pixel
    // (scale x + scroll, scale y + scroll) of the bitmap
    // (the center: half the size at the scale, rounded; the exact center in Q16: the size x the scale / 2)
    layer_center_x = (w * layer_scale + SCALE_ONE) >> (SCALE_SHIFT + 1);
    layer_center_y = (h * layer_scale + SCALE_ONE) >> (SCALE_SHIFT + 1);
    layer_scroll_x = jo_int2fixed(layer_center_x) - ((w * layer_scale) << (FIXED_SHIFT - SCALE_SHIFT - 1));
    layer_scroll_y = jo_int2fixed(layer_center_y) - ((h * layer_scale) << (FIXED_SHIFT - SCALE_SHIFT - 1));
    return (true);
}

/** @brief The block of the layer (see block) for its size: allocated again if it is too small
 *  @return false if out of high work RAM
 */
static bool                     alloc_block(void)
{
    const unsigned int          copy_bytes = (unsigned int)(layer_width * layer_height) * sizeof(jo_color);
    const unsigned int          zero_bytes = (unsigned int)layer_width * sizeof(jo_color);
    unsigned int                table_bytes;
    unsigned int                align;
    unsigned int                size;
    unsigned char               *tables;

    table_bytes = (unsigned int)layer_height * sizeof(dma_transfer);
    align = dma_table_alignment(table_bytes);
    // (the tables after the copy and the row, at the next multiple of their alignment)
    size = copy_bytes + zero_bytes + align - 1 + 2 * align;
    if (size > block_size)
    {
        if (block != JO_NULL)
            jo_free(block);
        block_size = 0;
        block = (unsigned char *)high_work_ram_alloc(size);
        if (block == JO_NULL)
            return (false);
        block_size = size;
    }
    layer_copy = (jo_color *)block;
    zero_row = (jo_color *)(block + copy_bytes);
    jo_memset(zero_row, 0, zero_bytes);
    tables = (unsigned char *)(((unsigned int)(block + copy_bytes + zero_bytes) + align - 1) & ~(align - 1));
    read_table = (dma_transfer *)tables;
    write_table = (dma_transfer *)(tables + align);
    return (true);
}

static void                     free_block(void)
{
    if (block != JO_NULL)
        jo_free(block);
    block = JO_NULL;
    block_size = 0;
}

static void                     build_tables(void)
{
    int                         y;

    for (JO_ZERO(y); y < layer_height; ++y)
    {
        read_table[y].count = layer_width * sizeof(jo_color);
        read_table[y].write = JO_SCU_ADDRESS(&layer_copy[y * layer_width]);
        read_table[y].read = JO_SCU_ADDRESS(JO_VDP1_FB + (y * FB_WIDTH + LAYER_LEFT) * sizeof(jo_color));
        write_table[y].count = layer_width * sizeof(jo_color);
        write_table[y].write = JO_SCU_ADDRESS((unsigned char *)layer_bitmap + y * JO_VDP2_WIDTH * sizeof(jo_color));
    }
    read_table[layer_height - 1].read |= JO_SCU_DMA_INDIRECT_END;
    set_write_source(false);
}

/** @brief Zoom and scroll of the screen of the layer */
static void                     set_screen_zoom(const jo_fixed zoom, const jo_fixed x, const jo_fixed y)
{
    if (layer_screen == JO_NBG0_SCREEN)
    {
        jo_vdp2_zoom_nbg0_fixed(zoom, zoom);
        jo_vdp2_move_nbg0_fixed(x, y);
    }
    else
    {
        jo_vdp2_zoom_nbg1_fixed(zoom, zoom);
        jo_vdp2_move_nbg1_fixed(x, y);
    }
}

/** @brief Give back the bitmap of the layer (NBG0: the text of jo_printf() again) */
static void                     free_bitmap(void)
{
    if (layer_bitmap == JO_NULL)
        return;
    jo_vdp2_disable_blend(layer_screen);
    set_screen_zoom(JO_FIXED_1, 0, 0);
    __jo_vdp2_free_rgb_bitmap(layer_screen, layer_bitmap);
    layer_bitmap = JO_NULL;
}

bool                            jo_3d_transparency_init(const int width, const jo_scroll_screen screen)
{
    unsigned short              sprites;
    bool                        was_running = running;

    if (render_running)
    {
# ifdef JO_DEBUG
        jo_core_error("Transparency: stop the render to image first");
# endif
        return (false);
    }
    if (running)
        scu_dma_stop();
    running = false;
    requested_width = width;
    if (screen != JO_NBG0_SCREEN && screen != JO_NBG1_SCREEN)
    {
# ifdef JO_DEBUG
        jo_core_error("Transparency: NBG0 or NBG1 only");
# endif
        jo_3d_transparency_stop();
        return (false);
    }
    // Another screen: the bitmap of the previous one is given back
    if (screen != layer_screen)
    {
        free_bitmap();
        layer_screen = screen;
        was_running = false;
    }
    if (!compute_layer(width))
    {
# ifdef JO_DEBUG
        // (40 characters per line on the screen of the errors)
        jo_core_error("Transparency: not in %dx%d", jo_core_get_screen_width(), jo_core_get_screen_height());
# endif
        jo_3d_transparency_stop();
        return (false);
    }
    // The bitmap of the screen: kept by a new call if it has enough rows (cleared), else allocated again
    if (layer_bitmap != JO_NULL && layer_height > bitmap_rows)
        free_bitmap();
    if (layer_bitmap != JO_NULL)
        jo_dma_fill32(layer_bitmap, 0, bitmap_rows * JO_VDP2_WIDTH * sizeof(jo_color));
    else if ((layer_bitmap = __jo_vdp2_set_rgb_bitmap(layer_screen, layer_height)) != JO_NULL)
        bitmap_rows = layer_height;
    else
    {
# ifdef JO_DEBUG
        jo_core_error("Transparency: NBG%d not available", layer_screen == JO_NBG0_SCREEN ? 0 : 1);
# endif
        jo_3d_transparency_stop();
        return (false);
    }
    if (!alloc_block())
    {
# ifdef JO_DEBUG
        jo_core_error("Transparency: out of high work RAM");
# endif
        jo_3d_transparency_stop();
        return (false);
    }
    build_tables();
    // The VDP1 draws up to the right of the frame buffer; the 3D of the screen stays in the screen
    jo_sprite_set_system_clipping_area(FB_WIDTH, jo_core_get_screen_height());
    system_clip_widened = true;
    // (the windows of the screen stay on the screen: only the window of the layer draws beyond it)
    __jo_vdp1_set_off_screen_window(true);
    jo_3d_select_window(JO_3D_NEAR_WINDOW);
    jo_3d_window_clip(true);
    // The 32768 colors bitmap, empty, zoomed to the screen, above the sprites
    set_screen_zoom(layer_scale << (FIXED_SHIFT - SCALE_SHIFT), layer_scroll_x, layer_scroll_y);
    sprites = __jo_vdp2_get_priority(JO_SPRITE_SCREEN);
    if (sprites >= JO_VDP2_MAX_PRIORITY)
    {
        sprites = JO_VDP2_MAX_PRIORITY - 1;
        __jo_vdp2_set_priority(JO_SPRITE_SCREEN, sprites);
    }
    __jo_vdp2_set_priority(layer_screen, sprites + 1);
    if (!was_running)
        jo_3d_transparency_set_blend(JO_VDP2_BLEND_RATIO, JO_3D_TRANSPARENCY_DEFAULT_RATIO);
    jo_memset((void *)frame_used, 0, sizeof(frame_used));
    cleared_frame = NO_FRAME;
    bitmap_empty = true;
    if (vblank_event < 0)
        vblank_event = jo_core_add_vblank_callback(jo_3d_transparency_vblank);
    arm_read();
    running = true;
    return (true);
}

void                            jo_3d_transparency_stop(void)
{
    if (vblank_event >= 0)
    {
        jo_core_remove_vblank_callback(vblank_event);
        vblank_event = -1;
    }
    if (system_clip_widened)
    {
        jo_sprite_set_system_clipping_area(jo_core_get_screen_width(), jo_core_get_screen_height());
        system_clip_widened = false;
    }
    if (!running && block == JO_NULL && layer_bitmap == JO_NULL)
        return;
    scu_dma_stop();
    running = false;
    in_layer = false;
    __jo_vdp1_set_off_screen_window(false);
    free_bitmap();
    free_block();
    if (mask_attributes != JO_NULL)
    {
        jo_free(mask_attributes);
        mask_attributes = JO_NULL;
        mask_capacity = 0;
    }
}

void                            jo_3d_transparency_set_blend(const jo_vdp2_blend_mode mode, const unsigned char ratio)
{
    jo_vdp2_set_blend_mode(mode);
    jo_vdp2_enable_blend(layer_screen, ratio);
}

void                            jo_3d_transparency_begin(void)
{
    const unsigned int          frame = __jo_vdp1_frame_tag();

    // (a jo_3d_transparency_end() missing: the layer closed first, else everything after it was drawn in the layer, frame
    // after frame)
    if (in_layer)
    {
#ifdef JO_DEBUG
        jo_core_error("jo_3d_transparency_end() missing");
#endif
        jo_3d_transparency_end();
    }
    if (!running)
        return;
    // The resolution of the screen changed: the layer follows it (or stops if it is not compatible)
    if ((jo_core_get_screen_width() != screen_width || jo_core_get_screen_height() != screen_height) &&
        !jo_3d_transparency_init(requested_width, layer_screen))
        return;
    in_layer = true;
    saved_window = __jo_vdp1_current_window;
    jo_3d_select_window(JO_3D_LAYER_WINDOW);
    jo_3d_transparency_set_clip(JO_NULL);
    jo_3d_window_clip(true);
    // The sprites drawn in the layer (billboards) clipped to it too: not on the screen beside it
    saved_sprite_clipping = __jo_sprite_attributes.clipping;
    jo_sprite_enable_clipping(false);
    if (cleared_frame != frame)
    {
        // The first use in this frame: clear the layer (color 0: transparent for the VDP2), behind everything. The
        // polygon is relative to the center of the window
        cleared_frame = frame;
        frame_used[frame & (FRAME_SLOTS - 1)] = frame + 1;
        clear_window_rect(-layer_center_x, -layer_center_y, layer_width - layer_center_x, layer_height - layer_center_y);
    }
    // The same view at the scale of the layer
    saved_projection_distance = jo_3d_get_projection_distance();
    __jo_vdp1_projection_distance = (jo_fixed)(((long long)saved_projection_distance * layer_scale) >> SCALE_SHIFT);
    __jo_vdp1_update_near_clip();
}

void                            jo_3d_transparency_end(void)
{
    if (!in_layer)
        return;
    jo_3d_transparency_mask_end();
    in_layer = false;
    __jo_vdp1_projection_distance = saved_projection_distance;
    __jo_vdp1_update_near_clip();
    __jo_sprite_attributes.clipping = saved_sprite_clipping;
    jo_3d_select_window(saved_window);
}

void                            jo_3d_transparency_set_clip(const jo_3d_rect * const rect)
{
    int                         left;
    int                         top;
    int                         right;
    int                         bottom;

    if (!in_layer)
        return;
    if (rect == JO_NULL)
    {
        left = 0;
        top = 0;
        right = layer_width - 1;
        bottom = layer_height - 1;
    }
    else
    {
        // The pixels of the layer that show the rectangle (see compute_layer(): the pixel x of the screen shows the pixel
        // x * scale + scroll of the layer), the edges in it
        left = (int)((((long long)rect->left * layer_scale << (FIXED_SHIFT - SCALE_SHIFT)) + layer_scroll_x) >> FIXED_SHIFT);
        top = (int)((((long long)rect->top * layer_scale << (FIXED_SHIFT - SCALE_SHIFT)) + layer_scroll_y) >> FIXED_SHIFT);
        right = (int)((((long long)(rect->right + 1) * layer_scale << (FIXED_SHIFT - SCALE_SHIFT)) + layer_scroll_x + JO_FIXED_1 - 1) >>
                      FIXED_SHIFT) - 1;
        bottom = (int)((((long long)(rect->bottom + 1) * layer_scale << (FIXED_SHIFT - SCALE_SHIFT)) + layer_scroll_y + JO_FIXED_1 - 1) >>
                       FIXED_SHIFT) - 1;
        left = JO_MAX(left, 0);
        top = JO_MAX(top, 0);
        right = JO_MIN(right, layer_width - 1);
        bottom = JO_MIN(bottom, layer_height - 1);
    }
    // (an empty rectangle: one pixel of the layer, nothing in it is seen)
    if (right < left || bottom < top)
    {
        right = left = JO_MIN(left, layer_width - 1);
        bottom = top = JO_MIN(top, layer_height - 1);
    }
    jo_3d_window(LAYER_LEFT + left, top, LAYER_LEFT + right, bottom, CLEAR_DEPTH, LAYER_LEFT + layer_center_x, layer_center_y);
}

/** @brief The polygon data of a mesh drawn as a mask (see __jo_3d_master_polygon_filter): its polygons untextured, of the
 *  color 0 (they erase what they cover in the layer), the same faces and the same sort
 *  @param pdata Polygon data of the mesh
 *  @return The polygon data of the mask, JO_NULL if out of memory (not drawn)
 */
static const jo_3d_polygon_data *mask_filter(const jo_3d_polygon_data * const pdata)
{
    jo_3d_polygon_attribute     *a;
    unsigned int                i;

    if (pdata->polygon_count > mask_capacity)
    {
        if (mask_attributes != JO_NULL)
            jo_free(mask_attributes);
        mask_capacity = pdata->polygon_count;
        if ((mask_attributes = (jo_3d_polygon_attribute *)jo_malloc(mask_capacity * sizeof(*mask_attributes))) == JO_NULL)
        {
            mask_capacity = 0;
# ifdef JO_DEBUG
            jo_core_error("Transparency: out of memory (masks)");
# endif
            return (JO_NULL);
        }
        for (JO_ZERO(i); i < mask_capacity; ++i)
        {
            a = &mask_attributes[i];
            a->texture_id = JO_3D_NO_TEXTURE;
            a->draw_mode = (JO_VDP1_COLOR_MODE_RGB | JO_VDP1_NO_GOURAUD) | JO_VDP1_DIRECTION_DRAW_MODE(JO_VDP1_POLYGON);
            a->color = JO_COLOR_Transparent;
            a->gouraud_table = JO_VDP1_NO_GOURAUD;
            a->direction = JO_VDP1_DIRECTION_COMMAND(JO_VDP1_POLYGON);
        }
    }
    for (JO_ZERO(i); i < pdata->polygon_count; ++i)
    {
        mask_attributes[i].plane = pdata->attributes[i].plane;
        mask_attributes[i].sort = (pdata->attributes[i].sort & JO_3D_SORT_MASK) | JO_VDP1_DIRECTION_OPTIONS(JO_VDP1_POLYGON) | JO_3D_NO_OPTION;
    }
    mask_data = *pdata;
    mask_data.attributes = mask_attributes;
    return (&mask_data);
}

/** @brief Forget the sprite of the masks if it is freed (see __jo_3d_transparency_sprites_freed): added again by the next
 *  jo_3d_transparency_mask_begin()
 *  @param sprite_id First sprite freed
 */
static void                     transparency_sprites_freed(const int sprite_id)
{
    if (mask_lut_sprite >= sprite_id)
        mask_lut_sprite = -1;
}

void                            jo_3d_transparency_mask_begin(void)
{
    static const jo_color       zeros[16] = {0};
    static const unsigned char  pixels[4] = {0};
    jo_color                    *lut;

    if (!in_layer || in_masks)
    {
#ifdef JO_DEBUG
        if (running)
            jo_core_error(in_masks ? "jo_3d_transparency_mask_end() missing" : "jo_3d_transparency_begin() missing");
#endif
        return;
    }
    if (mask_lut_sprite < 0)
    {
        mask_lut_sprite = jo_sprite_add_4bits_lut(pixels, 8, 1, zeros);
        __jo_3d_transparency_sprites_freed = transparency_sprites_freed;
    }
    in_masks = true;
    __jo_3d_master_polygon_filter = mask_filter;
    if (mask_lut_sprite >= 0)
    {
        lut = jo_sprite_get_lut(mask_lut_sprite);
        __jo_sprite_attributes.mask_lut = ((unsigned int)lut - JO_VDP1_VRAM) >> 3;
    }
}

void                            jo_3d_transparency_mask_end(void)
{
    if (!in_masks)
        return;
    in_masks = false;
    __jo_3d_master_polygon_filter = JO_NULL;
    JO_ZERO(__jo_sprite_attributes.mask_lut);
}

void                            jo_3d_transparency_mask(jo_3d_mesh * const mesh)
{
    if (!in_layer || mesh == JO_NULL)
        return;
    jo_3d_transparency_mask_begin();
    jo_3d_mesh_draw(mesh);
    jo_3d_transparency_mask_end();
}

bool                            jo_3d_transparency_is_used(void)
{
    return (running && cleared_frame == __jo_vdp1_frame_tag());
}

bool                            jo_3d_transparency_is_running(void)
{
    return (running);
}

void                            jo_3d_transparency_get_size(int * const width, int * const height)
{
    if (width != JO_NULL)
        *width = layer_width;
    if (height != JO_NULL)
        *height = layer_height;
}

jo_fixed                        jo_3d_transparency_get_scale(void)
{
    return (layer_scale << (FIXED_SHIFT - SCALE_SHIFT));
}

#else

/*
** The automatic change of the frame buffers (JO_FRAMERATE 1): the frame buffers change at each VBLANK, the frame drawn
** by the VDP1 can't be copied before its change. No layer
*/

bool                            jo_3d_transparency_init(const int width, const jo_scroll_screen screen)
{
    JO_UNUSED_ARG(width);
    JO_UNUSED_ARG(screen);
# ifdef JO_DEBUG
    jo_core_error("Transparency: needs JO_FRAMERATE > 1");
# endif
    return (false);
}

void                            jo_3d_transparency_stop(void)
{
}

void                            jo_3d_transparency_set_blend(const jo_vdp2_blend_mode mode, const unsigned char ratio)
{
    JO_UNUSED_ARG(mode);
    JO_UNUSED_ARG(ratio);
}

void                            jo_3d_transparency_begin(void)
{
}

void                            jo_3d_transparency_end(void)
{
}

void                            jo_3d_transparency_set_clip(const jo_3d_rect * const rect)
{
    JO_UNUSED_ARG(rect);
}

void                            jo_3d_transparency_mask(jo_3d_mesh * const mesh)
{
    JO_UNUSED_ARG(mesh);
}

void                            jo_3d_transparency_mask_begin(void)
{
}

void                            jo_3d_transparency_mask_end(void)
{
}

bool                            jo_3d_transparency_is_used(void)
{
    return (false);
}

bool                            jo_3d_transparency_is_running(void)
{
    return (false);
}

void                            jo_3d_transparency_get_size(int * const width, int * const height)
{
    if (width != JO_NULL)
        *width = 0;
    if (height != JO_NULL)
        *height = 0;
}

jo_fixed                        jo_3d_transparency_get_scale(void)
{
    return (JO_FIXED_1);
}

#endif /* !JO_FRAMERATE */

#endif /* !JO_COMPILE_WITH_3D_SUPPORT */

/*
** END OF FILE
*/
