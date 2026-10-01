#pragma once

void logger_task_init(unsigned long taskPriority);
void logger(const char *format, ...);
