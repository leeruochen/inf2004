/* Corrected sensor pipeline. Each consumer owns its state. See README.md. */
#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "message_buffer.h"

#define RING_LEN 10u
#define SAMPLE_PERIOD_MS 100u

static MessageBufferHandle_t avg_input, mean_input;
static QueueHandle_t print_q;
static TaskHandle_t sensor_handle, avg_handle, mean_handle, print_handle;
typedef struct { const char *name; float value; } print_msg_t;

void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task; (void)name;
    taskDISABLE_INTERRUPTS();
    for (;;) {}
}

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    for (;;) {}
}

static void say(const char *name, float value)
{
    print_msg_t message = {name, value};
    /* Task context only. Backpressure preserves messages if printing slows. */
    BaseType_t result = xQueueSend(print_q, &message, portMAX_DELAY);
    configASSERT(result == pdPASS);
}

static void sensor_task(void *unused)
{
    (void)unused;
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);
    TickType_t next = xTaskGetTickCount();
    for (;;) {
        float volts = adc_read() * 3.3f / 4096.0f;
        int16_t sample = (int16_t)((27.0f - (volts - 0.706f) / 0.001721f) * 10.0f);
        size_t sent = xMessageBufferSend(avg_input, &sample, sizeof sample, 0);
        configASSERT(sent == sizeof sample);
        sent = xMessageBufferSend(mean_input, &sample, sizeof sample, 0);
        configASSERT(sent == sizeof sample);
        /* If the pipeline cannot sustain this rate, stop in the assertion;
         * do not silently claim complete, periodic acquisition. */
        vTaskDelayUntil(&next, pdMS_TO_TICKS(SAMPLE_PERIOD_MS));
    }
}

static void avg_task(void *unused)
{
    (void)unused;
    int16_t ring[RING_LEN] = {0};
    unsigned head = 0, fill = 0;
    int32_t sum = 0;
    for (;;) {
        int16_t sample;
        size_t received = xMessageBufferReceive(avg_input, &sample, sizeof sample, portMAX_DELAY);
        configASSERT(received == sizeof sample);
        sum -= ring[head];
        ring[head] = sample;
        sum += sample;
        head = (head + 1) % RING_LEN;
        if (fill < RING_LEN) fill++;
        say("moving average", (float)sum / (float)fill / 10.0f);
    }
}

static void mean_task(void *unused)
{
    (void)unused;
    int64_t sum = 0;
    uint64_t count = 0;
    for (;;) {
        int16_t sample;
        size_t received = xMessageBufferReceive(mean_input, &sample, sizeof sample, portMAX_DELAY);
        configASSERT(received == sizeof sample);
        configASSERT(count < UINT64_MAX);
        configASSERT(sample <= 0 || sum <= INT64_MAX - sample);
        configASSERT(sample >= 0 || sum >= INT64_MIN - sample);
        sum += sample;
        count++;
        say("running mean", (float)((double)sum / (double)count / 10.0));
    }
}

static void print_task(void *unused)
{
    (void)unused;
    TickType_t last_health = xTaskGetTickCount();
    for (;;) {
        print_msg_t message;
        if (xQueueReceive(print_q, &message, pdMS_TO_TICKS(100)) == pdPASS)
            printf("%s: %.1f C\n", message.name, (double)message.value);
        TickType_t now = xTaskGetTickCount();
        if ((TickType_t)(now - last_health) >= pdMS_TO_TICKS(1000)) {
            printf("stack minimum free (words): sensor=%lu avg=%lu mean=%lu print=%lu\n",
                (unsigned long)uxTaskGetStackHighWaterMark(sensor_handle),
                (unsigned long)uxTaskGetStackHighWaterMark(avg_handle),
                (unsigned long)uxTaskGetStackHighWaterMark(mean_handle),
                (unsigned long)uxTaskGetStackHighWaterMark(print_handle));
            last_health = now;
        }
    }
}

int main(void)
{
    stdio_init_all();
    sleep_ms(3000);
    avg_input = xMessageBufferCreate(256);
    mean_input = xMessageBufferCreate(256);
    print_q = xQueueCreate(8, sizeof(print_msg_t));
    configASSERT(avg_input && mean_input && print_q);
    BaseType_t result;
    result = xTaskCreate(sensor_task, "sensor", 512, NULL, 5, &sensor_handle);
    configASSERT(result == pdPASS);
    result = xTaskCreate(avg_task, "avg", 512, NULL, 3, &avg_handle);
    configASSERT(result == pdPASS);
    result = xTaskCreate(mean_task, "mean", 512, NULL, 3, &mean_handle);
    configASSERT(result == pdPASS);
    result = xTaskCreate(print_task, "print", 1024, NULL, 2, &print_handle);
    configASSERT(result == pdPASS);
    vTaskStartScheduler();
    panic("Scheduler did not start");
}
