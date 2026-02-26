#include "spi_link.h"
#include "timebase.h"

#include <string.h>

#ifndef APP_ROLE_MASTER
#define APP_ROLE_MASTER 0
#endif
#ifndef APP_ROLE_SLAVE
#define APP_ROLE_SLAVE 0
#endif

#if (APP_ROLE_MASTER == APP_ROLE_SLAVE)
#error "Build exactly one role: APP_ROLE_MASTER=1 or APP_ROLE_SLAVE=1"
#endif

#define LINK_SPI              SPI1
#define LINK_SPI_RCC          RCC_SPI1
#define LINK_GPIO_RCC         RCC_GPIOA
#define LINK_PORT             GPIOA
#define LINK_AF               GPIO_AF5
#define LINK_PIN_CS           GPIO4
#define LINK_PIN_SCK          GPIO5
#define LINK_PIN_MISO         GPIO6
#define LINK_PIN_MOSI         GPIO7

/* Forward declaration: defined later in file */
static void transfer_timeout(void);

#if defined(STM32F4)
/* F4 uses 16-bit spi_send/spi_read; wrap for 8-bit frame transfer */
static inline void spi_send8(uint32_t spi, uint8_t data) { spi_send(spi, (uint16_t)data); }
static inline uint8_t spi_read8(uint32_t spi) { return (uint8_t)(spi_read(spi) & 0xFFu); }
#endif

#define T_RESP_TIMEOUT          (8000u)

#define EVQ_CAP               4u

typedef struct {
    volatile uint8_t busy;
    volatile uint8_t tx_len;
    volatile uint8_t rx_len;
    volatile uint8_t tx_idx;           
    volatile uint8_t expected_len;     
    volatile uint8_t frame_len_known;
    volatile uint8_t tx_is_response;   

    uint8_t tx_buf[SPI_LINK_FRAME_MAX];
    uint8_t rx_buf[SPI_LINK_FRAME_MAX];
    spi_link_event_t current;

    spi_link_event_t evq[EVQ_CAP];
    volatile uint8_t evq_head;
    volatile uint8_t evq_tail;

#if APP_ROLE_MASTER
    uint16_t expected_seq_id;
    volatile uint8_t last_transfer_ok;
    volatile uint8_t last_result;
    spi_link_event_t last_evt;
#endif

#if APP_ROLE_SLAVE
    volatile uint8_t resp_armed;
    uint8_t resp_len;
    uint16_t resp_seq;
    uint8_t resp_buf[SPI_LINK_FRAME_MAX];
#endif
} spi_link_ctx_t;

static spi_link_ctx_t g_ctx;

static void queue_push_isr(const spi_link_event_t *evt)
{
    uint8_t next = (uint8_t)((g_ctx.evq_head + 1u) % EVQ_CAP);
    if (next == g_ctx.evq_tail) {
        g_ctx.evq_tail = (uint8_t)((g_ctx.evq_tail + 1u) % EVQ_CAP); 
    }
    g_ctx.evq[g_ctx.evq_head] = *evt;
    g_ctx.evq_head = next;
}

static void spi_set_8bit(uint32_t spi)
{
#if defined(SPI_CR2_DS_8BIT)
    spi_set_data_size(spi, SPI_CR2_DS_8BIT);
    spi_fifo_reception_threshold_8bit(spi);
#else
    spi_set_dff_8bit(spi);
#endif
}

static void spi_hw_common_init(void)
{
    rcc_periph_clock_enable(LINK_GPIO_RCC);
    rcc_periph_clock_enable(LINK_SPI_RCC);

    gpio_mode_setup(LINK_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE,
                    LINK_PIN_SCK | LINK_PIN_MISO | LINK_PIN_MOSI);
    gpio_set_af(LINK_PORT, LINK_AF, LINK_PIN_SCK | LINK_PIN_MISO | LINK_PIN_MOSI);

    spi_disable(LINK_SPI);
    spi_set_full_duplex_mode(LINK_SPI);
    spi_set_unidirectional_mode(LINK_SPI);
    spi_set_clock_polarity_0(LINK_SPI);
    spi_set_clock_phase_0(LINK_SPI);
    spi_send_msb_first(LINK_SPI);
    spi_set_8bit(LINK_SPI);
}

static void fill_event_metadata(spi_link_event_t *evt, const uint8_t *rx, uint8_t rx_len)
{
    evt->rx_len = rx_len;
    memcpy(evt->rx_buf, rx, rx_len);
    if (rx_len < SPI_LINK_FRAME_MAX) {
        memset(&evt->rx_buf[rx_len], 0, (size_t)(SPI_LINK_FRAME_MAX - rx_len));
    }

    if (rx_len >= WIRE_FRAME_HDR_LEN) {
        evt->msg_type = wire_get_msg_type(rx);
        evt->seq_id = wire_get_seq_id(rx);
    } else {
        evt->msg_type = 0u;
        evt->seq_id = 0xFFFFu;
    }
}

