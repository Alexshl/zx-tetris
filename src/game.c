#include "game.h"
#include "render.h"
#include "pieces.h"
#include "sound.h"
#include <stdlib.h>
#include <arch/zx.h>

game_state_t G;

static const uint8_t period_table[30] = {
    48, 43, 38, 33, 28, 23, 18, 13, 8, 6,
    5, 5, 5,
    4, 4, 4,
    3, 3, 3,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    1
};

static uint8_t level_drop_period(uint8_t lvl)
{
    if (lvl > 29) lvl = 29;
    return period_table[lvl];
}

static void game_apply_score(uint8_t lines)
{
    static const uint16_t pts[5] = {0, 40, 100, 300, 1200};
    uint8_t new_level;
    if (lines > 4) lines = 4;
    G.score += (uint32_t)pts[lines] * (uint32_t)(G.level + 1);
    G.lines_total = (uint16_t)(G.lines_total + lines);
    new_level = (uint8_t)(G.lines_total / 10);
    if (new_level != G.level) {
        G.level = new_level;
        G.drop_period = level_drop_period(G.level);
    }
}

static uint8_t collides(piece_id_t id, uint8_t rot, int8_t r, int8_t c)
{
    uint16_t shape = piece_shapes[id][rot];
    uint8_t  dr, dc;
    int8_t   br, bc;
    for (dr = 0; dr < 4; ++dr) {
        for (dc = 0; dc < 4; ++dc) {
            if (!piece_bit(shape, dr, dc)) continue;
            br = r + (int8_t)dr;
            bc = c + (int8_t)dc;
            if (bc < 0 || bc >= BOARD_COLS) return 1;
            if (br >= BOARD_ROWS)           return 1;
            if (br >= 0 && G.board[br][bc]) return 1;
        }
    }
    return 0;
}

static uint8_t scan_and_clear_lines(void)
{
    uint8_t cleared = 0;
    int8_t  r, rr;
    uint8_t c, full;
    for (r = (int8_t)(BOARD_ROWS - 1); r >= 0; ) {
        full = 1;
        for (c = 0; c < BOARD_COLS; ++c) {
            if (!G.board[r][c]) { full = 0; break; }
        }
        if (full) {
            for (rr = r; rr > 0; --rr) {
                for (c = 0; c < BOARD_COLS; ++c) {
                    G.board[rr][c] = G.board[rr - 1][c];
                }
            }
            for (c = 0; c < BOARD_COLS; ++c) {
                G.board[0][c] = 0;
            }
            ++cleared;
            /* r не декрементируем — проверяем ту же строку после сдвига */
        } else {
            --r;
        }
    }
    return cleared;
}

static void redraw_board(void)
{
    uint8_t r, c, v, attr;
    for (r = 0; r < BOARD_ROWS; ++r) {
        for (c = 0; c < BOARD_COLS; ++c) {
            v = G.board[r][c];
            if (v) {
                attr = piece_colors[v - 1];
            } else {
                attr = (uint8_t)(PAPER_BLACK | INK_BLACK);
            }
            render_cell(r, c, attr);
        }
    }
}

static void lock_piece(void)
{
    uint16_t shape = piece_shapes[G.cur.id][G.cur.rot];
    uint8_t  dr, dc;
    int8_t   br, bc;
    for (dr = 0; dr < 4; ++dr) {
        for (dc = 0; dc < 4; ++dc) {
            if (!piece_bit(shape, dr, dc)) continue;
            br = G.cur.row + (int8_t)dr;
            bc = G.cur.col + (int8_t)dc;
            if (br >= 0 && br < BOARD_ROWS && bc >= 0 && bc < BOARD_COLS) {
                G.board[br][bc] = (uint8_t)(G.cur.id + 1);
            }
        }
    }
    G.lines_last = scan_and_clear_lines();
    if (G.lines_last) {
        game_apply_score(G.lines_last);
        redraw_board();
    }
    if (G.lines_last) {
        sfx_line_clear();
    } else {
        sfx_lock();
    }
}

