#include <Arduino.h>
//#include <SoftwareSerial.h>
#include <HardwareSerial.h>
#include <Arduino_FreeRTOS.h>
#include <task.h>

#define EXP_FACTOR 0.2
float new_guess,old_guess;

float exp_avg(float time_quantum,float old_guess);

typedef struct {
  TaskHandle_t handle;  //task handle
  TickType_t absolute_deadline; //time handle (deadline)
} TaskTimingInfo;

//Tasks
void Task1(void* param);
void Task2(void* param);

TaskTimingInfo Task_1, Task_2;


void setup() {
  // xtask creations here
  Serial.begin(115200);
  Serial.println("Beginning tasks");
  xTaskCreate(Task1,"T1",100,NULL,1,&Task_1.handle);
  xTaskCreate(Task2,"T2",100,NULL,2,&Task_2.handle);
}

void loop() {
  // irrelevant now
}

//this result goes to the task's deadline
float exp_avg(float time_quantum,float old_guess){
  new_guess = EXP_FACTOR*time_quantum + (1-EXP_FACTOR)*old_guess;
  return new_guess;
}

void Task1(void* param){
  (void) param;
  Serial.println("HI");
  vTaskDelay(1000/portMAX_DELAY);//1 sec delay
  
}
void Task2(void* param){
  (void) param;
  Serial.println("yo");
  vTaskDelay(500/portMAX_DELAY); //0.5 sec delay
}