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
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/adc.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "message_buffer.h"
#include "timers.h"

#define RING_LEN         10
#define MBUF_BYTES      256
#define PRINT_Q_LEN       8
#define SAMPLE_PERIOD_MS 100

/* ---- shared state ---- */
static MessageBufferHandle_t mbuf;
static QueueHandle_t         print_q;
static SemaphoreHandle_t     ring_mutex;

static int16_t ring[RING_LEN];
static uint8_t head = 0;
static uint8_t tail = 0;
static uint8_t fill = 0;

typedef enum {
    MSG_MOVING_AVG,
    MSG_RUNNING_MEAN,
    MSG_HEALTH,
} msg_kind_t;

typedef struct {
    msg_kind_t kind;
    float      value;
    uint32_t   extra;
} print_msg_t;

/* ------------------------------------------------------------------
 * Ring buffer of the last RING_LEN samples.
 * Both avg_task and mean_task push into this.
 * ------------------------------------------------------------------ */
static void ring_push(int16_t v)
{
    ring[head] = v;
    head = (head + 1) % RING_LEN;

    if (fill < RING_LEN) fill++;
    else                 tail = (tail + 1) % RING_LEN;
}

static int32_t ring_sum(void)
{
    int32_t s = 0;
    for (uint8_t i = 0; i < fill; i++)
        s += ring[(tail + i) % RING_LEN];
    return s;
}

/* ------------------------------------------------------------------
 * Running-mean helper. Each task uses it to track its own long-term
 * average of whatever it is looking at.
 * ------------------------------------------------------------------ */
static float running_mean(int16_t v)
{
    static float acc = 0.0f;
    static int   n   = 0;

    acc += v;
    n++;
    return acc / n;
}

/* ------------------------------------------------------------------
 * Hand a message to print_task. No task except print_task may printf.
 * ------------------------------------------------------------------ */
static void say(msg_kind_t kind, float value, uint32_t extra)
{
    print_msg_t m = { .kind = kind, .value = value, .extra = extra };
    xQueueSend(print_q, &m, 0);
}

/* ------------------------------------------------------------------
 * Tasks
 * ------------------------------------------------------------------ */
static void sensor_task(void *params)
{
    (void)params;

    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);

    while (true) {
        uint16_t raw = adc_read();

        /* RP2040 datasheet transfer function; x10 to keep one decimal */
        float volts = raw * 3.3f / 4096.0f;
        float degc  = 27.0f - (volts - 0.706f) / 0.001721f;
        int16_t sample = (int16_t)(degc * 10.0f);

        xMessageBufferSend(mbuf, &sample, sizeof(&sample), 0);

        vTaskDelay(1);
    }
}

static void avg_task(void *params)
{
    (void)params;

    while (true) {
        int16_t sample;
        size_t n = xMessageBufferReceive(mbuf, &sample, sizeof sample, portMAX_DELAY);
        if (n == 0) continue;

        ring_push(sample);

        int32_t avg = ring_sum() / RING_LEN;
        float   own = running_mean(sample);

        say(MSG_MOVING_AVG, avg / 10.0f, (uint32_t)own);
    }
}

static void mean_task(void *params)
{
    (void)params;

    while (true) {
        int16_t sample;
        size_t n = xMessageBufferReceive(mbuf, &sample, sizeof sample, portMAX_DELAY);
        if (n == 0) continue;

        ring_push(sample);

        float m = running_mean(sample);

        say(MSG_RUNNING_MEAN, m / 10.0f, 0);
    }
}

static void print_task(void *params)
{
    (void)params;

    while (true) {
        print_msg_t m;
        if (xQueueReceive(print_q, &m, portMAX_DELAY) != pdTRUE)
            continue;

        switch (m.kind) {
            case MSG_MOVING_AVG:
                printf("moving avg (last %d): %.1f C\n", RING_LEN, m.value);
                break;
            case MSG_RUNNING_MEAN:
                printf("running mean since boot: %.1f C\n", m.value);
                break;
            case MSG_HEALTH:
                printf("-- heap free: %u bytes --\n", (unsigned)m.extra);
                break;
        }
    }
}

/* Reports system health once a second. Runs in the timer service task. */
static void health_timer_cb(TimerHandle_t t)
{
    (void)t;
    say(MSG_HEALTH, 0.0f, (uint32_t)xPortGetFreeHeapSize());
}

int main(void)
{
    stdio_init_all();
    sleep_ms(3000);

    printf("BUG HUNT #5 - starting scheduler\n");

    mbuf       = xMessageBufferCreate(MBUF_BYTES);
    print_q    = xQueueCreate(PRINT_Q_LEN, sizeof(print_msg_t));
    ring_mutex = xSemaphoreCreateMutex();

    memset(ring, 0, sizeof ring);

    xTaskCreate(sensor_task, "sensor", configMINIMAL_STACK_SIZE,     NULL, 5, NULL);
    xTaskCreate(avg_task,    "avg",    configMINIMAL_STACK_SIZE * 2, NULL, 3, NULL);
    xTaskCreate(mean_task,   "mean",   configMINIMAL_STACK_SIZE * 2, NULL, 3, NULL);
    xTaskCreate(print_task,  "print",  configMINIMAL_STACK_SIZE,     NULL, 2, NULL);

    TimerHandle_t health = xTimerCreate("health", pdMS_TO_TICKS(1000),
                                        pdTRUE, NULL, health_timer_cb);
    xTimerStart(health, 0);

    vTaskStartScheduler();

    while (true);
}
