#!/usr/bin/env python3

import sys
import time
import math

INTERVAL = 1.0 / 30.0


def to_bytes(r: float, g: float, b: float) -> bytes:
    return bytes([int(r * 255), int(g * 255), int(b * 255)])


def hsv_to_rgb(h: float, s: float = 1.0, v: float = 1.0) -> tuple:
    """Convert HSV to RGB (h in 0..1)"""
    c = v * s
    x = c * (1 - abs((h * 6) % 2 - 1))
    m = v - c

    if h < 1 / 6:
        r, g, b = c, x, 0
    elif h < 2 / 6:
        r, g, b = x, c, 0
    elif h < 3 / 6:
        r, g, b = 0, c, x
    elif h < 4 / 6:
        r, g, b = 0, x, c
    elif h < 5 / 6:
        r, g, b = x, 0, c
    else:
        r, g, b = c, 0, x

    return (r + m, g + m, b + m)


def main():
    leds = int(input())
    speed = 0.2
    brightness = 0.95

    print(f"Starting rainbow animation for {leds} LEDs...", file=sys.stderr)

    t = 0.0
    while True:
        data = bytearray()

        for i in range(leds):
            hue = (t * speed + i / leds) % 1.0
            r, g, b = hsv_to_rgb(hue, s=1.0, v=brightness)
            data.extend(to_bytes(r, g, b))

        sys.stdout.buffer.write(b"\x00" + data)
        sys.stdout.buffer.flush()

        t += INTERVAL
        time.sleep(INTERVAL)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt, BrokenPipeError, OSError:
        print("\nAnimation stopped.", file=sys.stderr)
