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
#ifndef control
#define control

//dialog pointer
static int start_menu = 1;
static int choice_pause = 0;
static int d_pointer = 999;

//narrative variables
static int knows_about_sabrinas_breakup = 0;
static int knows_about_julias_mental_health = 0;
static int knows_about_sabrina_friendship = 0;
static int knows_about_the_arguing = 0;
static int knows_about_the_brother = 0;

//main controller function
void controller(){

    //debug
    //jo_set_printf_color_index(JO_COLOR_INDEX_White);
    //jo_printf(1, 1, "%d", d_pointer);

    //main menu
    if (jo_is_pad1_key_down(JO_KEY_START) && start_menu == 1){
        start_menu = 0;
        jo_clear_screen();
        d_pointer = 0;
        start();
    }

    //pointer motion trigger
    if (jo_is_pad1_key_down(JO_KEY_A) && start_menu == 0 && choice_pause == 0){

        jo_clear_screen();

        d_pointer++;

        //backgrounds
        if (d_pointer == 8){
            jo_clear_screen();
            dialogs[8]();
            dialogs[8] = JO_NULL;
            d_pointer = 9;
        }

        else if (d_pointer == 11){
            jo_clear_screen();
            dialogs[11]();
            dialogs[11] = JO_NULL;
            d_pointer = 12;
        }

        else if (d_pointer == 14){
            jo_clear_screen();
            dialogs[14]();
            dialogs[14] = JO_NULL;
            d_pointer = 15;
        }

        else if (d_pointer == 67){
            jo_clear_screen();
            dialogs[67]();
            dialogs[67] = JO_NULL;
            d_pointer = 68;
        }

        else if (d_pointer == 69){
            jo_clear_screen();
            dialogs[69]();
            dialogs[69] = JO_NULL;
            d_pointer = 70;
        }

        else if (d_pointer == 74){
            jo_clear_screen();
            dialogs[74]();
            dialogs[74] = JO_NULL;
            d_pointer = 75;
        }

        else if (d_pointer == 108){
            jo_clear_screen();
            dialogs[108]();
            dialogs[108] = JO_NULL;
            d_pointer = 109;
        }

        else if (d_pointer == 124){
            jo_clear_screen();
            dialogs[124]();
            dialogs[124] = JO_NULL;
            d_pointer = 125;
        }

        else if (d_pointer == 126){
            jo_clear_screen();
            dialogs[126]();
            dialogs[126] = JO_NULL;
            d_pointer = 127;
        }

        else if (d_pointer == 128){
            jo_clear_screen();
            dialogs[128]();
            dialogs[128] = JO_NULL;
            d_pointer = 129;
        }

        else if (d_pointer == 164){
            jo_clear_screen();
            dialogs[164]();
            dialogs[164] = JO_NULL;
            d_pointer = 165;
        }

        else if (d_pointer == 167){
            jo_clear_screen();
            dialogs[167]();
            dialogs[167] = JO_NULL;
            d_pointer = 168;
        }

        else if (d_pointer == 211){
            jo_clear_screen();
            dialogs[211]();
            dialogs[211] = JO_NULL;
            d_pointer = 212;
        }

        else if (d_pointer == 213){
            jo_clear_screen();
            dialogs[213]();
            dialogs[213] = JO_NULL;
            d_pointer = 214;
        }

        else if (d_pointer == 217){
            jo_clear_screen();
            dialogs[217]();
            dialogs[217] = JO_NULL;
            d_pointer = 218;
        }

        else if (d_pointer == 253){
            jo_clear_screen();
            dialogs[253]();
            dialogs[253] = JO_NULL;
            d_pointer = 254;
        }

        else if (d_pointer == 255){
            jo_clear_screen();
            dialogs[255]();
            dialogs[255] = JO_NULL;
            d_pointer = 256;
        }

        else if (d_pointer == 271){
            jo_clear_screen();
            dialogs[271]();
            dialogs[271] = JO_NULL;
            d_pointer = 272;
        }

        else if (d_pointer == 273){
                jo_clear_screen();
                dialogs[273]();
                dialogs[273] = JO_NULL;
                d_pointer = 274;
            }

        else if (d_pointer == 281){
                jo_clear_screen();
                dialogs[281]();
                dialogs[281] = JO_NULL;
                d_pointer = 282;
            }

        else if (d_pointer == 283){
                jo_clear_screen();
                dialogs[283]();
                dialogs[283] = JO_NULL;
                d_pointer = 284;
            }

        else if (d_pointer == 287){
                jo_clear_screen();
                dialogs[287]();
                dialogs[287] = JO_NULL;
                d_pointer = 288;
            }

        else if (d_pointer == 289){
                jo_clear_screen();
                dialogs[289]();
                dialogs[289] = JO_NULL;
                d_pointer = 290;
            }

        //choices
        else if (d_pointer == 104){
            jo_clear_screen();
            dialogs[104]();
            dialogs[104] = JO_NULL;
            choice_pause = 1;
        }

        else if (d_pointer == 191){
            jo_clear_screen();
            dialogs[191]();
            dialogs[191] = JO_NULL;
            choice_pause = 1;
        }

        else if (d_pointer == 248){
            jo_clear_screen();
            dialogs[248]();
            dialogs[248] = JO_NULL;
            choice_pause = 1;
        }

        else if (d_pointer == 270){
            jo_clear_screen();
            dialogs[270]();
            dialogs[270] = JO_NULL;
            choice_pause = 1;
        }

        else if (d_pointer == 301){
            jo_clear_screen();
            dialogs[301]();
            dialogs[301] = JO_NULL;
            choice_pause = 1;
        }

        else if(d_pointer == 264){
            if (knows_about_the_arguing == 1){
                dialogs[264]();
                dialogs[264] = JO_NULL;
            } else{
                dialogs[267]();
                dialogs[267] = JO_NULL;
                d_pointer = 267;
            }
        }

        else if(d_pointer == 268){
            if (knows_about_the_brother == 1){
                dialogs[268]();
                dialogs[268] = JO_NULL;
                d_pointer = 268;
            } else{
                dialogs[269]();
                dialogs[269] = JO_NULL;
                d_pointer = 269;
            }
        }

        else if(d_pointer == 325){
            d_pointer = 999;
            jo_clear_screen();
            draw_intro();
            start_menu = 1;
        }

        else {
            dialogs[d_pointer]();
            dialogs[d_pointer] = JO_NULL;
        }
    }

    //choices content
    if (jo_is_pad1_key_down(JO_KEY_B) && d_pointer == 104 && choice_pause == 1){
        jo_printf(1, 1, "%d", d_pointer);
        jo_clear_screen();
        choice_pause = 0;
        d_pointer = 401;
        dialogs[401]();
        dialogs[401] = JO_NULL;
    }

    if (jo_is_pad1_key_down(JO_KEY_C) && d_pointer == 104 && choice_pause == 1){
        jo_printf(1, 1, "%d", d_pointer);
        jo_clear_screen();
        choice_pause = 0;
        d_pointer = 411;
        dialogs[411]();
        dialogs[411] = JO_NULL;
    }

    if (jo_is_pad1_key_down(JO_KEY_A) && choice_pause == 0 && d_pointer == 408){
        jo_clear_screen();
        dialogs[105]();
        dialogs[105] = JO_NULL;
        d_pointer = 106;
        knows_about_sabrinas_breakup = 1;
    }

    if (jo_is_pad1_key_down(JO_KEY_A) && choice_pause == 0 && d_pointer == 420){
        jo_clear_screen();
        dialogs[105]();
        dialogs[105] = JO_NULL;
        d_pointer = 106;
        knows_about_julias_mental_health = 1;
    }

    if (jo_is_pad1_key_down(JO_KEY_B) && d_pointer == 191 && choice_pause == 1){
        jo_printf(1, 1, "%d", d_pointer);
        jo_clear_screen();
        choice_pause = 0;
        d_pointer = 421;
        dialogs[421]();
        dialogs[421] = JO_NULL;
    }

    if (jo_is_pad1_key_down(JO_KEY_C) && d_pointer == 191 && choice_pause == 1){
        jo_printf(1, 1, "%d", d_pointer);
        jo_clear_screen();
        choice_pause = 0;
        d_pointer = 441;
        dialogs[441]();
        dialogs[441] = JO_NULL;
    }

    if (jo_is_pad1_key_down(JO_KEY_A) && choice_pause == 0 && d_pointer == 435){
        jo_clear_screen();
        dialogs[192]();
        dialogs[192] = JO_NULL;
        d_pointer = 192;
        knows_about_sabrina_friendship = 1;
    }

    if (jo_is_pad1_key_down(JO_KEY_A) && choice_pause == 0 && d_pointer == 453){
        jo_clear_screen();
        dialogs[192]();
        dialogs[192] = JO_NULL;
        d_pointer = 192;
        knows_about_the_arguing = 1;
    }

    if (jo_is_pad1_key_down(JO_KEY_B) && d_pointer == 248 && choice_pause == 1){
        jo_printf(1, 1, "%d", d_pointer);
        jo_clear_screen();
        choice_pause = 0;
        d_pointer = 461;
        dialogs[461]();
        dialogs[461] = JO_NULL;
    }

    if (jo_is_pad1_key_down(JO_KEY_C) && d_pointer == 248 && choice_pause == 1){
        jo_printf(1, 1, "%d", d_pointer);
        jo_clear_screen();
        choice_pause = 0;
        d_pointer = 481;
        dialogs[481]();
        dialogs[481] = JO_NULL;
    }

    if (jo_is_pad1_key_down(JO_KEY_A) && choice_pause == 0 && (d_pointer == 475 || d_pointer == 491)){
        jo_clear_screen();
        dialogs[249]();
        dialogs[249] = JO_NULL;
        d_pointer = 249;
    }

    if (jo_is_pad1_key_down(JO_KEY_B) && d_pointer == 270 && choice_pause == 1){
        jo_printf(1, 1, "%d", d_pointer);
        jo_clear_screen();
        choice_pause = 0;
        d_pointer = 501;
        dialogs[501]();
        dialogs[501] = JO_NULL;
    }

    if (jo_is_pad1_key_down(JO_KEY_C) && d_pointer == 270 && choice_pause == 1){
        jo_printf(1, 1, "%d", d_pointer);
        jo_clear_screen();
        choice_pause = 0;
        d_pointer = 501;
        dialogs[511]();
        dialogs[511] = JO_NULL;
    }

    if (jo_is_pad1_key_down(JO_KEY_A) && start_menu == 0 && choice_pause == 0 && d_pointer == 506){
        jo_clear_screen();
        dialogs[271]();
        dialogs[271] = JO_NULL;
        d_pointer = 271;
    }

    if (jo_is_pad1_key_down(JO_KEY_A) && start_menu == 0 && choice_pause == 0 && d_pointer == 513){
        jo_clear_screen();
        draw_intro();
        start_menu = 1;
    }

    if (jo_is_pad1_key_down(JO_KEY_B) && d_pointer == 301 && choice_pause == 1){
        jo_printf(1, 1, "%d", d_pointer);
        jo_clear_screen();
        choice_pause = 0;
        d_pointer = 302;
        dialogs[302]();
        dialogs[302] = JO_NULL;
    }

    if (jo_is_pad1_key_down(JO_KEY_C) && d_pointer == 301 && choice_pause == 1){
        jo_printf(1, 1, "%d", d_pointer);
        jo_clear_screen();
        choice_pause = 0;
        d_pointer = 531;
        dialogs[531]();
        dialogs[531] = JO_NULL;
    }

    if (jo_is_pad1_key_down(JO_KEY_A) && start_menu == 0 && choice_pause == 0 && d_pointer == 533){
        jo_clear_screen();
        draw_intro();
        start_menu = 1;
    }

}

#endif
