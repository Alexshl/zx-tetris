#include "sound.h"
#include <sound.h>   /* z88dk newlib bit_beep(dur_ms, freq_hz) */

void sfx_lock(void)
{
    bit_beep(50, 200);
}

void sfx_line_clear(void)
{
    bit_beep(50, 600);
    bit_beep(50, 900);
    bit_beep(50, 1300);
}

void sfx_game_over(void)
{
    bit_beep(150, 400);
    bit_beep(150, 250);
    bit_beep(150, 150);
}
