/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2017, Johannes Fetz (johannesfetz@gmail.com)
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

/** @brief 2D position */
typedef struct
{
    int                 x;
    int                 y;
}                       jo_pos2D;

/** @brief 3D position
  * @remarks Inherits from jo_pos2D (cast friendly)
  */
typedef struct
{
    int                 x;
    int                 y;
    int                 z;
}                       jo_pos3D;

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

/** @brief Vector for 3D computation using fixed number
  * @remarks Inherits from jo_pos3D (cast friendly)
  */
typedef struct
{
    jo_pos3D;
}                       jo_vector;

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
    jo_size;
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

/** @brief Sprite attributes */
typedef struct
{
    unsigned short      effect;
    unsigned short      direction;
    unsigned int        color_table_index;
    unsigned int        gouraud_shading_index;
    unsigned int        fixed_scale;
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
    LEFT,
    RIGHT,
    UP,
    DOWN,
    UP_LEFT,
    UP_RIGHT,
    DOWN_LEFT,
    DOWN_RIGHT
}               jo_8_directions;

#endif /* !__JO_TYPES_H__ */

/*
** END OF FILE
*/
