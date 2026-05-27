# Task 02: Пустой стакан и HUD-заглушки

**Status**: DONE
**Зависит от**: 01
**Цель**: Нарисовать рамку игрового стакана 10×20 клеток и плейсхолдеры HUD справа. Никаких фигур, никакой логики — только статичная картинка.

## Acceptance criteria

- [ ] При запуске виден прямоугольник 10×20 пустых клеток
- [ ] Рамка стакана нарисована символьной графикой (вертикальные линии слева/справа, дно)
- [ ] Справа от стакана текст «SCORE 0», «LEVEL 0», «LINES 0»
- [ ] Никакого мерцания, экран статичный
- [ ] Программа крутится в бесконечном `intrinsic_halt()` — Fuse работает плавно

## Геометрия экрана

Spectrum: 256×192 пикс = 32×24 знакомест. Знакоместо = 8×8 пикс.

Стакан 10×20 клеток → размещаем по знакоместам:
- Столбцы знакомест: **8..17** (10 штук, левый край в пикс. координате 64)
- Строки знакомест: **2..21** (20 штук, верх в пикс. координате 16)
- Рамка вокруг: столбцы 7 и 18, строки 1 и 22

HUD справа:
- Столбец 20, начиная со строки 4:
  - строка 4: «SCORE»
  - строка 5: «     0»
  - строка 7: «LEVEL»
  - строка 8: «     0»
  - строка 10: «LINES»
  - строка 11: «     0»

## Шаги

### 1. Создать `src/render.h`

```c
#ifndef RENDER_H
#define RENDER_H

#define BOARD_COLS 10
#define BOARD_ROWS 20

#define BOARD_X 8    // column (char cell) of top-left of board
#define BOARD_Y 2    // row (char cell)

void render_init(void);
void render_clear_board(void);

#endif
```

### 2. Создать `src/render.c`

Использовать z88dk-функции:
- `zx_cls(attr)` — очистка экрана с заданным атрибутом
- `zx_border(color)` — цвет бордюра
- `printf("\x16 row col" "...")` — позиционирование текста
- Атрибуты по координатам знакомест: запись в `0x5800 + row*32 + col`

Базовый каркас:

```c
#include <arch/zx.h>
#include <stdio.h>
#include <string.h>
#include "render.h"

static void draw_frame(void) {
    // вертикальные линии: знак "|" в столбцах 7 и 18, строки 1..22
    for (uint8_t r = 1; r <= 22; ++r) {
        printf("\x16%c%c|", r, 7);
        printf("\x16%c%c|", r, 18);
    }
    // дно: "+----------+" в строке 22
    printf("\x16%c%c+----------+", 22, 7);
    // верх: "+----------+" в строке 0
    printf("\x16%c%c+----------+", 0, 7);
}

static void draw_hud_placeholders(void) {
    printf("\x16%c%c" "SCORE", 4, 20);
    printf("\x16%c%c" "     0", 5, 20);
    printf("\x16%c%c" "LEVEL", 7, 20);
    printf("\x16%c%c" "     0", 8, 20);
    printf("\x16%c%c" "LINES", 10, 20);
    printf("\x16%c%c" "     0", 11, 20);
}

void render_init(void) {
    zx_border(INK_BLACK);
    zx_cls(PAPER_BLACK | INK_WHITE);
    draw_frame();
    draw_hud_placeholders();
}

void render_clear_board(void) {
    // заполняем 10x20 знакомест пустым атрибутом (тёмный paper)
    uint8_t *attr = (uint8_t*)(0x5800 + BOARD_Y * 32 + BOARD_X);
    for (uint8_t r = 0; r < BOARD_ROWS; ++r) {
        memset(attr, PAPER_BLACK | INK_BLACK, BOARD_COLS);
        attr += 32;
    }
}
```

### 3. Обновить `src/main.c`

```c
#include <intrinsic.h>
#include "render.h"

int main(void) {
    render_init();
    render_clear_board();
    while (1) intrinsic_halt();
    return 0;
}
```

### 4. Обновить `Makefile`

Добавить `src/render.c` в `SRCS`:

```make
SRCS = src/main.c src/render.c
```

### 5. Собрать и запустить

```bash
make run
```

## Verification

- Стакан выглядит как прямоугольник с рамкой `+----------+` и `|` по бокам.
- Внутри стакана — равномерный чёрный фон (10×20 знакомест).
- Справа три блока HUD.
- Border экрана чёрный.

## Заметки

- `intrinsic_halt()` останавливает CPU до следующего vsync — это правильный способ idle-loop'а на Спектруме, иначе процессор греется.
- Подключай `<arch/zx.h>` для констант `INK_*`, `PAPER_*` и функций `zx_cls`, `zx_border`.
- Если рамка ASCII-символами выглядит уродливо, на task 08 можно заменить её UDG-символами (User Defined Graphics — пара переопределённых знаков для красивого угла и линии).

## Completion note

Реализовано 2026-05-26. **Первая итерация (printf + \x16 AT-контроль)** собралась чисто, но в Fuse выводила только сырые байты — newlib +zx с дефолтным `-startup=0` не подключает TTY-эмулятор, поэтому printf-рендеринг позиционируемого вывода недоступен. **Вывод для будущих задач**: в newlib +zx с startup=0 нельзя использовать printf для позиционируемого текста; только прямой рендеринг в bitmap и attribute file.

**Реальная реализация**: прямой рендеринг через `memset` для очистки bitmap (0x4000, 6144 байт = 0) и attribute file (0x5800, 768 байт = `PAPER_BLACK|INK_WHITE|BRIGHT` = 0x47), копирование 8-байтовых глифов из ROM-font (`0x3C00 + ch*8`) в bitmap через `zx_cxy2saddr(col, row)` с шагом +256 между pixel-rows внутри знакоместа.

**Дополнительный REWORK**: попаданы аргументы `zx_cxy2saddr` — функция ожидает `(x=col, y=row)`, а не `(row, col)`. **Для будущих задач**: всегда проверять сигнатуру `zx_cxy2saddr(uint8_t x, uint8_t y)` перед использованием.

Финальный артефакт: `build/tetris.tap` = 4294 байта (на 1542 байта меньше первой версии). Визуальная верификация в Fuse: чёрный фон, ярко-белая рамка `+----------+` с `|` по сторонам, 10×20 пусто поле, HUD справа (SCORE/0, LEVEL/0, LINES/0) на правильных местах, мерцания нет.
