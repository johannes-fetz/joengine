#ifndef __SHIP_H__
# define __SHIP_H__

# define SHIP_TILE_COUNT    (6)

typedef enum
{
    NONE,
    LEFT,
    RIGHT
}                       t_ship_horiz_move;

typedef struct
{
    int                 anim_id;
    int                 x;
    int                 y;
    int                 speed;
    t_ship_horiz_move   move;
    char                is_moving_horizontaly;
    char                reverse_animation;
    int                 shield_pos_x;
    int                 shield_pos_y;
    int                 shield_angle;
    int                 score;
}                       t_ship;

#endif /* !__SHIP_H__ */

/*
** END OF FILE
*/
