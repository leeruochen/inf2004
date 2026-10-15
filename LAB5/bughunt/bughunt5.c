/*
 * BUG HUNT #5 - It failed once, an hour in
 * INF2004 LAB 5
 *
 * The architecture from the Lab 5 exercise, built out properly:
 *
 *      sensor_task   reads the RP2040 temperature sensor every
 *                    SAMPLE_PERIOD_MS and publishes each sample
 *      avg_task      moving average over the last RING_LEN samples
 *      mean_task     simple running mean since boot
 *      print_task    the ONLY task allowed to call printf
 *
 * Requirement, unchanged from the lab: no printf outside print_task.
 *
 * NINE defects are planted, one of them in FreeRTOSConfig.h. It compiles, it
 * links, and it will run for a while. That is the problem.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/adc.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "message_buffer.h"
#include "timers.h"

#define RING_LEN         10u
#define MBUF_BYTES      256
#define PRINT_Q_LEN       8
#define SAMPLE_PERIOD_MS 100u

/* ---- shared state ---- */
// bug 4: a single message buffer (mbuf) was shared by two readers (avg_task and mean_task),
// violating FreeRTOS's single-writer, single-reader rule for message buffers.
// static MessageBufferHandle_t mbuf;
// fix 4: create two separate message buffers so each consumer has its own dedicated buffer.
static MessageBufferHandle_t avg_input, mean_input;
static QueueHandle_t         print_q;
static TaskHandle_t          sensor_handle, avg_handle, mean_handle, print_handle;

// bug 5: ring_mutex and global ring[] were shared across tasks without locking, and mean_task
// was also pushing into ring[], corrupting avg_task's sliding window.
// static SemaphoreHandle_t     ring_mutex;
// static int16_t ring[RING_LEN];
// static uint8_t head = 0;
// static uint8_t tail = 0;
// static uint8_t fill = 0;
// fix 5: remove global ring buffer and unused ring_mutex; make ring[] local inside avg_task
// so each consumer owns its own state and cannot interfere with the other.

typedef struct {
    const char *name;
    float       value;
} print_msg_t;

// fix 1: implement stack overflow and malloc failed hooks so the kernel halts immediately
// when a stack overflow or heap exhaustion is detected (alongside configASSERT and
// configCHECK_FOR_STACK_OVERFLOW 2 in FreeRTOSConfig.h).
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

/* ------------------------------------------------------------------
 * Ring buffer of the last RING_LEN samples.
 * Both avg_task and mean_task push into this.
 * ------------------------------------------------------------------ */
// bug 5: global ring_push and ring_sum were shared between avg_task and mean_task.
// static void ring_push(int16_t v) { ... }
// static int32_t ring_sum(void) { ... }
// fix 5: moved the ring buffer logic directly inside avg_task as local variables.

/* ------------------------------------------------------------------
 * Running-mean helper. Each task uses it to track its own long-term
 * average of whatever it is looking at.
 * ------------------------------------------------------------------ */
// bug 6: acc and n are static local vars in running_mean, and both avg_task and mean_task called it,
// sharing and corrupting the same accumulator. Also, float acc loses precision over thousands of samples.
// static float running_mean(int16_t v)
// {
//     static float acc = 0.0f;
//     static int   n   = 0;
//     acc += v;
//     n++;
//     return acc / n;
// }
// fix 6: remove the shared static helper completely; mean_task maintains its own local int64_t sum
// and uint64_t count so it is thread-safe and never suffers from floating-point rounding drift.

/* ------------------------------------------------------------------
 * Hand a message to print_task. No task except print_task may printf.
 * ------------------------------------------------------------------ */
static void say(const char *name, float value)
{
    // bug 8: xQueueSend used a timeout of 0 and ignored the return value, silently dropping
    // messages whenever print_q filled up.
    // print_msg_t m = { .kind = kind, .value = value, .extra = extra };
    // xQueueSend(print_q, &m, 0);
    // fix 8: called from task context only. Use portMAX_DELAY to apply backpressure if printing slows,
    // and configASSERT the return value so no message is ever silently discarded.
    print_msg_t message = {name, value};
    BaseType_t result = xQueueSend(print_q, &message, portMAX_DELAY);
    configASSERT(result == pdPASS);
}

/* ------------------------------------------------------------------
 * Tasks
 * ------------------------------------------------------------------ */
static void sensor_task(void *unused)
{
    (void)unused;

    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);

    // fix 3: initialise next wake time once before the loop for vTaskDelayUntil
    TickType_t next = xTaskGetTickCount();

    for (;;) {
        /* RP2040 datasheet transfer function; x10 to keep one decimal */
        float volts = adc_read() * 3.3f / 4096.0f;
        int16_t sample = (int16_t)((27.0f - (volts - 0.706f) / 0.001721f) * 10.0f);

        // bug 2: sample is int16_t (2 bytes), but &sample is a pointer (4 bytes).
        // Sending sizeof(&sample) sent 4 bytes, causing the 2-byte receive in consumers to fail and return 0.
        // bug 4: sent only to a single shared mbuf instead of one buffer per consumer.
        // bug 8: return value of xMessageBufferSend was ignored, allowing silent drops.
        // xMessageBufferSend(mbuf, &sample, sizeof(&sample), 0);
        // fix 2, 4, 8: send sizeof sample (2 bytes) to both avg_input and mean_input, and configASSERT
        // that the bytes were sent so the pipeline never silently drops data.
        size_t sent = xMessageBufferSend(avg_input, &sample, sizeof sample, 0);
        configASSERT(sent == sizeof sample);
        sent = xMessageBufferSend(mean_input, &sample, sizeof sample, 0);
        configASSERT(sent == sizeof sample);

        // bug 3: vTaskDelay(1) delayed for 1 tick (1 ms instead of SAMPLE_PERIOD_MS = 100 ms)
        // and used a relative delay, causing cumulative timing drift on every sample.
        // vTaskDelay(1);
        // fix 3: use vTaskDelayUntil with pdMS_TO_TICKS(SAMPLE_PERIOD_MS) for drift-free 100 ms sampling.
        vTaskDelayUntil(&next, pdMS_TO_TICKS(SAMPLE_PERIOD_MS));
    }
}

