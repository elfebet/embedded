#include "event_bus.h"
#include <stddef.h>

typedef struct {
    bus_event_t event;
    EventBusCallback_t callback;
    void *context;
} subscriber_t;

static subscriber_t s_subs[EVENT_BUS_MAX_SUBSCRIBERS];
static uint8_t s_subs_count = 0; // subscribes count

bool event_bus_subscribe(bus_event_t event, EventBusCallback_t cb, void *ctx) {
    if (cb == NULL || event >= EVENT_COUNT || s_subs_count >= EVENT_BUS_MAX_SUBSCRIBERS)
        return false;

    s_subs[s_subs_count++] = (subscriber_t){
        .event = event,
        .callback = cb,
        .context = ctx
    };

    return true;
}

void event_bus_publish(bus_event_t event, int32_t value) {
    for (uint8_t i = 0; i < s_subs_count; i++) {
        if (s_subs[i].event == event)
            s_subs[i].callback(s_subs[i].context, event, value);
    }
}
