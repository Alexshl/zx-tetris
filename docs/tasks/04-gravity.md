# Task 04: Гравитация, коллизии, лочинг

**Status**: DONE
**Зависит от**: 03
**Цель**: Фигура падает раз в N фреймов. При невозможности опуститься — «лочится» в массив `board[][]`, спавнится следующая (рандом). Коллизии со стенами и уже залоченными блоками работают.

## Acceptance criteria

- [ ] Фигура спавнится сверху по центру (колонка 3, строка 0) и плавно падает вниз
- [ ] При достижении дна или вершины стака фигура «фиксируется» — её клетки записываются в `board[][]`
- [ ] Сразу после лочинга спавнится новая случайная фигура
- [ ] Если новая фигура спавнится в занятой клетке — пока просто остановись (game-over обработаем в task 09)
- [ ] При падении нет мерцания (правильный erase → render порядок)

## Структуры

`src/game.h`:

```c
#ifndef GAME_H
#define GAME_H
#include <stdint.h>
#include "pieces.h"

#define COLS 10
#define ROWS 20

typedef struct {
    piece_id_t id;
    uint8_t    rot;   // 0..3
    int8_t     row;   // top-left of 4x4 box; может быть отрицательным на спавне
    int8_t     col;
} active_piece_t;

typedef struct {
    uint8_t board[ROWS][COLS];  // 0 = пусто, иначе piece_id+1
    active_piece_t cur;
    uint8_t       game_over;
} game_state_t;

extern game_state_t G;

void game_init(void);
uint8_t game_tick(void);   // вызывается раз в кадр; возвращает 1 если что-то поменялось

#endif
```

## Шаги

### 1. Random

z88dk даёт `rand()` (newlib). Для seed используем счётчик кадров или `FRAMES` (системная переменная ROM 23672). Можно так:

```c
#include <stdlib.h>
#include <stdint.h>

static uint8_t prng_byte(void) {
    return (uint8_t)rand();
}
```

### 2. `src/game.c`

```c
#include "game.h"
#include "render.h"
#include "pieces.h"
#include <stdlib.h>

game_state_t G;

static uint8_t collides(piece_id_t id, uint8_t rot, int8_t r, int8_t c) {
    uint16_t shape = piece_shapes[id][rot];
    for (uint8_t dr = 0; dr < 4; ++dr) {
        for (uint8_t dc = 0; dc < 4; ++dc) {
            if (!piece_bit(shape, dr, dc)) continue;
            int8_t br = r + dr;
            int8_t bc = c + dc;
            if (bc < 0 || bc >= COLS) return 1;
            if (br >= ROWS)            return 1;
            if (br >= 0 && G.board[br][bc]) return 1;
        }
    }
    return 0;
}

static void lock_piece(void) {
    uint16_t shape = piece_shapes[G.cur.id][G.cur.rot];
    for (uint8_t dr = 0; dr < 4; ++dr) {
        for (uint8_t dc = 0; dc < 4; ++dc) {
            if (!piece_bit(shape, dr, dc)) continue;
            int8_t br = G.cur.row + dr;
            int8_t bc = G.cur.col + dc;
            if (br >= 0 && br < ROWS && bc >= 0 && bc < COLS)
                G.board[br][bc] = (uint8_t)(G.cur.id + 1);
        }
    }
}

static void spawn(void) {
    G.cur.id  = (piece_id_t)(rand() % PIECE_COUNT);
    G.cur.rot = 0;
    G.cur.row = 0;
    G.cur.col = 3;
    if (collides(G.cur.id, G.cur.rot, G.cur.row, G.cur.col)) {
        G.game_over = 1;
    }
}

void game_init(void) {
    for (uint8_t r = 0; r < ROWS; ++r)
        for (uint8_t c = 0; c < COLS; ++c)
            G.board[r][c] = 0;
    G.game_over = 0;
    spawn();
    render_piece(G.cur.id, G.cur.rot, G.cur.row, G.cur.col);
}

uint8_t game_tick(void) {
    if (G.game_over) return 0;

    if (collides(G.cur.id, G.cur.rot, G.cur.row + 1, G.cur.col)) {
        lock_piece();
        // re-render залоченные клетки в "статичном" цвете
        spawn();
        if (!G.game_over)
            render_piece(G.cur.id, G.cur.rot, G.cur.row, G.cur.col);
        return 1;
    } else {
        erase_piece(G.cur.id, G.cur.rot, G.cur.row, G.cur.col);
        G.cur.row++;
        render_piece(G.cur.id, G.cur.rot, G.cur.row, G.cur.col);
        return 1;
    }
}
```

### 3. Главный цикл в `main.c`

```c
#include <intrinsic.h>
#include "render.h"
#include "game.h"

int main(void) {
    render_init();
    render_clear_board();
    game_init();

    uint8_t frame = 0;
    const uint8_t drop_period = 48;  // ~1 секунда

    while (!G.game_over) {
        intrinsic_halt();
        frame++;
        if (frame >= drop_period) {
            frame = 0;
            game_tick();
        }
    }
    while (1) intrinsic_halt();
    return 0;
}
```

### 4. Сборка

Добавить `src/game.c` в `SRCS`.

## Verification

Запусти `make run`. Должно:
1. Появиться фигура сверху.
2. Раз в секунду она опускается на одну клетку.
3. Достигнув дна — остаётся там нарисованной в своём цвете.
4. Сверху появляется новая случайная фигура.
5. Стак постепенно заполняется.

## Заметки

- При лочинге клетки фигуры уже нарисованы (это последний кадр перед лочингом), поэтому отдельно их перерисовывать не обязательно — атрибуты сохранятся.
- `rand()` в z88dk даёт детерминированную последовательность, если не сидить. Это нормально для MVP. Сидинг от `FRAMES` (адрес 23672) можно сделать позже.
- `drop_period = 48` фреймов = 0.96 с. В task 07 это станет переменной от уровня.

## Completion note

Реализовано 2026-05-26. Созданы `src/game.h` и `src/game.c` с функциями инициализации, проверки коллизий (стены, пол, залоченные клетки с защитой `br >= 0`), лочинга фигуры в массив board и рандомного спавна. Фигура падает один раз в DROP_PERIOD=48 фреймов (~0.96с), game loop переписан с `intrinsic_halt()` и counter-based gravity. Двойное определение констант избегнуто переиспользованием BOARD_COLS/BOARD_ROWS из render.h. Сборка чистая, tetris.tap=6273б.
