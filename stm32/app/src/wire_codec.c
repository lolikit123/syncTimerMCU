#include "wire_codec.h"

static uint16_t crc16_ccitt_update(uint16_t crc, uint8_t byte)
{
    crc ^= (uint16_t)byte << 8;
    for (int i = 0; i < 8; i++) {
        if (crc & 0x8000u) {
            crc = (crc << 1) ^ 0x1021u;
        } else {
            crc = crc << 1;
        }
    }
    return crc;
}

uint16_t wire_crc16(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFFu;
    for (size_t i = 0; i < len; i++) {
        crc = crc16_ccitt_update(crc, data[i]);
    }
    return crc;
}

void wire_crc16_append(uint8_t *data, size_t data_len)
{
    uint16_t crc = wire_crc16(data, data_len);
    data[data_len]     = (uint8_t)(crc & 0xFFu);
    data[data_len + 1] = (uint8_t)((crc >> 8) & 0xFFu);
}

bool wire_crc16_verify(const uint8_t *data, size_t data_len)
{
    if (data_len < WIRE_FRAME_CRC_LEN) {
        return false;
    }
    size_t payload_len = data_len - WIRE_FRAME_CRC_LEN;
    uint16_t crc = wire_crc16(data, payload_len);
    uint16_t recv = (uint16_t)data[payload_len] | ((uint16_t)data[payload_len + 1] << 8);
    return crc == recv;
}

uint8_t wire_get_msg_type(const uint8_t *rx)
{
    return rx[WIRE_OFF_MSG_TYPE];
}

uint16_t wire_get_seq_id(const uint8_t *rx)
{
    return wire_read_u16_le(rx, WIRE_OFF_SEQ_ID);
}

uint8_t wire_get_payload_len(const uint8_t *rx)
{
    return rx[WIRE_OFF_PAYLOAD_LEN];
}

uint16_t wire_read_u16_le(const uint8_t *buf, size_t off)
{
    return (uint16_t)buf[off] | ((uint16_t)buf[off + 1] << 8);
}

uint16_t wire_frame_total_len(uint8_t payload_len)
{
    return (uint16_t)WIRE_FRAME_HDR_LEN + (uint16_t)payload_len + (uint16_t)WIRE_FRAME_CRC_LEN;
}

void wire_sync_resp_set_t3(uint8_t *frame_buf, uint64_t t3_us)
{
    uint8_t *p = frame_buf + WIRE_OFF_PAYLOAD + WIRE_SYNC_RESP_OFF_T3;
    for (uint8_t i = 0; i < 8u; i++) {
        p[i] = (uint8_t)(t3_us >> (8u * i));
    }
}

void wire_write_u16_le(uint8_t *buf, size_t off, uint16_t val) {
    buf[off]     = (uint8_t)(val & 0xFFu);
    buf[off + 1] = (uint8_t)((val >> 8) & 0xFFu);
}
void wire_set_seq_id(uint8_t *frame_buf, uint16_t seq_id) {
    wire_write_u16_le(frame_buf, WIRE_OFF_SEQ_ID, seq_id);
}