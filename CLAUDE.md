# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Классический Тетрис для **ZX Spectrum 128K**, написан на C, собирается через **z88dk** (C → Z80 asm → `.tap`), запускается в **Fuse for macOS**. Это пет-проект; владелец — новичок в low-level разработке, поэтому все технические решения и архитектурные обоснования должны быть явно записаны, а не «подразумеваться».

## Source of truth

Реализация ведётся **строго по задачам** из `docs/tasks/`:

- `docs/tasks/INDEX.md` — индекс с прогрессом, статусами (TODO/IN PROGRESS/BLOCKED/DONE) и порядком выполнения
- `docs/tasks/NN-*.md` — каждая задача отдельным файлом со своими acceptance criteria
- `docs/start.md` — полный архитектурный план (для понимания «почему именно так»)

Перед любым кодом — прочитать соответствующую задачу. Не отступать от её acceptance criteria и не добавлять фичи вне скоупа MVP (см. секцию «Вне скоупа MVP» в `INDEX.md`).

## Build / run

```bash
docker compose build                              # собрать/обновить образ (один раз, ~5-10 минут)
docker compose run --rm build                     # собрать build/tetris.tap
docker compose run --rm shell                     # интерактивный shell в контейнере
docker compose run --rm smoke                     # smoke-тест: проверяет что .tap загружается и PC=0x0038
docker compose run --rm integration               # интеграционные тесты через ZEsarUX ZRCP
```

Compose-конфигурация: `compose.yaml`. Переменные окружения: скопируй `.env.example` в `.env`.

Требуется только **Docker Desktop**. Хостовой `z88dk` и `fredm-fuse` не нужны.
Подробности: `docker/README.md`.

## Architecture (big picture)

Игра — однопоточный фрейм-цикл на 50 Гц (vsync через `intrinsic_halt()`). Состояние полностью в глобальной структуре `game_state_t G`. Модули разделены по слоям, между ними строгое направление зависимостей:

```
main.c            оркестрация цикла: input → game → render → hud → sound
  ├─ input.c      опрос клавиатуры (port 0xFE), debounce/autorepeat
  ├─ game.c      ─ логика: board[20][10], gravity, коллизии, line clear, scoring
  │    └─ pieces.c  таблица 7×4 тетромино как uint16_t битмапы
  ├─ render.c     знакоместная отрисовка стакана и фигур (8×8 px на клетку),
  │              HUD-цифры с кешем previous values (без мерцания)
  └─ sound.c      три SFX через ROM `bit_beep`
```

Ключевые архитектурные решения, которые нельзя «упрощать»:

- **1 клетка Тетриса = 1 знакоместо 8×8 пикс** — спектрумовский attribute clash сам становится механизмом цвета фигуры. Стак позиционируется в char-cells `(8..17, 2..21)`, HUD в столбце 20+.
- **`board[ROWS][COLS]` хранит id фигуры (1..7), не цвет** — render берёт цвет из `piece_colors[]`. Отделяет логику от презентации.
- **Состояние `G` глобальное** — на Z80 это и быстрее, и читается проще, чем передача указателей. Никаких аллокаций в куче.
- **`drop_period` зависит от уровня по NES-таблице** — не выдумывать своих значений (см. `07-scoring-levels.md`).
- **`const`-таблицы (`piece_shapes`, `piece_colors`, period table)** должны попасть в code-сегмент, не съедая RAM. z88dk это делает автоматически при правильных pragma.

## Workflow: 5-агентный пайплайн

Любая задача из `docs/tasks/` проходит через **пять агентов строго по порядку**:

1. **planner** — читает задачу, сверяется с состоянием кода и официальной z88dk документацией, выдаёт уточнённый план реализации. Если в задаче нет нужного API или формулы — ищет в интернете, **не выдумывает**.
2. **coder** — реализует код по плану. Собирает `make`, фиксирует ошибки.
3. **tester** — парсит секцию `## Test plan` из задачи, прогоняет соответствующие compose-сервисы, читает артефакты (`artifacts/smoke.txt`, `artifacts/integration-*.json`), возвращает **PASS / FAIL / INFRA_ERROR**. Не редактирует код.
4. **reviewer** — проверяет каждый пункт acceptance criteria из задачи, читает диф и tester-артефакты, может вернуть **REWORK** с конкретным списком проблем.
5. **documenter** — обновляет статус задачи в `docs/tasks/INDEX.md` и в шапке файла задачи на **DONE**, добавляет короткую запись о том, что в итоге было сделано.

