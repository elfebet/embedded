#pragma once
#include <stdint.h>

/* Non-owning view; the buffer must remain valid while in use and may be
 * overwritten by a later capture. */
typedef struct {
    const uint16_t *data;
    uint32_t count;
} scope_samples_t;
