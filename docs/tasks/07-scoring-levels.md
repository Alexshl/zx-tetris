# Task 07: Счёт, уровень, ускорение

**Status**: DONE
**Зависит от**: 06
**Цель**: Считать очки по NES-формуле, увеличивать уровень каждые 10 очищенных линий, ускорять падение по таблице уровней.

## Acceptance criteria

- [ ] Score растёт после каждой очистки линий по формуле `points[N] * (level + 1)`
- [ ] Каждые 10 линий уровень увеличивается на 1
- [ ] `drop_period` (фреймов между шагами гравитации) уменьшается с уровнем по таблице
- [ ] На уровне 9+ фигура падает заметно быстрее, чем на 0
- [ ] Hard drop не даёт бонусных очков в MVP (или даёт +2/клетка — на твой выбор)

## Формулы

NES Tetris scoring (за один лочинг, в зависимости от количества очищенных линий N):

| N | Название | Очки |
|---|---|---|
| 0 | — | 0 |
| 1 | Single | 40 × (level+1) |
| 2 | Double | 100 × (level+1) |
| 3 | Triple | 300 × (level+1) |
| 4 | Tetris | 1200 × (level+1) |

Уровень растёт каждые 10 линий (`lines_total / 10`). Таблица `drop_period` (фреймов):

| level | period (frames @ 50 Hz) | ~delay |
|---|---|---|
| 0  | 48 | 0.96 c |
| 1  | 43 | 0.86 c |
| 2  | 38 | 0.76 c |
| 3  | 33 | 0.66 c |
| 4  | 28 | 0.56 c |
| 5  | 23 | 0.46 c |
| 6  | 18 | 0.36 c |
| 7  | 13 | 0.26 c |
| 8  | 8  | 0.16 c |
| 9  | 6  | 0.12 c |
| 10–12 | 5 | 0.10 c |
| 13–15 | 4 | 0.08 c |
| 16–18 | 3 | 0.06 c |
| 19–28 | 2 | 0.04 c |
| 29+   | 1 | 0.02 c (kill screen) |

## Шаги

### 1. Расширить `game.h`

```c
extern uint32_t score;
extern uint16_t lines_total;
extern uint8_t  level;
extern uint8_t  drop_period;

void game_apply_score(uint8_t lines);  // вызвать после clear
uint8_t level_drop_period(uint8_t lvl);
```

### 2. `game.c`

```c
uint32_t score = 0;
uint16_t lines_total = 0;
uint8_t  level = 0;
uint8_t  drop_period = 48;

static const uint8_t period_table[] = {
    48,43,38,33,28,23,18,13,8,6,
    5,5,5,
    4,4,4,
    3,3,3,
    2,2,2,2,2,2,2,2,2,2,
    1
};

uint8_t level_drop_period(uint8_t lvl) {
    if (lvl > 29) lvl = 29;
    return period_table[lvl];
}

void game_apply_score(uint8_t lines) {
    static const uint16_t pts[5] = {0, 40, 100, 300, 1200};
    if (lines > 4) lines = 4;
    score += (uint32_t)pts[lines] * (level + 1);
    lines_total += lines;
    uint8_t new_level = lines_total / 10;
    if (new_level != level) {
        level = new_level;
        drop_period = level_drop_period(level);
    }
}
```

### 3. Вызвать из `game_tick` после лочинга

После `lines_cleared_last = scan_and_clear_lines();` добавь:

```c
if (lines_cleared_last)
    game_apply_score(lines_cleared_last);
```

### 4. Использовать `drop_period` в `main.c`

Заменить `const uint8_t drop_period = 48;` на чтение глобальной переменной:

```c
extern uint8_t drop_period;
...
if (++frame >= drop_period) { frame = 0; game_tick(); }
```

### 5. Опционально: бонус за hard drop

В `game_hard_drop` посчитать сколько строк пролетела фигура и добавить `score += rows * 2`. NES не давал этого, но современные версии да.

## Verification

- Очисти 1 линию — score должен стать 40 (на уровне 0).
- Очисти Tetris (4 линии) — score = 1200.
- После 10 линий — `drop_period` упал с 48 до 43 (визуально чуть быстрее).
- После 100 линий (level 10) — игра ОЧЕНЬ быстрая.

> HUD пока не обновляется — числа в task 08. На этой задаче проверяй через дебаг (можно временно вызвать `printf("\x16\x00\x00score=%lu lvl=%u", score, level)` в углу).

## Заметки

- `uint32_t` для score: на NES максимум ~1.6M, помещается. Если хочется поменьше — `uint16_t` (max 65535) для casual игры тоже хватит.
- `period_table` лежит в `const` массиве → z88dk кладёт в code/ROM-область, не съедает RAM.
- На уровне 29 фигура движется быстрее, чем игрок может реагировать — это «kill screen», норма для NES Tetris.

## Completion note

Реализовано 2026-05-26. Добавлены четыре поля в `game_state_t` (`score`, `lines_total`, `level`, `drop_period`), что соответствует архитектурному правилу «всё состояние в G». Реализована NES-формула подсчёта очков с множителем по уровню, автоматический рост уровня каждые 10 линий и динамическое ускорение падения по таблице из 30 значений с защитой от overflow.
