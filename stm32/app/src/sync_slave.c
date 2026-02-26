#include <stdint.h>
#include "spi_link.h"
#include "uart_log.h"
#include "FreeRTOS.h"
#include "task.h"

void link_test_task_slave(void *arg)
{
    (void)arg;
    spi_link_event_t evt;

    vTaskDelay(pdMS_TO_TICKS(500));

    for (;;) {
        if (spi_link_take_event(&evt)) {
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