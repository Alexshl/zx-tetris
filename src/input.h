#ifndef INPUT_H
#define INPUT_H
#include <stdint.h>

typedef struct {
    uint8_t left;       /* edge or autorepeat */
    uint8_t right;
    uint8_t soft_drop;  /* held */
    uint8_t rotate;     /* edge only */
    uint8_t hard_drop;  /* edge only */
} input_state_t;

void input_init(void);
void input_poll(input_state_t *out);

#endif
