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
int* counter =0;

void CaptureImage(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        if (xSemaphoreTake(g_frame_mutex, portMAX_DELAY) == pdTRUE) {
            // camera_fb_t *new_fb = esp_camera_fb_get();

            // if (new_fb) {
            //     // Shift previous frameB to frameA before overwriting frameB
            //     if (frameB) {
            //         if (frameA) esp_camera_fb_return(frameA);
            //         frameA = frameB;
            //         frameA_ready = true;
            //     }

            //     frameB = new_fb;
            //     frameB_ready = true;
            // }    
    fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        return;
    }

            xSemaphoreGive(g_frame_mutex);
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(CAPTURE_PERIOD_MS));
    }
}


void Interpolator(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        // if (frameA_ready && frameB_ready) {
        //     if (xSemaphoreTake(g_interp_mutex, portMAX_DELAY) == pdTRUE &&
        //         xSemaphoreTake(g_frame_mutex, portMAX_DELAY) == pdTRUE) {

        //         if (frameA && frameB && frameA->len == frameB->len) {
        //             interpFrame = (camera_fb_t *)malloc(sizeof(camera_fb_t));
        //             if (interpFrame) {
        //                 interpFrame->len = frameA->len;
        //                 interpFrame->width = frameA->width;
        //                 interpFrame->height = frameA->height;
        //                 interpFrame->format = frameA->format;
        //                 interpFrame->buf = (uint8_t *)malloc(frameA->len);

        //                 // Format: YUYV (Y0 U Y1 V)
        //                 // Average Y (0,2,4,...) from A and B
        //                 // Copy U/V chroma (1,3,5,...) directly from A
        //                 if (interpFrame->buf) {
        //                     for (size_t i = 0; i < frameA->len; i += 2) {
        //                         uint8_t yA = frameA->buf[i];
        //                         uint8_t yB = frameB->buf[i];
        //                         interpFrame->buf[i] = (yA + yB) >> 1;  // avg luminance
        //                         interpFrame->buf[i + 1] = frameA->buf[i + 1]; // copy chroma
        //                     }
        //                     interp_ready = true;
        //                 }
            //         }
            //     }

            //     xSemaphoreGive(g_frame_mutex);
            //     xSemaphoreGive(g_interp_mutex);

            //     // Flags reset for next pair
            //     frameA_ready = false;
            //     frameB_ready = false;
            // }
            //No interpolation here. Interpolation in python
            *counter++;
            printf("%d",*counter);
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(INTERPOLATE_PERIOD_MS));
}


void Transmitter(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        // Send latest frameA
        if (xSemaphoreTake(g_frame_mutex, portMAX_DELAY) == pdTRUE) {
        //     if (frameA) {
        //         Serial.write("*A*", 3);
        //         uint32_t len = frameA->len;
        //         Serial.write((uint8_t *)&len, 4);
        //         Serial.write(frameA->buf, frameA->len);
        //     }
        //     xSemaphoreGive(g_frame_mutex);
        // }

        // // Send interpolated frame
        // if (interp_ready && xSemaphoreTake(g_interp_mutex, portMAX_DELAY) == pdTRUE) {
        //     Serial.write("*I*", 3);
        //     uint32_t len = interpFrame->len;
        //     Serial.write((uint8_t *)&len, 4);
        //     Serial.write(interpFrame->buf, interpFrame->len);

        //     free(interpFrame->buf);
        //     free(interpFrame);
        //     interpFrame = NULL;
        //     interp_ready = false;
        //     xSemaphoreGive(g_interp_mutex);
        // }

        // // Send latest frameB
        // if (xSemaphoreTake(g_frame_mutex, portMAX_DELAY) == pdTRUE) {
        //     if (frameB) {
        //         Serial.write("*B*", 3);
        //         uint32_t len = frameB->len;
        //         Serial.write((uint8_t *)&len, 4);
        //         Serial.write(frameB->buf, frameB->len);
        //     }
    Serial.write("*S*", 3); 
    uint32_t len = fb->len;
    Serial.write((uint8_t *)&len, 4);
    Serial.write(fb->buf, fb->len);
    esp_camera_fb_return(fb);
            xSemaphoreGive(g_frame_mutex);
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TRANSMIT_PERIOD_MS));
    }
}