static void finish_transfer_isr(void)
{
    spi_disable_rx_buffer_not_empty_interrupt(LINK_SPI);

#if APP_ROLE_MASTER
    gpio_set(LINK_PORT, LINK_PIN_CS);
#endif

    g_ctx.current.ts_rx_done_us = timebase_now_us();

    uint8_t clipped = g_ctx.rx_len;
    if (clipped > SPI_LINK_FRAME_MAX) {
        clipped = SPI_LINK_FRAME_MAX;
    }

    if (!wire_crc16_verify(g_ctx.rx_buf, (size_t)clipped)) {
        #if APP_ROLE_MASTER
              g_ctx.last_result = (uint8_t)SPI_LINK_RESULT_ERR_CRC;
        #endif
              g_ctx.busy = 0u;
              return;
          }

#if APP_ROLE_MASTER
    if (wire_get_seq_id(g_ctx.rx_buf) != g_ctx.expected_seq_id) {
        g_ctx.last_result = (uint8_t)SPI_LINK_RESULT_ERR_SEQ;
        g_ctx.busy = 0u;
        return;
    }
      #endif

    fill_event_metadata(&g_ctx.current, g_ctx.rx_buf, clipped);

#if APP_ROLE_SLAVE
    if (!g_ctx.tx_is_response) {
        queue_push_isr(&g_ctx.current);
    } else {
        g_ctx.resp_armed = 0u;
    }
#else
    g_ctx.last_evt = g_ctx.current;
    g_ctx.last_result = (uint8_t)SPI_LINK_RESULT_OK;
    g_ctx.last_transfer_ok = 1u;
    queue_push_isr(&g_ctx.current);
#endif

    g_ctx.busy = 0u;
}

void spi_link_init_master(void)
{
#if APP_ROLE_MASTER
    memset(&g_ctx, 0, sizeof(g_ctx));

    spi_hw_common_init();
    spi_set_master_mode(LINK_SPI);
    spi_set_baudrate_prescaler(LINK_SPI, SPI_CR1_BR_FPCLK_DIV_16);
    spi_enable_software_slave_management(LINK_SPI);
    spi_set_nss_high(LINK_SPI);

    gpio_mode_setup(LINK_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LINK_PIN_CS);
    gpio_set(LINK_PORT, LINK_PIN_CS);

    nvic_set_priority(NVIC_SPI1_IRQ, 0x40);
    nvic_enable_irq(NVIC_SPI1_IRQ);

    spi_enable(LINK_SPI);
#endif
}

void spi_link_init_slave(void)
{
#if APP_ROLE_SLAVE
    memset(&g_ctx, 0, sizeof(g_ctx));

    spi_hw_common_init();
    spi_set_slave_mode(LINK_SPI);
    spi_enable_software_slave_management(LINK_SPI);
    spi_set_nss_low(LINK_SPI);

    gpio_mode_setup(LINK_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, LINK_PIN_CS);

    rcc_periph_clock_enable(RCC_SYSCFG);
    exti_select_source(EXTI4, LINK_PORT);
    exti_set_trigger(EXTI4, EXTI_TRIGGER_FALLING);
    exti_enable_request(EXTI4);

    nvic_set_priority(NVIC_EXTI4_IRQ, 0x30);
    nvic_enable_irq(NVIC_EXTI4_IRQ);
    nvic_set_priority(NVIC_SPI1_IRQ, 0x40);
    nvic_enable_irq(NVIC_SPI1_IRQ);

    spi_enable(LINK_SPI);
#endif
}

bool spi_link_master_start_txrx(const uint8_t *tx, uint8_t len)
{
#if APP_ROLE_MASTER
    if ((tx == NULL) || (len == 0u) || (len > SPI_LINK_FRAME_MAX)) {
        return false;
    }

    bool accepted = false;
    CM_ATOMIC_BLOCK() {
        if (!g_ctx.busy) {
            g_ctx.busy = 1u;
            accepted = true;
        }
    }
    if (!accepted) {
        return false;
    }

    memcpy(g_ctx.tx_buf, tx, len);
    if (len < SPI_LINK_FRAME_MAX) {
        memset(&g_ctx.tx_buf[len], 0, (size_t)(SPI_LINK_FRAME_MAX - len));
    }
    memset(g_ctx.rx_buf, 0, sizeof(g_ctx.rx_buf));

    g_ctx.tx_len = len;
    g_ctx.rx_len = 0u;
    g_ctx.tx_idx = 1u;          
    g_ctx.expected_len = len;   
    g_ctx.frame_len_known = 1u;
    g_ctx.tx_is_response = 0u;

    g_ctx.current.ts_cs_fall_us = 0u;
    g_ctx.current.ts_rx_done_us = 0u;
    g_ctx.current.msg_type = 0u;
    g_ctx.current.seq_id = 0xFFFFu;
    g_ctx.current.rx_len = 0u;
    g_ctx.expected_seq_id = wire_get_seq_id(g_ctx.tx_buf);
    g_ctx.last_transfer_ok = 0u;

    gpio_clear(LINK_PORT, LINK_PIN_CS);
    g_ctx.current.ts_cs_fall_us = timebase_now_us();

    spi_send8(LINK_SPI, g_ctx.tx_buf[0]);
    spi_enable_rx_buffer_not_empty_interrupt(LINK_SPI);
    return true;
#else
    (void)tx;
    (void)len;
    return false;
#endif
}

