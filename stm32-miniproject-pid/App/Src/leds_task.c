#include "leds_task.h"
#include "motor_fsm.h"
#include "event_bus.h"

#include "main.h"
#include "FreeRTOS.h"
#include "timers.h"
#include "logger_task.h"

#define HEARTBEAT_MS   500u
#define FAST_BLINK_MS  100u

static StaticTimer_t s_heart_tcb;
static TimerHandle_t s_heart_timer;

static void on_motor_state(void *ctx, bus_event_t event, int32_t value) {
    motor_state_t state = (motor_state_t)value;
    HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, state == MS_RUN_CW ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, state == MS_RUN_CCW ? GPIO_PIN_SET : GPIO_PIN_RESET);
    logger("[LED] change led on motor state");
}

static void on_reg_settled(void *ctx, bus_event_t event, int32_t settled) {
    int period_ms = settled ? HEARTBEAT_MS : FAST_BLINK_MS;
    xTimerChangePeriod(s_heart_timer, pdMS_TO_TICKS(period_ms), 0);
    logger("[LED] on reg settled. period_ms: %d", period_ms);
}

static void on_heartbeat(TimerHandle_t timer) {
    HAL_GPIO_TogglePin(LED_HEART_GPIO_Port, LED_HEART_Pin);
}

void leds_task_init() {
    s_heart_timer = xTimerCreateStatic("heart", pdMS_TO_TICKS(FAST_BLINK_MS), pdTRUE, NULL, on_heartbeat, &s_heart_tcb);

    bool ok = event_bus_subscribe(EVENT_MOTOR_STATE, on_motor_state, NULL) &&
              event_bus_subscribe(EVENT_REG_SETTLED, on_reg_settled, NULL);
    configASSERT(ok && s_heart_timer != NULL);

    xTimerStart(s_heart_timer, 0);
}
