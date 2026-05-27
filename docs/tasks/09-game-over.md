# Task 09: Game Over и рестарт

**Status**: DONE
**Зависит от**: 08
**Цель**: Когда новая фигура не может заспавниться — показать «GAME OVER», подождать клавишу, перезапустить игру с нулём.

## Acceptance criteria

- [ ] При невозможности спавна — игра останавливается
- [ ] По центру стакана появляется надпись «GAME OVER»
- [ ] Под ней — «PRESS ENTER»
- [ ] Нажатие Enter полностью сбрасывает состояние: board, score, level, lines, и запускает новую партию
- [ ] До нажатия Enter обновление фигур заморожено, HUD показывает финальные значения

## Шаги

### 1. Game-over уже есть

В `game.c` функция `spawn()` уже выставляет `G.game_over = 1` при коллизии. Используем это.

### 2. Экран Game Over

Расширить `render.h`:

```c
void render_game_over(void);
void render_clear_game_over(void);
```

`render.c`:

```c
void render_game_over(void) {
    // Центр стакана — строка 10, колонка 9 (-5 от центра текста "GAME OVER" = 9 знаков)
    // Стакан: cols 8..17, rows 2..21. Центр (12, 12).
    // "GAME OVER" = 9 символов, центр на ст. 8 + (10 - 9)/2 = 8.5 → ст. 9
    printf("\x16%c%c" "GAME OVER",  10, 9);
    printf("\x16%c%c" "PRESS ENT",  12, 9);
}

void render_clear_game_over(void) {
    printf("\x16%c%c" "         ", 10, 9);
    printf("\x16%c%c" "         ", 12, 9);
}
```

### 3. Сброс состояния

Расширить `game.h`:

```c
void game_reset(void);
```

`game.c`:

```c
void game_reset(void) {
    score = 0;
    lines_total = 0;
    level = 0;
    drop_period = level_drop_period(0);
    lines_cleared_last = 0;
    G.game_over = 0;
    for (uint8_t r = 0; r < ROWS; ++r)
        for (uint8_t c = 0; c < COLS; ++c)
            G.board[r][c] = 0;
    spawn();
}
```

### 4. Главный цикл в `main.c`

```c
#include <input.h>

int main(void) {
    render_init();
    render_hud_force_redraw();
    game_init();
    input_init();

    while (1) {
        // основной игровой цикл
        render_clear_board();
        render_redraw_board();
        render_hud(score, lines_total, level);
        if (!G.game_over) render_piece(G.cur.id, G.cur.rot, G.cur.row, G.cur.col);

        uint8_t frame = 0;
        input_state_t in;
        while (!G.game_over) {
            intrinsic_halt();
            input_poll(&in);
            if (in.left)      game_move(-1);
            if (in.right)     game_move(+1);
            if (in.rotate)    game_rotate();
            if (in.hard_drop) game_hard_drop();
            if (in.soft_drop) { game_tick(); frame = 0; }
            else if (++frame >= drop_period) { frame = 0; game_tick(); }
            render_hud(score, lines_total, level);
        }

        render_game_over();
        // ждём Enter
        while (1) {
            intrinsic_halt();
            if (in_key_pressed(IN_KEY_SCANCODE_ENTER)) break;
        }
        // дождаться отпускания, чтобы не сразу-рестарт
        while (in_key_pressed(IN_KEY_SCANCODE_ENTER)) intrinsic_halt();

        render_clear_game_over();
        game_reset();
        render_hud_force_redraw();
    }
    return 0;
}
```

### 5. Сборка

Без изменений в Makefile.

## Verification

- Спецально заполни верх стакана, спровоцируй game over.
- «GAME OVER / PRESS ENT» появляется по центру.
- HUD показывает финальный счёт.
- Enter — экран очищается, начинается новая игра с нулевым счётом, фигура падает сверху.
- В новой игре всё работает без артефактов от предыдущей.

## Заметки

- Чтобы быстрее тестировать game-over, добавь в Makefile цель `make debug` с `-DDEBUG_FAST_DEATH` и используй её, чтобы стартовать с почти полным стаканом.
- В будущем здесь можно показать «high score» (потребует сохранения — но на Спектруме «сохранения» — это запись на ленту, что нетривиально; обычно high-score хранится только в памяти и сбрасывается перезагрузкой).
- Анимация «затухания» (заполнение стакана белым снизу вверх перед game-over экраном) — стандартный приём NES Tetris. Можно добавить отдельной задачей.

## Completion note

Реализовано 2026-05-26. Реализована функция `render_game_over()`, которая выводит «GAME OVER» и «PRESS ENT» (сокращено из-за ограничения ширины стакана 10 клеток) через прямую установку символов и атрибутов в буфер экрана. Добавлена `game_reset()` для обнуления состояния. Главный цикл переписан с внешней бесконечной сессией (restart loop) и внутренним game loop. После game over ожидается нажатие Enter с защитой от удержания (release-press-release). HUD автоматически замораживается на финальных значениях — просто не вызывается в ветке game over. Текст GAME OVER стирается автоматически через `redraw_board()` так как находится в пределах стакана. Сборка без warnings.