static void avg_task(void *unused)
{
    (void)unused;

    // fix 5: ring buffer state is local to avg_task so no other task can touch it and no mutex is needed.
    int16_t ring[RING_LEN] = {0};
    unsigned head = 0, fill = 0;
    int32_t sum = 0;

    for (;;) {
        int16_t sample;
        // fix 4 & 8: read from dedicated avg_input buffer and assert that sizeof sample bytes arrived.
        size_t received = xMessageBufferReceive(avg_input, &sample, sizeof sample, portMAX_DELAY);
        configASSERT(received == sizeof sample);

        sum -= ring[head];
        ring[head] = sample;
        sum += sample;
        head = (head + 1) % RING_LEN;
        if (fill < RING_LEN) fill++;

        // bug 6: avg_task unnecessarily called the non-reentrant running_mean(sample), corrupting mean_task's state.
        // float own = running_mean(sample);
        // bug 7: divided by constant RING_LEN (10) using integer division instead of the actual sample count (fill).
        // int32_t avg = ring_sum() / RING_LEN;
        // say(MSG_MOVING_AVG, avg / 10.0f, (uint32_t)own);
        // fix 6 & 7: removed running_mean() call and divide by (float)fill so the first 9 samples are averaged accurately.
        say("moving average", (float)sum / (float)fill / 10.0f);
    }
}

static void mean_task(void *unused)
{
    (void)unused;

    // fix 6: use local 64-bit integer sum and count inside mean_task to avoid shared static state
    // and prevent floating-point precision loss over long runs.
    int64_t sum = 0;
    uint64_t count = 0;

    for (;;) {
        int16_t sample;
        // fix 4 & 8: read from dedicated mean_input buffer and assert that sizeof sample bytes arrived.
        size_t received = xMessageBufferReceive(mean_input, &sample, sizeof sample, portMAX_DELAY);
        configASSERT(received == sizeof sample);

        // bug 5: mean_task was calling ring_push(sample), polluting avg_task's sliding window.
        // ring_push(sample);
        // bug 6: called shared non-reentrant running_mean(sample) with float accumulator.
        // float m = running_mean(sample);
        // fix 5 & 6: removed ring_push(sample) and accumulate into local int64_t sum with overflow checks.
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

    // fix 8: instead of using a software timer callback (which cannot block on print_q),
    // track the 1-second health interval directly inside print_task and print stack high-water marks.
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

    // fix 4 & 8: create dedicated message buffers for each consumer and assert creation succeeded.
    avg_input  = xMessageBufferCreate(MBUF_BYTES);
    mean_input = xMessageBufferCreate(MBUF_BYTES);
    print_q    = xQueueCreate(PRINT_Q_LEN, sizeof(print_msg_t));
    configASSERT(avg_input && mean_input && print_q);

    // bug 9: print_task was given only configMINIMAL_STACK_SIZE (256 words), which overflows when
    // calling printf with %.1f floating-point formatting and silently corrupts adjacent memory.
    // Also, xTaskCreate return values were not checked.
    // xTaskCreate(sensor_task, "sensor", configMINIMAL_STACK_SIZE,     NULL, 5, NULL);
    // xTaskCreate(avg_task,    "avg",    configMINIMAL_STACK_SIZE * 2, NULL, 3, NULL);
    // xTaskCreate(mean_task,   "mean",   configMINIMAL_STACK_SIZE * 2, NULL, 3, NULL);
    // xTaskCreate(print_task,  "print",  configMINIMAL_STACK_SIZE,     NULL, 2, NULL);
    // fix 9: increase print_task stack to 1024 words (and sensor_task to 512 words), save task handles
    // for high-water mark monitoring, and assert that every xTaskCreate call succeeds.
    BaseType_t result;
    result = xTaskCreate(sensor_task, "sensor", 512,  NULL, 5, &sensor_handle);
    configASSERT(result == pdPASS);
    result = xTaskCreate(avg_task,    "avg",    512,  NULL, 3, &avg_handle);
    configASSERT(result == pdPASS);
    result = xTaskCreate(mean_task,   "mean",   512,  NULL, 3, &mean_handle);
    configASSERT(result == pdPASS);
    result = xTaskCreate(print_task,  "print",  1024, NULL, 2, &print_handle);
    configASSERT(result == pdPASS);

    vTaskStartScheduler();
    panic("Scheduler did not start");
}