bool spi_link_slave_response(uint8_t *tx, uint8_t len, uint16_t seq_id)
{
#if APP_ROLE_SLAVE
    if ((tx == NULL) || (len == 0u) || (len > SPI_LINK_FRAME_MAX)) {
        return false;
    }

    CM_ATOMIC_BLOCK() {
        memcpy(g_ctx.resp_buf, tx, len);
        g_ctx.resp_len = len;
        g_ctx.resp_seq = seq_id;
        g_ctx.resp_armed = 1u;
    }
    return true;
#else
    (void)tx;
    (void)len;
    (void)seq_id;
    return false;
#endif
}

bool spi_link_take_event(spi_link_event_t *out)
{
    if (out == NULL) {
        return false;
    }

    bool has = false;
    CM_ATOMIC_BLOCK() {
        if (g_ctx.evq_head != g_ctx.evq_tail) {
            *out = g_ctx.evq[g_ctx.evq_tail];
            g_ctx.evq_tail = (uint8_t)((g_ctx.evq_tail + 1u) % EVQ_CAP);
            has = true;
        }
    }
    return has;
}

#if APP_ROLE_SLAVE
void exti4_isr(void)
{
    if (!exti_get_flag_status(EXTI4)) {
        return;
    }
    exti_reset_request(EXTI4);

    if ((gpio_get(LINK_PORT, LINK_PIN_CS) & LINK_PIN_CS) != 0u) {
        return; 
    }
    if (g_ctx.busy) {
        return;
    }

    g_ctx.busy = 1u;
    g_ctx.rx_len = 0u;
    g_ctx.tx_idx = 1u; 
    memset(g_ctx.rx_buf, 0, sizeof(g_ctx.rx_buf));

    g_ctx.current.ts_cs_fall_us = timebase_now_us();
    g_ctx.current.ts_rx_done_us = 0u;
    g_ctx.current.msg_type = 0u;
    g_ctx.current.seq_id = 0xFFFFu;
    g_ctx.current.rx_len = 0u;

    if (g_ctx.resp_armed) {
        g_ctx.tx_is_response = 1u;
        g_ctx.tx_len = g_ctx.resp_len;
        g_ctx.expected_len = g_ctx.resp_len;  
        g_ctx.frame_len_known = 1u;
        memcpy(g_ctx.tx_buf, g_ctx.resp_buf, g_ctx.resp_len);

        if ((g_ctx.tx_len >= wire_frame_total_len(WIRE_SYNC_RESP_PAYLOAD_LEN)) &&
        (wire_get_msg_type(g_ctx.tx_buf) == WIRE_MSG_SYNC_RESP) &&
        (wire_read_u16_le(g_ctx.tx_buf, WIRE_OFF_SYNC) == WIRE_SYNC_WORD)) 
        {
        wire_sync_resp_set_t3(g_ctx.tx_buf, g_ctx.current.ts_cs_fall_us);
        }
    } else {
        g_ctx.tx_is_response = 0u;
        g_ctx.tx_len = 0u;                 
        g_ctx.expected_len = SPI_LINK_FRAME_MAX;
        g_ctx.frame_len_known = 0u;
        memset(g_ctx.tx_buf, 0, sizeof(g_ctx.tx_buf));
    }

    spi_send8(LINK_SPI, g_ctx.tx_buf[0]); 
    spi_enable_rx_buffer_not_empty_interrupt(LINK_SPI);
}
#endif

