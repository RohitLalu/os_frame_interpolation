#ifndef CAM_TASKS_H
#define CAM_TASKS_H

#include <Arduino.h>
#include "esp_camera.h"
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "task_config.h"

typedef struct {
    TaskHandle_t handle;
    const char* taskName;
    TickType_t   deadline; // absolute deadline or period in ticks
} EdfTaskInfo;

// Globals are defined in cam_tasks.cpp
extern EdfTaskInfo CaptureTask;
extern EdfTaskInfo InterpolateTask;
extern EdfTaskInfo TransmitTask;
extern EdfTaskInfo SchedulerTask;

extern EdfTaskInfo CAM_TASKLIST[NUM_MANAGED_TASKS];
extern SemaphoreHandle_t g_task_list_mutex;

// Function declarations
void Scheduler(void* parameters);
void CaptureImage(void* parameters);
void Interpolator(void* parameters);
void Transmitter(void* parameters);
void init_mutexes();

#endif // CAM_TASKS_H