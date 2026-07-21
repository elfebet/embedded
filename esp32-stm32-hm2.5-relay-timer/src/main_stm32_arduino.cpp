#include <Arduino.h>
#include "config.h"

HardwareTimer *timer = nullptr;
volatile bool fanState = false;

void onFanTimer() {
    fanState = !fanState;
    digitalWrite(FAN_PIN, fanState ? HIGH : LOW);

    // re-start timer
    uint64_t next_time = fanState ? FAN_RUN_TIME_US : FAN_OFF_TIME_US;

    timer->setOverflow(next_time, MICROSEC_FORMAT);
    timer->resume();
}

void setup() {
    pinMode(FAN_PIN, OUTPUT);
    digitalWrite(FAN_PIN, LOW);

    timer = new HardwareTimer(TIM2);
    timer->attachInterrupt(onFanTimer);

    onFanTimer();
}

void loop() {
    // ignore loop
}