#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/timers.h>

#define NUM_MANAGED_TASKS 3 //excluding scheduler
//PRIORITIES
#define PRIO_EDF_MANAGER (configMAX_PRIORITIES - 1) 
#define PRIO_WORKER_HIGH (configMAX_PRIORITIES - 2) 
#define PRIO_WORKER_BASE 3
//DEADLINES AND PERIODS
#define CAPTURE_PERIOD_MS 500 // 2fps
#define INTERPOLATE_PERIOD_MS 600 // 60 ps
#define TRANSMIT_PERIOD_MS 700 // 10 PS
#define SCHEDULER_PERIOD_MS 10 //10ms

void Scheduler (void *pvParameters);
void CaptureImage (void *pvParameters);
void Interpolator (void *pvParameters);
void Transmitter (void *pvParameters);


//pcb block struct
typedef struct {
    TaskHandle_t handle;
    const char* taskName;
    TickType_t   deadline; //deadline itself is period
}EdfTaskInfo;

EdfTaskInfo CaptureTask = {NULL,"CaptureImages",pdMS_TO_TICKS(CAPTURE_PERIOD_MS)};
EdfTaskInfo InterpolateTask = {NULL,"InterpolateImages",pdMS_TO_TICKS(INTERPOLATE_PERIOD_MS)};;
EdfTaskInfo TransmitTask = {NULL,"TransmitImages",pdMS_TO_TICKS(TRANSMIT_PERIOD_MS)};

EdfTaskInfo CAM_TASKLIST[NUM_MANAGED_TASKS]={CaptureTask,InterpolateTask,TransmitTask}; //dont include scheduler here

// A mutex to protect the list
SemaphoreHandle_t g_task_list_mutex;

void Scheduler(void *pvParameters) {
  const TickType_t xFrequency = pdMS_TO_TICKS(SCHEDULER_PERIOD_MS); //runs every 10 ms
  TickType_t xLastWakeTime = xTaskGetTickCount();

  for (;;) {
    if (xSemaphoreTake(g_task_list_mutex, portMAX_DELAY) == pdTRUE) {
      
    int size_TASKLIST = sizeof(CAM_TASKLIST)/sizeof(CAM_TASKLIST[0]);
      // Sort by the earliest deadline
    //   std::sort(TASKLIST.begin(), TASKLIST.end(), compareByDeadline);
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

void CaptureImage(void* pvParameters){
    //nothing
    //delay(1000);
    for(;;){
        camera_fb_t *fb = esp_camera_fb_get();
        if (fb) {
            xQueueSend(frameQueue, &fb, portMAX_DELAY);
        }
        vTaskDelay(pdMS_TO_TICKS(66)); // approx 15 FPS
    }
    printf("Capture image running");
    }
}

void Interpolator(void* pvParameters){
    //nothing
    //delay(1000);
    for(;;){
    printf("Interpolation running");
    }
}

void Transmitter(void* pvParameters){
    //nothing
    //delay(1000);
    camera_fb_t *fb = NULL;
    
    for(;;){
        if (xQueueReceive(interpQueue, &fb, portMAX_DELAY) == pdTRUE) {

            if (fb == NULL) {
                Serial.println("[Transmitter] Received null frame!");
                continue;
            }
            
            const char *startMarker = "*S*";
            Serial.write(startMarker, 3);

            uint32_t frame_len = fb->len;
            Serial.write((uint8_t *)&frame_len, sizeof(frame_len));

            size_t bytes_written = Serial.write(fb->buf, fb->len);
            if (bytes_written != fb_len) {
                Serial.printf("[Transmitter] Warning: Only %u/%u bytes written!\n", (unsigned int)bytes_written, (unsigned int)fb->len);
            }

            const char *endMarker = "*E*";
            Serial.write(endMarker, 3);

            esp_camera_fb_return(fb);
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }   
    printf("Transmission running");
}
