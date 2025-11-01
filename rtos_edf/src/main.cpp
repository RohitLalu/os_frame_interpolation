#include <Arduino.h>
#include "cam_tasks.h"
#include "task_config.h"

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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_YUV422; // Output YUV422
  
  // QVGA (320x240) is fast and small
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12; // 10-63 (lower is better quality)
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    //Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  }
}

void blink_work(){
  digitalWrite(4,HIGH); 
  delay(1000);
  digitalWrite(4,LOW);
  delay(1000);
}

void blink_doesnt_work(){
  digitalWrite(4,HIGH); 
  delay(3000);
  digitalWrite(4,LOW);
  delay(3000);
  digitalWrite(4,HIGH);
}

//Running stuff
void setup() {
  Serial.begin(921600);
  init_camera();
  pinMode(4,OUTPUT);//indicates serial works and camera is initialised
  blink_work();
  blink_work();
  init_mutexes();


  //start with tasks here
xTaskCreate(
        CaptureImage,
        CaptureTask.taskName,
        configMINIMAL_STACK_SIZE,
        (void*)&CAM_TASKLIST[0], 
        PRIO_WORKER_BASE,
        &CAM_TASKLIST[0].handle 
    );

  xTaskCreate(
        Interpolator,
        InterpolateTask.taskName,
        configMINIMAL_STACK_SIZE,
        (void*)&CAM_TASKLIST[1], 
        PRIO_WORKER_BASE,
        &CAM_TASKLIST[1].handle 
    );
  xTaskCreate(
        Transmitter,
        TransmitTask.taskName,
        configMINIMAL_STACK_SIZE,
        (void*)&CAM_TASKLIST[2], 
        PRIO_WORKER_BASE,
        &CAM_TASKLIST[2].handle 
    );

  xTaskCreate(
        Scheduler,
        "SchedulerTasks",
        configMINIMAL_STACK_SIZE,
        NULL,PRIO_EDF_MANAGER,
        NULL
    );
}

void loop(){
  //nothing here
}