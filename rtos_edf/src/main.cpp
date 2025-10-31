// #include <Arduino.h>
// #include "esp_camera.h"
// #include <freertos/FreeRTOS.h>
// #include <freertos/task.h>
// #include <freertos/queue.h>
// #include <math.h>

// // --- Task Configuration ---
// #define NUM_MANAGED_TASKS 3

// // Define our task periods in milliseconds
// const int CAPTURE_PERIOD_MS = 500; // ~2fps
// const int INTERPOLATE_PERIOD_MS = 16; // ~60fps
// const int DISPLAY_PERIOD_MS = 100; // 10fps

// // EMA (Exponential Moving Average) smoothing factor
// const float EMA_ALPHA = 0.3;

// // --- Task Control Block (TCB) Struct ---
// // This is our custom struct to manage task state for the scheduler
// typedef struct {
//     TaskHandle_t handle;
//     const char* taskName;
//     TickType_t deadline; // Absolute deadline in ticks
//     TickType_t period; // Period of the task in ticks: always constant
    
//     // For EMA calculation
//     float emaExecTime; 
//     TickType_t estimatedExecTime;
// } TaskControlBlock_t;

// // --- Global Array of Our Tasks ---
// TaskControlBlock_t managedTasks[NUM_MANAGED_TASKS];

// // --- Task Function Prototypes ---
// void taskScheduler(void *pvParameters);
// void taskCapture(void *pvParameters);
// void taskInterpolate(void *pvParameters);
// void taskDisplay(void *pvParameters);

// // --- TaskControlBlock_t pointer for each task ---
// // We pass this to the task on creation so it can update its own info
// TaskControlBlock_t tcbCapture;
// TaskControlBlock_t tcbInterpolate;
// TaskControlBlock_t tcbDisplay;


// // --- Arduino setup() ---
// // Runs once at the beginning
// void setup() {
//     Serial.begin(115200);
//     while (!Serial) {
//         ; // Wait for serial port to connect
//     }
//     Serial.println("--- Custom EDF Scheduler (Proof of Concept) ---");
//     Serial.println("Monitoring for deadline misses...");

//     // Initialize our Task Control Blocks
//     tcbCapture = {NULL, "Capture", 0, pdMS_TO_TICKS(CAPTURE_PERIOD_MS), 0.0, 0};
//     tcbInterpolate = {NULL, "Interpolate", 0, pdMS_TO_TICKS(INTERPOLATE_PERIOD_MS), 0.0, 0};
//     tcbDisplay = {NULL, "Display", 0, pdMS_TO_TICKS(DISPLAY_PERIOD_MS), 0.0, 0};

//     // Add them to the global array for the scheduler
//     managedTasks[0] = tcbCapture;
//     managedTasks[1] = tcbInterpolate;
//     managedTasks[2] = tcbDisplay;
    
//     // Create the worker tasks first (with low priority)
//     xTaskCreate(
//         taskCapture,
//         tcbCapture.taskName,
//         configMINIMAL_STACK_SIZE,
//         (void*)&managedTasks[0], // Pass a pointer to its own TCB
//         5, // Low initial priority
//         &managedTasks[0].handle // Store the handle in its TCB
//     );

//     xTaskCreate(
//         taskInterpolate,
//         tcbInterpolate.taskName,
//         configMINIMAL_STACK_SIZE,
//         (void*)&managedTasks[1],
//         5,
//         &managedTasks[1].handle
//     );

//     xTaskCreate(
//         taskDisplay,
//         tcbDisplay.taskName,
//         configMINIMAL_STACK_SIZE,
//         (void*)&managedTasks[2],
//         5,
//         &managedTasks[2].handle
//     );

//     // Create the scheduler task with the HIGHEST priority
//     xTaskCreate(
//         taskScheduler,
//         "Scheduler",
//         configMINIMAL_STACK_SIZE,
//         NULL,
//         configMAX_PRIORITIES - 1, // Highest priority
//         NULL
//     );
    
