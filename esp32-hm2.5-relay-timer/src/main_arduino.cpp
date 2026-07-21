#include <Arduino.h>
#include "config.h"

hw_timer_t *timer = NULL;
volatile bool fanState = false;

// ISR (Interrupt Service Routine)
void IRAM_ATTR onFanTimer() {
    fanState = !fanState;
    digitalWrite(FAN_PIN, fanState ? HIGH : LOW);

    // re-start timer
    uint64_t next_time = fanState ? FAN_RUN_TIME_US : FAN_OFF_TIME_US;    
    timerAlarm(timer, next_time, false, 0); // false - do not autoreload, will restart in ISR
}

void setup() {
    pinMode(FAN_PIN, OUTPUT);
    digitalWrite(FAN_PIN, LOW);

    // init timer 
    timer = timerBegin(1000000);
    timerAttachInterrupt(timer, &onFanTimer);

    // start timer
    onFanTimer();
}

void loop() {
    // ignore loop
}