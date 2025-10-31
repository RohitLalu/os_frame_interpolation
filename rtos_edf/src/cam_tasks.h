#ifndef CAM_TASKS_H
#define CAM_TASKS_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "task_config.h"

// Task structure declarations
struct TaskItem {
    const char* taskName;
    TaskHandle_t handle;
    TickType_t deadline;
    TickType_t period;
    uint8_t priority;
};

// External declarations of global variables
extern TaskItem CaptureTask;
extern TaskItem InterpolateTask;
extern TaskItem TransmitTask;
extern TaskItem CAM_TASKLIST[3];
extern SemaphoreHandle_t g_task_list_mutex;

// Function declarations
void Scheduler(void* parameters);
void CaptureImage(void* parameters);
void Interpolator(void* parameters);
void Transmitter(void* parameters);

#endif // CAM_TASKS_H