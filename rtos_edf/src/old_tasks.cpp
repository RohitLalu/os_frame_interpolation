// void CaptureImage(void *pvParameters) {
//     TickType_t xLastWakeTime = xTaskGetTickCount();

//     for (;;) {
//         // if (xSemaphoreTake(g_frame_mutex, portMAX_DELAY) == pdTRUE) {
//             // camera_fb_t *new_fb = esp_camera_fb_get();

//             // if (new_fb) {
//             //     // Shift previous frameB to frameA before overwriting frameB
//             //     if (frameB) {
//             //         if (frameA) esp_camera_fb_return(frameA);
//             //         frameA = frameB;
//             //         frameA_ready = true;
//             //     }

//             //     frameB = new_fb;
//             //     frameB_ready = true;
//             // }    
//     fb = esp_camera_fb_get();
//     if (!fb) {
//         Serial.println("Camera capture failed");
//         return;
//     }

//         //     xSemaphoreGive(g_frame_mutex);
//         // }

//         vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(CAPTURE_PERIOD_MS));
//     }
// }


// void Interpolator(void *pvParameters) {
//     TickType_t xLastWakeTime = xTaskGetTickCount();

//     for (;;) {
//         // if (frameA_ready && frameB_ready) {
//         //     if (xSemaphoreTake(g_interp_mutex, portMAX_DELAY) == pdTRUE &&
//         //         xSemaphoreTake(g_frame_mutex, portMAX_DELAY) == pdTRUE) {

//         //         if (frameA && frameB && frameA->len == frameB->len) {
//         //             interpFrame = (camera_fb_t *)malloc(sizeof(camera_fb_t));
//         //             if (interpFrame) {
//         //                 interpFrame->len = frameA->len;
//         //                 interpFrame->width = frameA->width;
//         //                 interpFrame->height = frameA->height;
//         //                 interpFrame->format = frameA->format;
//         //                 interpFrame->buf = (uint8_t *)malloc(frameA->len);

//         //                 // Format: YUYV (Y0 U Y1 V)
//         //                 // Average Y (0,2,4,...) from A and B
//         //                 // Copy U/V chroma (1,3,5,...) directly from A
//         //                 if (interpFrame->buf) {
//         //                     for (size_t i = 0; i < frameA->len; i += 2) {
//         //                         uint8_t yA = frameA->buf[i];
//         //                         uint8_t yB = frameB->buf[i];
//         //                         interpFrame->buf[i] = (yA + yB) >> 1;  // avg luminance
//         //                         interpFrame->buf[i + 1] = frameA->buf[i + 1]; // copy chroma
//         //                     }
//         //                     interp_ready = true;
//         //                 }
//             //         }
//             //     }

//             //     xSemaphoreGive(g_frame_mutex);
//             //     xSemaphoreGive(g_interp_mutex);

//             //     // Flags reset for next pair
//             //     frameA_ready = false;
//             //     frameB_ready = false;
//             // }
//             //No interpolation here. Interpolation in python
//             counter++;
//             digitalWrite(33, counter%2);
//             //printf("Counter: %d\n", *counter);
//         }

//         vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(INTERPOLATE_PERIOD_MS));
// }


// void Transmitter(void *pvParameters) {
//     TickType_t xLastWakeTime = xTaskGetTickCount();

//     for (;;) {
//         // Send latest frameA
//         // if (xSemaphoreTake(g_frame_mutex, portMAX_DELAY) == pdTRUE) {
//         //     if (frameA) {
//         //         Serial.write("*A*", 3);
//         //         uint32_t len = frameA->len;
//         //         Serial.write((uint8_t *)&len, 4);
//         //         Serial.write(frameA->buf, frameA->len);
//         //     }
//         //     xSemaphoreGive(g_frame_mutex);
//         // }

//         // // Send interpolated frame
//         // if (interp_ready && xSemaphoreTake(g_interp_mutex, portMAX_DELAY) == pdTRUE) {
//         //     Serial.write("*I*", 3);
//         //     uint32_t len = interpFrame->len;
//         //     Serial.write((uint8_t *)&len, 4);
//         //     Serial.write(interpFrame->buf, interpFrame->len);

//         //     free(interpFrame->buf);
//         //     free(interpFrame);
//         //     interpFrame = NULL;
//         //     interp_ready = false;
//         //     xSemaphoreGive(g_interp_mutex);
//         // }

//         // // Send latest frameB
//         // if (xSemaphoreTake(g_frame_mutex, portMAX_DELAY) == pdTRUE) {
//         //     if (frameB) {
//         //         Serial.write("*B*", 3);
//         //         uint32_t len = frameB->len;
//         //         Serial.write((uint8_t *)&len, 4);
//         //         Serial.write(frameB->buf, frameB->len);
//         //     }
//     Serial.write("*S*", 3); 
//     uint32_t len = fb->len;
//     Serial.write((uint8_t *)&len, 4);
//     Serial.write(fb->buf, fb->len);
//     esp_camera_fb_return(fb);
//         //     xSemaphoreGive(g_frame_mutex);
//         // }

//     vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TRANSMIT_PERIOD_MS));
//     }
// }