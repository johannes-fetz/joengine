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
/** @file internal/water.h
 *  @author Johannes Fetz
 *
 *  @brief Water: internal functions of the engine (see jo/water.h), between water.c and water_objects.c
 *  @warning Not part of the API: only for the source files of the engine (included after jo/water.h). The games
 *  include jo/jo.h, which doesn't include this file
 *  @bug No known bugs.
 */

#ifndef __JO_INTERNAL_WATER_H__
# define __JO_INTERNAL_WATER_H__

#if defined(JO_COMPILE_WITH_3D_SUPPORT) && defined(JO_COMPILE_WITH_DUAL_CPU_SUPPORT)

/** @brief The steps of a frame of the water for the objects under and above it (water_objects.c) */
typedef enum
{
    /** @brief The master CPU before the slave CPU draws: the view of the frame (the matrix of the camera) */
    __JO_WATER_OBJECTS_VIEW,
    /** @brief The slave CPU after the water: the objects under the water */
    __JO_WATER_OBJECTS_DRAW_ON_SLAVE,
    /** @brief The end of the frame: the objects above the water are given again in the next frame */
    __JO_WATER_OBJECTS_END_FRAME
}                           __jo_water_objects_step;

/** @brief The objects of the water at each step of a frame (water_objects.c): JO_NULL until the first object is added (the
 *  code of the objects is not in a game that never adds one) */
extern void                 (*__jo_water_objects_frame)(const jo_water * const w, const __jo_water_objects_step step);

/** @brief The camera of the water in the frame (see jo_water_update())
 *  @param w Water
 *  @param x The camera (units, y up)
 *  @param y
 *  @param z
 *  @param view_tan Tangent of half the field of view x 16 (+ 10 %)
 */
void                        __jo_water_camera(const jo_water * const w, int * const x, int * const y, int * const z, int * const view_tan);

#endif /* JO_COMPILE_WITH_3D_SUPPORT && JO_COMPILE_WITH_DUAL_CPU_SUPPORT */

#endif /* !__JO_INTERNAL_WATER_H__ */

/*
** END OF FILE
*/
