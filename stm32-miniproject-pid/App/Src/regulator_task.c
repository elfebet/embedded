#include "regulator_task.h"
#include "pid_controller.h"
#include "event_bus.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdlib.h>
#include "logger_task.h"

typedef enum {
    RCMD_SP_DELTA,
    RCMD_SP_SET
} reg_cmd_type_t;

typedef struct {
    reg_cmd_type_t type;
    int32_t value_mv;
} reg_cmd_t;

#define REG_STACK_WORDS  256u // 256*4 = 1 KB
#define REG_QUEUE_LEN    16u  // 16 commands
#define REG_PERIOD_MS    10u  // PID рахує кожні 10 мс (Ts = 0,01 с, 100 Гц)

/* Перевірено на моделі цього RC-кола (з навантаженням ступенів і шумом АЦП),
   крок 0 → 1,5 В: перерегулювання 0,5 %, вихід на уставку ±2 % за 0,73 с.
   Ті самі коефіцієнти працюють і з C1 = 41…61 мкФ, і з C1 = 10 мкФ.        */
#define PID_KP             2.0f      // пропорційний коефіцієнт
#define PID_KI             3.0f      // інтегральний коефіцієнт
#define PID_KD             0.2f      // диференційний коефіцієнт
#define PID_DFILT_TAU_S    0.02f     // стала часу фільтра D-складової, 20 мс

#define SP_STEP_MV          100      // одне клацання енкодера змінює уставку на 100 мВ
#define SP_MIN_MV             0      // мінімальна уставка
#define SP_MAX_MV          3000      // максимальна (< 3300: регулятору потрібен запас)
#define SP_INITIAL_MV      1500      // уставка після ввімкнення

#define SETTLED_BAND_MV      30      // "вийшов на уставку" = похибка ≤ 30 мВ …
#define SETTLED_TIME_MS     300u     // … протягом 300 мс поспіль

static StaticTask_t  s_tcb;
static StackType_t   s_stack[REG_STACK_WORDS];
static StaticQueue_t s_qcb;
static uint8_t       s_qbuf[REG_QUEUE_LEN * sizeof(reg_cmd_t)];
static QueueHandle_t s_queue;

static const pwm_t *s_pwm;
static const adc_t *s_adc;

volatile reg_status_t g_reg;
volatile pid_tuning_t g_pid_tuning = { PID_KP, PID_KI, PID_KD };

static void on_encoder(void *ctx, bus_event_t event, int32_t steps) {
    reg_cmd_t cmd = { RCMD_SP_DELTA, steps * SP_STEP_MV }; // +1 detent -> +100 мВ
    xQueueSend(s_queue, &cmd, 0);
}

static void on_button(void *ctx, bus_event_t event, int32_t value) {
    reg_cmd_t cmd = { RCMD_SP_SET, SP_MIN_MV }; // button -> setpoint 0 V
    xQueueSend(s_queue, &cmd, 0);
}

static int32_t clamp_sp(int32_t mv) {
    if (mv < SP_MIN_MV) return SP_MIN_MV;
    if (mv > SP_MAX_MV) return SP_MAX_MV;
    return mv;
}

// увімкнути лічильник тактів процесора (для вимірювання часу)
static void dwt_init(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;   // увімкнути блок трасування ядра Cortex-M4
    DWT->CYCCNT = 0;                                  // обнулити лічильник тактів
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;             // запустити: +1 кожен такт (84 млн/с)
}

static void regulatorTask(void *arg) {
    pid_data_t pid;
    pid_tuning_t applied = { PID_KP, PID_KI, PID_KD };
    pid_init(&pid, applied.kp, applied.ki, applied.kd,
             REG_PERIOD_MS / 1000.0f,        // Ts = 10/1000 = 0,01 s
             PID_DFILT_TAU_S, 0.0f, 1.0f);   // τ = 0,02 s, output 0..1

    int32_t sp_mv = SP_INITIAL_MV; // setpoint — 1500 mV
    bool settled = false;          // вийшли на уставку?
    TickType_t in_band_since = 0;  // коли увійшли в смугу ±30 мВ
    bool in_band  = false;         // зараз у смузі?

    const uint32_t cycles_per_us = HAL_RCC_GetHCLKFreq() / 1000000u; // 84 такти = 1 мкс
    const TickType_t period = pdMS_TO_TICKS(REG_PERIOD_MS);          // 10 мс у тіках
    TickType_t last = xTaskGetTickCount();                           // точка відліку періоду

    for (;;) {
        if ((TickType_t)(xTaskGetTickCount() - last) >= 2u * period)
            g_reg.overruns++;

        vTaskDelayUntil(&last, period);
        uint32_t c0 = DWT->CYCCNT;

        reg_cmd_t cmd;
        while (xQueueReceive(s_queue, &cmd, 0) == pdTRUE) {
            sp_mv = clamp_sp(cmd.type == RCMD_SP_DELTA ? sp_mv + cmd.value_mv : cmd.value_mv);
        }

        // --- values changed in debug
        pid_tuning_t want = {
            g_pid_tuning.kp,
            g_pid_tuning.ki,
            g_pid_tuning.kd
        };
        if (want.kp != applied.kp || want.ki != applied.ki || want.kd != applied.kd) {
            applied = want;
            pid_set_gains(&pid, applied.kp, applied.ki, applied.kd);
        }

        int32_t y_mv = (int32_t)adc_read_mv(s_adc);
        float u = pid_update(&pid, sp_mv / 1000.0f, y_mv / 1000.0f);
        pwm_set_duty(s_pwm, u);

        int32_t err = sp_mv - y_mv;
        if (abs(err) <= SETTLED_BAND_MV) {
            if (!in_band) {
                in_band = true;
                in_band_since = xTaskGetTickCount();
            }
        } else {
            in_band = false;
        }
        bool now_settled = in_band && (TickType_t)(xTaskGetTickCount() - in_band_since) >= pdMS_TO_TICKS(SETTLED_TIME_MS);
        if (now_settled != settled) {
            settled = now_settled;
            event_bus_publish(EVENT_REG_SETTLED, settled ? 1 : 0);
        }

        // telemetry for debug
        g_reg.setpoint_mv = sp_mv;
        g_reg.measured_mv = y_mv;
        g_reg.error_mv = err;
        g_reg.duty_pct = u * 100.0f; // 0..1 -> 0..100 %
        g_reg.p = pid.p_term;
        g_reg.i = pid.i_term;
        g_reg.d = pid.d_term;
        g_reg.settled = settled;

        uint32_t us = (DWT->CYCCNT - c0) / cycles_per_us;
        g_reg.exec_us = us;
        if (us > g_reg.exec_us_max)
            g_reg.exec_us_max = us;
    }
}

void regulator_task_init(const pwm_t *pwm, const adc_t *adc, UBaseType_t taskPriority) {
    s_pwm = pwm;
    s_adc = adc;
    dwt_init();

    s_queue = xQueueCreateStatic(REG_QUEUE_LEN, sizeof(reg_cmd_t), s_qbuf, &s_qcb);

    bool ok = event_bus_subscribe(EVENT_ENCODER_STEP, on_encoder, NULL) &&
              event_bus_subscribe(EVENT_BUTTON, on_button,  NULL);

    TaskHandle_t task = xTaskCreateStatic(
        regulatorTask,
        "pid",
        REG_STACK_WORDS,
        NULL,
        taskPriority,
        s_stack,
        &s_tcb
    );
    configASSERT(s_queue != NULL && ok && task != NULL);
}
