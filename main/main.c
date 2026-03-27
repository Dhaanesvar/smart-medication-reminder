#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Shared variable
int current_bpm = 0;

// ------------------------
// Task 1: Heartbeat Simulator
// ------------------------
void heartbeat_task(void *pvParameters)
{
    while (1)
    {
        // Generate random BPM (60 - 140)
        current_bpm = (rand() % 80) + 60;

        printf("\n💓 Simulated BPM: %d\n", current_bpm);

        vTaskDelay(pdMS_TO_TICKS(2000)); // every 2 sec
    }
}

// ------------------------
// Task 2: Health Logic
// ------------------------
void health_task(void *pvParameters)
{
    while (1)
    {
        if (current_bpm > 100)
        {
            printf("⚠ ALERT: High heart rate! Stay calm, take deep breath.\n");
        }
        else
        {
            printf("✅ Heart rate normal.\n");
        }

        vTaskDelay(pdMS_TO_TICKS(2500)); // every 2.5 sec
    }
}

// ------------------------
// Task 3: Medicine Reminder
// ------------------------
void medicine_task(void *pvParameters)
{
    while (1)
    {
        printf("💊 Reminder: Time to take your medicine!\n");

        vTaskDelay(pdMS_TO_TICKS(10000)); // every 10 sec
    }
}

// ------------------------
// Main Function
// ------------------------
void app_main(void)
{
    // Seed random generator
    srand(time(NULL));

    // Create tasks
    xTaskCreate(heartbeat_task, "Heartbeat Task", 2048, NULL, 2, NULL);
    xTaskCreate(health_task, "Health Task", 2048, NULL, 1, NULL);
    xTaskCreate(medicine_task, "Medicine Task", 2048, NULL, 1, NULL);
}