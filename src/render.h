#ifndef RENDER_H
#define RENDER_H

#include <stdint.h>
#include "pieces.h"

#define BOARD_COLS 10
#define BOARD_ROWS 20
#define BOARD_X    8
#define BOARD_Y    2

void render_init(void);
void render_clear_board(void);

void render_cell(uint8_t row, uint8_t col, uint8_t attr);
void render_piece(piece_id_t id, uint8_t rot, uint8_t row, uint8_t col);
void erase_piece(piece_id_t id, uint8_t rot, uint8_t row, uint8_t col);

void render_hud(uint32_t score, uint16_t lines, uint8_t level);
void render_hud_force_redraw(void);

void render_game_over(void);

#endif /* RENDER_H */
