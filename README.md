# syncTimeMCU

Deterministic Time Synchronization for Distributed MCU Networks  
**("Poor man's PTP for robotics")**

## Overview

`syncTimeMCU` is an R&D project focused on sub-50 microsecond clock synchronization across low-cost microcontrollers without hardware PTP support.

The target use cases are:
- multi-node sensor fusion
- synchronized ADC sampling
- coordinated motor control
- deterministic distributed logging
- Hard real-time synchronization path is SPI-only; ESP32/Wi-Fi is used for gateway/telemetry, not for clock discipline.

## Project Goals

The system must:
1. Achieve inter-node synchronization accuracy of **<= +/-50 us**
2. Maintain stable synchronization for **at least 1 hour**
3. Recover synchronization after communication loss or node reset

## Non-Functional Requirements

| Parameter | Target |
| --- | --- |
| Synchronization accuracy | <= +/-50 us |
| Communication jitter tolerance | up to 2 ms |
| CPU load (sync subsystem) | <= 60% |
| RAM budget (sync stack/module) | <= 8 KB |
| Packet loss tolerance | 10% |

## Engineering Constraints

- No dedicated PHY timestamping
- No hardware PTP support
- Variable Wi-Fi latency (telemetry path only, not used in hard real-time sync loop)
- Clock drift up to 100 ppm

## Hardware Platform

- **2-3x STM32F411CEU6** (measurement/control nodes)
- **1x ESP32-WROOM-32U** (gateway / Wi-Fi bridge / telemetry)
- Optional: **Orange Pi PC Plus** (logging & analysis)

## Software Stack

- FreeRTOS
- HAL drivers (timers, SPI, interrupts, DMA where possible)
- Custom lightweight sync protocol (2-way timestamp exchange)
- Drift estimation + software clock steering

## High-Level Architecture

- **Master-Slave clock model**
  - One node acts as Master Clock
  - Other nodes are Slave Clocks
- **Layered design**
  1. HAL (timers + transport)
  2. Timebase driver (monotonic microsecond clock)
  3. Sync protocol layer
  4. Drift estimation module
  5. Application layer
  6. Diagnostics/logging

## Synchronization Method (Planned)

- Two-way timestamp exchange (`t1, t2, t3, t4`)
- Offset and path delay estimation
- Drift estimation in ppm
- Clock correction by software steering (and optional prescaler tuning)
- Outlier rejection for burst latency conditions

## Validation Metrics

The project evaluates:
- time offset between nodes
- offset jitter
- drift (ppm)
- reconvergence time after fault
- CPU load
- RAM usage of sync stack/module

## Failure Injection Tests

The system must be tested against:
- 10% packet loss
- slave reset during runtime
- master reset during runtime
- communication burst delay (e.g. +5 ms)
- simulated temperature-induced drift variation

## Current Status

### In progress / TODO
- [X] Define wire protocol frame format
- [ ] Implement hardware timestamping at SPI transaction boundaries
- [ ] Build Master and Slave sync state machines
- [ ] Implement offset + delay estimation
- [ ] Implement drift estimator (ppm) and clock steering
- [ ] Add heartbeat timeout + holdover mode
- [ ] Add diagnostics and structured logging
- [ ] Build long-run stability test harness (1h)
- [ ] Run failure-injection campaign and document results

## Success Criteria (Definition of Done)

The project is considered successful when:
- synchronization remains within **+/-50 us** under nominal operation,
- and recovers to target bounds after defined fault scenarios,
- with acceptable CPU/RAM limits for embedded deployment.

## Future Extensions

- synchronized ADC trigger distribution
- synchronized PWM phase control
- multi-hop / mesh-style time transfer
- IEEE 1588-light behavior benchmarking

---

If you want to contribute, open an issue.