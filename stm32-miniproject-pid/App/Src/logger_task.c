#include "logger_task.h"
#include "main.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "cmsis_os.h"

#define LOG_QUEUE_SIZE   16
#define LOG_BUF_LENGTH   64

typedef struct {
    char text[LOG_BUF_LENGTH];
} LogMessage_t;

static StaticTask_t  s_task;
static StackType_t   s_stack[LOG_BUF_LENGTH];
static StaticQueue_t s_qcb;
static uint8_t       s_qbuf[LOG_QUEUE_SIZE * sizeof(LogMessage_t)];
static QueueHandle_t s_queue;

void logTask(void *argument) {
    LogMessage_t msg;
    for(;;) {
        if (xQueueReceive(s_queue, &msg, portMAX_DELAY) == pdPASS) {
            HAL_UART_Transmit(&huart6, (uint8_t*)msg.text, strlen(msg.text), 100);
        }
    }
}

void logger_task_init(unsigned long taskPriority) {
    s_queue = xQueueCreateStatic(LOG_QUEUE_SIZE, sizeof(LogMessage_t), s_qbuf, &s_qcb);

    TaskHandle_t task = xTaskCreateStatic(logTask, "logger", LOG_BUF_LENGTH, NULL, taskPriority, s_stack, &s_task);
    configASSERT(task != NULL && s_queue != NULL);
}

void logger(const char *format, ...) {
    LogMessage_t msg;
    va_list args;

    va_start(args, format);
    vsnprintf(msg.text, LOG_BUF_LENGTH - 3, format, args);
    va_end(args);
    strncat(msg.text, "\r\n", 3);

    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        xQueueSend(s_queue, &msg, pdMS_TO_TICKS(10));
    } else {
        HAL_UART_Transmit(&huart6, (uint8_t*)msg.text, strlen(msg.text), HAL_MAX_DELAY);
    }
}
