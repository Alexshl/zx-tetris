#include <arch/zx.h>
#include <string.h>
#include <stdint.h>
#include "render.h"
#include "pieces.h"

#define ERASE_ATTR (PAPER_BLACK | INK_BLACK)

#define SCREEN_BITMAP_ADDR  ((uint8_t*)0x4000)
#define SCREEN_BITMAP_SIZE  6144
#define SCREEN_ATTR_ADDR    ((uint8_t*)0x5800)
#define SCREEN_ATTR_SIZE    768
#define ROM_FONT_BASE       ((const uint8_t*)0x3C00)

#define FRAME_TOP_ROW    1
#define FRAME_BOTTOM_ROW 22
#define FRAME_LEFT_COL   7
#define FRAME_RIGHT_COL  18

#define HUD_COL  20

static void put_char(uint8_t row, uint8_t col, uint8_t ch)
{
    const uint8_t *src = ROM_FONT_BASE + ((uint16_t)ch * 8);
    uint8_t *dst = zx_cxy2saddr(col, row);
    uint8_t i;
    for (i = 0; i < 8; ++i) {
        *dst = src[i];
        dst += 256;
    }
}

static void put_str(uint8_t row, uint8_t col, const char *s)
{
    while (*s) {
        put_char(row, col, (uint8_t)*s);
        ++col;
        ++s;
    }
}

static void draw_frame(void)
{
    uint8_t r;
    put_str(FRAME_TOP_ROW,    FRAME_LEFT_COL, "+----------+");
    put_str(FRAME_BOTTOM_ROW, FRAME_LEFT_COL, "+----------+");
    for (r = (FRAME_TOP_ROW + 1); r < FRAME_BOTTOM_ROW; ++r) {
        put_char(r, FRAME_LEFT_COL,  '|');
        put_char(r, FRAME_RIGHT_COL, '|');
    }
}

static void draw_hud_placeholders(void)
{
    put_str(4,  HUD_COL, "SCORE");
    put_str(7,  HUD_COL, "LEVEL");
    put_str(10, HUD_COL, "LINES");
}

/* Cache: sentinel values force a redraw on first render_hud() call */
static uint32_t prev_score = 0xFFFFFFFFul;
static uint16_t prev_lines = 0xFFFFu;
static uint8_t  prev_level = 0xFFu;

/* Right-aligned, space-padded decimal into buf[width+1].
   First digit always written so v=0 produces "     0". */
static void u32_to_padded(uint32_t v, char *buf, uint8_t width)
{
    int8_t i;
    uint8_t first;
    buf[width] = '\0';
    first = 1;
    for (i = (int8_t)(width - 1); i >= 0; --i) {
        if (!first && v == 0) {
            buf[i] = ' ';
        } else {
            buf[i] = (char)('0' + (uint8_t)(v % 10));
            v = v / 10;
            first = 0;
        }
    }
}

void render_init(void)
{
    zx_border(INK_BLACK);
    memset(SCREEN_BITMAP_ADDR, 0,                                SCREEN_BITMAP_SIZE);
    memset(SCREEN_ATTR_ADDR,   PAPER_BLACK | INK_WHITE | BRIGHT, SCREEN_ATTR_SIZE);
    draw_frame();
    draw_hud_placeholders();
}

void render_clear_board(void)
{
    uint8_t *attr = SCREEN_ATTR_ADDR + ((uint16_t)BOARD_Y * 32) + BOARD_X;
    uint8_t r;
    for (r = 0; r < BOARD_ROWS; ++r) {
        memset(attr, PAPER_BLACK | INK_BLACK, BOARD_COLS);
        attr += 32;
    }
}

void render_cell(uint8_t row, uint8_t col, uint8_t attr)
{
    uint8_t cx = BOARD_X + col;
    uint8_t cy = BOARD_Y + row;
    uint8_t *dst  = zx_cxy2saddr(cx, cy);
    uint8_t *attp = zx_cxy2aaddr(cx, cy);
    uint8_t i;
    for (i = 0; i < 8; ++i) {
        *dst = 0xFF;
        dst += 256;
    }
    *attp = attr;
}

void render_piece(piece_id_t id, uint8_t rot, uint8_t row, uint8_t col)
{
    uint16_t shape = piece_shapes[id][rot & 3];
    uint8_t attr   = piece_colors[id];
    uint8_t r, c;
    for (r = 0; r < 4; ++r) {
        for (c = 0; c < 4; ++c) {
            if (piece_bit(shape, r, c)) {
                render_cell(row + r, col + c, attr);
            }
        }
    }
}

void erase_piece(piece_id_t id, uint8_t rot, uint8_t row, uint8_t col)
{
    uint16_t shape = piece_shapes[id][rot & 3];
    uint8_t r, c;
    for (r = 0; r < 4; ++r) {
        for (c = 0; c < 4; ++c) {
            if (piece_bit(shape, r, c)) {
                render_cell(row + r, col + c, ERASE_ATTR);
            }
        }
    }
}

void render_hud(uint32_t score, uint16_t lines, uint8_t level)
{
    char buf[8];
    if (score != prev_score) {
        u32_to_padded(score, buf, 6);
        put_str(5, HUD_COL, buf);
        prev_score = score;
    }
    if (level != prev_level) {
        u32_to_padded((uint32_t)level, buf, 6);
        put_str(8, HUD_COL, buf);
        prev_level = level;
    }
    if (lines != prev_lines) {
        u32_to_padded((uint32_t)lines, buf, 6);
        put_str(11, HUD_COL, buf);
        prev_lines = lines;
    }
}

void render_hud_force_redraw(void)
{
    prev_score = 0xFFFFFFFFul;
    prev_lines = 0xFFFFu;
    prev_level = 0xFFu;
}

void render_game_over(void)
{
    uint8_t *attr;
    uint8_t  i;
    put_str(10, 8, "GAME OVER");
    put_str(12, 8, "PRESS ENT");
    attr = SCREEN_ATTR_ADDR + ((uint16_t)10 * 32) + 8;
    for (i = 0; i < 9; ++i) attr[i] = PAPER_BLACK | INK_WHITE | BRIGHT;
    attr = SCREEN_ATTR_ADDR + ((uint16_t)12 * 32) + 8;
    for (i = 0; i < 9; ++i) attr[i] = PAPER_BLACK | INK_WHITE | BRIGHT;
}
