#include <intrinsic.h>
#include "render.h"
#include "game.h"
#include "input.h"
#include "sound.h"
#include <input.h>   /* для in_key_pressed / IN_KEY_SCANCODE_ENTER */

int main(void)
{
    uint8_t       frame;
    input_state_t in;

    render_init();
    render_clear_board();
    game_init();
    input_init();
    render_hud_force_redraw();
    render_hud(G.score, G.lines_total, G.level);

    while (1) {
        frame = 0;

        while (!G.game_over) {
            intrinsic_ei();
            intrinsic_halt();
            render_hud(G.score, G.lines_total, G.level);
            input_poll(&in);
            if (in.left)      game_move(-1);
            if (in.right)     game_move(+1);
            if (in.rotate)    game_rotate();
            if (in.hard_drop) { game_hard_drop();      frame = 0; continue; }
            if (in.soft_drop) { game_soft_drop_step(); frame = 0; continue; }
            frame++;
            if (frame >= G.drop_period) {
                frame = 0;
                game_tick();
            }
        }

        sfx_game_over();
        render_game_over();

        while (in_key_pressed(IN_KEY_SCANCODE_ENTER))  { intrinsic_ei(); intrinsic_halt(); }
        while (!in_key_pressed(IN_KEY_SCANCODE_ENTER)) { intrinsic_ei(); intrinsic_halt(); }
        while (in_key_pressed(IN_KEY_SCANCODE_ENTER))  { intrinsic_ei(); intrinsic_halt(); }

        game_reset();
        input_init();
        render_hud_force_redraw();
        render_hud(G.score, G.lines_total, G.level);
    }

    return 0;
}
