#include <string.h>
#include "wire_codec.h"
#include "spi_link.h"
#include "timebase.h"
#include "uart_log.h"
#include "FreeRTOS.h"
#include "task.h"

void link_test_task_master(void *arg)
{
    (void)arg;
    uint8_t tx_buf[SPI_LINK_FRAME_MAX];
    uint16_t seq = 0u;
    const uint16_t sync_req_len = 20u; // 10 + 8 payload + 2 CRC

    vTaskDelay(pdMS_TO_TICKS(500));

    for (;;) {
        memset(tx_buf, 0, sizeof(tx_buf));
        wire_write_u16_le(tx_buf, WIRE_OFF_SYNC, WIRE_SYNC_WORD);
        tx_buf[WIRE_OFF_VERSION] = 0x01u;
        tx_buf[WIRE_OFF_MSG_TYPE] = WIRE_MSG_SYNC_REQ;
        wire_set_seq_id(tx_buf, seq);
        wire_write_u16_le(tx_buf, WIRE_OFF_ACK_SEQ, 0xFFFFu);
        tx_buf[WIRE_OFF_FLAGS] = 0u;
        tx_buf[WIRE_OFF_PAYLOAD_LEN] = 8u;
        {
            uint64_t t1 = timebase_now_us();
            uint8_t *p = &tx_buf[WIRE_OFF_PAYLOAD];
            for (uint8_t i = 0u; i < 8u; i++) {
                p[i] = (uint8_t)(t1 >> (8u * i));
            }
        }
        wire_crc16_append(tx_buf, 18u);

        spi_link_event_t evt;
        spi_link_result_t result;
        spi_link_master_txrx_with_retry(tx_buf, sync_req_len,
            LINK_DEFAULT_MAX_RETRIES, &evt, &result);

        uart1_write_str("TS role=master cs=");
        uart1_write_u64(evt.ts_cs_fall_us);
        uart1_write_str(" rx_done=");
        uart1_write_u64(evt.ts_rx_done_us);
        uart1_write_str("\r\n");
        uart1_write_str("LINK role=master result=");
        uart1_write_str(spi_link_result_string(result));
        uart1_write_str("\r\n");

        seq++;
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}