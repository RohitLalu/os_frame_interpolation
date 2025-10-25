#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <task.h>
#include <math.h>

// --- Task Configuration ---
#define NUM_MANAGED_TASKS 3

// Define our task periods in milliseconds
const int CAPTURE_PERIOD_MS = 33; // ~30fps
const int INTERPOLATE_PERIOD_MS = 16; // ~60fps
const int DISPLAY_PERIOD_MS = 100; // 10fps

// EMA (Exponential Moving Average) smoothing factor
const float EMA_ALPHA = 0.3;

// --- Task Control Block (TCB) Struct ---
// This is our custom struct to manage task state for the scheduler
typedef struct {
    TaskHandle_t handle;
    const char* taskName;
    TickType_t deadline; // Absolute deadline in ticks
    TickType_t period; // Period of the task in ticks
    
    // For EMA calculation
    float emaExecTime; 
    TickType_t estimatedExecTime;
} TaskControlBlock_t;

// --- Global Array of Our Tasks ---
TaskControlBlock_t managedTasks[NUM_MANAGED_TASKS];

// --- Task Function Prototypes ---
void taskScheduler(void *pvParameters);
void taskCapture(void *pvParameters);
void taskInterpolate(void *pvParameters);
void taskDisplay(void *pvParameters);

// --- TaskControlBlock_t pointer for each task ---
// We pass this to the task on creation so it can update its own info
TaskControlBlock_t tcbCapture;
TaskControlBlock_t tcbInterpolate;
TaskControlBlock_t tcbDisplay;


// --- Arduino setup() ---
// Runs once at the beginning
void setup() {
    Serial.begin(115200);
    while (!Serial) {
        ; // Wait for serial port to connect
    }
    Serial.println("--- Custom EDF Scheduler (Proof of Concept) ---");
    Serial.println("Monitoring for deadline misses...");

    // Initialize our Task Control Blocks
    tcbCapture = {NULL, "Capture", 0, pdMS_TO_TICKS(CAPTURE_PERIOD_MS), 0.0, 0};
    tcbInterpolate = {NULL, "Interpolate", 0, pdMS_TO_TICKS(INTERPOLATE_PERIOD_MS), 0.0, 0};
    tcbDisplay = {NULL, "Display", 0, pdMS_TO_TICKS(DISPLAY_PERIOD_MS), 0.0, 0};

    // Add them to the global array for the scheduler
    managedTasks[0] = tcbCapture;
    managedTasks[1] = tcbInterpolate;
    managedTasks[2] = tcbDisplay;
    
    // Create the worker tasks first (with low priority)
    xTaskCreate(
        taskCapture,
        tcbCapture.taskName,
        configMINIMAL_STACK_SIZE,
        (void*)&managedTasks[0], // Pass a pointer to its own TCB
        5, // Low initial priority
        &managedTasks[0].handle // Store the handle in its TCB
    );

    xTaskCreate(
        taskInterpolate,
        tcbInterpolate.taskName,
        configMINIMAL_STACK_SIZE,
        (void*)&managedTasks[1],
        5,
        &managedTasks[1].handle
    );

    xTaskCreate(
        taskDisplay,
        tcbDisplay.taskName,
        configMINIMAL_STACK_SIZE,
        (void*)&managedTasks[2],
        5,
        &managedTasks[2].handle
    );

    // Create the scheduler task with the HIGHEST priority
    xTaskCreate(
        taskScheduler,
        "Scheduler",
        configMINIMAL_STACK_SIZE,
        NULL,
        configMAX_PRIORITIES - 1, // Highest priority
        NULL
    );
    
    // Note: vTaskStartScheduler() is called implicitly by the Arduino FreeRTOS port
}


