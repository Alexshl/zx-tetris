#ifndef GAME_H
#define GAME_H
#include <stdint.h>
#include "pieces.h"
#include "render.h"   /* для BOARD_COLS/BOARD_ROWS */

typedef struct {
    piece_id_t id;
    uint8_t    rot;
    int8_t     row;
    int8_t     col;
} active_piece_t;

typedef struct {
    uint8_t        board[BOARD_ROWS][BOARD_COLS];
    active_piece_t cur;
    uint8_t        game_over;
    uint8_t        lines_last;   /* 0..4, очищено при последнем лочинге */
    uint32_t       score;
    uint16_t       lines_total;
    uint8_t        level;
    uint8_t        drop_period;
} game_state_t;

extern game_state_t G;

void    game_init(void);
uint8_t game_tick(void);

void game_move(int8_t dc);
void game_rotate(void);
void game_hard_drop(void);
void game_soft_drop_step(void);
void game_reset(void);

#endif /* GAME_H */
