#include "app.h"
#include "main.h"
#include "cmsis_os.h"

#define RED_DURATION         5000
#define RED_YELLOW_DURATION  2000
#define GREEN_DURATION       5000
#define GREEN_BLINK_DURATION 3000
#define YELLOW_DURATION      2000
#define BLINK_DELAY          500

typedef enum {
    CMD_LED_OFF = 0,
    CMD_LED_ON,
    CMD_LED_BLINK
} Command_t;

static TaskHandle_t xRedTaskHandle = NULL;
static TaskHandle_t xYellowTaskHandle = NULL;
static TaskHandle_t xGreenTaskHandle = NULL;

static void vLedTask(GPIO_TypeDef* port, uint16_t pin) {
    uint32_t rxCMD = CMD_LED_OFF;
    Command_t currentCMD = CMD_LED_OFF;
    TickType_t xTicksToWait = portMAX_DELAY;

    while(1) {
        if (xTaskNotifyWait(0x00, UINT32_MAX, &rxCMD, xTicksToWait) == pdTRUE) {
            currentCMD = (Command_t)rxCMD;
            xTicksToWait = (currentCMD == CMD_LED_BLINK) ? pdMS_TO_TICKS(BLINK_DELAY) : portMAX_DELAY;

            if (currentCMD == CMD_LED_OFF) {
                HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
            } else if (currentCMD == CMD_LED_ON) {
                HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
            }
        } else if (currentCMD == CMD_LED_BLINK) {
            HAL_GPIO_TogglePin(port, pin);
        }
    }
}

void vRedLedTask(void *args) {
    vLedTask(LED_Red_GPIO_Port, LED_Red_Pin);
}

void vYellowLedTask(void *args) {
    vLedTask(LED_Yellow_GPIO_Port, LED_Yellow_Pin);
}

void vGreenLedTask(void *args) {
    vLedTask(LED_Green_GPIO_Port, LED_Green_Pin);
}

void vControllerTask(void *args) {
    while(1) {
        // red color, 5 sec
        xTaskNotify(xRedTaskHandle, CMD_LED_ON, eSetValueWithOverwrite);
        xTaskNotify(xYellowTaskHandle, CMD_LED_OFF, eSetValueWithOverwrite);
        xTaskNotify(xGreenTaskHandle, CMD_LED_OFF, eSetValueWithOverwrite);
        vTaskDelay(pdMS_TO_TICKS(RED_DURATION));

        // red + yellow colors, 2 sec
        xTaskNotify(xYellowTaskHandle, CMD_LED_ON, eSetValueWithOverwrite);
        vTaskDelay(pdMS_TO_TICKS(RED_YELLOW_DURATION));

        // green color, 5 sec
        xTaskNotify(xRedTaskHandle, CMD_LED_OFF, eSetValueWithOverwrite);
        xTaskNotify(xYellowTaskHandle, CMD_LED_OFF, eSetValueWithOverwrite);
        xTaskNotify(xGreenTaskHandle, CMD_LED_ON, eSetValueWithOverwrite);
        vTaskDelay(pdMS_TO_TICKS(GREEN_DURATION));

        // green blink, 3 sec
        xTaskNotify(xGreenTaskHandle, CMD_LED_BLINK, eSetValueWithOverwrite);
        vTaskDelay(pdMS_TO_TICKS(GREEN_BLINK_DURATION));

        // yellow, 2 sec
        xTaskNotify(xGreenTaskHandle, CMD_LED_OFF, eSetValueWithOverwrite);
        xTaskNotify(xYellowTaskHandle, CMD_LED_ON, eSetValueWithOverwrite);
        vTaskDelay(pdMS_TO_TICKS(YELLOW_DURATION));
    }
}

void app_start(void) {
    const UBaseType_t priority = osPriorityNormal+2;
    const uint16_t stackSize = 256;

    xTaskCreate(vRedLedTask, "led_red", stackSize, NULL, priority, &xRedTaskHandle);
    xTaskCreate(vYellowLedTask, "led_yellow", stackSize, NULL, priority, &xYellowTaskHandle);
    xTaskCreate(vGreenLedTask, "led_green", stackSize, NULL, priority, &xGreenTaskHandle);

    xTaskCreate(vControllerTask, "controller", stackSize, NULL, osPriorityNormal+1, NULL);
    vTaskStartScheduler();
}
