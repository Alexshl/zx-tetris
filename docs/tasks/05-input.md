# Task 05: Управление с клавиатуры

**Status**: DONE
**Зависит от**: 04
**Цель**: QAOP-управление + Space (hard drop) + Enter (rotate). Дебаунс — поворот и hard drop срабатывают по фронту, движение влево/вправо имеет autorepeat.

## Acceptance criteria

- [ ] `O` — фигура двигается влево, `P` — вправо
- [ ] `A` — soft drop (ускоренное падение, пока нажата)
- [ ] `Enter` или `Q` — поворот по часовой стрелке
- [ ] `Space` — hard drop (фигура мгновенно падает до низа и лочится)
- [ ] При зажатии левой/правой — фигура движется с autorepeat ~10 Hz после задержки 0.25 с
- [ ] Поворот и hard drop срабатывают РОВНО ОДИН раз на нажатие
- [ ] Невозможные ходы (упор в стенку или блок) просто игнорируются, без визуальных артефактов

## Шаги

### 1. `src/input.h`

```c
#ifndef INPUT_H
#define INPUT_H
#include <stdint.h>

typedef struct {
    uint8_t left;       // edge or repeat
    uint8_t right;
    uint8_t soft_drop;  // held
    uint8_t rotate;     // edge only
    uint8_t hard_drop;  // edge only
} input_state_t;

void input_init(void);
void input_poll(input_state_t *out);   // вызывать раз в кадр

#endif
```

### 2. `src/input.c`

```c
#include "input.h"
#include <input.h>     // z88dk newlib: in_key_pressed + scancodes

#define REPEAT_DELAY 12   // фреймов до начала autorepeat
#define REPEAT_RATE  4    // фреймов между autorepeat

static struct {
    uint8_t left_held_frames;
    uint8_t right_held_frames;
    uint8_t rotate_was_down;
    uint8_t hard_was_down;
} S;

void input_init(void) {
    S.left_held_frames = 0;
    S.right_held_frames = 0;
    S.rotate_was_down = 0;
    S.hard_was_down = 0;
}

static uint8_t autorepeat(uint8_t held_now, uint8_t *frames) {
    if (!held_now) { *frames = 0; return 0; }
    if (*frames == 0) { (*frames)++; return 1; }   // первый раз
    (*frames)++;
    if (*frames < REPEAT_DELAY) return 0;
    if ((*frames - REPEAT_DELAY) % REPEAT_RATE == 0) return 1;
    return 0;
}

void input_poll(input_state_t *out) {
    uint8_t l = in_key_pressed(IN_KEY_SCANCODE_o);
    uint8_t r = in_key_pressed(IN_KEY_SCANCODE_p);
    uint8_t d = in_key_pressed(IN_KEY_SCANCODE_a);
    uint8_t rot = in_key_pressed(IN_KEY_SCANCODE_q)
                | in_key_pressed(IN_KEY_SCANCODE_ENTER);
    uint8_t hd = in_key_pressed(IN_KEY_SCANCODE_SPACE);

    out->left  = autorepeat(l, &S.left_held_frames);
    out->right = autorepeat(r, &S.right_held_frames);
    out->soft_drop = d ? 1 : 0;
    out->rotate    = (rot && !S.rotate_was_down) ? 1 : 0;
    out->hard_drop = (hd  && !S.hard_was_down)   ? 1 : 0;
    S.rotate_was_down = rot;
    S.hard_was_down   = hd;
}
```

### 3. Расширить `src/game.h/c`

Добавить публичные функции, использующие `collides`:

```c
void game_move(int8_t dc);   // -1 / +1
void game_rotate(void);      // CW
void game_hard_drop(void);
void game_soft_drop_step(void);  // как один шаг гравитации
```

Реализация:

```c
void game_move(int8_t dc) {
    if (G.game_over) return;
    if (collides(G.cur.id, G.cur.rot, G.cur.row, G.cur.col + dc)) return;
    erase_piece(G.cur.id, G.cur.rot, G.cur.row, G.cur.col);
    G.cur.col += dc;
    render_piece(G.cur.id, G.cur.rot, G.cur.row, G.cur.col);
}

void game_rotate(void) {
    if (G.game_over) return;
    uint8_t new_rot = (G.cur.rot + 1) & 3;
    if (collides(G.cur.id, new_rot, G.cur.row, G.cur.col)) return;
    erase_piece(G.cur.id, G.cur.rot, G.cur.row, G.cur.col);
    G.cur.rot = new_rot;
    render_piece(G.cur.id, G.cur.rot, G.cur.row, G.cur.col);
}

void game_hard_drop(void) {
    if (G.game_over) return;
    erase_piece(G.cur.id, G.cur.rot, G.cur.row, G.cur.col);
    while (!collides(G.cur.id, G.cur.rot, G.cur.row + 1, G.cur.col)) {
        G.cur.row++;
    }
    render_piece(G.cur.id, G.cur.rot, G.cur.row, G.cur.col);
    // лочинг произойдёт в следующем game_tick
}

void game_soft_drop_step(void) {
    game_tick();  // одна гравитационная клетка
}
```

### 4. Главный цикл в `main.c`

```c
#include <intrinsic.h>
#include "render.h"
#include "game.h"
#include "input.h"

int main(void) {
    render_init();
    render_clear_board();
    game_init();
    input_init();

    uint8_t frame = 0;
    const uint8_t drop_period = 48;
    input_state_t in;

    while (!G.game_over) {
        intrinsic_halt();
        input_poll(&in);
        if (in.left)      game_move(-1);
        if (in.right)     game_move(+1);
        if (in.rotate)    game_rotate();
        if (in.hard_drop) game_hard_drop();
        if (in.soft_drop) { game_soft_drop_step(); frame = 0; continue; }
        if (++frame >= drop_period) { frame = 0; game_tick(); }
    }
    while (1) intrinsic_halt();
    return 0;
}
```

### 5. Сборка

Добавить `src/input.c` в `SRCS`.

## Verification

- Удерживание O/P — фигура едет до стены и упирается, без артефактов.
- Q/Enter — каждый раз ровно один поворот, даже если зажать.
- Space — фигура мгновенно «приклеивается» к низу.
- A — фигура падает заметно быстрее.

## Заметки

- `IN_KEY_SCANCODE_*` определены в `<input.h>` (z88dk newlib). На Spectrum scan'ятся группы из 8 клавиш через порты — функция инкапсулирует это.
- Если хочется проверить scancodes — посмотри `z88dk-zx-spectrum-input.md` в репо z88dk.
- На реальном железе клавиатура «дребезжит» — поэтому autorepeat должен быть на 12+ фреймах задержки. Меньше — ловится «двойной шаг» при одном нажатии.

## Completion note

Реализовано 2026-05-26. Создан модуль input с дебаунсом для rotate/hard_drop (edge detection через флаги `was_down`) и autorepeat для движения (12 фреймов задержки, затем 4 кадра между повторами). В game.c добавлены четыре функции: `game_move(dc)`, `game_rotate()`, `game_hard_drop()` с inline лочингом, `game_soft_drop_step()`. main.c переписан с input_poll в начале цикла и условиями для каждого ввода, soft_drop сбрасывает frame. Сборка успешна.
