# Roadmap

## Milestone M1 - SPI Timebase & Transport
**Goal:** deterministic local clocks and deterministic SPI communication path.  
**Exit criteria:**
- [ ] monotonic microsecond timebase on all nodes
- [ ] timestamp capture at SPI transaction boundaries (CS edge / ISR entry / DMA completion)
- [ ] SPI frame format defined (`sync_word`, `seq_id`, `msg_type`, payload, `crc16`)
- [ ] timeout, sequence validation, and retry policy implemented

## Milestone M2 - Sync Protocol v1 (Master-Slave, 2 nodes)
**Goal:** basic synchronization over SPI using 2-way timestamp exchange.  
**Exit criteria:**
- [ ] `t1/t2/t3/t4` exchange implemented over SPI cycles
- [ ] offset and path delay estimation running online
- [ ] stable operation in short test (>10 min)

## Milestone M3 - Drift Compensation & Long-Run Stability
**Goal:** long-term stability with drift correction.  
**Exit criteria:**
- [ ] ppm estimator implemented and validated
- [ ] clock steering applied safely (bounded correction, no large time jumps)
- [ ] 1-hour run report generated

## Milestone M4 - Fault Recovery
**Goal:** robust behavior under communication loss and reset scenarios.  
**Exit criteria:**
- [ ] 10% packet loss test passed
- [ ] slave reset recovery verified
- [ ] master reset recovery verified
- [ ] reconvergence time measured and documented

## Milestone M5 - Stress & Jitter Characterization
**Goal:** quantify sync limits under realistic load and delay variation.  
**Exit criteria:**
- [ ] CPU stress test up to 90% load completed
- [ ] random jitter injection (up to 2 ms) and burst delay tests completed
- [ ] jitter vs convergence trade-off documented

## Milestone M6 - Final Report & Optional Extensions
**Goal:** publish engineering-quality results and optional benchmarks.  
**Exit criteria:**
- [ ] plots: offset, jitter, ppm, reconvergence
- [ ] CPU/RAM budget report
- [ ] architecture decisions, limitations, and future work documented

**Extra**
- [ ] UART vs SPI comparison as a non-core extension