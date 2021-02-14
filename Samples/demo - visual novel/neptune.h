/*-------------------------------------------------------------------------------+
|   ~+         *       +                                                         |
|      '                  |     _   _  ___________ _____ _   _ _   _  _____      |
|  ()    .-.,="``"=.    - o -  | \ | ||  ___| ___ \_   _| | | | \ | ||  ___|     |
|        '=/_       \     |    |  \| || |__ | |_/ / | | | | | |  \| || |__       |
|     *   |  '=._    |         | . ` ||  __||  __/  | | | | | | . ` ||  __|      |
|          \     `=./`,     '  | |\  || |___| |     | | | |_| | |\  || |___      |
|  +    .   '=.__.=' `='   *   \_| \_/\____/\_|     \_/  \___/\_| \_/\____/      |
|                                                              BY: @G_Sansigolo  |
+-------------------------------------------------------------------------------*/
#ifndef neptune
#define neptune

/**
 * set_background()
 * plot background image
 */
void set_background(char* filename, char* folder){
    jo_img bg;

    bg.data = NULL;
    jo_tga_loader(&bg, folder, filename, JO_COLOR_Green);
    jo_set_background_sprite(&bg, 0, 0);
    jo_free_img(&bg);
}

/**
 * d()
 * draw text on the screen
 */
void d(char* text, int line){
    jo_set_printf_color_index(JO_COLOR_INDEX_White);
    if (line == 1)
        jo_printf(12, 21+(2), text);
    else
        jo_printf(1, 21+(2*line), text);
}

/**
 * h()
 * draw text on the screen
 */
void h(char* text){
    jo_set_printf_color_index(JO_COLOR_INDEX_Red);
    jo_printf(1, 21+(2*1), text);
}

/**
 * d_n()
 * draw text on the screen
 */
void d_n(char* text, int line){
    jo_set_printf_color_index(JO_COLOR_INDEX_White);
    if (line == 1)
        jo_printf(1, 21+(2), text);
    else
        jo_printf(1, 21+(2*line), text);
}

#endif
