---
name: task
description: Run a task from docs/tasks/ through the 4-agent pipeline (planner → coder → reviewer → documenter). Usage — /task <id> for a specific task, or /task without args to pick up the next TODO. The skill orchestrates the pipeline, handles REWORK loops, and stops only when the task is DONE or genuinely blocked.
---

# /task — Tetris task pipeline

Запускает задачу из `docs/tasks/` через жёсткий пайплайн **planner → coder → reviewer → documenter**. Главный сеанс **не пишет код сам** — только оркестрирует агентов.

## Аргумент

- `/task 03` — запустить задачу с этим ID
- `/task` (без аргумента) — взять первую `TODO` из `docs/tasks/INDEX.md`

## Шаги

### 0. Определить задачу

1. Прочитать `docs/tasks/INDEX.md`.
2. Если аргумент указан — взять `docs/tasks/<id>-*.md` (найти по префиксу через Glob).
3. Если без аргумента — найти первую строку со статусом `TODO`, взять её ID.
4. Если все задачи `DONE` — сказать «все задачи завершены» и выйти.

### 1. Поменять статус на IN PROGRESS

Через **Edit** в `docs/tasks/INDEX.md` и в шапке файла задачи: `TODO → IN PROGRESS`. Это делается главным сеансом, не агентом — это просто маркер.

### 2. Planner phase

Вызвать агента `planner` через **Agent** tool:

```
Agent({
  subagent_type: "planner",
  description: "Plan task NN",
  prompt: "Задача: docs/tasks/NN-*.md. Прочитай её и выдай refined plan по шаблону из своего system prompt. Это первый прогон — feedback нет."
})
```

Сохрани вывод как `plan_v1`.

### 3. Coder phase

Вызвать `coder` с `plan_v1`:

```
Agent({
  subagent_type: "coder",
  description: "Implement task NN",
  prompt: "Refined plan ниже. Реализуй его согласно своему system prompt.\n\n<вставить plan_v1>"
})
```

Сохрани вывод как `code_report_v1`.

Если `code_report_v1.status == HALT_NEED_PLAN_FIX`:
- Перейти к шагу **2** заново с дополнительным feedback из `code_report_v1.feedback_to_planner` в промпте planner'a.
- Максимум 3 итерации planner ↔ coder. Если на третий раз снова HALT — поменять статус на **BLOCKED** и сообщить пользователю.

### 4. Reviewer phase

Вызвать `reviewer`:

```
Agent({
  subagent_type: "reviewer",
  description: "Review task NN",
  prompt: "Задача: docs/tasks/NN-*.md\nRefined plan: <plan_vN>\nCoder report: <code_report_vN>\n\nВыдай verdict по своему шаблону."
})
```

Сохрани вывод как `review_v1`.

Если `review_v1.status == REWORK`:
- Если `review_v1.suggested_next_agent == "planner"` → шаг **2** с feedback из issues.
- Иначе → шаг **3** заново с issues в виде feedback для coder'a.
- Максимум 3 итерации coder ↔ reviewer. Если на третий раз REWORK — **BLOCKED**.

Если `APPROVED` → шаг **5**.

### 5. Documenter phase

Вызвать `documenter`:

```
Agent({
  subagent_type: "documenter",
  description: "Mark task NN done",
  prompt: "Задача NN завершена. Reviewer note: <короткая выдержка>. Сегодняшняя дата: <YYYY-MM-DD из system context>. Обнови INDEX.md и шапку файла задачи. Действуй по своему system prompt."
})
```

### 6. Финальный отчёт пользователю

Один короткий блок:

```
Task NN — DONE
Planner iterations: X
Coder iterations: Y
Reviewer verdict: APPROVED
Files changed: ...
Next pending task: task MM — <title>

Ручная проверка: `make run` → загрузить .tap в Fuse → пройтись по acceptance criteria задачи.
```

## Правила оркестрации

- **Главный сеанс не пишет код напрямую.** Только Edit для пометки IN PROGRESS и финальный отчёт пользователю. Всё остальное — через Agent.
- **Каждый агент получает только свои входы** (см. их system prompts). Не передавай coder'у задачу напрямую — он работает по плану от planner'a.
- **Не запускай `make run`** — это ручная верификация пользователем в Fuse.
- **Лимит итераций** — 3 для каждой пары (planner↔coder, coder↔reviewer). После лимита — BLOCKED.
- **BLOCKED** означает: проставить статус **BLOCKED** в INDEX.md и шапке задачи, сообщить пользователю конкретно, что заблокировало (последний feedback / последний issues-список).

## Что если задача 01 (setup-toolchain)

Особый случай: установка z88dk требует ~15-минутной локальной сборки и `brew install`. Это нельзя автоматически — coder остановится на шаге установки и вернёт **HALT** с просьбой к пользователю сделать установку вручную.

Главный сеанс в этом случае:
1. Сообщает пользователю команды (`git clone --recursive https://github.com/z88dk/z88dk.git ~/tools/z88dk && cd ~/tools/z88dk && ./build.sh` и `brew install --cask fredm-fuse`).
2. Ждёт подтверждения, что тулчейн установлен (`zcc +zx --help` работает).
3. Запускает coder'a уже на «оставшуюся часть task 01» — создание Makefile, zpragma.inc, src/main.c и проверочную сборку.

## Что НЕ делает /task

- Не выбирает задачу за пользователя без явного запроса (но если `/task` без аргумента — да, берёт первую TODO).
- Не реорганизует `docs/tasks/`.
- Не редактирует `CLAUDE.md` или агентов.
- Не правит чужие задачи, даже если кажется, что в них ошибка — это отдельный диалог с пользователем.
