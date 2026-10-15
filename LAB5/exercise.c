#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "message_buffer.h"
#include "semphr.h"

/* ==========================================================================
 * CONSTANTS & PRIORITIES
 * ========================================================================== */
#define BUTTON_PIN                  21
#define DEBOUNCE_MS                 200

#define TASK_STACK_WORDS            (configMINIMAL_STACK_SIZE * 4)
#define MSG_BUFFER_SIZE_BYTES       64
#define PRINT_QUEUE_LENGTH          16
#define PRINT_MSG_MAX_LEN           96

/* Rate-Monotonic / Deadline-Driven Priority Assignment */
#define PRIO_PRINT_TASK             2   // Lowest: asynchronous I/O
#define PRIO_RUNNING_AVG_TASK       4   // Low: O(1) math, soft deadline
#define PRIO_MOVING_AVG_TASK        5   // Medium: O(N) window loop, soft deadline
#define PRIO_SENSOR_TASK            7   // High: strict 1 Hz periodic sampling
#define PRIO_BUTTON_TASK            8   // Highest: immediate user-event handling

/* Struct for structured messages sent to the Gatekeeper Print Task */
typedef struct {
    char text[PRINT_MSG_MAX_LEN];
} log_msg_t;

/* Message types sent to the consumer tasks (Sensor reading vs. Reset command) */
typedef enum {
    CMD_NEW_SAMPLE,
    CMD_RESET
} payload_type_t;

typedef struct {
    payload_type_t type;
    float value;
} sensor_packet_t;

/* ==========================================================================
 * KERNEL OBJECT HANDLES
 * ========================================================================== */
/* Two distinct message buffers: one per single-writer/single-reader pair */
static MessageBufferHandle_t xMovingAvgMsgBuffer;
static MessageBufferHandle_t xRunningAvgMsgBuffer;

/* Multi-writer, single-reader queue for the Gatekeeper Print Task */
static QueueHandle_t xPrintQueue;

/* Binary semaphore for Deferred Interrupt Handling (Extension A) */
static SemaphoreHandle_t xButtonSemaphore;

/* ==========================================================================
 * HELPER FUNCTIONS
 * ========================================================================== */
/* Thread-safe non-blocking print wrapper that sends strings to the Gatekeeper */
static void send_to_print_task(const char *str) {
    log_msg_t msg;
    strncpy(msg.text, str, PRINT_MSG_MAX_LEN - 1);
    msg.text[PRINT_MSG_MAX_LEN - 1] = '\0';
    xQueueSend(xPrintQueue, &msg, 0);
}

/* Read onboard RP2040 temperature sensor on ADC channel 4 */
static float read_onboard_temperature(void) {
    const float conversionFactor = 3.3f / (1 << 12);
    float adc = (float)adc_read() * conversionFactor;
    return 27.0f - (adc - 0.706f) / 0.001721f;
}

/* Report stack high-water mark once at startup (Constraint 8) */
static void report_stack_hwm(const char *task_name) {
    char buf[PRINT_MSG_MAX_LEN];
    UBaseType_t hwm = uxTaskGetStackHighWaterMark(NULL);
    snprintf(buf, sizeof(buf), "[STARTUP HWM] %s free stack: %lu words\n",
             task_name, (unsigned long)hwm);
    send_to_print_task(buf);
}

/* ==========================================================================
 * ISR & EXTENSION A: DEFERRED INTERRUPT HANDLER (GP21)
 * ========================================================================== */
void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == BUTTON_PIN && (events & GPIO_IRQ_EDGE_FALL)) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        /* Rule 1 & 2: Only use FromISR variants; never block or print here */
        xSemaphoreGiveFromISR(xButtonSemaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void button_task(__unused void *params) {
    TickType_t xLastPressTick = 0;
    const TickType_t xDebounceTicks = pdMS_TO_TICKS(DEBOUNCE_MS);
    sensor_packet_t reset_pkt = { .type = CMD_RESET, .value = 0.0f };

    report_stack_hwm("ButtonTask");

    while (true) {
        /* Wait indefinitely in Blocked state for ISR signal */
        xSemaphoreTake(xButtonSemaphore, portMAX_DELAY);

        /* Task-level Debouncing */
        TickType_t xNow = xTaskGetTickCount();
        if ((xNow - xLastPressTick) >= xDebounceTicks) {
            xLastPressTick = xNow;

            /* Send reset packets to both consumers */
            xMessageBufferSend(xMovingAvgMsgBuffer, &reset_pkt, sizeof(reset_pkt), 0);
            xMessageBufferSend(xRunningAvgMsgBuffer, &reset_pkt, sizeof(reset_pkt), 0);

            send_to_print_task("[BUTTON] GP21 pressed -> Averages & history reset!\n");
        }
    }
}

/* ==========================================================================
 * REQUIREMENT 1: PERIODIC SENSOR TASK (1 Hz, Drift-Free)
 * ========================================================================== */
void sensor_task(__unused void *params) {
    TickType_t xLastWakeTime;
    const TickType_t xPeriod = pdMS_TO_TICKS(1000);
    char buf[PRINT_MSG_MAX_LEN];

    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);

    report_stack_hwm("SensorTask");

    /* Initialise ONCE before the loop to prevent cumulative drift */
    xLastWakeTime = xTaskGetTickCount();

    while (true) {
        xTaskDelayUntil(&xLastWakeTime, xPeriod);

        float temp = read_onboard_temperature();
        sensor_packet_t pkt = { .type = CMD_NEW_SAMPLE, .value = temp };

        /* Send to Consumer 1: Moving-Average Task */
        if (xMessageBufferSend(xMovingAvgMsgBuffer, &pkt, sizeof(pkt), 0) == 0) {
            send_to_print_task("[WARN] MovingAvg buffer full, sample dropped\n");
        }

        /* Send to Consumer 2: Running-Average Task */
        if (xMessageBufferSend(xRunningAvgMsgBuffer, &pkt, sizeof(pkt), 0) == 0) {
            send_to_print_task("[WARN] RunningAvg buffer full, sample dropped\n");
        }

        snprintf(buf, sizeof(buf), "[SENSOR] Raw Temperature = %.2f C\n", temp);
        send_to_print_task(buf);
    }
}

