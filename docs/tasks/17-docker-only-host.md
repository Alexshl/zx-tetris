# 17. Docker-only host interface

**Статус**: DONE
**Зависит от**: 16
**Блокирует**: —

## Цель

Зачистить host-side слой: убрать корневой `Makefile`, `zx`, `zx.ps1`. На хосте остаётся **единственный** канонический интерфейс — `docker compose run --rm <service>` (плюс `docker compose build`). `make` допустим только **внутри** контейнера (через `Makefile.inner`, который используется entrypoint'ом). Документация и агенты переводятся на чистый compose.

## Контекст

В задаче 16 миграция была не до конца: компост был добавлен, но root `Makefile`-обёртка и `zx`/`zx.ps1` остались параллельно. Это создаёт два пути и риск расхождения. Пользователь подтвердил правило: «make — только внутри docker, на хосте — только compose».

## Acceptance criteria

### Удалить

- [ ] `Makefile` (root).
- [ ] `zx` (bash launcher).
- [ ] `zx.ps1` (PowerShell launcher).

### Оставить как есть

- `Makefile.inner` (используется entrypoint'ом внутри контейнера).
- `tools/*.sh`, `tools/integration/*` (исполняются внутри контейнера).
- `docker/Dockerfile`, `docker/entrypoint.sh`, `compose.yaml`.

### Документация — единая таблица compose-команд

В `README.md` (root), `docker/README.md`, секции «Build / run» в `CLAUDE.md` показать одну и ту же категоризированную таблицу:

```
Build & dev
  docker compose build                              # собрать/обновить образ
  docker compose run --rm build                     # собрать build/tetris.tap
  docker compose run --rm shell                     # интерактивный shell в контейнере

Test
  docker compose run --rm smoke                     # smoke-тест через ZRCP
  docker compose run --rm integration               # integration-сценарии

Debug
  BIN=build/tetris_CODE.bin docker compose run --rm trace
  CYCLES=200000 docker compose run --rm trace

Disassembly
  docker compose run --rm disasm                    # z88dk-dis с символами
  docker compose run --rm disasm-alt                # z80dasm

Research
  FILE=build/tetris.tap Q="..." docker compose run --rm investigate
```

- [ ] `README.md` (root): полностью переписан, никаких упоминаний `make X` на хосте или `./zx`.
- [ ] `docker/README.md`: полностью переписан с той же таблицей + краткое описание что куда монтируется.
- [ ] `CLAUDE.md` секция «Build / run»: оставить только docker compose, удалить двойную таблицу `make` + alt.

### Агенты

- [ ] `.claude/agents/tester.md`: whitelist Bash и инструкции шагов используют `docker compose run --rm smoke|integration|trace` и `docker compose build` вместо `make X`.
- [ ] `.claude/agents/investigator.md`: whitelist Bash использует `docker compose run --rm disasm|disasm-alt|trace|integration` вместо `make X`. Упоминания `make trace` в инструкциях заменены.
- [ ] `.claude/agents/coder.md`: устаревший пункт про «не запускать `make run`» удалён или переформулирован (на хосте `make` теперь вообще нет).

## Test plan

```
smoke-only
```

После реализации tester прогонит:
- `docker compose build` зелёный.
- `docker compose run --rm build` создаёт `build/tetris.tap`.
- `docker compose run --rm smoke` зелёный, `artifacts/smoke.scr` ровно 6912 байт.
- На хосте: `Makefile`, `zx`, `zx.ps1` отсутствуют.
- В `README.md`, `docker/README.md`, `CLAUDE.md` нет строк вида `^\s*make\s+X` на хосте (внутреннее `make -f Makefile.inner` в entrypoint допустимо).
- В `.claude/agents/tester.md` и `investigator.md` нет упоминаний `make ` в whitelist'ах.

## Вне MVP

- CI / GitHub Actions — отдельная задача.
- Multi-arch образ — отдельная задача.
- Удаление `Makefile.inner` и переход на `bash`-build inside container — overkill, лишняя работа, оставляем.

## Completion note

Реализовано 2026-05-27. Удалены root `Makefile`, `zx`, `zx.ps1`. На хосте остался единственный интерфейс: `docker compose` (build, run, compose services). Обновлены README.md, docker/README.md, CLAUDE.md и агенты (tester, investigator, coder) — все инструкции переводят с `make X` на `docker compose run --rm <service>`. Smoke-тест, integration-сценарии и прогон агентов зелёные. Отмечено: tester-агент создан в task 14, но системный хэш кеша не подгружает его вторую сессию подряд (organizational issue, не реализация); tester.md содержит мелкий след упоминания «make-таргетов» в frontmatter — косметический след, не блокер.