//     // Note: vTaskStartScheduler() is called implicitly by the Arduino FreeRTOS port
// }


// // --- Main Scheduler Task (EDF Logic) ---
// void taskScheduler(void *pvParameters) {
//     (void) pvParameters;
    
//     const TickType_t xPeriod = pdMS_TO_TICKS(10); // Run scheduler every 10ms
//     TickType_t xLastWakeTime = xTaskGetTickCount();

//     for (;;) {
//         TickType_t currentTime = xTaskGetTickCount();
        
//         // --- 1. Sort Tasks by Deadline (Bubble Sort) ---
//         // A simple sort is fine for 3 tasks.
//         for (int i = 0; i < NUM_MANAGED_TASKS - 1; i++) {
//             for (int j = 0; j < NUM_MANAGED_TASKS - i - 1; j++) {
//                 // Compare absolute deadlines
//                 if (managedTasks[j].deadline > managedTasks[j + 1].deadline) {
//                     // Swap
//                     TaskControlBlock_t temp = managedTasks[j];
//                     managedTasks[j] = managedTasks[j + 1];
//                     managedTasks[j + 1] = temp;
//                 }
//             }
//         }

//         // --- 2. Assign Priorities (EDF) & Check for Misses ---
//         Serial.println("--- SCHEDULER TICK ---");
//         int basePriority = configMAX_PRIORITIES - 2; // Highest allowable worker priority
        
//         for (int i = 0; i < NUM_MANAGED_TASKS; i++) {
//             // Check for deadline miss
//             if (managedTasks[i].deadline < currentTime) {
//                 Serial.print("!!! DEADLINE MISS: ");
//                 Serial.print(managedTasks[i].taskName);
//                 Serial.print(" (Missed by: ");
//                 Serial.print(currentTime - managedTasks[i].deadline);
//                 Serial.println(" ticks) !!!");
//             }
            
//             // Assign priority: task[0] (earliest deadline) gets highest priority
//             if (managedTasks[i].handle != NULL) {
//                 vTaskPrioritySet(managedTasks[i].handle, basePriority - i);
                
//                 // Log to serial
//                 Serial.print(managedTasks[i].taskName);
//                 Serial.print(" | Prio: ");
//                 Serial.print(basePriority - i);
//                 Serial.print(" | Deadline: ");
//                 Serial.print(managedTasks[i].deadline);
//                 Serial.print(" | Est. Exec: ");
//                 Serial.println(managedTasks[i].estimatedExecTime);
//             }
//         }
//         Serial.println("------------------------");

//         // Wait for the next scheduler cycle
//         vTaskDelayUntil(&xLastWakeTime, xPeriod);
//     }
// }

// void loop() {
//     //nothing here
// }

#include <Arduino.h>
#include "esp_camera.h"

// Pre-define the camera model (for the pins file)
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// --- Function to initialize the camera ---
void init_camera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; // Output JPEG
  
  // QVGA (320x240) is fast and small
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12; // 10-63 (lower is better quality)
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  }
}

// --- setup() runs once on boot ---
void setup() {
  // CRITICAL: Start Serial at a VERY high baud rate
  Serial.begin(921600);
  Serial.println("Serial-to-JPEG Streamer Initializing...");
  
  // Initialize the camera
  init_camera();
  
  Serial.println("Camera OK. Starting stream.");
}

// --- loop() runs repeatedly ---
void loop() {
  camera_fb_t *fb = NULL;
  
  // 1. Capture a frame
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  // 2. Send the Start Marker ("*S*")
  Serial.write("*S*", 3); 

  // 3. Send the 4-byte frame length
  uint32_t len = fb->len;
  Serial.write((uint8_t *)&len, 4);

  // 4. Send the actual JPEG data
  Serial.write(fb->buf, fb->len);

  // 5. CRITICAL: Return the frame buffer to be reused
  esp_camera_fb_return(fb);

  // 6. Wait for 333ms to get 3 FPS
  delay(333);
}