/* ==========================================================================
 * REQUIREMENT 2: MOVING-AVERAGE TASK (10-Sample Sliding Window)
 * ========================================================================== */
void moving_avg_task(__unused void *params) {
    /* Local variables (not static) so the task function remains reentrant! */
    float window[10] = {0.0f};
    int index = 0;
    int count = 0;
    sensor_packet_t pkt;
    char buf[PRINT_MSG_MAX_LEN];

    report_stack_hwm("MovingAvgTask");

    while (true) {
        size_t rx = xMessageBufferReceive(xMovingAvgMsgBuffer, &pkt, sizeof(pkt), portMAX_DELAY);
        if (rx == sizeof(pkt)) {
            if (pkt.type == CMD_RESET) {
                memset(window, 0, sizeof(window));
                index = 0;
                count = 0;
            } else if (pkt.type == CMD_NEW_SAMPLE) {
                window[index] = pkt.value;
                index = (index + 1) % 10;
                if (count < 10) {
                    count++;
                }

                /* Recompute sum over the active window to avoid floating-point drift */
                float sum = 0.0f;
                for (int i = 0; i < count; i++) {
                    sum += window[i];
                }

                snprintf(buf, sizeof(buf), "[MOVING AVG (n=%d/10)] = %.2f C\n", count, sum / count);
                send_to_print_task(buf);
            }
        }
    }
}

/* ==========================================================================
 * REQUIREMENT 3: RUNNING-AVERAGE TASK (Cumulative Average Since Startup)
 * ========================================================================== */
void running_avg_task(__unused void *params) {
    double cumulative_sum = 0.0; /* Use double to minimize precision loss over time */
    uint32_t total_count = 0;
    sensor_packet_t pkt;
    char buf[PRINT_MSG_MAX_LEN];

    report_stack_hwm("RunningAvgTask");

    while (true) {
        size_t rx = xMessageBufferReceive(xRunningAvgMsgBuffer, &pkt, sizeof(pkt), portMAX_DELAY);
        if (rx == sizeof(pkt)) {
            if (pkt.type == CMD_RESET) {
                cumulative_sum = 0.0;
                total_count = 0;
            } else if (pkt.type == CMD_NEW_SAMPLE) {
                cumulative_sum += (double)pkt.value;
                total_count++;

                float running_avg = (float)(cumulative_sum / (double)total_count);
                snprintf(buf, sizeof(buf), "[RUNNING AVG (total=%lu)] = %.2f C\n",
                         (unsigned long)total_count, running_avg);
                send_to_print_task(buf);
            }
        }
    }
}

/* ==========================================================================
 * REQUIREMENT 4: GATEKEEPER PRINT TASK (Sole caller of printf)
 * ========================================================================== */
void print_task(__unused void *params) {
    log_msg_t msg;

    /* Can print directly here since this IS the print task */
    printf("[STARTUP HWM] PrintTask free stack: %lu words\n",
           (unsigned long)uxTaskGetStackHighWaterMark(NULL));

    while (true) {
        /* Block indefinitely until a message arrives in the print queue */
        if (xQueueReceive(xPrintQueue, &msg, portMAX_DELAY) == pdTRUE) {
            printf("%s", msg.text);
        }
    }
}

/* ==========================================================================
 * INITIALISATION & LAUNCH
 * ========================================================================== */
void vLaunch(void) {
    /* 1. Create IPC primitives BEFORE starting tasks/scheduler */
    xMovingAvgMsgBuffer  = xMessageBufferCreate(MSG_BUFFER_SIZE_BYTES);
    xRunningAvgMsgBuffer = xMessageBufferCreate(MSG_BUFFER_SIZE_BYTES);
    xPrintQueue          = xQueueCreate(PRINT_QUEUE_LENGTH, sizeof(log_msg_t));
    xButtonSemaphore     = xSemaphoreCreateBinary();

    configASSERT(xMovingAvgMsgBuffer != NULL);
    configASSERT(xRunningAvgMsgBuffer != NULL);
    configASSERT(xPrintQueue != NULL);
    configASSERT(xButtonSemaphore != NULL);

    /* 2. Configure GP21 Push Button Interrupt (Extension A) */
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);
    gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    /* 3. Create Tasks */
    xTaskCreate(print_task,       "PrintTask",      TASK_STACK_WORDS, NULL, PRIO_PRINT_TASK,       NULL);
    xTaskCreate(running_avg_task, "RunningAvgTask", TASK_STACK_WORDS, NULL, PRIO_RUNNING_AVG_TASK, NULL);
    xTaskCreate(moving_avg_task,  "MovingAvgTask",  TASK_STACK_WORDS, NULL, PRIO_MOVING_AVG_TASK,  NULL);
    xTaskCreate(sensor_task,      "SensorTask",     TASK_STACK_WORDS, NULL, PRIO_SENSOR_TASK,      NULL);
    xTaskCreate(button_task,      "ButtonTask",     TASK_STACK_WORDS, NULL, PRIO_BUTTON_TASK,      NULL);

    /* 4. Start the scheduler */
    vTaskStartScheduler();
}

int main(void) {
    stdio_init_all();
    vLaunch();
    return 0;
}