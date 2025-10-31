// #include <Arduino.h>
// #include "esp_camera.h"

// // Pre-define the camera model (for the pins file)
// #define CAMERA_MODEL_AI_THINKER
// #include "camera_pins.h"

// // --- Function to initialize the camera ---
// void init_camera() {
//   camera_config_t config;
//   config.ledc_channel = LEDC_CHANNEL_0;
//   config.ledc_timer = LEDC_TIMER_0;
//   config.pin_d0 = Y2_GPIO_NUM;
//   config.pin_d1 = Y3_GPIO_NUM;
//   config.pin_d2 = Y4_GPIO_NUM;
//   config.pin_d3 = Y5_GPIO_NUM;
//   config.pin_d4 = Y6_GPIO_NUM;
//   config.pin_d5 = Y7_GPIO_NUM;
//   config.pin_d6 = Y8_GPIO_NUM;
//   config.pin_d7 = Y9_GPIO_NUM;
//   config.pin_xclk = XCLK_GPIO_NUM;
//   config.pin_pclk = PCLK_GPIO_NUM;
//   config.pin_vsync = VSYNC_GPIO_NUM;
//   config.pin_href = HREF_GPIO_NUM;
//   config.pin_sccb_sda = SIOD_GPIO_NUM;
//   config.pin_sccb_scl = SIOC_GPIO_NUM;
//   config.pin_pwdn = PWDN_GPIO_NUM;
//   config.pin_reset = RESET_GPIO_NUM;
//   config.xclk_freq_hz = 20000000;
//   config.pixel_format = PIXFORMAT_JPEG; // Output JPEG
  
//   // QVGA (320x240) is fast and small
//   config.frame_size = FRAMESIZE_QVGA;
//   config.jpeg_quality = 12; // 10-63 (lower is better quality)
//   config.fb_count = 1;

//   esp_err_t err = esp_camera_init(&config);
//   if (err != ESP_OK) {
//     Serial.printf("Camera init failed with error 0x%x", err);
//     ESP.restart();
//   }
// }

// // --- setup() runs once on boot ---
// void setup() {
//   // CRITICAL: Start Serial at a VERY high baud rate
//   Serial.begin(921600);
//   Serial.println("Serial-to-JPEG Streamer Initializing...");
  
//   // Initialize the camera
//   init_camera();
  
//   Serial.println("Camera OK. Starting stream.");
// }

// // --- loop() runs repeatedly ---
// void loop() {
//   camera_fb_t *fb = NULL;
  
//   // 1. Capture a frame
//   fb = esp_camera_fb_get();
//   if (!fb) {
//     Serial.println("Camera capture failed");
//     return;
//   }

//   // 2. Send the Start Marker ("*S*")
//   Serial.write("*S*", 3); 

//   // 3. Send the 4-byte frame length
//   uint32_t len = fb->len;
//   Serial.write((uint8_t *)&len, 4);

//   // 4. Send the actual JPEG data
//   Serial.write(fb->buf, fb->len);

//   // 5. CRITICAL: Return the frame buffer to be reused
//   esp_camera_fb_return(fb);

//   // 6. Wait for 333ms to get 3 FPS
//   delay(333);
// }