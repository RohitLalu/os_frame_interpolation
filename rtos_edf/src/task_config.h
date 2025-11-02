#ifndef TASK_CONFIG_H
#define TASK_CONFIG_H

#include "FreeRTOSConfig.h" // For configMAX_PRIORITIES

// Number of managed tasks
#define NUM_MANAGED_TASKS 3 //excluding scheduler

// Task priorities
#define PRIO_EDF_MANAGER (configMAX_PRIORITIES - 1) 
#define PRIO_WORKER_HIGH (configMAX_PRIORITIES - 2) 
#define PRIO_WORKER_BASE 3

// Task periods and deadlines (in milliseconds)
#define CAPTURE_PERIOD_MS 333    // 3 fps
#define INTERPOLATE_PERIOD_MS 170 // 10 ps
#define TRANSMIT_PERIOD_MS 100   // 1 fps
#define SCHEDULER_PERIOD_MS 10   // 10ms

#endif // TASK_CONFIG_H