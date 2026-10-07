#!/usr/bin/env python3
"""Guarded Acer PH16-72 EC WMI lightbar probe.

The kernel module in this directory is intentionally root-only.  This client
never writes unless a write subcommand includes --arm.
"""

from __future__ import annotations

import argparse
import fcntl
import os
import struct
import sys
from pathlib import Path


GUID = "7A4DDFE7-5B5D-40B4-8595-4408E0CC7F56"
DEVICE = "/dev/acer_ec_wmi_probe"
METHOD_SET_LED = 2
METHOD_SET_KB_BACKLIGHT = 20
METHOD_GET_KB_BACKLIGHT = 21
MAX_DATA = 64
REQUEST = struct.Struct("<IIIII64s64s")
IOC_CALL = (3 << 30) | (REQUEST.size << 16) | (0xA5 << 8)


def parse_color(value: str) -> tuple[int, int, int]:
    value = value.removeprefix("#")
    if len(value) != 6:
        raise argparse.ArgumentTypeError("color must be six hexadecimal digits")
    try:
        return tuple(bytes.fromhex(value))  # type: ignore[return-value]
    except ValueError as exc:
        raise argparse.ArgumentTypeError("color must be six hexadecimal digits") from exc


def parse_byte(value: str) -> int:
    try:
        parsed = int(value, 0)
    except ValueError as exc:
        raise argparse.ArgumentTypeError("expected an integer") from exc
    if not 0 <= parsed <= 255:
        raise argparse.ArgumentTypeError("value must be between 0 and 255")
    return parsed


def frame(mode: int, speed: int, brightness: int, direction: int,
          color: tuple[int, int, int], tail: int = 0x02) -> bytes:
    red, green, blue = color
    if not 0 <= mode <= 255:
        raise ValueError("mode must fit in one byte")
    if not 0 <= speed <= 255 or not 0 <= brightness <= 255:
        raise ValueError("speed and brightness must fit in one byte")
    if not 0 <= direction <= 254:
        raise ValueError("direction must be between 0 and 254")

    # This is the exact layout recovered from AcerECLightbarController.dll:
    # [mode, speed, brightness, 0, direction + 1, R, G, B, 3, 2, 0...].
    return bytes((mode, speed, brightness, 0, direction + 1,
                  red, green, blue, 0x03, tail, 0, 0, 0, 0, 0, 0))


def call(device: str, method: int, payload: bytes, output_capacity: int = MAX_DATA) -> bytes:
    if len(payload) > MAX_DATA:
        raise ValueError("payload is too large")
    if output_capacity > MAX_DATA:
        raise ValueError("output buffer is too large")

    request = bytearray(REQUEST.pack(method, len(payload), output_capacity, 0, 0,
                                     payload.ljust(MAX_DATA, b"\0"), b"\0" * MAX_DATA))
    with open(device, "r+b", buffering=0) as handle:
        fcntl.ioctl(handle.fileno(), IOC_CALL, request, True)

    method_id, input_len, capacity, output_len, acpi_status, _, output = REQUEST.unpack(request)
    if acpi_status:
        raise OSError(f"WMI method {method_id} failed with ACPI status 0x{acpi_status & 0xffffffff:08x}")
    return output[:output_len]


def require_arm(args: argparse.Namespace) -> None:
    if not args.arm:
        raise SystemExit("Refusing to write. Repeat the command with --arm after confirming the frame.")


def write_frame(args: argparse.Namespace, payload: bytes) -> None:
    require_arm(args)
    print(f"WMI method {METHOD_SET_KB_BACKLIGHT}, frame: {payload.hex(' ')}")
    output = call(args.device, METHOD_SET_KB_BACKLIGHT, payload)
    print(f"ACPI reply: {output.hex(' ') if output else '<empty>'}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--device", default=DEVICE)
    subparsers = parser.add_subparsers(dest="command", required=True)

    read = subparsers.add_parser("read", help="read the firmware's current 16-byte state")
    read.set_defaults(kind="read")

    for name, mode, help_text in (
        ("static", 0x00, "send a static-color frame"),
        ("direct", 0xFF, "send the unimplemented Windows Direct-mode candidate"),
    ):
        command = subparsers.add_parser(name, help=help_text)
        command.add_argument("--color", type=parse_color, default=(255, 0, 0))
        command.add_argument("--brightness", type=parse_byte, default=100)
        command.add_argument("--speed", type=parse_byte, default=0)
        command.add_argument("--direction", type=parse_byte, default=0)
        command.add_argument("--tail", type=parse_byte, default=0x02)
        command.add_argument("--arm", action="store_true")
        command.set_defaults(kind="write", mode=mode)

    off = subparsers.add_parser("off", help="send an armed black static frame")
    off.add_argument("--arm", action="store_true")
    off.set_defaults(kind="write", mode=0, color=(0, 0, 0), brightness=0,
                     speed=0, direction=0, tail=0x02)

    sweep = subparsers.add_parser("sweep", help="test one candidate reserved/address byte")
    sweep.add_argument("index", type=int, choices=range(10, 16))
    sweep.add_argument("--value", type=parse_byte, default=1)
    sweep.add_argument("--mode", type=parse_byte, default=0xFF)
    sweep.add_argument("--color", type=parse_color, default=(255, 0, 0))
    sweep.add_argument("--brightness", type=parse_byte, default=100)
    sweep.add_argument("--arm", action="store_true")
    sweep.set_defaults(kind="sweep")

    args = parser.parse_args()
    if not Path(args.device).exists():
        raise SystemExit(f"{args.device} is missing; build/load acer_ec_wmi_probe.ko first")
    if not Path(f"/sys/bus/wmi/devices/{GUID}-8").exists():
        raise SystemExit(f"Acer WMI GUID {GUID} is not present on this machine")

    if args.kind == "read":
        output = call(args.device, METHOD_GET_KB_BACKLIGHT, struct.pack("<Q", 1))
        print(f"Current WMI state ({len(output)} bytes): {output.hex(' ')}")
        return 0

    if args.kind == "sweep":
        payload = bytearray(frame(args.mode, 0, args.brightness, 0, args.color))
        payload[args.index] = args.value
        write_frame(args, bytes(payload))
        return 0

    write_frame(args, frame(args.mode, args.speed, args.brightness,
                             args.direction, args.color, args.tail))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except OSError as error:
        print(f"error: {error}", file=sys.stderr)
        raise SystemExit(1)
