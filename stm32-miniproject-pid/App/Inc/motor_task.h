#pragma once
#include "motor_fsm.h"

void motor_task_init(motor_t motor, const motor_fsm_config_t *config, unsigned long taskPriority);

bool motor_task_post(motor_event_t event);
motor_state_t motor_task_get_state(void);
uint32_t motor_task_dropped_events(void);
