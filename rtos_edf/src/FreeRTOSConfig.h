// In FreeRTOSConfig.h
#define configMAX_PRIORITIES        ( 7 ) // 7 levels 
#define configUSE_MUTEXES           ( 1 )
#define configUSE_TICK_HOOK         ( 0 )
#define INCLUDE_vTaskPrioritySet    ( 1 ) 
#define INCLUDE_vTaskSuspend        ( 1 ) // Useful for task management
#define configTICK_RATE_HZ          (20000000)
#define configMINIMAL_STACK_SIZE    (10)