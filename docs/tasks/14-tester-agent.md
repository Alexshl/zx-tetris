# 14. Tester-агент в пайплайне

**Статус**: DONE
**Зависит от**: 12 (минимально — smoke), желательно 13 (для полноценных integration-проверок)
**Блокирует**: —

## Цель

Добавить пятого агента **`tester`** между `coder` и `reviewer`. Сейчас reviewer оценивает только diff — поведение программы он проверить объективно не может. Tester закрывает этот пробел: прогоняет make-таргеты (smoke, integration, trace), читает артефакты, докладывает PASS/FAIL/INFRA_ERROR. Это превращает acceptance criteria из «обещание coder'а» в «проверенный факт».

## Контекст

Пайплайн становится: `planner → coder → tester → reviewer → documenter`.

Переходы:

- `tester PASS` → к **reviewer** (который теперь обязан прочитать `artifacts/` как evidence).
- `tester FAIL` → REWORK к **coder** с конкретным указанием расхождения.
- `tester FAIL: test plan mismatch` → к **planner** (план неправильный, не код).
- `tester INFRA_ERROR` (Docker не стартовал, эмулятор крашнулся) — пайплайн останавливается, уведомление пользователя, **без** REWORK.

## Acceptance criteria

- [ ] Создан `.claude/agents/tester.md` по образцу существующих агентов:
  - Описание роли, входы, выходы.
  - Доступные инструменты: `Bash` (whitelist: `make smoke`, `make integration`, `make trace`, `make docker-build`, `docker logs`, `ls artifacts/`, `cat artifacts/*`), `Read`, `Grep`. **Без `Edit`/`Write`**.
  - Чёткий формат выходного отчёта (PASS / FAIL list / INFRA_ERROR), включающий пути к артефактам.
  - Запрет на правку кода, спеки, конфигов.
- [ ] Обновлён `.claude/skills/task/SKILL.md`:
  - Шаг tester между coder и reviewer.
  - Логика ветвления PASS/FAIL/INFRA_ERROR.
  - Лимит REWORK-итераций coder↔tester (рекомендуется 3, чтобы не зацикливаться).
- [ ] Обновлён `CLAUDE.md` проекта:
  - В секции «Workflow: 4-агентный пайплайн» — теперь 5-агентный, обновлён список и переходы.
  - Добавлено требование: каждая новая задача в `docs/tasks/` ОБЯЗАНА иметь секцию `## Test plan`. Без неё tester возвращает FAIL.
- [ ] Обновлён `.claude/agents/reviewer.md`: теперь reviewer обязан прочитать как минимум `artifacts/smoke.txt` (если задача не `skip`) и сослаться на него в своём отчёте.
- [ ] Создан `.claude/agents/_template-task.md` (или обновлён существующий шаблон) с секцией `## Test plan` и тремя режимами:
  - `skip` — нет рантайм-поведения для проверки (например, чисто-инфраструктурная задача).
  - `smoke-only` — достаточно прогонки `make smoke`.
  - `scenarios: [...]` — список сценариев из `tools/integration/scenarios/`.

## Test plan

- **Pozitiv**: создать в `docs/tasks/` тестовую задачу `99-test-pipeline.md` с `Test plan: smoke-only`. Запустить `/task 99`. Убедиться, что в логе пайплайна виден шаг tester между coder и reviewer, и что reviewer ссылается на `artifacts/smoke.txt`.
- **Negative — FAIL → REWORK**: в той же тестовой задаче coder сознательно ломает сборку (заглушка с syntax error). Tester должен дать FAIL, пайплайн идёт обратно к coder, **не** к reviewer.
- **Negative — отсутствие test plan**: создать задачу без секции `## Test plan`. Tester возвращает FAIL с понятным сообщением.
- **INFRA_ERROR**: остановить Docker Desktop. `/task 99` → tester возвращает INFRA_ERROR, пайплайн останавливается без REWORK.

## Технические заметки

- Tester использует те же `make`-таргеты, что и человек. Никаких отдельных «agent-only» путей — это исключает расхождение.
- Для определения, какой режим test plan применять, tester парсит файл задачи: ищет в YAML/markdown-секции `## Test plan` ключи `skip`, `smoke-only`, `scenarios`. Простой grep, без полноценного парсера.

## Вне MVP

- Параллельный прогон сценариев — обходим в MVP, делаем последовательно.
- Автоматическое заведение нового сценария по test plan — пока сценарии пишет человек/coder вручную в `tools/integration/scenarios/`.

## Completion note

Реализовано 2026-05-27. Пятый агент `tester` интегрирован в пайплайн между coder и reviewer: парсит `## Test plan` из задачи, прогоняет make-таргеты (smoke/integration/trace), возвращает PASS/FAIL/INFRA_ERROR с автоматическим маршрутингом (FAIL→coder, INFRA_ERROR→BLOCKED, PASS→reviewer). Обновлены `.claude/agents/tester.md`, `CLAUDE.md`, `reviewer.md`, `.claude/skills/task/SKILL.md`, создан `.claude/agents/_template.md`. Замечание: самотест пайплайна (positive/negative/infra прогоны) отложен на запуск задачи 15.
