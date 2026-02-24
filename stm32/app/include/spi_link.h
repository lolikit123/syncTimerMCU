#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "wire_codec.h"

#include <libopencm3/cm3/cortex.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/f4/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/syscfg.h>

typedef struct {
    uint64_t ts_cs_fall_us;
    uint64_t ts_rx_done_us;   
    uint16_t seq_id;
    uint8_t  msg_type;
    uint8_t  rx_len;
    uint8_t  rx_buf[SPI_LINK_FRAME_MAX];
} spi_link_event_t;

void spi_link_init_master(void);
void spi_link_init_slave(void);

bool spi_link_master_start_txrx(const uint8_t *tx, uint8_t len);
bool spi_link_slave_response(uint8_t *tx, uint8_t len, uint16_t seq_id);
bool spi_link_take_event(spi_link_event_t *out);