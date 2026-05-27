# Task 08: HUD — отображение SCORE / LEVEL / LINES

**Status**: DONE
**Зависит от**: 07
**Цель**: Показывать актуальные значения счёта, уровня и линий справа от стакана. Обновлять только при изменении, чтобы не мерцать.

## Acceptance criteria

- [ ] Под надписями SCORE/LEVEL/LINES показаны актуальные числа
- [ ] При изменении любого из значений HUD сразу обновляется
- [ ] При отсутствии изменений HUD НЕ перерисовывается (нет лишних printf)
- [ ] Числа выровнены по правому краю в поле фиксированной ширины (например, 6 символов)
- [ ] Score форматируется без `%lu` мусора (z88dk newlib поддерживает long, но проверить)

## Шаги

### 1. Расширить `render.h`

```c
void render_hud(uint32_t score, uint16_t lines, uint8_t level);
void render_hud_force_redraw(void);  // первый вывод (вызвать в render_init)
```

### 2. Реализация в `render.c`

Подход: кешируем предыдущие значения, выводим только при изменении.

```c
#include <stdio.h>

static uint32_t prev_score = 0xFFFFFFFFul;
static uint16_t prev_lines = 0xFFFF;
static uint8_t  prev_level = 0xFF;

static void print_at(uint8_t row, uint8_t col, const char *s) {
    printf("\x16%c%c%s", row, col, s);
}

static void print_u32_right(uint8_t row, uint8_t col, uint32_t v, uint8_t width) {
    char buf[12];
    sprintf(buf, "%*lu", (int)width, (unsigned long)v);
    print_at(row, col, buf);
}

static void print_u16_right(uint8_t row, uint8_t col, uint16_t v, uint8_t width) {
    char buf[8];
    sprintf(buf, "%*u", (int)width, (unsigned)v);
    print_at(row, col, buf);
}

void render_hud(uint32_t score, uint16_t lines, uint8_t level) {
    if (score != prev_score) {
        print_u32_right(5, 20, score, 6);
        prev_score = score;
    }
    if (level != prev_level) {
        print_u16_right(8, 20, level, 6);
        prev_level = level;
    }
    if (lines != prev_lines) {
        print_u16_right(11, 20, lines, 6);
        prev_lines = lines;
    }
}

void render_hud_force_redraw(void) {
    prev_score = 0xFFFFFFFFul;
    prev_lines = 0xFFFF;
    prev_level = 0xFF;
}
```

### 3. Вызвать из главного цикла

В `main.c` после `intrinsic_halt()`:

```c
render_hud(score, lines_total, level);
```

И в начале (после `render_init`):

```c
render_hud_force_redraw();
render_hud(0, 0, 0);
```

### 4. Убрать плейсхолдеры из render.c (task 02)

Изначально функция `draw_hud_placeholders` рисовала "     0" — теперь это делает `render_hud`. Оставь только заголовки SCORE/LEVEL/LINES, числа удали.

## Verification

- При запуске видны три нуля под заголовками.
- Очисти линию — SCORE обновляется немедленно, без мерцания.
- Достигни 10 линий — LEVEL = 1, LINES = 10.
- Очисти 4 за раз — SCORE прыгает на 1200, без визуальных глитчей.

## Заметки

- `sprintf("%lu", ...)` в z88dk newlib работает, но раздувает бинарь — добавляет код long-формат printf. Если бинарь становится крупный, можно сделать ручной `u32_to_str` через деление.
- `%*u` (минимальная ширина) — стандартный printf, работает в newlib.
- Управляющий байт `\x16 row col` — это «AT row,col» из ZX BASIC, координаты в знакоместах (0..23, 0..31).
- HUD занимает 12 знакомест в ширину (с колонки 20 до 31) — достаточно для 6-значного числа.

## Completion note

Реализовано 2026-05-26. Отклонение от изначального плана: вместо printf/sprintf и `\x16` AT-control (не работают в z88dk newlib +zx с startup=0) реализован прямой рендер через `put_str` (копирование ROM-font glyph в bitmap) с собственной функцией `u32_to_padded` для преобразования uint32_t в строку с right-padding пробелами. В render.c добавлены file-scope cache переменные для предыдущих значений score/lines/level, функции `render_hud()` с cache-сравнением и `render_hud_force_redraw()` для сброса sentinel-значений. Все числа выводятся правый край в HUD без мерцания. В будущих задачах при необходимости показать число — использовать `u32_to_padded` + `put_str`, не printf.
