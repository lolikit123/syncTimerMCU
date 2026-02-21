#include "timebase.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/f4/nvic.h>
#include <libopencm3/cm3/cortex.h>

static volatile uint64_t g_tim2_overflows = 0;

void tim2_isr(void)
{
    if (timer_get_flag(TIM2, TIM_SR_UIF)) {
        g_tim2_overflows++;
        timer_clear_flag(TIM2, TIM_SR_UIF);
    }
}

void timebase_init(void)
{
    rcc_periph_clock_enable(RCC_TIM2);

    timer_disable_irq(TIM2, TIM_DIER_UIE);
    timer_disable_counter(TIM2);

    timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

    //Make sure timer is set to 1MHz
    uint32_t tim_clk = rcc_get_timer_clk_freq(TIM2);
    uint32_t psc = (tim_clk / 1000000) - 1;
    timer_set_prescaler(TIM2, psc);
    timer_set_period(TIM2, 0xFFFFFFFF);
    timer_enable_counter(TIM2);

    timer_generate_event(TIM2,TIM_EGR_UG);
    timer_clear_flag(TIM2, TIM_SR_UIF);
    g_tim2_overflows = 0;

    nvic_set_priority(NVIC_TIM2_IRQ, 0x40);
    nvic_enable_irq(NVIC_TIM2_IRQ);

    timer_enable_irq(TIM2, TIM_DIER_UIE);
    timer_enable_counter(TIM2);
}

uint64_t timebase_now_us(void)
{
    uint32_t hi, lo;
    //"disables interrupts for the next command or block of code the interrupt mask is automatically restored"
    CM_ATOMIC_BLOCK() {
        hi = g_tim2_overflows;
        lo = timer_get_counter(TIM2);
        if (timer_get_flag(TIM2, TIM_SR_UIF)) {
            hi++;
            lo = timer_get_counter(TIM2);
        }
    }
    return ((uint64_t)hi << 32) | lo;
}