Переходы:
- Если **tester** возвращает **FAIL** — управление переходит к **coder** (или к **planner**, если `suggested_next_agent = planner`).
- Если **tester** возвращает **INFRA_ERROR** — пайплайн останавливается без REWORK, пользователю сообщается о проблеме с инфраструктурой (Docker не отвечает).
- Если **reviewer** возвращает **REWORK** — управление переходит обратно к **coder** с конкретным списком замечаний.
- Если **coder** обнаруживает, что план не соответствует задаче — обратно к **planner**.

Главный сеанс **не пишет код напрямую** для задач из `docs/tasks/`. Он только оркестрирует пайплайн.

### Требование: Test plan в каждой задаче

Каждый файл `docs/tasks/NN-*.md` **обязан** содержать секцию `## Test plan` с одним из режимов:
- `skip: <причина>` — для задач без рантайм-поведения (документация, инфраструктура).
- `smoke-only: <ожидание>` — для задач, где достаточно `make smoke`.
- `scenarios: [name1, ...]` — для задач с конкретными integration-сценариями.

Без этой секции tester вернёт FAIL с `suggested_next_agent = planner`.

Шаблон новой задачи: `docs/tasks/_template.md`.

### Запуск пайплайна

Слэш-команда `/task <id>` запускает пайплайн для задачи. Примеры:

```
/task 01        # toolchain setup + skeleton
/task 04        # gravity
```

Без аргумента берётся первая `TODO` из `INDEX.md`.

Подробные инструкции для каждой роли — в `.claude/agents/{planner,coder,tester,reviewer,documenter}.md`. Сам пайплайн — в `.claude/skills/task/SKILL.md`.

## Hardware reference

Скил **`zx-arch`** (`.claude/skills/zx-arch/SKILL.md`) — справочник по архитектуре ZX Spectrum 48K/128K: memory map, screen layout (с формулой адреса пикселя), attribute file, keyboard matrix (port 0xFE), бипер, AY-3-8912, 50 Hz interrupt, 128K bank switching через 0x7FFD. Содержит проверенные факты со ссылками на breakintoprogram.co.uk, worldofspectrum.org, sinclair.wiki.zxnet.co.uk.

Агенты `planner`/`coder`/`reviewer` обращаются к нему перед любой работой с прямыми адресами, портами или ручным расчётом screen-адресов.

## Research-агент `investigator`

Ad-hoc агент (НЕ участвует в 5-агентном пайплайне). Принимает произвольный Z80-бинарь и свободный вопрос пользователя.

```bash
FILE=build/tetris.tap Q="как устроен главный игровой цикл" docker compose run --rm investigate
```

`tools/investigate.sh` сначала делает recon (file, hexdump, strings, z88dk-dis, z80dasm) в `artifacts/investigations/<ts>-<slug>/`, затем главный сеанс вызывает `Task investigator` с указанием на эту директорию.

Ограничения:
- Модель: opus (явно прописана).
- Read-only вне `artifacts/investigations/<dir>/`. Не редактирует src/, docs/, tools/, docker/, .claude/, Makefile, CLAUDE.md, zpragma.inc.
- Tools: Read, Grep, Glob, Bash (whitelist: z88dk-dis, z80dasm, hexdump, xxd, file, strings, `docker compose run --rm disasm`/`disasm-alt`/`trace`/`integration`, python3), WebSearch, WebFetch.

Полная инструкция: `.claude/agents/investigator.md`.

## Запреты

- **Не выдумывать API z88dk.** Если функция не упомянута в задаче — найти в [z88dk wiki](https://github.com/z88dk/z88dk/wiki/Platform---Sinclair-ZX-Spectrum) или [getting-started doc](https://github.com/z88dk/z88dk/blob/master/doc/ZXSpectrumZSDCCnewlib_01_GettingStarted.md) через WebFetch/WebSearch перед использованием.
- **Не выходить за скоуп задачи.** Если по ходу всплыла полезная фича, не описанная в acceptance criteria, — оставить заметку в файле задачи в секции «Вне MVP», но не реализовывать.
- **Не менять архитектуру без обоснования.** Если задача требует перестройки модулей — это **BLOCKED**, новая задача, согласование с пользователем.
- **Не правит `docs/start.md`** без явной просьбы пользователя — это исходный архитектурный план, который должен сохранять начальное состояние решений. Реальные отклонения от него фиксируются в completion notes задач.

## Стиль кода

- C89-совместимый базис (sccz80 и zsdcc оба умеют), но допустимы `stdint.h` типы и `//`-комментарии (z88dk их принимает).
- Без динамической памяти: только статические массивы и глобальное состояние.
- Без `float` — на Z80 это эмуляция, дорого. Скоринг — `uint32_t`.
- Имена функций: `модуль_действие` (`game_tick`, `render_cell`, `input_poll`).
- Комментарии — только когда «почему», а не «что» (стандарт проекта).
