#include "pieces.h"
#include <arch/zx.h>

const uint16_t piece_shapes[PIECE_COUNT][4] = {
    /* I */ { 0x0F00, 0x2222, 0x00F0, 0x4444 },
    /* O */ { 0x0660, 0x0660, 0x0660, 0x0660 },
    /* T */ { 0x0E40, 0x4C40, 0x4E00, 0x4640 },
    /* S */ { 0x06C0, 0x4620, 0x06C0, 0x4620 },
    /* Z */ { 0x0C60, 0x2640, 0x0C60, 0x2640 },
    /* L */ { 0x2E00, 0x4460, 0x0E80, 0xC440 },
    /* J */ { 0x8E00, 0x6440, 0x0E20, 0x44C0 },
};

const uint8_t piece_colors[PIECE_COUNT] = {
    INK_CYAN    | PAPER_BLACK,
    INK_YELLOW  | PAPER_BLACK,
    INK_MAGENTA | PAPER_BLACK,
    INK_GREEN   | PAPER_BLACK,
    INK_RED     | PAPER_BLACK,
    INK_YELLOW  | PAPER_BLACK | BRIGHT,
    INK_BLUE    | PAPER_BLACK | BRIGHT,
};
