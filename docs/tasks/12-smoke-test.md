# 12. Smoke-тест .tap через ZRCP

**Статус**: DONE
**Зависит от**: 11
**Блокирует**: 14

## Цель

После каждого `make` уметь автоматически загрузить `build/tetris.tap` в ZEsarUX, проиграть N кадров, снять скриншот и дамп регистров. Это даёт reviewer-агенту (и человеку) объективное доказательство «билд не падает, экран рисуется».

## Контекст

Smoke не проверяет геймплей — только что программа не уходит в ROM, не зависает на одном PC, и экран не пустой. Этого достаточно как «зелёный свет» большинству задач из MVP-цикла.

## Acceptance criteria

- [ ] Создан `tools/smoke.sh`:
  - Принимает путь к `.tap` (по умолчанию `build/tetris.tap`) и количество кадров (по умолчанию 200).
  - Запускает ZEsarUX в фоне с `--machine 128k --tape <tap> --enable-autoload --enable-remoteprotocol --remoteprotocol-port 10000 --quickexit`, переменная окружения `SDL_VIDEODRIVER=dummy`.
  - Ждёт готовность ZRCP (poll на TCP 10000 с таймаутом 10 сек).
  - Через ZRCP: `run-for <frames>`, затем `save-screen /work/artifacts/smoke.scr`, `get-registers`, `dump-memory 0x5800 768` (attribute file).
  - Пишет в `/work/artifacts/smoke.txt`: версию ZEsarUX, регистры, первую/последнюю строку attribute file.
  - Корректно гасит ZEsarUX (`exit` через ZRCP), exit code 0 при успехе.
- [ ] `docker/entrypoint.sh` команда `smoke` зовёт `tools/smoke.sh` внутри контейнера.
- [ ] `Makefile`: таргет `make smoke` зависит от `build/tetris.tap` и запускает `docker run` с маунтом `build:/work/build:ro` и `artifacts:/work/artifacts`.
- [ ] После `make smoke` на текущей рабочей сборке Тетриса:
  - `artifacts/smoke.scr` существует, размер ровно 6912 байт (стандартный размер Spectrum screen dump).
  - `artifacts/smoke.txt` содержит непустую строку с PC ≠ 0x0000 и ≠ адресов основного ROM-цикла.

## Test plan

- Дефолтный smoke (см. acceptance): проверяет сам себя.
- Negative: подменить `build/tetris.tap` пустым файлом → `make smoke` должен дать **non-zero** exit и понятное сообщение «tap load failed» в `smoke.txt` или в stderr. Это будущий tester-агент должен распознавать как FAIL.

## Технические заметки

- ZRCP — текстовый протокол, документация в `ZEsarUX/docs/zrcp.md` и через команду `help` в самой сессии. Команды: `run`, `cpu-step`, `run-for <frames>`, `get-registers`, `read-memory <addr> <len>`, `save-screen <path>`, `enter-cpu-step`, `exit`.
- ZEsarUX может потребовать `--enable-debug` для некоторых команд — planner-агент должен проверить актуальный набор.
- `--quickexit` гарантирует, что emулятор завершится при разрыве ZRCP, не оставляя зомби-процессов в контейнере.

## Вне MVP

- Дифф скриншота против эталона — отдельная задача (потребует pHash или попиксельное сравнение, обходим в MVP).
- Multi-frame видео — не нужно для smoke.

## Completion note

Реализовано 2026-05-27. Инструмент `tools/smoke.sh` реализован с корректировками под реальный ZEsarUX 12.1 / ZRCP: опция `--enable-autoload` отсутствует (autoload срабатывает автоматически для `--tape`), ZRCP-команда `run-for` заменена на host-side sleep, `dump-memory` на `read-memory` (правильное имя), адреса в `read-memory` парсятся как десятичные (0x5800 = 22528). Проверка прошла: smoke.scr 6912 байт, smoke.txt содержит данные от ZEsarUX 12.1, PC=0x0038 (нормальный IM1 interrupt vector), attribute file не совпадает с ROM-сигнатурой.
