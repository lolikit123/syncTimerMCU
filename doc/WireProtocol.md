# 1.Assumptions

### Transport time: only SPI, one frame on one window.
### Endianness:little-endian for all manybites. (Little-endian is the default memory format for ARM processors)
### Every frame: have seq_id, ack_seq, crc16. ()
### Protocol should work determestic and lightweight(As specification said in README.md in Non-Functional Requirements).

# 2.Frame format
| **Offset** | **Commands** | **Type** | **Size** | **Description** |
| :--------- | :----------- | :------: | :------: | :-------------- |
| 0 | sync_word | u16 | 2 | Constant 0xA55A |
| 2 | version | u8 | 1 | 0x01 |
| 3 | msg_type | u8 | 1 | Type of msg |
| 4 | seq_id | u16 | 2 | Number of frames |
| 6 | ack_seq | u16 | 2 | Last correct response otherwise 0xFFFF |
| 8 | flags | u8 | 1 | Bits of status |
| 9 | payload_len | u8 | 1 | 0-32 |
| 10 | payload | bytes | 0-32 | Data of type msg_type |
| 10+N | crc16 | u16 | 2 | CRC from header+payload |

*FRAME_MAX = 44 B(10B Header+32B payload + 2B CRC).*
*CRC: CRC-16/CCITT-FALSE (poly=0x1021, init=0xFFFF, xrout=0x0000, without reflection).*

# 3.Flags

**Bit 0:** *ACK-REQ* request of accept.
**Bit 1:** *RETRY* try send the same frame.
**Bit 2:** *ERROR* sender send local error.
**Bit 3:** *HOLDOVER* node in hold mode.
**Bits 4-7:** Reserved.

# 4.Type of messege and payload
| **msg_type** | **Name** | **Payload** |
| ------------ | -------- | ----------- |
| 0x01 | HELLO | node_id:u8, role:u8, boot_id:u32, caps:u16|
| 0x10 | SYNC_REQ | t1_us:u64 |
| 0x11 | SYNC_RESP | t1_us:u64, t2_us:u64, t3_us:u64 |
| 0x12 | SYNC_ADJ | offset_corr_ns:i32, drift_ppb:i32, quality:u16 |
| 0x20 | HEARTBEAT | uptime_ms:u32, state:u8, reserved:u8 |
| 0x7F | NACK | err_code:u8, offending_msg:u8, offending_seq:u16 |

## 4.1 Why msg_type numbers like this

### This is not hardware requirement, this is protocol design choice.
### 0x00 is as reserved/invalid for wrong/not response.
### NACK = 0x7F as special error/fallback code, for me it's gonna be easy to spot.

## 4.2 msg_type ranges for future grow

| **Range** | **Group** | **Examples** |
| :--- | :--- | :--- |
| 0x00 | RESERVED | invalid/default value, parser safety |
| 0x01-0x0F | Session/Link control | HELLO, capability, role, boot sync |
| 0x10-0x1F | Time sync core | SYNC_REQ, SYNC_RESP, SYNC_ADJ, future sync msgs |
| 0x20-0x2F | Health/State | HEARTBEAT, holdover state, uptime/status |
| 0x30-0x3F | Servo/Control | clock steering params, limits, mode switch |
| 0x40-0x4F | Diagnostics | counters, jitter stats, delay stats |
| 0x50-0x5F | Fault injection/Test | packet loss test, burst delay test commands |
| 0x60-0x6F | Future topology | multi-node, relay, mesh extension |
| 0x70-0x7E | Detailed errors | BAD_CRC, BAD_LENGTH, SEQ_ERROR, STATE_ERROR |
| 0x7F | NACK (generic) | one common negative response/fallback |
| 0x80-0xBF | Reserved for v2 | bigger protocol without break old v1 |
| 0xC0-0xFF | Experimental/Vendor | lab/debug/private messages |

**Rules for good pratic in this**:
*1) even/odd request/response*
*2) Braking change new version*

# 5.Definition od timestmap t1-t4

### **t1**: Master timestamp on *CS falling edge* with tansition of *SYNC_REQ*.
### **t2**: Slave timestamp on RX-complete (ISR/DMA complete) for that *SYNC_REQ*.
### **t3**: Slave timestamp on *CS falling edge* with transition of response *SYNC_RESP*.
### **t4**: Master timestamp on RX-complete frame *SYNC_RESP*.

# 6.Calculation of offset/delay

### Master calculate when recieve *SYNC_RESP* and has *t4* from local capture.
### offset_us = ((t2 - t1) - (t4 - t3)) / 2
### delay_us = ((t2 - t1) + (t4 - t3)) / 2
### For safe math use signed 64-bit.

*Reject sample if CRC/seq is not valid or if delay_us is outlier.*
*For clock steering use filtered offset (example: median + EMA).*

# 7.Timeout/Retry/Sequence Validation

### T_RESP_TIMEOUT = max(8000us, 4*slot_period_us).
### MAX_RETRY = 3 for same seq_id with *RETRY* bit.
### After MAX_RETRY node go to *HOLDOVER*.
### Duplicate frame accept only when seq_id = last_seq_id and RETRY=1.
### Bad crc16 or bad payload_len for msg_type drop frame and send *NACK*.
### ack_seq should be monotonic increase, if not then sequence error or peer reset.

# 8.States

### Master states: *BOOT -> ACQUIRE -> TRACKING -> HOLDOVER*.
### Slave states: *BOOT -> LISTEN -> TRACKING -> HOLDOVER*.
### Exit form *HOLDOVER*: min. 3 more fix of probe *SYNC_RESP*.

# 9.Error NACK
| **err_code** | **Name** | **Description** |
| :--------- | :----------- | :-------------- |
| 0x01 | BAD_CRC | CRC not match |
| 0x02 | UNKNOWN_MSG | msg_type not supported |
| 0x03 | BAD_LENGTH | payload_len not valid for this msg_type |
| 0x04 | SEQ_ERROR | seq_id/ack_seq broken order |
| 0x05 | BUSY | local queue/buffer full |
| 0x06 | STATE_ERROR | msg not allowed in current state |