// --- Main Scheduler Task (EDF Logic) ---
void taskScheduler(void *pvParameters) {
    (void) pvParameters;
    
    const TickType_t xPeriod = pdMS_TO_TICKS(10); // Run scheduler every 10ms
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        TickType_t currentTime = xTaskGetTickCount();
        
        // --- 1. Sort Tasks by Deadline (Bubble Sort) ---
        // A simple sort is fine for 3 tasks.
        for (int i = 0; i < NUM_MANAGED_TASKS - 1; i++) {
            for (int j = 0; j < NUM_MANAGED_TASKS - i - 1; j++) {
                // Compare absolute deadlines
                if (managedTasks[j].deadline > managedTasks[j + 1].deadline) {
                    // Swap
                    TaskControlBlock_t temp = managedTasks[j];
                    managedTasks[j] = managedTasks[j + 1];
                    managedTasks[j + 1] = temp;
                }
            }
        }

        // --- 2. Assign Priorities (EDF) & Check for Misses ---
        Serial.println("--- SCHEDULER TICK ---");
        int basePriority = configMAX_PRIORITIES - 2; // Highest allowable worker priority
        
        for (int i = 0; i < NUM_MANAGED_TASKS; i++) {
            // Check for deadline miss
            if (managedTasks[i].deadline < currentTime) {
                Serial.print("!!! DEADLINE MISS: ");
                Serial.print(managedTasks[i].taskName);
                Serial.print(" (Missed by: ");
                Serial.print(currentTime - managedTasks[i].deadline);
                Serial.println(" ticks) !!!");
            }
            
            // Assign priority: task[0] (earliest deadline) gets highest priority
            if (managedTasks[i].handle != NULL) {
                vTaskPrioritySet(managedTasks[i].handle, basePriority - i);
                
                // Log to serial
                Serial.print(managedTasks[i].taskName);
                Serial.print(" | Prio: ");
                Serial.print(basePriority - i);
                Serial.print(" | Deadline: ");
                Serial.print(managedTasks[i].deadline);
                Serial.print(" | Est. Exec: ");
                Serial.println(managedTasks[i].estimatedExecTime);
            }
        }
        Serial.println("------------------------");

        // Wait for the next scheduler cycle
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

// --- Worker Task: Simulate Image Capture ---
void taskCapture(void *pvParameters) {
    TaskControlBlock_t* myTCB = (TaskControlBlock_t*)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        TickType_t startTime = xTaskGetTickCount();

        // --- 1. SIMULATE WORK ---
        // Simulate a task that takes ~10-15ms
        long iterations = 50000 + (rand() % 10000); // Variable workload
        for (volatile long i = 0; i < iterations; i++) {}
        
        TickType_t endTime = xTaskGetTickCount();
        TickType_t actualExecTime = endTime - startTime;

        // --- 2. UPDATE TCB (Critical Section) ---
        taskENTER_CRITICAL();
        // Update EMA
        myTCB->emaExecTime = (EMA_ALPHA * actualExecTime) + ((1.0 - EMA_ALPHA) * myTCB->emaExecTime);
        myTCB->estimatedExecTime = (TickType_t)ceil(myTCB->emaExecTime);
        // Set next absolute deadline
        myTCB->deadline = xLastWakeTime + myTCB->period;
        taskEXIT_CRITICAL();

        // --- 3. WAIT FOR NEXT PERIOD ---
        vTaskDelayUntil(&xLastWakeTime, myTCB->period);
    }
}

// --- Worker Task: Simulate Interpolation ---
void taskInterpolate(void *pvParameters) {
    TaskControlBlock_t* myTCB = (TaskControlBlock_t*)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        TickType_t startTime = xTaskGetTickCount();
        
        // --- 1. SIMULATE WORK ---
        // Simulate a shorter, more frequent task (~5-8ms)
        long iterations = 20000 + (rand() % 5000); 
        for (volatile long i = 0; i < iterations; i++) {}
        
        TickType_t endTime = xTaskGetTickCount();
        TickType_t actualExecTime = endTime - startTime;

        // --- 2. UPDATE TCB (Critical Section) ---
        taskENTER_CRITICAL();
        myTCB->emaExecTime = (EMA_ALPHA * actualExecTime) + ((1.0 - EMA_ALPHA) * myTCB->emaExecTime);
        myTCB->estimatedExecTime = (TickType_t)ceil(myTCB->emaExecTime);
        myTCB->deadline = xLastWakeTime + myTCB->period;
        taskEXIT_CRITICAL();
        
        // --- 3. WAIT FOR NEXT PERIOD ---
        vTaskDelayUntil(&xLastWakeTime, myTCB->period);
    }
}

// --- Worker Task: Simulate Display ---
void taskDisplay(void *pvParameters) {
    TaskControlBlock_t* myTCB = (TaskControlBlock_t*)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        TickType_t startTime = xTaskGetTickCount();
        
        // --- 1. SIMULATE WORK ---
        // Simulate a slow I/O task (~20ms)
        long iterations = 70000 + (rand() % 10000); 
        for (volatile long i = 0; i < iterations; i++) {}
        
        TickType_t endTime = xTaskGetTickCount();
        TickType_t actualExecTime = endTime - startTime;
        
        // --- 2. UPDATE TCB (Critical Section) ---
        taskENTER_CRITICAL();
        myTCB->emaExecTime = (EMA_ALPHA * actualExecTime) + ((1.0 - EMA_ALPHA) * myTCB->emaExecTime);
        myTCB->estimatedExecTime = (TickType_t)ceil(myTCB->emaExecTime);
        myTCB->deadline = xLastWakeTime + myTCB->period;
        taskEXIT_CRITICAL();
        
        // --- 3. WAIT FOR NEXT PERIOD ---
        vTaskDelayUntil(&xLastWakeTime, myTCB->period);
    }
}

// Arduino loop()
// Not used when FreeRTOS is running.
void loop() {}
