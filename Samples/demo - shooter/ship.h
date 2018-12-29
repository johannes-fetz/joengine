#ifndef __SHIP_H__
# define __SHIP_H__

# define SHIP_TILE_COUNT    (6)

typedef enum
{
    SHIP_MOVE_NONE,
    SHIP_MOVE_LEFT,
    SHIP_MOVE_RIGHT
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
    jo_pos2D            shield_pos;
    int                 score;
}                       t_ship;

#endif /* !__SHIP_H__ */

/*
** END OF FILE
*/
