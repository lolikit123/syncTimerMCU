#include "Test.h"
#include "timebase.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdint.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/f4/nvic.h>
#include <libopencm3/stm32/usart.h>

static volatile uint32_t g_task_backsteps = 0;
static volatile uint32_t g_isr_backsteps = 0;
static volatile uint32_t g_task_samples = 0;
static volatile uint32_t g_isr_samples = 0;

static void uart1_write_str(const char *s)
{
    while (*s) {
        usart_send_blocking(USART1, (uint16_t)*s++);
    }
}

static void uart1_write_u64(uint64_t v)
{
    char buf[21];
    int i = 20;
    buf[i] = '\0';

    do {
        buf[--i] = (char)('0' + (v % 10u));
        v /= 10u;
    } while (v);

    uart1_write_str(&buf[i]);
}

void tim3_isr(void)
{
    if (!timer_get_flag(TIM3, TIM_SR_UIF)) {
        return;
    }
    timer_clear_flag(TIM3, TIM_SR_UIF);

    static uint64_t prev_us = 0;
    uint64_t now_us = timebase_now_us();
    if (now_us < prev_us) {
        g_isr_backsteps++;
    }
    prev_us = now_us;
    g_isr_samples++;
}

static void isr_load_timer_setup(void)
{
    rcc_periph_clock_enable(RCC_TIM3);

    timer_disable_counter(TIM3);
    timer_disable_irq(TIM3, TIM_DIER_UIE);

    uint32_t tim_clk = rcc_get_timer_clk_freq(TIM3);
    timer_set_prescaler(TIM3, (tim_clk / 1000000u) - 1u); /* 1 MHz */
    timer_set_period(TIM3, 49u);                           /* 20 kHz */

    timer_set_counter(TIM3, 0u);
    timer_generate_event(TIM3, TIM_EGR_UG);
    timer_clear_flag(TIM3, TIM_SR_UIF);

    nvic_set_priority(NVIC_TIM3_IRQ, 0x50);
    nvic_enable_irq(NVIC_TIM3_IRQ);

    timer_enable_irq(TIM3, TIM_DIER_UIE);
    timer_enable_counter(TIM3);
}

static void timebase_test_task(void *arg)
{
    (void)arg;

    uint64_t prev_us = timebase_now_us();
    TickType_t last_report = xTaskGetTickCount();

    for (;;) {
        for (uint32_t i = 0; i < 1000; ++i) {
            uint64_t now_us = timebase_now_us();
            if (now_us < prev_us) {
                g_task_backsteps++;
            }
            prev_us = now_us;
            g_task_samples++;
        }

        TickType_t now = xTaskGetTickCount();
        if ((now - last_report) >= pdMS_TO_TICKS(1000)) {
            uart1_write_str("TB task_back=");
            uart1_write_u64(g_task_backsteps);
            uart1_write_str(" isr_back=");
            uart1_write_u64(g_isr_backsteps);
            uart1_write_str(" task_samp=");
            uart1_write_u64(g_task_samples);
            uart1_write_str(" isr_samp=");
            uart1_write_u64(g_isr_samples);
            uart1_write_str("\r\n");
            last_report = now;
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void timebase_test_start(void)
{
    g_task_backsteps = 0;
    g_isr_backsteps = 0;
    g_task_samples = 0;
    g_isr_samples = 0;

    isr_load_timer_setup();
    xTaskCreate(timebase_test_task, "tbtest", 256, NULL, tskIDLE_PRIORITY + 2, NULL);
}