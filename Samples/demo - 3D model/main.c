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

/*
** 3D model loaded from a JOM file
**
** The model is made with a 3D software (or by hand): models/house.obj and models/house.mtl (Wavefront OBJ), converted
** by the converter of the Jo Engine:
**
**     python3 ../../Compiler/COMMON/jo_model.py models/house.obj -o cd/HOUSE.JOM --embed-textures
**
** With --embed-textures, the textures of the materials (models/brick.png) are in cd/HOUSE.JOM: one file for the model and
** its textures. Without it, the converter writes them next to the JOM file (cd/BRICK.BIN), loaded from the CD.
** The options of the materials (MTL): see Compiler/COMMON/jo_model.py. The file format: see jo_engine/jo/model.h
*/

#include <jo/jo.h>

static jo_camera                cam;
static jo_3d_model              *house;

static void                     my_draw(void)
{
    static jo_angle             angle;

    jo_printf(12, 1, "*3D model demo*");
    jo_3d_camera_look_at(&cam);
    jo_3d_push_matrix();
    {
        jo_3d_translate_matrix(0, 20, 0);
        jo_3d_rotate_matrix_x(-20);
        jo_3d_rotate_matrix_y(angle);
        jo_3d_model_draw(house);
        angle += 1;
    }
    jo_3d_pop_matrix();
    jo_printf(0, 26, "HOUSE.JOM: %d points, %d polygons  ", house->mesh->data.point_count, house->mesh->data.polygon_count);
    jo_printf(0, 27, "Materials: %d, textures: %d, groups: %d  ", house->material_count, house->texture_count, house->group_count);
}

void                            jo_main(void)
{
    jo_core_init(JO_COLOR_RGB(110, 160, 230));
    jo_3d_camera_init(&cam);
    // The model and its textures (the textures are loaded once, even if several models use them)
    house = jo_3d_load_model("HOUSE.JOM");
    jo_core_add_callback(my_draw);
    jo_core_run();
}

/*
** END OF FILE
*/
