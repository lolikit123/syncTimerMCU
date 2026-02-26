import argparse
import re
import sys
import serial
import time

PAT_TB = re.compile(r"TB task_back=(\d+) isr_back=(\d+) task_samp=(\d+) isr_samp=(\d+)")
PAT_TS_MASTER = re.compile(r"TS role=master cs=(\d+) rx_done=(\d+)")
PAT_TS_SLAVE = re.compile(r"TS role=slave cs=(\d+) rx_done=(\d+)")
PAT_LINK_MASTER = re.compile(r"LINK role=master result=(\w+)")
PAT_LINK_SLAVE = re.compile(r"LINK role=slave msg_type=(\d+) seq=(\d+)")
READY_MARKER = "M1_READY"


def open_serial_with_retry(port: str, baud: int, wait_port_sec: int):
    deadline = time.time() + wait_port_sec
    last_exc = None
    while time.time() < deadline:
        try:
            s = serial.Serial(port, baud, timeout=1)
            s.reset_input_buffer()
            return s
        except (serial.SerialException, OSError, FileNotFoundError) as exc:
            last_exc = exc
            time.sleep(0.5)
    raise SystemExit(f"FAIL: UART not ready after {wait_port_sec}s on {port}: {last_exc}")


def wait_for_ready(ser, timeout_sec: int) -> None:
    deadline = time.time() + timeout_sec
    while time.time() < deadline:
        line = ser.readline().decode(errors="ignore").strip()
        if READY_MARKER in line or "READY" in line:
            return
    raise SystemExit(f"FAIL: no {READY_MARKER} within {timeout_sec}s")


def run_phase1_timebase(ser, duration_sec: int, role: str) -> None:
    last = None
    t_end = time.time() + duration_sec
    while time.time() < t_end:
        line = ser.readline().decode(errors="ignore").strip()
        m = PAT_TB.search(line)
        if m:
            last = tuple(map(int, m.groups()))
    if not last:
        sys.exit("FAIL: phase1 timebase – no TB line")
    task_back, isr_back, task_samp, isr_samp = last
    if task_back != 0 or isr_back != 0:
        sys.exit(f"FAIL: phase1 timebase – backsteps task={task_back} isr={isr_back}")
    if task_samp <= 0 or isr_samp <= 0:
        sys.exit("FAIL: phase1 timebase – sample counters don't increase")
    print(f"PASS: phase1 timebase (role={role})")


def run_phase2_timestamps(ser, duration_sec: int, role: str) -> None:
    ts_lines = []
    t_end = time.time() + duration_sec
    while time.time() < t_end:
        line = ser.readline().decode(errors="ignore").strip()
        if role == "master" and PAT_TS_MASTER.search(line):
            ts_lines.append(line)
        elif role == "slave" and PAT_TS_SLAVE.search(line):
            ts_lines.append(line)
    if not ts_lines:
        sys.exit(f"FAIL: phase2 timestamps – no TS line (role={role})")
    for line in ts_lines:
        m = PAT_TS_MASTER.search(line) if role == "master" else PAT_TS_SLAVE.search(line)
        if m:
            cs, rx_done = int(m.group(1)), int(m.group(2))
            if rx_done > 0 and rx_done < cs:
                sys.exit(f"FAIL: phase2 timestamps – rx_done={rx_done} < cs={cs}")
    print(f"PASS: phase2 timestamps (role={role}, lines={len(ts_lines)})")


def run_phase3_link(ser, duration_sec: int, role: str) -> None:
    link_lines = []
    t_end = time.time() + duration_sec
    while time.time() < t_end:
        line = ser.readline().decode(errors="ignore").strip()
        if role == "master" and PAT_LINK_MASTER.search(line):
            link_lines.append(line)
        elif role == "slave" and PAT_LINK_SLAVE.search(line):
            link_lines.append(line)
    if not link_lines:
        sys.exit(f"FAIL: phase3 link – no LINK line (role={role})")
    print(f"PASS: phase3 link (role={role}, lines={len(link_lines)})")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", default="/dev/ttyACM0")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--role", required=True, choices=["master", "slave"])
    ap.add_argument("--wait-port-sec", type=int, default=30)
    ap.add_argument("--wait-ready-sec", type=int, default=30)
    ap.add_argument("--duration-phase1", type=int, default=60, help="Timebase phase (sec)")
    ap.add_argument("--duration-phase2", type=int, default=15, help="Timestamps phase (sec)")
    ap.add_argument("--duration-phase3", type=int, default=10, help="Link phase (sec)")
    args = ap.parse_args()

    ser = open_serial_with_retry(args.port, args.baud, args.wait_port_sec)
    try:
        wait_for_ready(ser, args.wait_ready_sec)
        run_phase1_timebase(ser, args.duration_phase1, args.role)
        run_phase2_timestamps(ser, args.duration_phase2, args.role)
        run_phase3_link(ser, args.duration_phase3, args.role)
    finally:
        ser.close()
    print("PASS: all M1 phases")


if __name__ == "__main__":
    main()