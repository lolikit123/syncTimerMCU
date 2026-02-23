import argparse, re, sys, serial, time
pat = re.compile(r"TB task_back=(\d+) isr_back=(\d+) task_samp=(\d+) isr_samp=(\d+)")

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

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", default="/dev/ttyACM0")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--duration", type=int, default=60)
    ap.add_argument("--wait-port-sec", type=int, default=30)
    args = ap.parse_args()

    last = None

    ser = open_serial_with_retry(args.port, args.baud, args.wait_port_sec)
    t_end = time.time() + args.duration
    try:
        while time.time() < t_end:
            line = ser.readline().decode(errors="ignore").strip()
            m = pat.search(line)
            if m:
                last = tuple(map(int, m.groups()))
    finally:
        ser.close()

    if not last:
        sys.exit("FAIL: no TB line")

    task_back, isr_back, task_samp, isr_samp = last
    if task_back != 0 or isr_back != 0:
        sys.exit(f"FAIL: backsteps task={task_back} isr={isr_back}")
    if task_samp <= 0 or isr_samp <= 0:
        sys.exit("FAIL: sample counters dosen't increase")

    print(f"PASS: task_back={task_back} isr_back={isr_back} task_samp={task_samp} isr_samp={isr_samp}")

if __name__ == "__main__":
    main()