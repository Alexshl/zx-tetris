#include "input.h"
#include <input.h>   /* z88dk newlib — IN_KEY_SCANCODE_* + in_key_pressed */

#define REPEAT_DELAY 12
#define REPEAT_RATE  4

static struct {
    uint8_t left_held_frames;
    uint8_t right_held_frames;
    uint8_t rotate_was_down;
    uint8_t hard_was_down;
} S;

void input_init(void)
{
    S.left_held_frames  = 0;
    S.right_held_frames = 0;
    S.rotate_was_down   = 0;
    S.hard_was_down     = 0;
}

static uint8_t autorepeat(uint8_t held_now, uint8_t *frames)
{
    if (!held_now) { *frames = 0; return 0; }
    if (*frames == 0) { *frames = 1; return 1; }
    (*frames)++;
    if (*frames < REPEAT_DELAY) return 0;
    if ((uint8_t)(*frames - REPEAT_DELAY) % REPEAT_RATE == 0) return 1;
    return 0;
}

void input_poll(input_state_t *out)
{
    uint8_t l, r, d, rot, hd;
    /* ZX-стрелки: Caps Shift + 5/6/7/8 (Fuse мапит клавиатурные стрелки на эти физ. клавиши). */
    l   = in_key_pressed(IN_KEY_SCANCODE_5)     ? 1 : 0;   /* left  */
    r   = in_key_pressed(IN_KEY_SCANCODE_8)     ? 1 : 0;   /* right */
    d   = in_key_pressed(IN_KEY_SCANCODE_6)     ? 1 : 0;   /* down  */
    rot = (in_key_pressed(IN_KEY_SCANCODE_7) || in_key_pressed(IN_KEY_SCANCODE_ENTER)) ? 1 : 0;  /* up / enter */
    hd  = in_key_pressed(IN_KEY_SCANCODE_SPACE) ? 1 : 0;

    out->left      = autorepeat(l, &S.left_held_frames);
    out->right     = autorepeat(r, &S.right_held_frames);
    out->soft_drop = d;
    out->rotate    = (rot && !S.rotate_was_down) ? 1 : 0;
    out->hard_drop = (hd  && !S.hard_was_down)   ? 1 : 0;
    S.rotate_was_down = rot;
    S.hard_was_down   = hd;
}
