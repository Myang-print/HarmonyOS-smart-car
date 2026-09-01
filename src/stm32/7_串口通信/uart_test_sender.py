"""Send binary motor-control frames to the STM32 over a USB-TTL adapter."""

from __future__ import annotations

import argparse
import time

try:
    import serial
except ImportError as exc:
    raise SystemExit("Missing pyserial. Install it with: python -m pip install pyserial") from exc


FRAMES = {
    "stop": bytes.fromhex("FC 00 00 00 00 FD"),
    "forward": bytes.fromhex("FC 00 64 00 64 FD"),
    "backward": bytes.fromhex("FC 01 64 01 64 FD"),
    "left": bytes.fromhex("FC 00 32 00 96 FD"),
    "right": bytes.fromhex("FC 00 96 00 32 FD"),
    "spin_left": bytes.fromhex("FC 01 64 00 64 FD"),
    "spin_right": bytes.fromhex("FC 00 64 01 64 FD"),
    "invalid": bytes.fromhex("FC 02 64 00 64 FD"),
    "ascii": b"FC00640064FD",
}

STOP_FRAME = FRAMES["stop"]


def write_frame(port: serial.Serial, frame: bytes) -> None:
    port.write(frame)
    port.flush()
    print("TX:", frame.hex(" ").upper())


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Send the STM32 car protocol as real binary bytes."
    )
    parser.add_argument("port", help="USB-TTL serial port, for example COM5")
    parser.add_argument("action", choices=[*FRAMES, "timeout"])
    parser.add_argument(
        "--duration",
        type=float,
        default=3.0,
        help="test duration in seconds; default: 3",
    )
    parser.add_argument(
        "--interval",
        type=float,
        default=0.5,
        help="frame refresh interval in seconds; default: 0.5",
    )
    args = parser.parse_args()

    if args.duration <= 0 or args.interval <= 0:
        parser.error("--duration and --interval must be positive")

    with serial.Serial(args.port, 115200, timeout=0.2) as port:
        time.sleep(0.2)
        try:
            if args.action == "timeout":
                print("Sending forward once, then waiting for the 2-second failsafe.")
                write_frame(port, FRAMES["forward"])
                time.sleep(args.duration)
            else:
                deadline = time.monotonic() + args.duration
                while time.monotonic() < deadline:
                    write_frame(port, FRAMES[args.action])
                    time.sleep(args.interval)
        finally:
            for _ in range(3):
                write_frame(port, STOP_FRAME)
                time.sleep(0.05)


if __name__ == "__main__":
    main()
