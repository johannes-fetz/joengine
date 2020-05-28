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
#include <SGL_CPK.H>
#include "jo/conf.h"
#include "jo/types.h"
#include "jo/sega_saturn.h"
#include "jo/smpc.h"
#include "jo/core.h"
#include "jo/tools.h"
#include "jo/malloc.h"
#include "jo/video.h"
#include "jo/colors.h"
#include "jo/image.h"
#include "jo/sprites.h"

#ifdef JO_COMPILE_WITH_VIDEO_SUPPORT

#define	PCM_ADDR				((void*)0x25a20000)
#define	PCM_SIZE				(4096L*16)
#define	RING_BUF_SIZ			(1024L*100)

int __jo_video_sprite_id;

typedef struct
{
    void                        *movie_buffer;
    void                        *decode_buffer;
    unsigned int                decode_buffer_size;
    void                        *work_buffer;
    jo_img                      img;
    GfsHn                       gfs;
    CpkHn                       cpk;
    int                         event_id;
    jo_video_onstop_callback    onstop;
}                               jo_video_cpk;

static jo_video_cpk             __jo_video_cpk;

void             jo_video_stop(void)
{
    if (__jo_video_cpk.cpk != JO_NULL)
    {
        CPK_Stop(__jo_video_cpk.cpk);
        __jo_video_cpk.cpk = JO_NULL;
    }
    if (__jo_video_cpk.event_id != -1)
    {
        jo_core_remove_callback(__jo_video_cpk.event_id);
        __jo_video_cpk.event_id = -1;
    }
    if (__jo_video_cpk.gfs != JO_NULL)
    {
        GFS_Close(__jo_video_cpk.gfs);
        __jo_video_cpk.gfs = JO_NULL;
    }
    if (__jo_video_cpk.decode_buffer != JO_NULL)
    {
        jo_free(__jo_video_cpk.decode_buffer);
        __jo_video_cpk.decode_buffer = JO_NULL;
    }
    if (__jo_video_cpk.work_buffer != JO_NULL)
    {
        jo_free(__jo_video_cpk.work_buffer);
        __jo_video_cpk.work_buffer = JO_NULL;
    }
    if (__jo_video_cpk.movie_buffer != JO_NULL)
    {
        jo_free(__jo_video_cpk.movie_buffer);
        __jo_video_cpk.movie_buffer = JO_NULL;
    }
    if (__jo_video_cpk.onstop != JO_NULL)
    {
        __jo_video_cpk.onstop();
        __jo_video_cpk.onstop = JO_NULL;
    }
}

static void			__jo_internal_cpk(void)
{
    CPK_Task(__jo_video_cpk.cpk);
    if (CPK_IsDispTime(__jo_video_cpk.cpk) == TRUE)
    {
        __jo_video_cpk.img.data = __jo_video_cpk.decode_buffer;
        jo_sprite_replace(&__jo_video_cpk.img, __jo_video_sprite_id);
        CPK_CompleteDisp(__jo_video_cpk.cpk);
    }
    if (CPK_GetPlayStatus(__jo_video_cpk.cpk) == CPK_STAT_PLAY_END)
        jo_video_stop();
}

