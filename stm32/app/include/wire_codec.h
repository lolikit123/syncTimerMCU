#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// From WireProtocol.md
#define FRAME_HDR_LEN           10u
#define FRAME_CRC_LEN           2u
#define FRAME_MAX_PAYLOAD       32u
#define FRAME_SYNC_WORD         0xA55Au

#define WIRE_FRAME_HDR_LEN     FRAME_HDR_LEN
#define WIRE_FRAME_CRC_LEN     FRAME_CRC_LEN
#define WIRE_FRAME_MAX_PAYLOAD FRAME_MAX_PAYLOAD
#define WIRE_SYNC_WORD         FRAME_SYNC_WORD
#define WIRE_FRAME_MAX         (WIRE_FRAME_HDR_LEN + WIRE_FRAME_MAX_PAYLOAD + WIRE_FRAME_CRC_LEN)
#define SPI_LINK_FRAME_MAX     WIRE_FRAME_MAX

// Offsets
#define WIRE_OFF_SYNC           0u
#define WIRE_OFF_VERSION        2u
#define WIRE_OFF_MSG_TYPE       3u
#define WIRE_OFF_SEQ_ID         4u
#define WIRE_OFF_ACK_SEQ        6u
#define WIRE_OFF_FLAGS          8u
#define WIRE_OFF_PAYLOAD_LEN    9u
#define WIRE_OFF_PAYLOAD        10u

#define WIRE_MSG_SYNC_REQ        0x10u
#define WIRE_MSG_SYNC_RESP       0x11u

// SYNC_RESP payload: t1 at 0, t2 at 8, t3 at 16.
#define WIRE_SYNC_RESP_PAYLOAD_LEN  24u
#define WIRE_SYNC_RESP_OFF_T3       16u

uint16_t wire_crc16(const uint8_t *data, size_t len);
void wire_crc16_append(uint8_t *data, size_t data_len);
bool wire_crc16_verify(const uint8_t *data, size_t data_len);

uint8_t  wire_get_msg_type(const uint8_t *rx);
uint16_t wire_get_seq_id(const uint8_t *rx);
uint8_t  wire_get_payload_len(const uint8_t *rx);
uint16_t wire_read_u16_le(const uint8_t *buf, size_t off);

uint16_t wire_frame_total_len(uint8_t payload_len);

void wire_sync_resp_set_t3(uint8_t *frame_buf, uint64_t t3_us);

void wire_write_u16_le(uint8_t *buf, size_t off, uint16_t val);
void wire_set_seq_id(uint8_t *frame_buf, uint16_t seq_id);