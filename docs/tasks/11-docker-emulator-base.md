# 11. Docker-эмулятор: базовый образ

**Статус**: DONE
**Зависит от**: 01 (toolchain — будет деприкейтнут этой задачей)
**Блокирует**: 12, 13, 14, 15

## Цель

Собрать **единственный Docker-образ**, в котором живёт ВЕСЬ тулчейн: компилятор z88dk (`zcc`), эмулятор ZEsarUX, трейсер `z88dk-ticks`, два дизассемблера (`z88dk-dis`, `z80dasm`). На хосте остаётся только Docker Desktop — никакого нативного z88dk и Fuse cask.

Эта задача — фундамент будущего фреймворка. Tetris с этого момента собирается внутри контейнера.

## Контекст

Сейчас и сборка, и проверка завязаны на хостовой macOS. Это блокирует CI, исследовательских агентов, и каждый новый разработчик/проект должен ставить тулчейн локально. Решение — полностью контейнеризовать.

## Acceptance criteria

- [ ] Создан `docker/Dockerfile` (Ubuntu 24.04 base, multi-stage).
- [ ] В финальном образе установлены и доступны в `$PATH`:
  - `zcc` (z88dk), `z88dk-ticks`, `z88dk-dis` — из z88dk.
  - `zesarux` — собран из исходников `chernandezba/zesarux` с поддержкой `--enable-remoteprotocol` и работы под `SDL_VIDEODRIVER=dummy`.
  - `z80dasm` — из apt.
  - `make`, `gcc` — для сборки Тетриса внутри контейнера.
- [ ] Создан `docker/entrypoint.sh` с подкомандами: `build` (дефолт), `shell`, `smoke`, `integration`, `trace`, `disasm`, `investigate`. Кроме `build` и `shell` — заглушки (реализуются в 12–15, но должны существовать).
- [ ] В `Makefile`:
  - **Удалены** все ссылки на хостовой `zcc`/`z88dk` и `fredm-fuse`.
  - `make docker-build` — сборка образа `zx-tetris-emu:latest`.
  - `make` / `make build` — `docker run --rm -v $(pwd):/work zx-tetris-emu build`, создаёт `build/tetris.tap` идентичный нативному (по поведению — smoke в задаче 12 должен пройти).
  - `make shell` — интерактивный shell в контейнере с примонтированным репо.
  - `make run` — удалён (или переопределён как «build + headless preview»).
- [ ] Обновлён `CLAUDE.md`, секция «Build / run»: требование `z88dk` в `$PATH` и `fredm-fuse` cask заменено на «требуется Docker Desktop». Все остальные секции остаются.
- [ ] В шапке `docs/tasks/01-setup-toolchain.md` добавлена пометка «**DEPRECATED**: native path replaced by Docker in task 11» (сама задача 01 остаётся для истории).
- [ ] Создан `artifacts/.gitignore` (`*` кроме `.gitignore`).
- [ ] Создан `.dockerignore` (исключает `build/`, `artifacts/`, `.git`, `docs/`).
- [ ] `docker/README.md` (короткий, ~30 строк): требования, `make docker-build`, `make shell`, что куда монтируется, какой пользователь внутри.

## Test plan

`smoke-only`:
- `make docker-build` завершается без ошибок.
- `docker run --rm zx-tetris-emu:latest shell -c 'zcc --help && zesarux --version && z88dk-ticks --help | head -1 && z88dk-dis --help 2>&1 | head -1 && z80dasm --version'` печатает версии всех пяти инструментов без падений.
- `make build` (после `make clean`) создаёт непустой `build/tetris.tap`.
- На чистом хосте: убираем z88dk из PATH (или проверяем на CI-машине, где его нет) — `make build` всё ещё работает.

## Вне MVP

- Кеширование слоёв сборки ZEsarUX между PR (BuildKit cache mount) — добавим если время сборки станет проблемой.
- arm64-native сборка для Apple Silicon — попробовать, но если ZEsarUX не соберётся под arm64, fallback к amd64 через эмуляцию приемлем.

## Completion note

Реализовано 2026-05-27. Multi-stage Docker-образ на базе Alpine/musl и финального Ubuntu 24.04 с полным тулчейном: z88dk/zcc, z88dk-ticks, z88dk-dis, zesarux 12.1 из исходников, z80dasm, make/gcc. Все 5 инструментов в PATH. Тетрис собирается через `make build` без хостового тулчейна (9408 байт .tap). Образ 1.55 GB, amd64-only, на Apple Silicon работает через Rosetta 2. Корневой Dockerfile и compose.yaml убраны, добавлен Makefile.inner. Шапка INDEX.md и описание workflow обновлены для актуальности с Docker-средой.