bool					jo_video_open_file(const char *const filename)
{
    Sint32              fid;
    CpkHeader           *header;
    CpkCreatePara       para;

    jo_video_stop();
    fid = GFS_NameToId((Sint8 *)filename);
    if (fid < 0)
    {
#ifdef JO_DEBUG
        jo_core_error("%s: File not found", filename);
#endif
        return (false);
    }
    if ((__jo_video_cpk.gfs = GFS_Open(fid)) == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("%s: GFS_Open() failed", filename);
#endif
        return (false);
    }
    if ((__jo_video_cpk.movie_buffer = jo_malloc_with_behaviour(RING_BUF_SIZ, JO_MALLOC_TRY_REUSE_BLOCK)) == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("%s: Out of memory #1", filename);
#endif
        jo_video_stop();
        return (false);
    }
    if ((__jo_video_cpk.work_buffer = jo_malloc_with_behaviour(CPK_15WORK_BSIZE * sizeof(unsigned int), JO_MALLOC_TRY_REUSE_BLOCK)) == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("%s: Out of memory #2", filename);
#endif
        jo_video_stop();
        return (false);
    }
    CPK_PARA_WORK_ADDR(&para) = __jo_video_cpk.work_buffer;
    CPK_PARA_WORK_SIZE(&para) = CPK_15WORK_BSIZE;
    CPK_PARA_BUF_ADDR(&para) = __jo_video_cpk.movie_buffer;
    CPK_PARA_BUF_SIZE(&para) = RING_BUF_SIZ;
    CPK_PARA_PCM_ADDR(&para) = PCM_ADDR;
    CPK_PARA_PCM_SIZE(&para) = PCM_SIZE;
    if ((__jo_video_cpk.cpk = CPK_CreateGfsMovie(&para, __jo_video_cpk.gfs)) == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("CPK_CreateGfsMovie() failed");
#endif
        jo_video_stop();
        return (false);
    }
    CPK_SetColor(__jo_video_cpk.cpk, CPK_COLOR_15BIT);
    CPK_PreloadHeader(__jo_video_cpk.cpk);
    header = CPK_GetHeader(__jo_video_cpk.cpk);
#ifdef JO_DEBUG
    if (JO_MOD_POW2(header->width, 8) != 0)
    {
        jo_core_error("%s: Video width must be a multiple of 8", filename);
        jo_video_stop();
        return (false);
    }
#endif
    __jo_video_cpk.decode_buffer_size = header->width * header->height * sizeof(unsigned short);
    if ((__jo_video_cpk.decode_buffer = jo_malloc_with_behaviour(__jo_video_cpk.decode_buffer_size, JO_MALLOC_TRY_REUSE_BLOCK)) == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("%s: Out of memory #3", filename);
#endif
        jo_video_stop();
        return (false);
    }
    __jo_video_cpk.img.width = header->width;
    __jo_video_cpk.img.height = header->height;
    __jo_video_cpk.img.data = __jo_video_cpk.decode_buffer;
    if (__jo_video_sprite_id < 0)
        __jo_video_sprite_id = jo_sprite_add(&__jo_video_cpk.img);
    else
        __jo_video_sprite_id = jo_sprite_replace(&__jo_video_cpk.img, __jo_video_sprite_id);
    if (__jo_video_sprite_id < 0)
    {
        jo_video_stop();
        return (false);
    }
    CPK_SetDecodeAddr(__jo_video_cpk.cpk, __jo_video_cpk.decode_buffer, JO_MULT_BY_2(header->width));
    return (true);
}

bool                    jo_video_pause(void)
{
    if (__jo_video_cpk.cpk == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Video not ready");
#endif
        return (false);
    }
    CPK_Pause(__jo_video_cpk.cpk, CPK_PAUSE_ON_AT_ONCE);
    return (true);
}

bool                    jo_video_play(const jo_video_onstop_callback onstop)
{
    if (__jo_video_cpk.cpk == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Video not ready");
#endif
        return (false);
    }
    if (__jo_video_cpk.event_id < 0)
    {
        __jo_video_cpk.event_id = jo_core_add_callback(__jo_internal_cpk);
        if (__jo_video_cpk.event_id < 0)
        {
#ifdef JO_DEBUG
            jo_core_error("Fail to create video callback");
#endif
            jo_video_stop();
            return (false);
        }
    }
    __jo_video_cpk.onstop = onstop;
    CPK_Start(__jo_video_cpk.cpk);
    return (true);
}

#ifdef JO_DEBUG

static void     __jo_cpk_error_handling(void *obj, Sint32 error_code)
{
    JO_UNUSED_ARG(obj);
    jo_core_error("Cinepak Error 0x%x", (CpkErrCode)error_code);
}

#endif

bool		                        jo_video_init(void)
{
    __jo_video_cpk.decode_buffer = JO_NULL;
    __jo_video_cpk.work_buffer = JO_NULL;
    __jo_video_cpk.movie_buffer = JO_NULL;
    __jo_video_cpk.gfs = JO_NULL;
    __jo_video_cpk.cpk = JO_NULL;
    __jo_video_cpk.onstop = JO_NULL;
    __jo_video_cpk.event_id = -1;
    __jo_video_sprite_id = -1;
    if (CPK_Init() != TRUE)
        return (false);
#ifdef JO_DEBUG
    CPK_SetErrFunc(__jo_cpk_error_handling, JO_NULL);
#endif
    jo_core_add_vblank_callback(CPK_VblIn);
    return (true);
}

#endif /* !JO_COMPILE_WITH_VIDEO_SUPPORT */

/*
** END OF FILE
*/
