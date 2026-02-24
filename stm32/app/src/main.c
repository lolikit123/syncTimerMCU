#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include "timebase.h"
#if APP_ENABLE_TIMEBASE_TESTS
{#include "Test.h"}
#endif
#include "spi_link.h"

#include "FreeRTOS.h"
#include "task.h"

#ifndef APP_ENABLE_TIMEBASE_TESTS
#define APP_ENABLE_TIMEBASE_TESTS 0
#endif

static void clock_setup(void)
{
    rcc_clock_setup_pll(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_84MHZ]);
}

static void gpio_setup(void)
{
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);
    gpio_set(GPIOC, GPIO13);
}

static void usart1_setup(void)
{
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_USART1);

    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9 | GPIO10);
    gpio_set_af(GPIOA, GPIO_AF7, GPIO9 | GPIO10);

    usart_set_baudrate(USART1, 115200);
    usart_set_databits(USART1, 8);
    usart_set_stopbits(USART1, USART_STOPBITS_1);
    usart_set_parity(USART1, USART_PARITY_NONE);
    usart_set_mode(USART1, USART_MODE_TX_RX);
    usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
    usart_enable(USART1);
}

static void uart1_write_str(const char *s)
{
    while (*s) {
        usart_send_blocking(USART1, (uint16_t)*s++);
    }
}

static void hello_task(void *arg)
{
    (void)arg;
    for (;;) {
        gpio_toggle(GPIOC, GPIO13);
        //uart1_write_str("Hello from FreeRTOS on USART1\r\n");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

int main(void)
{
    clock_setup();
    gpio_setup();
    usart1_setup();
    timebase_init();
    
    
    #if APP_ENABLE_TIMEBASE_TESTS
    {
        timebase_test_start();
    }
    #endif

    xTaskCreate(hello_task, "hello", 256, NULL, tskIDLE_PRIORITY + 1, NULL);
    vTaskStartScheduler();

    for (;;) {}
}