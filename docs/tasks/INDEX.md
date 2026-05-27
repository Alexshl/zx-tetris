# Tetris ZX Spectrum 128K — Индекс задач

**Проект**: Классический Тетрис для ZX Spectrum 128K
**Тулчейн**: Docker контейнер (z88dk, ZEsarUX, дизассемблеры)
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
| 11  | [Docker-эмулятор: базовый образ](11-docker-emulator-base.md) | DONE | `docker/Dockerfile`, `docker/entrypoint.sh`, `Makefile` |
| 12  | [Smoke-тест .tap через ZRCP](12-smoke-test.md)              | DONE   | `tools/smoke.sh`, `Makefile`                  |
| 13  | [Интеграционные сценарии + ticks-трейс](13-integration-harness.md) | DONE   | `tools/integration/`, `tools/trace.sh`     |
| 14  | [Tester-агент в пайплайне](14-tester-agent.md)              | DONE   | `.claude/agents/tester.md`, `CLAUDE.md`, `.claude/skills/task/SKILL.md` |
| 15  | [Research-агент investigator (Opus)](15-investigator-agent.md) | DONE   | `.claude/agents/investigator.md`, `tools/investigate.sh` |
| 16  | [Миграция на docker compose (cross-platform)](16-docker-compose-migration.md) | DONE | `compose.yaml`, `.env.example`, `Makefile` (slim) |
| 17  | [Docker-only host interface](17-docker-only-host.md) | DONE | удаление root `Makefile`/`zx`/`zx.ps1`, обновление README/агентов |

## Легенда статусов

- **TODO** — не начато
- **IN PROGRESS** — в работе
- **BLOCKED** — ждёт прояснения или внешней зависимости
- **DONE** — реализовано и проверено в Fuse

## Как работать с индексом

1. Открыть текущую задачу по ссылке из таблицы.
2. Выполнить шаги, отметить acceptance criteria.
3. Запустить `docker compose run --rm build` (или `docker compose run --rm shell` для интерактивной работы). После задач 12+ доступны `docker compose run --rm smoke`, `docker compose run --rm integration` и др. Полный список — в `README.md` и `docker/README.md`.
4. Когда всё чисто — поменять статус на **DONE** в этом файле и в шапке задачи.
5. Перейти к следующей.

## Следующий шаг

Все задачи MVP игры выполнены 2026-05-26, все инфраструктурные задачи завершены 2026-05-27. Host-side полностью очищен: на хосте остаётся только `docker compose` (ни root `Makefile`, ни `zx`, ни `zx.ps1`). Tetris с этого момента — референсное приложение к фреймворку, а не самоцель.

Реализовано: **1–17** (игра полностью → Docker базовый образ → smoke-тест → integration-сценарии → tester-агент → investigator-агент → миграция на docker compose с cross-platform поддержкой → очистка host interface).

Пользователь готов к переносу фреймворка (тулчейн в Docker, скилл zx-arch, агенты, docker compose) в отдельный обезличенный репозиторий для новых Z80-проектов. Это задача переноса/переоформления, не расширения функционала Tetris.

## Вне скоупа MVP (для будущих задач)

- Next-piece превью
- Hold piece, ghost piece, SRS wall-kicks
- AY-музыка (нужен Vortex Tracker / AYFX и player)
- Kempston joystick
- Меню, таблица рекордов, настройки
- Загрузка на реальное железо (DivMMC, аудиовход)
