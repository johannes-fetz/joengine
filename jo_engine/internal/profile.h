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
/** @file internal/profile.h
 *  @author Johannes Fetz
 *
 *  @brief Profiler: internal functions of the engine (see jo/profile.h)
 *  @warning Not part of the API: only for the source files of the engine (included after jo/profile.h). The games
 *  include jo/jo.h, which doesn't include this file
 *  @bug No known bugs.
 */

#ifndef __JO_INTERNAL_PROFILE_H__
# define __JO_INTERNAL_PROFILE_H__

#ifdef JO_COMPILE_WITH_PROFILER_SUPPORT

/** @brief The pixels of the polygons are counted (see jo_profile_count_pixels()) */
extern bool                         __jo_profile_pixels;

/** @brief A job of the slave CPU starts (Internal usage, called by the slave CPU): its maximums are reset if the master CPU
 *  asked for it (see __jo_profile_end_frame()) */
void                                __jo_profile_slave_job(void);

/** @brief Initialize the profiler (Internal usage, called by jo_core_init()) */
void                                __jo_profile_init(void);

/** @brief End of a frame (Internal usage, called by jo_core_run()) */
void                                __jo_profile_end_frame(void);

/** @brief The VDP1 starts drawing a new command table (Internal usage, called during the VBLANK) */
void                                __jo_profile_vdp1_start(void);

#else

# define __jo_profile_slave_job()               do {} while (0)

#endif /* !JO_COMPILE_WITH_PROFILER_SUPPORT */

#endif /* !__JO_INTERNAL_PROFILE_H__ */

/*
** END OF FILE
*/
