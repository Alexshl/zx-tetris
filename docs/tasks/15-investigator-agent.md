# 15. Research-агент `investigator`

**Статус**: DONE
**Зависит от**: 11 (тулчейн в Docker), 13 (дизассемблеры и trace)
**Блокирует**: —

## Цель

Создать ad-hoc исследовательского агента (`model: opus`), который умеет брать произвольный Z80-бинарь (`.tap`/`.sna`/`.z80`/`.bin` — собственный билд или чужая игра) и отвечать на свободные вопросы пользователя: «как это устроено», «где формат уровней», «что за рутина по адресу X». Это поворот проекта от «Тетрис как продукт» к «фреймворк для разработки и исследования Z80-приложений», где Tetris — первый референс.

Investigator **не входит в линейный пайплайн** разработки. Пользователь вызывает его по требованию через `make investigate`.

## Контекст

Сами по себе дизассемблеры (`z88dk-dis`, `z80dasm`) дают тонну сырого ассемблера. Чтобы извлечь из него смысл — нужен агент, способный комбинировать статический анализ, экспериментирование в эмуляторе и поиск по литературе (worldofspectrum.org и т.п.). Opus-модель — потому что задачи требуют рассуждения и опоры на hardware-знания.

## Acceptance criteria

- [ ] Создан `.claude/agents/investigator.md`:
  - Frontmatter: `model: opus` (явно).
  - Whitelist инструментов: `Bash` (только `z88dk-dis`, `z80dasm`, `hexdump`, `xxd`, `file`, `strings`, `make disasm`, `make disasm-alt`, `make trace`, запуск ZEsarUX в headless + ZRCP через готовый клиент), `Read`, `Grep`, `Glob`, `WebSearch`, `WebFetch`.
  - **Без** `Edit`/`Write` в исходниках проекта. Запись разрешена только в `artifacts/investigations/<ts>-<slug>/`.
  - Описание workflow: recon → static disasm → strings → live exploration (если нужно) → hypothesize/verify → report.
  - Формат отчёта `report.md`: резюме (3–5 строк), memory map, аннотированные фрагменты, **разделение Verified / Hypothesis**, ссылки на источники.
  - Явное требование: перед выводами о портах/банках/screen-адресах прочитать скилл `zx-arch`.
- [ ] Создан `tools/investigate.sh`:
  - Принимает `FILE=<path>` и `Q="<вопрос>"`.
  - Создаёт `artifacts/investigations/$(date +%Y%m%d-%H%M%S)-<slug-из-вопроса>/`.
  - Делает initial recon: `file`, `hexdump -C | head -50`, `strings`, `z88dk-dis` и `z80dasm` дампы, кладёт всё в эту директорию.
  - Записывает `prompt.md` с вопросом пользователя и путями к артефактам.
  - Выводит инструкцию: «recon готов в `<path>`, запусти агента investigator на этом prompt.md» (агент инвокается главным сеансом — не из shell-скрипта).
- [ ] `Makefile`: `make investigate FILE=<path> Q="<вопрос>"` — зовёт `investigate.sh` внутри контейнера, оставляет агент-инвокацию пользователю.
- [ ] Создан `artifacts/investigations/.gitignore` (`*` кроме `.gitignore` — чужие бинари в репо не лежат).
- [ ] В `CLAUDE.md` добавлена секция «Research-агент investigator» с примером запуска и ограничениями.

## Test plan

`scenarios: [self-tetris, foreign-tap-recon, no-edit-guard]`

- **self-tetris**: `make investigate FILE=build/tetris.tap Q="как устроен главный игровой цикл"`. Recon успешно генерируется. После запуска агента (вручную) — отчёт `report.md` существует, упоминает функцию `_main` или `_game_tick` (имена есть в `build/tetris.map`), содержит секции Verified и Hypothesis, ссылается на конкретные адреса.
- **foreign-tap-recon**: `make investigate FILE=<любой публично доступный .tap, например, manic-miner.tap из worldofspectrum> Q="где находится код карты уровней"`. Recon-артефакты создаются, дизассемблеры отрабатывают без падений (даже без `.map`).
- **no-edit-guard**: investigator не имеет инструментов `Edit`/`Write` вне `artifacts/investigations/`. Проверяется чтением `.claude/agents/investigator.md` — whitelist инструментов явно ограничен.

## Технические заметки

- Запуск самого LLM-агента — отдельный шаг, не часть `make investigate`. Скрипт только готовит recon-артефакты; вызов агента делает пользователь или главный сеанс командой типа `Task investigator <prompt.md>`. Это сознательное разделение, чтобы инфраструктурная часть и LLM-часть были независимо отлаживаемы.
- Slug из вопроса — простой sanitize: lowercase + замена не-`[a-z0-9]` на `-`, обрезка до 40 символов.
- Большие бинари (>64KB, например snapshot 128K с 5 банками) — дизассемблируются по банкам отдельно, agent сам решает порядок.

## Вне MVP

- Автоматический pattern-matching на известные форматы упаковки (ZX0, ZX7, MegaLZ) — добавится отдельной задачей, когда понадобится.
- Sharing отчётов между investigations (кросс-референс рутин в двух разных играх) — потом.
- Web-UI для просмотра отчётов — не нужно, markdown в файловой системе достаточен.

## Completion note

Реализовано 2026-05-27. Создан opus-агент investigator с workflow recon→disasm→live→hypothesize→verify→report, инструмент investigate.sh для подготовки артефактов (hexdump, strings, дизассемблеры, prompt.md), Docker интеграция (file, xxd, bsdextrautils), записанные рекомендации в CLAUDE.md. Фреймворк для исследования произвольных Z80-бинарей готов. Tester станет доступным в следующей сессии из-за кеширования списка агентов на старте.