static void spawn(void)
{
    G.cur.id  = (piece_id_t)(rand() % PIECE_COUNT);
    G.cur.rot = 0;
    G.cur.row = 0;
    G.cur.col = 3;
    if (collides(G.cur.id, G.cur.rot, G.cur.row, G.cur.col)) {
        G.game_over = 1;
    }
}

void game_init(void)
{
    uint8_t r, c;
    for (r = 0; r < BOARD_ROWS; ++r) {
        for (c = 0; c < BOARD_COLS; ++c) {
            G.board[r][c] = 0;
        }
    }
    G.game_over = 0;
    G.lines_last = 0;
    G.score = 0;
    G.lines_total = 0;
    G.level = 0;
    G.drop_period = 48;
    spawn();
    if (!G.game_over) {
        render_piece(G.cur.id, G.cur.rot, (uint8_t)G.cur.row, (uint8_t)G.cur.col);
    }
}

uint8_t game_tick(void)
{
    if (G.game_over) return 0;

    if (collides(G.cur.id, G.cur.rot, G.cur.row + 1, G.cur.col)) {
        lock_piece();
        spawn();
        if (!G.game_over) {
            render_piece(G.cur.id, G.cur.rot, (uint8_t)G.cur.row, (uint8_t)G.cur.col);
        }
        return 1;
    }
    erase_piece(G.cur.id, G.cur.rot, (uint8_t)G.cur.row, (uint8_t)G.cur.col);
    G.cur.row++;
    render_piece(G.cur.id, G.cur.rot, (uint8_t)G.cur.row, (uint8_t)G.cur.col);
    return 1;
}

void game_move(int8_t dc)
{
    if (G.game_over) return;
    if (collides(G.cur.id, G.cur.rot, G.cur.row, G.cur.col + dc)) return;
    erase_piece(G.cur.id, G.cur.rot, (uint8_t)G.cur.row, (uint8_t)G.cur.col);
    G.cur.col = (int8_t)(G.cur.col + dc);
    render_piece(G.cur.id, G.cur.rot, (uint8_t)G.cur.row, (uint8_t)G.cur.col);
}

void game_rotate(void)
{
    uint8_t new_rot;
    if (G.game_over) return;
    new_rot = (uint8_t)((G.cur.rot + 1) & 3);
    if (collides(G.cur.id, new_rot, G.cur.row, G.cur.col)) return;
    erase_piece(G.cur.id, G.cur.rot, (uint8_t)G.cur.row, (uint8_t)G.cur.col);
    G.cur.rot = new_rot;
    render_piece(G.cur.id, G.cur.rot, (uint8_t)G.cur.row, (uint8_t)G.cur.col);
}

void game_hard_drop(void)
{
    if (G.game_over) return;
    erase_piece(G.cur.id, G.cur.rot, (uint8_t)G.cur.row, (uint8_t)G.cur.col);
    while (!collides(G.cur.id, G.cur.rot, G.cur.row + 1, G.cur.col)) {
        G.cur.row++;
    }
    render_piece(G.cur.id, G.cur.rot, (uint8_t)G.cur.row, (uint8_t)G.cur.col);
    lock_piece();
    spawn();
    if (!G.game_over) {
        render_piece(G.cur.id, G.cur.rot, (uint8_t)G.cur.row, (uint8_t)G.cur.col);
    }
}

void game_soft_drop_step(void)
{
    game_tick();
}

void game_reset(void)
{
    uint8_t r, c;
    for (r = 0; r < BOARD_ROWS; ++r) {
        for (c = 0; c < BOARD_COLS; ++c) {
            G.board[r][c] = 0;
        }
    }
    G.game_over   = 0;
    G.lines_last  = 0;
    G.score       = 0;
    G.lines_total = 0;
    G.level       = 0;
    G.drop_period = level_drop_period(0);

    redraw_board();   /* стирает старые залоченные клетки + текст GAME OVER внутри стакана */

    spawn();
    if (!G.game_over) {
        render_piece(G.cur.id, G.cur.rot, (uint8_t)G.cur.row, (uint8_t)G.cur.col);
    }
}