void spi1_isr(void)
{
    if ((SPI_SR(LINK_SPI) & SPI_SR_RXNE) == 0u) {
        return;
    }

    uint8_t in = spi_read8(LINK_SPI);
    uint64_t now = timebase_now_us();
    uint64_t elapsed = now - g_ctx.current.ts_cs_fall_us;
    if (elapsed > (uint64_t)T_RESP_TIMEOUT) {
        transfer_timeout();
        return;
    }

    if (!g_ctx.busy) {
        (void)in;
        return;
    }

    if (g_ctx.rx_len < SPI_LINK_FRAME_MAX) {
        g_ctx.rx_buf[g_ctx.rx_len] = in;
    }
    if (g_ctx.rx_len < 0xFFu) {
        g_ctx.rx_len++;
    }

#if APP_ROLE_MASTER
    if (g_ctx.tx_idx < g_ctx.tx_len) {
        spi_send8(LINK_SPI, g_ctx.tx_buf[g_ctx.tx_idx]);
        g_ctx.tx_idx++;
    }

    if ((g_ctx.rx_len >= g_ctx.expected_len) &&
        (g_ctx.tx_idx >= g_ctx.tx_len) &&
        ((SPI_SR(LINK_SPI) & SPI_SR_BSY) == 0u)) {
        finish_transfer_isr();
    }
#endif

#if APP_ROLE_SLAVE
    if (!g_ctx.frame_len_known && (g_ctx.rx_len >= WIRE_FRAME_HDR_LEN)) {
        uint16_t sync = wire_read_u16_le(g_ctx.rx_buf, WIRE_OFF_SYNC);
        uint8_t payload_len = wire_get_payload_len(g_ctx.rx_buf);
        uint16_t total = wire_frame_total_len(payload_len);

        if ((sync == WIRE_SYNC_WORD) &&
            (payload_len <= WIRE_FRAME_MAX_PAYLOAD) &&
            (total <= WIRE_FRAME_MAX)) {
            g_ctx.expected_len = (uint8_t)total;
        } else {
            g_ctx.expected_len = WIRE_FRAME_MAX;
        }
        g_ctx.frame_len_known = 1u;
    }

    if (g_ctx.tx_idx < g_ctx.tx_len) {
        spi_send8(LINK_SPI, g_ctx.tx_buf[g_ctx.tx_idx]);
    } else {
        spi_send8(LINK_SPI, 0x00u);
    }
    if (g_ctx.tx_idx < SPI_LINK_FRAME_MAX) {
        g_ctx.tx_idx++;
    }

    if (g_ctx.frame_len_known && (g_ctx.rx_len >= g_ctx.expected_len)) {
        finish_transfer_isr();
    }
#endif
}

static void transfer_timeout(void){
    spi_disable_rx_buffer_not_empty_interrupt(LINK_SPI);

#if APP_ROLE_MASTER
    gpio_set(LINK_PORT, LINK_PIN_CS);
    g_ctx.last_result = (uint8_t)SPI_LINK_RESULT_ERR_TIMEOUT;
    g_ctx.last_transfer_ok = 0u;
#endif

    g_ctx.busy = 0u;
}

void spi_link_check_timeout(void)
{
    if (!g_ctx.busy) {
        return;
    }
    if (g_ctx.current.ts_cs_fall_us == 0u) {
        return;
    }
    uint64_t now = timebase_now_us();
    uint64_t elapsed = now - g_ctx.current.ts_cs_fall_us;
    if (elapsed > (uint64_t)T_RESP_TIMEOUT) {
        transfer_timeout(); 
    }
}

#if APP_ROLE_MASTER
bool spi_link_master_txrx_with_retry(const uint8_t *tx, uint8_t len, uint8_t max_retries,
                                     spi_link_event_t *out, spi_link_result_t *result)
{
    if ((tx == NULL) || (len == 0u) || (len > SPI_LINK_FRAME_MAX) || (out == NULL)) {
        if (result != NULL) {
            *result = SPI_LINK_RESULT_ERR_ARG;
        }
        return false;
    }
    if (max_retries > 5u) {
        max_retries = 5u;
    }

    for (uint8_t attempt = 0u; attempt <= max_retries; attempt++) {
        g_ctx.last_transfer_ok = 0u;
        if (!spi_link_master_start_txrx(tx, len)) {
            if (result != NULL) {
                *result = SPI_LINK_RESULT_ERR_BUSY;
            }
            return false;
        }
        while (g_ctx.busy && !g_ctx.last_transfer_ok) {
            spi_link_check_timeout();
        }
        if (g_ctx.last_transfer_ok) {
            *out = g_ctx.last_evt;
            g_ctx.last_transfer_ok = 0u;
            if (result != NULL) {
                *result = SPI_LINK_RESULT_OK;
            }
            return true;
        }
    }

    if (result != NULL) {
        *result = (spi_link_result_t)g_ctx.last_result;
    }
    return false;
}
#endif

uint8_t spi_link_result_to_byte(spi_link_result_t r)
{
    return (uint8_t)r;
}

const char *spi_link_result_string(spi_link_result_t r)
{
    static const char *const s[] = {
        "OK", "TIMEOUT", "CRC", "SEQ", "RETRIES", "BUSY", "ARG"
    };
    if ((unsigned)r >= (unsigned)SPI_LINK_RESULT_COUNT) {
        return "?";
    }
    return s[r];
}