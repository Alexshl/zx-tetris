#ifndef PIECES_H
#define PIECES_H
#include <stdint.h>

typedef enum {
    PIECE_I = 0, PIECE_O, PIECE_T, PIECE_S, PIECE_Z, PIECE_L, PIECE_J,
    PIECE_COUNT
} piece_id_t;

extern const uint16_t piece_shapes[PIECE_COUNT][4];
extern const uint8_t  piece_colors[PIECE_COUNT];

#define piece_bit(shape, r, c) ((uint8_t)(((shape) >> (15 - ((r) * 4 + (c)))) & 1u))

#endif /* PIECES_H */
