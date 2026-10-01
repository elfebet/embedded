#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    EVENT_ENCODER_STEP = 0,
    EVENT_BUTTON,
    EVENT_MOTOR_STATE,
    EVENT_REG_SETTLED,
    EVENT_COUNT
} bus_event_t;

typedef void (*EventBusCallback_t)(void *ctx, bus_event_t event, int32_t value);

#define EVENT_BUS_MAX_SUBSCRIBERS  8

bool event_bus_subscribe(bus_event_t event, EventBusCallback_t cb, void *ctx);
void event_bus_publish (bus_event_t event, int32_t value);
