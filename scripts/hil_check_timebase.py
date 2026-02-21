import argparse, re, sys, serial, time
pat = re.compile(r"TB task_back=(\d+) isr_back=(\d+) task_samp=(\d+) isr_samp=(\d+)")

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", default="/dev/ttyACM0")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--duration", type=int, default=60)
    args = ap.parse_args()

    last = None
    t_end = time.time() + args.duration

    ser = serial.Serial(args.port, args.baud, timeout=1)
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