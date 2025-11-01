#include <Arduino.h>
#include "esp_camera.h"
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/timers.h>
#include "cam_tasks.h"
#include "task_config.h"

void Scheduler (void *pvParameters);
void CaptureImage (void *pvParameters);
void Interpolator (void *pvParameters);
void Transmitter (void *pvParameters);

EdfTaskInfo CaptureTask = {NULL, "CaptureImages", pdMS_TO_TICKS(CAPTURE_PERIOD_MS)};
EdfTaskInfo InterpolateTask = {NULL, "InterpolateImages", pdMS_TO_TICKS(INTERPOLATE_PERIOD_MS)};
EdfTaskInfo TransmitTask = {NULL, "TransmitImages", pdMS_TO_TICKS(TRANSMIT_PERIOD_MS)};
EdfTaskInfo SchedulerTask = {NULL, "SchedulerTask", pdMS_TO_TICKS(SCHEDULER_PERIOD_MS)};

EdfTaskInfo CAM_TASKLIST[NUM_MANAGED_TASKS] = {CaptureTask, InterpolateTask, TransmitTask}; // don't include scheduler here

// Initialize mutexes in setup or before starting tasks
SemaphoreHandle_t g_task_list_mutex;
SemaphoreHandle_t g_framebuf_mutex;
void init_mutexes() {
    g_task_list_mutex = xSemaphoreCreateMutex();
    g_framebuf_mutex = xSemaphoreCreateMutex();
    if (g_task_list_mutex == NULL || g_framebuf_mutex == NULL) {
        Serial.println("Error: Could not create mutexes!");
        while(1); // Halt if mutex creation fails
    }
}

void Scheduler(void *pvParameters) {
  const TickType_t xFrequency = pdMS_TO_TICKS(SCHEDULER_PERIOD_MS); //runs every 10 ms
  TickType_t xLastWakeTime = xTaskGetTickCount();

  for (;;) {
    if (xSemaphoreTake(g_task_list_mutex, portMAX_DELAY) == pdTRUE) {
    int size_TASKLIST = sizeof(CAM_TASKLIST)/sizeof(CAM_TASKLIST[0]);
      // Sort by the earliest deadline
      EdfTaskInfo temp_task;
      for (int i=0;i<size_TASKLIST;i++){
        temp_task=CAM_TASKLIST[i];
        for (int j=i+1;j<size_TASKLIST;j++){
            if (temp_task.deadline > CAM_TASKLIST[j].deadline){
                CAM_TASKLIST[i]=CAM_TASKLIST[j];
                CAM_TASKLIST[j]=temp_task;
                temp_task=CAM_TASKLIST[i];
            }
        }
      }
      // Earliest deadline gets highest priority
      for (int i = 0; i < size_TASKLIST; ++i) {
        UBaseType_t newPriority = PRIO_WORKER_HIGH - i;
        if (newPriority < PRIO_WORKER_BASE) {
          newPriority = PRIO_WORKER_BASE; //edge case prevention
        }
        // Setting the priority
        vTaskPrioritySet(CAM_TASKLIST[i].handle, newPriority);
      }
      xSemaphoreGive(g_task_list_mutex);
    }
  // Wait for the next period
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
}
}

camera_fb_t *fb = NULL; //global frame buffer pointer

void CaptureImage(void* pvParameters){
  TickType_t xlastWakeTime = xTaskGetTickCount();
    for(;;){        
        //printf("Capture image running");
        xSemaphoreGive(g_task_list_mutex);
        if (xSemaphoreTake(g_framebuf_mutex, portMAX_DELAY) == pdTRUE) {
            fb = esp_camera_fb_get();
            if (!fb) {
                //capture failed, release mutex and continue
                xSemaphoreGive(g_framebuf_mutex);
                continue;
            }
            xSemaphoreGive(g_framebuf_mutex);
        }
    vTaskDelayUntil(&xlastWakeTime, (TickType_t)pdMS_TO_TICKS(CAPTURE_PERIOD_MS));
    }
}

void Interpolator(void* pvParameters){
  TickType_t xlastWakeTime = xTaskGetTickCount();
    for(;;){        
        //printf("Interpolate image running");
    vTaskDelayUntil(&xlastWakeTime, (TickType_t)pdMS_TO_TICKS(INTERPOLATE_PERIOD_MS));
    }
}

void Transmitter(void* pvParameters){
  TickType_t xlastWakeTime = xTaskGetTickCount();
    for(;;){        
        //printf("Transmission of image running");
        xSemaphoreGive(g_task_list_mutex);

        // Protect frame buffer access
        if (xSemaphoreTake(g_framebuf_mutex, portMAX_DELAY) == pdTRUE) {
            if (fb) {
                Serial.write("*S*", 3); 
                uint32_t len = fb->len;
                Serial.write((uint8_t *)&len, 4);
                Serial.write(fb->buf, fb->len);
                esp_camera_fb_return(fb);
                fb = NULL; // Clear ptr
            }
            xSemaphoreGive(g_framebuf_mutex);
        }
    vTaskDelayUntil(&xlastWakeTime, (TickType_t)pdMS_TO_TICKS(TRANSMIT_PERIOD_MS));
    }
}
