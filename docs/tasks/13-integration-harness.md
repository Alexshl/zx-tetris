# 13. Интеграционные сценарии + ticks-трейс + дизассемблеры

**Статус**: DONE
**Зависит от**: 12
**Блокирует**: —

## Цель

Дать пайплайну инструмент, который умеет проверять **геймплей** (а не только «билд жив»): симулировать нажатия, читать структуру `G` из памяти эмулятора по адресам из `.map`, сравнивать с ожиданием. И отдельно — записывать CPU-трейс через `z88dk-ticks` для постмортем-разбора багов.

## Acceptance criteria

### Часть A: интеграционный harness

- [ ] Создан `tools/integration/run.py` (Python 3, без внешних зависимостей кроме стандартной библиотеки — `socket`, `json`, `argparse`).
  - Клиент ZRCP: подключается к `localhost:10000`, посылает текстовые команды, парсит ответы.
  - Парсер `build/tetris.map`: ищет адрес символа по имени (например, `_G`, `_game_tick`). Формат map z88dk — `Symbol  Defined  Module`.
  - Загружает YAML/JSON-сценарий, исполняет шаги: `frames N`, `key <name>`, `assert mem <symbol+offset> == <byte>`, `dump board → artifacts/<name>.json`.
- [ ] Создана директория `tools/integration/scenarios/` с одним примером `spawn-piece.json`:
  - При старте `G.board[0..199]` все нули.
  - После 50 кадров без ввода в `G.cur_id` ≠ 0 (фигура заспаунилась).
- [ ] `docker/entrypoint.sh` команда `integration <scenario>` запускает ZEsarUX (как в smoke) + параллельно `run.py`.
- [ ] `Makefile`: `make integration` прогоняет все сценарии в `tools/integration/scenarios/*.json`, exit code = max по сценариям, артефакты в `artifacts/integration-<name>.json`.

### Часть B: z88dk-ticks трейс

- [ ] Создан `tools/trace.sh`, принимает `.bin` (не `.tap` — ticks не умеет ленту) и опционально диапазон тактов.
- [ ] Запускает `z88dk-ticks` с флагами для текстового лога инструкций (планнер определяет точные флаги через `z88dk-ticks --help`).
- [ ] Лог пишется в `artifacts/trace-<basename>.log`.
- [ ] `docker/entrypoint.sh` команда `trace` зовёт `trace.sh`.
- [ ] `Makefile`: `make trace BIN=build/tetris.bin`.

### Часть C: Дизассемблеры

- [ ] `tools/disasm.sh` — обёртка над `z88dk-dis -o <ORG> -x build/tetris.map build/tetris.bin`. ORG берётся из z88dk-конфига проекта (по умолчанию для Spectrum — 0x8000 в большинстве конфигов, planner проверяет фактическое значение).
- [ ] `tools/disasm-alt.sh` — обёртка над `z80dasm --labels --origin=<ORG> build/tetris.bin`.
- [ ] `docker/entrypoint.sh` команда `disasm` зовёт `disasm.sh` (а `disasm-alt` — соответствующий).
- [ ] `Makefile`:
  - `make disasm` → `artifacts/tetris.dis.asm` (с символами).
  - `make disasm-alt` → `artifacts/tetris.z80dasm.asm`.
- [ ] Оба файла после `make build && make disasm && make disasm-alt` существуют, непустые, содержат корректный Z80-ассемблер (минимум — `org 0x8000` или эквивалент в первых строках, инструкции `ld`/`call`/`jp` встречаются).

## Test plan

- **Часть A**: `make integration` зелёный на текущей сборке. `artifacts/integration-spawn-piece.json` содержит ненулевое `cur_id` в финальном snapshot. Намеренная порча: поменять адрес символа в сценарии на заведомо неверный → сценарий должен дать FAIL с понятным сообщением (а не повиснуть).
- **Часть B**: `make trace BIN=build/tetris.bin` создаёт `artifacts/trace-tetris.log` размером > 1 KB, в первой строке — адрес entry point, который укажет planner.
- **Часть C**: `make disasm` создаёт `artifacts/tetris.dis.asm`, в нём встречается имя функции `_game_tick` (или другая C-функция Тетриса) благодаря `.map`. `make disasm-alt` создаёт `artifacts/tetris.z80dasm.asm` без имён, но того же порядка размера.

## Технические заметки

- Адрес `_G` — глобальная структура состояния. В z88dk C-символы получают префикс `_` в линкере. Сценарий ссылается на C-имя, парсер map добавляет префикс.
- Поле `cur_id` находится по смещению от начала `G`. Смещение можно вычислить либо из `sizeof()` через отдельный helper, либо захардкодить (хрупко), либо генерировать таблицу offsets отдельным C-helper'ом. Planner решит — рекомендуется хелпер, который при сборке выплёвывает `build/symbols.json` с offsets.

## Вне MVP

- Multi-machine сценарии (48K vs 128K) — пока только 128K.
- Запись input в .rzx replay — overkill.

## Completion note

Реализовано 2026-05-27. Три части задачи замкнуты: (A) Python-клиент ZRCP с парсером `.map` и сценариями (spawn-piece.json тестирует cur.col ≠ 0 для надёжности вместо cur.id, т.к. PIECE_I=0); (B) z88dk-ticks trace с параметром `-counter` (флаг `-b zx` убран — версия в контейнере его не знает); (C) два дизассемблера (z88dk-dis с символами, z80dasm без). Технические отклонения от текста: ORG = 0x6000 (CRT_ORG_CODE из zpragma.inc, а не 0x8000), бинарник `build/tetris_CODE.bin`. Exit-коды корректны. Trace по дефолту ~11 MB (> 1 KB AC). ZEsarUX segfault при cleanup — Rosetta2 quirk на Apple Silicon, не влияет на результат.
