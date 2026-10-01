#include "motor_task.h"
#include "event_bus.h"
#include "logger_task.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define MTR_STACK_WORDS  256u // 256 words × 4 byte = 1 KB
#define MTR_QUEUE_LEN    16u
#define MTR_TICK_MS      10u

static StaticTask_t  s_tcb;
static StackType_t   s_stack[MTR_STACK_WORDS];
static StaticQueue_t s_qcb;
static uint8_t       s_qbuf[MTR_QUEUE_LEN * sizeof(motor_event_t)];
static QueueHandle_t s_queue;

static motor_fsm_t   s_fsm;
static volatile motor_state_t s_published_state = MS_IDLE;
static volatile uint32_t  s_dropped;

static inline uint32_t now_ms(void) {
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);   // тіки ОС × мс на тік (1)
}

static void on_encoder(void *ctx, bus_event_t event, int32_t steps) {
    motor_event_t ev = (steps > 0) ? MEV_STEP_CW : MEV_STEP_CCW;
    for (int32_t n = (steps > 0) ? steps : -steps; n > 0; n--) {
        motor_task_post(ev);
    }
}

static void on_button(void *ctx, bus_event_t event, int32_t value) {
    motor_task_post(MEV_STOP);
}

static void on_fsm_state(void *ctx, motor_state_t state) {
    s_published_state = state;
    event_bus_publish(EVENT_MOTOR_STATE, (int32_t)state);
    logger("[FSM] Motor state: %s", motor_fsm_state_name(state));
}

static void motorTask(void *arg) {
    motor_event_t event;
    for (;;) {
        if (xQueueReceive(s_queue, &event, pdMS_TO_TICKS(MTR_TICK_MS)) == pdTRUE) {
            motor_fsm_dispatch(&s_fsm, event, now_ms());
        }
        motor_fsm_dispatch(&s_fsm, MEV_TICK, now_ms());
    }
}

void motor_task_init(motor_t motor, const motor_fsm_config_t *config, unsigned long taskPriority) {
    motor_fsm_init(&s_fsm, config, motor, on_fsm_state, NULL, 0);

    s_queue = xQueueCreateStatic(MTR_QUEUE_LEN, sizeof(motor_event_t), s_qbuf, &s_qcb);

    bool ok = event_bus_subscribe(EVENT_ENCODER_STEP, on_encoder, NULL) &&
              event_bus_subscribe(EVENT_BUTTON, on_button,  NULL);

    TaskHandle_t task = xTaskCreateStatic(
            motorTask,
            "motor",
            MTR_STACK_WORDS,
            NULL,
            taskPriority,
            s_stack,
            &s_tcb
    );
    configASSERT(s_queue != NULL && ok && task != NULL);
}

bool motor_task_post(motor_event_t event) {
    if (xQueueSend(s_queue, &event, 0) == pdTRUE)
        return true;

    s_dropped++;
    return false;
}

motor_state_t motor_task_get_state(void) {
    return s_published_state;
}

uint32_t motor_task_dropped_events(void) {
    return s_dropped;
}
