# 16. Миграция на docker compose (cross-platform)

**Статус**: DONE
**Зависит от**: 11, 12, 13, 15
**Блокирует**: —

## Цель

Сделать так, чтобы фреймворк работал **одинаково на macOS, Linux и Windows** без зависимостей от GNU make, bash, WSL. На любой платформе должен хватать только установленного Docker Desktop. Канонический интерфейс — `docker compose run --rm <service>`.

## Контекст

В задачах 11-15 все операции (build, shell, smoke, integration, trace, disasm, disasm-alt, investigate) были навешаны на Makefile-таргеты. Это плохо для кросс-платформенности:

- **Windows**: GNU make + bash + Linux-style пути требуют WSL/Git Bash. Не работает «из коробки».
- **Дублирование**: каждый Makefile-таргет вручную пишет один и тот же `docker run --rm --user ... -v ... -w /work $(IMAGE) <cmd>`.
- **Compose-нативные фичи**: профили, named volumes, depends_on, build context, env_file, переменные `.env` — всё это есть бесплатно, но не используется.

Решение: перенести все операции в `compose.yaml` как сервисы, использовать `docker compose run --rm <service>`. Makefile становится тонкой обёрткой для Unix-удобства (можно оставить для пользователей, привыкших к `make smoke`), но не обязательной.

## Acceptance criteria

- [ ] Создан `compose.yaml` в корне репо со сервисами:
  - `build` (default) — собирает `build/tetris.tap`.
  - `shell` — интерактивный shell в контейнере (`tty: true`, `stdin_open: true`).
  - `smoke` — прогон smoke-теста.
  - `integration` — прогон integration-сценариев.
  - `trace` — z88dk-ticks трейс (BIN/CYCLES через env).
  - `disasm` — z88dk-dis.
  - `disasm-alt` — z80dasm.
  - `investigate` — recon-фаза (FILE/Q через env).
- [ ] Все сервисы используют один и тот же образ (`build:` context = `.`, dockerfile = `docker/Dockerfile`), просто меняют `command:` / `entrypoint:` args.
- [ ] Образ собирается через `docker compose build` (не отдельным `docker build`).
- [ ] Mount: `./:/work` (полный repo), user override через `${UID}:${GID}` с дефолтом (если env пуст — берётся из `.env`).
- [ ] Создан `.env.example` с переменными `UID`, `GID`, `IMAGE_TAG`, опц. `BIN`, `CYCLES`, `FILE`, `Q`, `ORG`.
- [ ] **Windows-совместимость**: пути к bind-mount работают на Windows. Использовать только относительные пути (`./`) — Docker Desktop на Windows их корректно нормализует. Не использовать `$(pwd)` (баш-специфика).
- [ ] **Cross-platform launcher** (опционально, но желательно): `zx` (bash) и `zx.ps1` (PowerShell) — однострочные обёртки `docker compose run --rm $1 "${@:2}"`. Лежат в корне репо.
- [ ] `Makefile` остаётся как тонкая Unix-обёртка: каждый таргет = одна строка `docker compose run --rm <service>`. Удалить дублирование mount/user/image — это теперь в compose.
- [ ] Если Makefile нельзя сохранить (например, на Windows у нас нет make) — должно работать чистое `docker compose run --rm <service>`.
- [ ] Обновлены: `docker/README.md`, `CLAUDE.md` (секция Build/run) — теперь канонический способ `docker compose run --rm <service>`, Makefile упоминается как удобная Unix-обёртка.
- [ ] `.gitignore` дополнен `.env` (только example в git, реальный .env — локальный).
- [ ] Удалены или обновлены прямые `docker run`-вызовы в Makefile.

## Test plan

`smoke-only`: после миграции `docker compose run --rm build` создаёт `build/tetris.tap`, `docker compose run --rm smoke` проходит зелёным и кладёт `artifacts/smoke.scr` (6912 байт) + `smoke.txt`. На macOS — успешно. Дополнительно: `make smoke` остаётся зелёным (Unix-обёртка).

## Технические заметки

- `docker compose` (v2) поддерживает все нужные фичи. `docker-compose` (v1) — deprecated, не использовать.
- Для `shell` и интерактивных сценариев — `docker compose run --rm` (не `up`), чтобы контейнер удалялся после выхода.
- `docker compose` на Windows работает с PowerShell без WSL (Docker Desktop включает compose). На macOS / Linux — то же.
- Переменные окружения для `trace`/`investigate` передаются как `BIN=... docker compose run --rm trace` или через `.env`.

## Вне MVP

- GitHub Actions / CI workflow — добавится в отдельной задаче.
- Multi-arch образ (нативный arm64 ZEsarUX) — отдельная задача.
- Volume для z88dk кеша между сборками — отдельная задача, если время сборки станет проблемой.

## Completion note

Реализовано 2026-05-27. Создан `compose.yaml` с 8 сервисами (build, shell, smoke, integration, trace, disasm, disasm-alt, investigate) через YAML anchor для переиспользования defaults. Единый образ на все сервисы, cross-platform mount `./:/work`, user overrides с дефолтами из `.env.example`. Makefile переписан как тонкая Unix-обёртка; на Windows работает чистый `docker compose run --rm <service>` из PowerShell. `docker/README.md` и `CLAUDE.md` обновлены; пути используют относительные адреса без bash-специфики. Smoke-тест и build проходят зелёным на macOS.
