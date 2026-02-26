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

#define LINK_DEFAULT_MAX_RETRIES  3u

typedef enum {
    SPI_LINK_RESULT_OK = 0,
    SPI_LINK_RESULT_ERR_TIMEOUT,
    SPI_LINK_RESULT_ERR_CRC,
    SPI_LINK_RESULT_ERR_SEQ,
    SPI_LINK_RESULT_ERR_MAX_RETRIES,
    SPI_LINK_RESULT_ERR_BUSY,
    SPI_LINK_RESULT_ERR_ARG,
    SPI_LINK_RESULT_COUNT
} spi_link_result_t;

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
bool spi_link_master_txrx_with_retry(const uint8_t *tx, 
    uint8_t len, 
    uint8_t max_retries,
    spi_link_event_t *out, 
    spi_link_result_t *result);
bool spi_link_slave_response(uint8_t *tx, uint8_t len, uint16_t seq_id);
uint8_t spi_link_result_to_byte(spi_link_result_t r);
const char *spi_link_result_string(spi_link_result_t r);
bool spi_link_take_event(spi_link_event_t *out);
void spi_link_check_timeout(void);