# Tetris ZX Spectrum 128K — Индекс задач

**Проект**: Классический Тетрис для ZX Spectrum 128K
**Тулчейн**: z88dk (C → Z80 asm) + Fuse for macOS
**Полный план**: [`docs/start.md`](../start.md)

## Прогресс

| #   | Задача                                                      | Статус | Файлы выхода                                  |
| --- | ----------------------------------------------------------- | ------ | --------------------------------------------- |
| 01  | [Setup toolchain + skeleton](01-setup-toolchain.md)         | DONE   | `Makefile`, `zpragma.inc`, `src/main.c`       |
| 02  | [Пустой стакан и рамка](02-empty-playfield.md)              | DONE   | `src/render.c`, `src/render.h`                |
| 03  | [Данные тетромино и отрисовка фигуры](03-piece-data.md)     | DONE   | `src/pieces.c`, `src/pieces.h`                |
| 04  | [Гравитация, коллизии, лочинг](04-gravity.md)               | DONE   | `src/game.c`, `src/game.h`                    |
| 05  | [Управление с клавиатуры](05-input.md)                      | DONE   | `src/input.c`, `src/input.h`                  |
| 06  | [Очистка заполненных линий](06-line-clear.md)               | DONE   | `src/game.c` (расширение)                     |
| 07  | [Счёт, уровни, ускорение](07-scoring-levels.md)             | DONE   | `src/game.c` (расширение)                     |
| 08  | [HUD: SCORE / LEVEL / LINES](08-hud-display.md)             | DONE   | `src/render.c` (расширение)                   |
| 09  | [Game Over и рестарт](09-game-over.md)                      | DONE   | `src/main.c`, `src/game.c` (расширение)       |
| 10  | [Звуковые эффекты на бипере](10-sound-fx.md)                | DONE   | `src/sound.c`, `src/sound.h`                  |

## Легенда статусов

- **TODO** — не начато
- **IN PROGRESS** — в работе
- **BLOCKED** — ждёт прояснения или внешней зависимости
- **DONE** — реализовано и проверено в Fuse

## Как работать с индексом

1. Открыть текущую задачу по ссылке из таблицы.
2. Выполнить шаги, отметить acceptance criteria.
3. Запустить `make run` и проверить визуально в Fuse.
4. Когда всё чисто — поменять статус на **DONE** в этом файле и в шапке задачи.
5. Перейти к следующей.

## Следующий шаг

Все задачи MVP выполнены 2026-05-26. Классический Тетрис для ZX Spectrum 128K завершён.

## Вне скоупа MVP (для будущих задач)

- Next-piece превью
- Hold piece, ghost piece, SRS wall-kicks
- AY-музыка (нужен Vortex Tracker / AYFX и player)
- Kempston joystick
- Меню, таблица рекордов, настройки
- Загрузка на реальное железо (DivMMC, аудиовход)
