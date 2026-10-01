#include "input_task.h"
#include "event_bus.h"
#include "FreeRTOS.h"
#include "task.h"

#define INPUT_STACK_WORDS      192u
#define MAX_STEPS_PER_PERIOD   4
#define INPUT_PERIOD_MS        10u

static StaticTask_t s_tcb;
static StackType_t  s_stack[INPUT_STACK_WORDS];

static encoder_t *s_enc;
static encoder_button_t *s_btn;

static void inputTask(void *arg) {
    TickType_t last = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(INPUT_PERIOD_MS)); // wakeup every 10 ms

        if (encoder_button_poll_pressed(s_btn)) {
            event_bus_publish(EVENT_BUTTON, 1);
        }

        int32_t det = encoder_read_detents(s_enc); // usual 0 or ±1
        if (det >  MAX_STEPS_PER_PERIOD) det =  MAX_STEPS_PER_PERIOD;
        if (det < -MAX_STEPS_PER_PERIOD) det = -MAX_STEPS_PER_PERIOD;
        if (det != 0) {
            event_bus_publish(EVENT_ENCODER_STEP, det);
        }
    }
}

void input_task_init(encoder_t *encoder, encoder_button_t *button, UBaseType_t taskPriority) {
    s_enc = encoder;
    s_btn = button;

    TaskHandle_t task = xTaskCreateStatic(inputTask, "input_task", INPUT_STACK_WORDS, NULL, taskPriority, s_stack, &s_tcb);
    configASSERT(task != NULL);
}
