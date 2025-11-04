#include <Arduino.h>
#include "esp_camera.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "cam_tasks.h"
#include "task_config.h"

EdfTaskInfo CaptureTask = {NULL, "Capture", pdMS_TO_TICKS(CAPTURE_PERIOD_MS)};
EdfTaskInfo InterpolateTask = {NULL, "Interpolate", pdMS_TO_TICKS(INTERPOLATE_PERIOD_MS)};
EdfTaskInfo TransmitTask = {NULL, "Transmit", pdMS_TO_TICKS(TRANSMIT_PERIOD_MS)};
EdfTaskInfo SchedulerTask = {NULL, "Scheduler", pdMS_TO_TICKS(SCHEDULER_PERIOD_MS)};

EdfTaskInfo CAM_TASKLIST[NUM_MANAGED_TASKS] = {
    CaptureTask,
    InterpolateTask,
    TransmitTask
};

camera_fb_t *frameA = NULL;
camera_fb_t *frameB = NULL;
camera_fb_t *interpFrame = NULL;

volatile bool frameA_ready = false;
volatile bool frameB_ready = false;
volatile bool interp_ready = false;

SemaphoreHandle_t g_task_list_mutex;
SemaphoreHandle_t g_frame_mutex;
SemaphoreHandle_t g_interp_mutex;

void init_mutexes() {
    g_task_list_mutex = xSemaphoreCreateMutex();
    g_frame_mutex = xSemaphoreCreateMutex();
    g_interp_mutex = xSemaphoreCreateMutex();

    if (!g_task_list_mutex || !g_frame_mutex || !g_interp_mutex) {
        Serial.println("Error: Could not create mutexes!");
        while (1);
    }
}

void Scheduler(void *pvParameters) {
    const TickType_t xFrequency = pdMS_TO_TICKS(SCHEDULER_PERIOD_MS);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        if (xSemaphoreTake(g_task_list_mutex, portMAX_DELAY) == pdTRUE) {
            int size_TASKLIST = sizeof(CAM_TASKLIST) / sizeof(CAM_TASKLIST[0]);
            EdfTaskInfo temp_task;

            // Sort by earliest deadline
            for (int i = 0; i < size_TASKLIST; i++) {
                temp_task = CAM_TASKLIST[i];
                for (int j = i + 1; j < size_TASKLIST; j++) {
                    if (temp_task.deadline > CAM_TASKLIST[j].deadline) {
                        CAM_TASKLIST[i] = CAM_TASKLIST[j];
                        CAM_TASKLIST[j] = temp_task;
                        temp_task = CAM_TASKLIST[i];
                    }
                }
            }

            // Assign priorities (highest = earliest deadline)
            for (int i = 0; i < size_TASKLIST; ++i) {
                UBaseType_t newPriority = PRIO_WORKER_HIGH - i;
                if (newPriority < PRIO_WORKER_BASE)
                    newPriority = PRIO_WORKER_BASE;
                vTaskPrioritySet(CAM_TASKLIST[i].handle, newPriority);
            }

            xSemaphoreGive(g_task_list_mutex);
        }

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

camera_fb_t *fb = NULL;
int counter =0;

void CaptureImage(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        // Wait to acquire the lock before touching the global 'fb'
        if (xSemaphoreTake(g_frame_mutex, portMAX_DELAY) == pdTRUE) {
            
            //return the OLD frame buffer 
            if (fb) {
                esp_camera_fb_return(fb);
                fb = NULL;
            }

            //NEW frame buffer.
            fb = esp_camera_fb_get();
            if (!fb) {
                Serial.println("Camera capture failed");
            }

            // 3. Release the lock
            xSemaphoreGive(g_frame_mutex);
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(CAPTURE_PERIOD_MS));
    }
}
void Interpolator(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {      
            //No interpolation here. Interpolation in python
            counter++;
            digitalWrite(25, counter%2);
            analogWrite(4, counter%256);
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(INTERPOLATE_PERIOD_MS));
}


void Transmitter(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        // Wait to acquire the lock to safely read the 'fb' pointer
        if (xSemaphoreTake(g_frame_mutex, portMAX_DELAY) == pdTRUE) {
            
            // Only send data if the frame buffer pointer is valid
            if (fb) {
                Serial.write("*S*", 3); 
                uint32_t len = fb->len;
                Serial.write((uint8_t *)&len, 4);
                Serial.write(fb->buf, fb->len);
    
            }
            
            // Release the lock
            xSemaphoreGive(g_frame_mutex);
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TRANSMIT_PERIOD_MS));
    }
}
