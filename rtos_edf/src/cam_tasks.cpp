#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/timers.h>

//#include <vector>
//#include <algorithm>

#define NUM_MANAGED_TASKS 3 //excluding scheduler
#define PRIO_EDF_MANAGER (configMAX_PRIORITIES - 2) 
#define PRIO_WORKER_HIGH (configMAX_PRIORITIES - 4) 
#define PRIO_WORKER_BASE 2
#define EMA_ALPHA 0.3


// Define our task periods in milliseconds
const int CAPTURE_PERIOD_MS = 500; // 2fps
const int INTERPOLATE_PERIOD_MS = 600; // 60 ps
const int TRANSMIT_PERIOD_MS = 100; // 10 PS
const int SCHEDULER_PERIOD_MS = 10;

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

EdfTaskInfo g_edfTaskList[NUM_MANAGED_TASKS]; //dont include scheduler here
g_edfTaskList[0]=CaptureTask;
g_edfTaskList[1]=InterpolateTask;
g_edfTaskList[2]=TransmitTask;

// A mutex to protect the list
SemaphoreHandle_t g_task_list_mutex;

void Scheduler(void *pvParameters) {
  Serial.println("EDF Manager Task started.");
  const TickType_t xFrequency = pdMS_TO_TICKS(SCHEDULER_PERIOD_MS); //runs every 10 ms
  TickType_t xLastWakeTime = xTaskGetTickCount();

  for (;;) {
    if (xSemaphoreTake(g_task_list_mutex, portMAX_DELAY) == pdTRUE) {
      
      // Sort by the earliest deadline
    //   std::sort(g_edfTaskList.begin(), g_edfTaskList.end(), compareByDeadline);
      EdfTaskInfo temp_task;
      for (int i=0;i<g_edfTaskList.size();i++){
        temp_task=g_edfTaskList[i];
        for (int j=i+1;j<g_edfTaskList.size();j++){
            if (temp_task.deadline > g_edfTaskList[j].deadline){
                g_edfTaskList[i]=g_edfTaskList[j];
                g_edfTaskList[j]=temp_task;
                temp_task=g_edfTaskList[i];
            }
        }
      }

      // Earliest deadline gets highest priority
      for (int i = 0; i < g_edfTaskList.size(); ++i) {
        UBaseType_t newPriority = PRIO_WORKER_HIGH - i;
        if (newPriority < PRIO_WORKER_BASE) {
          newPriority = PRIO_WORKER_BASE; //edge case prevention
        }
        // Setting the priority
        vTaskPrioritySet(g_edfTaskList[i].handle, newPriority);
      }
      xSemaphoreGive(g_task_list_mutex);
    }
  }
  // Wait for the next period
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
}

void CaptureImage(void* pvParameters){
    //nothing
    delay(1000);
}

void Interpolator(void* pvParameters){
    //nothing
    delay(1000);
}

void Transmitter(void* pvParameters){
    //nothing
    delay(1000);
}

//not here
//put it in void setup
// xTaskCreate(
//         CaptureImage,
//         CaptureTask.taskName,
//         configMINIMAL_STACK_SIZE,
//         (void*)&g_edfTaskList[0], 
//         PRIO_WORKER_BASE,
//         &g_edfTaskList[0].handle 
//     );