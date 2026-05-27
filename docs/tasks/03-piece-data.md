# Task 03: Данные тетромино и отрисовка одной фигуры

**Status**: DONE
**Зависит от**: 02
**Цель**: Объявить 7 фигур × 4 поворота как компактную таблицу, реализовать функции «нарисовать фигуру в позиции», «стереть фигуру». Прокликать все фигуры/повороты клавишами для визуальной проверки.

## Acceptance criteria

- [ ] В коде есть таблица 7×4 фигур, каждая = 4×4 битмапа
- [ ] Одна фигура отрисовывается в верхней части стакана с цветом по типу
- [ ] Клавиша «1» переключает тип фигуры (циклически 0..6)
- [ ] Клавиша «2» переключает поворот (0..3)
- [ ] Все 7 фигур и все 4 поворота визуально корректны (I — палка, O — квадрат, T/S/Z/L/J — соответствующие формы)

## Дизайн данных

Каждая фигура в одном повороте — `uint16_t` где 16 бит = 4×4 битов (row-major, MSB = верхний-левый).

```c
typedef enum { PIECE_I=0, PIECE_O, PIECE_T, PIECE_S, PIECE_Z, PIECE_L, PIECE_J, PIECE_COUNT } piece_id_t;

extern const uint16_t piece_shapes[PIECE_COUNT][4];   // [id][rotation]
extern const uint8_t  piece_colors[PIECE_COUNT];      // INK + PAPER_BLACK
```

Цветовая карта (INK на чёрном PAPER):
| id | имя | INK |
|---|---|---|
| 0 | I | 5 (CYAN) |
| 1 | O | 6 (YELLOW) |
| 2 | T | 3 (MAGENTA) |
| 3 | S | 4 (GREEN) |
| 4 | Z | 2 (RED) |
| 5 | L | 6 (YELLOW) + BRIGHT |
| 6 | J | 1 (BLUE) + BRIGHT |

(BRIGHT для L и J — чтобы отличать от O и обычного синего фона.)

Пример формы I в 4 поворотах (битмапы 4×4, MSB = top-left):

```
rotation 0:    rotation 1:   rotation 2:   rotation 3:
0000           0010          0000          0100
1111           0010          0000          0100
0000           0010          1111          0100
0000           0010          0000          0100

0x0F00         0x2222        0x00F0        0x4444
```

Аналогично для остальных шести фигур. Конкретные значения — в таблице ниже.

## Шаги

### 1. `src/pieces.h`

```c
#ifndef PIECES_H
#define PIECES_H
#include <stdint.h>

typedef enum {
    PIECE_I=0, PIECE_O, PIECE_T, PIECE_S, PIECE_Z, PIECE_L, PIECE_J,
    PIECE_COUNT
} piece_id_t;

extern const uint16_t piece_shapes[PIECE_COUNT][4];
extern const uint8_t  piece_colors[PIECE_COUNT];

// row-major bit at (r,c) in shape word
static inline uint8_t piece_bit(uint16_t shape, uint8_t r, uint8_t c) {
    return (shape >> (15 - (r * 4 + c))) & 1;
}

#endif
```

### 2. `src/pieces.c`

```c
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
```

> **Важно**: перепроверь битмапы вручную, нарисовав на бумаге. Если фигура выглядит не как должна — поправь конкретное hex-значение. Это рутинная работа, легко промахнуться на бит.

### 3. Расширить `src/render.c/h`

Добавить:

```c
void render_cell(uint8_t row, uint8_t col, uint8_t attr); // одна клетка
void render_piece(piece_id_t id, uint8_t rot, uint8_t row, uint8_t col);
void erase_piece (piece_id_t id, uint8_t rot, uint8_t row, uint8_t col);
```

Реализация `render_cell`: получить адреса экрана и атрибута для знакоместа (BOARD_X+col, BOARD_Y+row), записать 8 байт `0xFF` в битмап и `attr` в атрибут.

```c
void render_cell(uint8_t row, uint8_t col, uint8_t attr) {
    uint8_t cx = BOARD_X + col;
    uint8_t cy = BOARD_Y + row;
    uint8_t *screen = zx_cxy2saddr(cx, cy);
    for (uint8_t i = 0; i < 8; ++i) {
        *screen = (attr & 0x40) ? 0x00 : 0xFF;  // если PAPER_BLACK без INK = пусто
        screen = (uint8_t*)((uintptr_t)screen + 256); // следующая пиксельная строка в знакоместе
    }
    *zx_cxy2aaddr(cx, cy) = attr;
}
```

> Проще: для блока всегда пишем `0xFF` × 8 (полный квадрат), цвет = атрибут. Для «пусто» вызывать `render_cell(r,c, PAPER_BLACK|INK_BLACK)`.

`render_piece` / `erase_piece` — итерируют 4×4 битмапа, для каждого выставленного бита вызывают `render_cell`.

### 4. Тестовый цикл в `main.c`

```c
#include <input.h>
#include <intrinsic.h>
#include "render.h"
#include "pieces.h"

int main(void) {
    render_init();
    render_clear_board();

    piece_id_t id = PIECE_I;
    uint8_t rot = 0;
    render_piece(id, rot, 0, 3);

    while (1) {
        intrinsic_halt();
        if (in_key_pressed(IN_KEY_SCANCODE_1)) {
            erase_piece(id, rot, 0, 3);
            id = (id + 1) % PIECE_COUNT;
            render_piece(id, rot, 0, 3);
            while (in_key_pressed(IN_KEY_SCANCODE_1)) intrinsic_halt();
        }
        if (in_key_pressed(IN_KEY_SCANCODE_2)) {
            erase_piece(id, rot, 0, 3);
            rot = (rot + 1) & 3;
            render_piece(id, rot, 0, 3);
            while (in_key_pressed(IN_KEY_SCANCODE_2)) intrinsic_halt();
        }
    }
    return 0;
}
```

### 5. Сборка

Добавить `src/pieces.c` в `SRCS` в Makefile, `make run`.

## Verification

Прокликать все 7×4 = 28 состояний. Каждое должно выглядеть как ожидаемая форма соответствующей фигуры. Особенно проверить S/Z (зеркальны) и L/J (зеркальны).

## Заметки

- Если фигура «вытекает» из стакана при отрисовке — это ОК на этой задаче. Бoundary-check появится в task 04 вместе с коллизиями.
- z88dk-функция `zx_cxy2saddr(x, y)` берёт координаты в знакоместах (0..31, 0..23), возвращает указатель на верхнюю пиксельную строку этого знакоместа.
- В битмапе Спектрума пиксельные строки внутри одного знакоместа НЕ идят подряд по памяти — между ними шаг 256 байт. Это особенность раскладки экрана; z88dk это инкапсулирует, но если будешь оптимизировать — почитай про "ZX Spectrum screen layout".

## Completion note

Реализовано 2026-05-26. Созданы `src/pieces.h` (enum piece_id_t, extern таблицы, макрос piece_bit вместо static inline — sccz80 не поддерживает inline), `src/pieces.c` (7×4 битмапа фигур + 7 цветов с BRIGHT для L/J). В render.h/render.c добавлены три функции render_cell/render_piece/erase_piece (рендер solid 8×8 блока через 0xFF в bitmap + cell-color в attribute file). Все 28 hex-значений побитово проверены — соответствуют классическим NES-формам. Тестовый цикл main.c с клавишами 1/2 работает без ошибок, сборка зелёная (5267 байт).
