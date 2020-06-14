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
/** @file types.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine Types
 *  @bug No known bugs.
 */

#ifndef __JO_TYPES_H__
# define __JO_TYPES_H__

/** @brief 15 bits color type */
typedef unsigned short jo_color;

/** @brief Fixed point Q16.16 number
 *  @remarks SGL compatible (FIXED)
 *  @remarks Integer part is located in the upper 16 bits and a decimal part in the lower 16 bits
 *  @remarks Valid range is -32767.99998 to 32767.99998
 */
typedef int             jo_fixed;

/** @brief 2D position */
typedef struct
{
    int                 x;
    int                 y;
}                       jo_pos2D;

/** @brief 2D position using fixed number */
typedef struct
{
    jo_fixed            x;
    jo_fixed            y;
}                       jo_pos2D_fixed;

/** @brief 3D position
  * @remarks Inherits from jo_pos2D (cast friendly)
  */
typedef struct
{
    int                 x;
    int                 y;
    int                 z;
}                       jo_pos3D;

/** @brief 3D position using floating numbers
  */
typedef struct
{
    float               x;
    float               y;
    float               z;
}                       jo_pos3Df;

/** @brief 3D position using fixed numbers
  */
typedef struct
{
    jo_fixed            x;
    jo_fixed            y;
    jo_fixed            z;
}                       jo_pos3D_fixed;

/** @brief 3D rotation angles
  */
typedef struct
{
    int                 rx;
    int                 ry;
    int                 rz;
}                       jo_rot3D;

/** @brief 3D rotation angles
  */
typedef struct
{
    float               rx;
    float               ry;
    float               rz;
}                       jo_rot3Df;

/** @brief Object for 2D manipulation (storyboard) using fixed number
  * @remarks Inherits from jo_pos3D (cast friendly)
  */
typedef struct
{
    jo_pos3D;
    short               rz;
}                       jo_2d_object_attributes;

/** @brief Object for 3D manipulation (storyboard) using fixed number
  * @remarks Inherits from jo_2d_object_attributes (cast friendly)
  */
typedef struct
{
    jo_2d_object_attributes;
    short               rx;
    short               ry;
}                       jo_3d_object_attributes;

/** @brief Vector for 3D computation using floating numbers */
typedef struct
{
    float               x;
    float               y;
    float               z;
}                       jo_vectorf;

/** @brief Vector for 3D computation using integer
  * @remarks Inherits from jo_pos3D (cast friendly)
  */
typedef struct
{
    jo_pos3D;
}                       jo_vector;

/** @brief Vector for 3D computation using fixed number
  * @remarks Inherits from jo_pos3D_fixed (cast friendly)
  */
typedef struct
{
    jo_pos3D_fixed;
}                       jo_vector_fixed;

/** @brief 4x4 MATRIX for 3D computation using floating numbers */
typedef union
{
	float               m[4][4];
	float               table[16];
	struct
	{
		float           m00, m01, m02, m03;
		float           m10, m11, m12, m13;
		float           m20, m21, m22, m23;
		float           m30, m31, m32, m33;
	};
}                       jo_matrixf;

/** @brief 4x4 MATRIX for 3D computation using fixed number */
typedef union
{
	int                 m[4][4];
	int                 table[16];
	struct
	{
		int             m00, m01, m02, m03;
		int             m10, m11, m12, m13;
		int             m20, m21, m22, m23;
		int             m30, m31, m32, m33;
	};
}                       jo_matrix;

/** @brief Size struct */
typedef struct      __jo_size
{
    unsigned short	width;
    unsigned short	height;
}					jo_size;

/** @brief Texture definition
  * @remarks Inherits from jo_size (cast friendly)
  */
typedef struct
{
    unsigned short	    width;
    unsigned short	    height;
    unsigned short      adr;
    unsigned short      size;
}                       jo_texture_definition;

/** @brief Picture definition */
typedef struct
{
    unsigned short      index;
    unsigned short      color_mode;
    void                *data;
}                       jo_picture_definition;

/** @brief 15 bits image struct */
typedef struct
{
    unsigned short	width;
    unsigned short	height;
    jo_color        *data;
}					jo_img;

/** @brief 8 bits image struct */
typedef struct
{
    unsigned short	width;
    unsigned short	height;
    unsigned char   *data;
}					jo_img_8bits;

/** @brief Raw image struct */
typedef struct
{
    unsigned short	width;
    unsigned short	height;
    void            *data;
}					jo_raw_img;

/** @brief Palette contents struct */
typedef struct
{
    jo_color    *data;
    int         id;
}               jo_palette;

/** @brief Struct of one tile (pos and size) */
typedef struct
{
    int                 x;
    int                 y;
    unsigned short	width;
    unsigned short	height;
}					jo_tile;

/** @brief Sprite attributes */
typedef struct
{
    unsigned short      effect;
    unsigned short      direction;
    unsigned int        color_table_index;
    unsigned int        gouraud_shading_index;
    jo_fixed            fixed_scale_x;
    jo_fixed            fixed_scale_y;
    unsigned int        clipping;
}                       jo_sprite_attributes;

/** @brief Float IEEE 754 format */
typedef union
{
    float f;
    struct
    {
        unsigned int sign : 1;
        unsigned int exponent : 8;
        unsigned int mantissa : 23;

    } field;
}                       jo_IEEE754;

/** @brief File definition */
typedef struct
{
    int         id;
    int         size;
    int         read;
    void        *handle;
    char        *read_buffer;
    int         read_index;
}               jo_file;

/** @brief 8 directions definition */
typedef enum
{
    NONE,
    LEFT,
    RIGHT,
    UP,
    DOWN,
    UP_LEFT,
    UP_RIGHT,
    DOWN_LEFT,
    DOWN_RIGHT
}               jo_8_directions;

/** @brief Sega Saturn Scroll Screen Ids */
typedef enum
{
    /* Technical Name */
    JO_NBG0_SCREEN = (1<<0),
    JO_NBG1_SCREEN = (1<<1),
    JO_NBG2_SCREEN = (1<<2),
    JO_NBG3_SCREEN = (1<<3),
    JO_RBG0_SCREEN = (1<<4),
    JO_BACK_SCREEN = (1<<5),
    JO_SPRITE_SCREEN = (1<<6),
    /* User friendly alias */
    JO_VDP2_3D_PLANE_SCREEN = JO_RBG0_SCREEN,
    JO_LNCL_SCREEN = JO_BACK_SCREEN,
    /* Helper */
    JO_ALL_SCROLL_SCREEN = JO_NBG0_SCREEN | JO_NBG1_SCREEN | JO_NBG2_SCREEN | JO_NBG3_SCREEN | JO_RBG0_SCREEN | JO_BACK_SCREEN | JO_SPRITE_SCREEN
}               jo_scroll_screen;

#endif /* !__JO_TYPES_H__ */

/*
** END OF FILE
*/
