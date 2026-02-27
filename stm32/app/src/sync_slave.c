#include <stdint.h>
#include "spi_link.h"
#include "uart_log.h"
#include "FreeRTOS.h"
#include "task.h"
#include "wire_codec.h"

void link_test_task_slave(void *arg)
{
    (void)arg;
    spi_link_event_t evt;
    uint8_t rx_buf[SPI_LINK_FRAME_MAX];

    vTaskDelay(pdMS_TO_TICKS(500));

    for (;;) {
        if (spi_link_take_event(&evt)) {
            if (evt.msg_type == WIRE_MSG_SYNC_REQ) {
                memset(resp_buf, 0, sizeof(resp_buf));
                wire_write_u16_le(resp_buf, WIRE_OFF_SYNC, WIRE_SYNC_WORD);
                resp_buf[WIRE_OFF_VERSION] = 0x01u;
                resp_buf[WIRE_OFF_MSG_TYPE] = WIRE_MSG_SYNC_RESP;
                wire_set_seq_id(resp_buf, evt.seq_id);
                wire_write_u16_le(resp_buf, WIRE_OFF_ACK_SEQ, evt.seq_id);
                resp_buf[WIRE_OFF_FLAGS] = 0u;
                resp_buf[WIRE_OFF_PAYLOAD_LEN] = WIRE_SYNC_RESP_PAYLOAD_LEN;
                memcpy(&resp_buf[WIRE_OFF_PAYLOAD], evt.rx_buf + WIRE_OFF_PAYLOAD, 8u);
                uint64_t t2 = evt.ts_rx_done_us;
                uint8_t *p2 = &resp_buf[WIRE_OFF_PAYLOAD + 8];
                for (uint8_t i = 0; i < 8; i++) p2[i] = (uint8_t)(t2 >> (8 * i));
                uint64_t t3 = evt.ts_cs_fall_us; 
                uint8_t *p3 = &resp_buf[WIRE_OFF_PAYLOAD + 16];
                for (uint8_t i = 0; i < 8; i++) p3[i] = (uint8_t)(t3 >> (8 * i));
                size_t resp_len = wire_frame_total_len(WIRE_SYNC_RESP_PAYLOAD_LEN);
                wire_crc16_append(resp_buf, (size_t)(resp_len - 2));
                spi_link_slave_response(resp_buf, (uint8_t)resp_len, evt.seq_id);
            }
            uart1_write_str("TS role=slave cs=");
            uart1_write_u64(evt.ts_cs_fall_us);
            uart1_write_str(" rx_done=");
            uart1_write_u64(evt.ts_rx_done_us);
            uart1_write_str("\r\n");
            uart1_write_str("LINK role=slave msg_type=");
            {
                char buf[2];
                buf[1] = '\0';
                buf[0] = (char)('0' + (evt.msg_type / 100));
                if (buf[0] != '0') uart1_write_str(buf);
                buf[0] = (char)('0' + ((evt.msg_type / 10) % 10));
                uart1_write_str(buf);
                buf[0] = (char)('0' + (evt.msg_type % 10));
                uart1_write_str(buf);
            }
            uart1_write_str(" seq=");
            uart1_write_u64((uint64_t)evt.seq_id);
            uart1_write_str("\r\n");
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}