#pragma once

#define SERVO_GPIO 5

void servo_timer_setup(void);
void servo_set_angle(int angle);
int servo_get_angle();