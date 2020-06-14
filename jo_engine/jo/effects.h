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
/** @file effects.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine Function prototypes special effects (Laser beam, etc)
 *  @bug No known bugs.
 */

#ifndef __JO_EFFECTS_H__
# define __JO_EFFECTS_H__

#ifdef JO_COMPILE_WITH_EFFECTS_SUPPORT

/*
██╗  ██╗ ██████╗ ███╗   ███╗██╗███╗   ██╗ ██████╗     ██╗      █████╗ ███████╗███████╗██████╗
██║  ██║██╔═══██╗████╗ ████║██║████╗  ██║██╔════╝     ██║     ██╔══██╗██╔════╝██╔════╝██╔══██╗
███████║██║   ██║██╔████╔██║██║██╔██╗ ██║██║  ███╗    ██║     ███████║███████╗█████╗  ██████╔╝
██╔══██║██║   ██║██║╚██╔╝██║██║██║╚██╗██║██║   ██║    ██║     ██╔══██║╚════██║██╔══╝  ██╔══██╗
██║  ██║╚██████╔╝██║ ╚═╝ ██║██║██║ ╚████║╚██████╔╝    ███████╗██║  ██║███████║███████╗██║  ██║
╚═╝  ╚═╝ ╚═════╝ ╚═╝     ╚═╝╚═╝╚═╝  ╚═══╝ ╚═════╝     ╚══════╝╚═╝  ╚═╝╚══════╝╚══════╝╚═╝  ╚═╝
*/

/** @brief Homing laser effect definition
 */
typedef struct
{
    jo_vector_fixed     source;
    bool                use_intermediate_point_a;
    jo_vector_fixed     intermediate_point_a;
    bool                use_intermediate_point_b;
    jo_vector_fixed     intermediate_point_b;
    jo_vector_fixed     target;
    jo_fixed            length;
    jo_fixed            horizontal_thickness;
    jo_fixed            vertical_thickness;
    jo_fixed            section_step;
    int                 sprite_id;
    bool                center_sprites_according_to_thickness;
}                       jo_homing_laser;

/** @brief Init jo_homing_laser struct
 *  @param homing_laser Homing laser definition
 *  @param sprite_id Sprite ID
 *  @param thickness Horizontal Laser thickness (1 to 256)
 *  @param z Default z index (500 for example)
 *  @param nb_sections Number of section in the curve aka smoothness (4 to 64 and must be a multiple of 2).
 *  @remarks nb_sections=20 Should be enough for most case
 */
void    jo_init_homing_laser(jo_homing_laser * const homing_laser, const int sprite_id, const unsigned short thickness,
                             const int z, const unsigned short nb_sections);

/** @brief Draw homing laser effect
 *  @param homing_laser Homing laser definition
 *  @remarks source, intermediate_point_a, intermediate_point_b, target and length must be set before this call
 */
void    jo_draw_homing_laser(jo_homing_laser * const homing_laser);

#endif /* !JO_COMPILE_WITH_EFFECTS_SUPPORT */

#endif /* !__JO_EFFECTS_H__ */

/*
** END OF FILE
*/
