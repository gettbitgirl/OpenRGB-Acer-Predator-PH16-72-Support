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
import time
from pathlib import Path


GUID = "7A4DDFE7-5B5D-40B4-8595-4408E0CC7F56"
DEVICE = "/dev/acer_ec_wmi_probe"
METHOD_SET_LED = 2
METHOD_SET_RGB_KB = 6
METHOD_GET_RGB_KB = 7
METHOD_SET_KB_BACKLIGHT = 20
METHOD_GET_KB_BACKLIGHT = 21
MAX_DATA = 64
REQUEST = struct.Struct("<IIIII64s64s")
IOC_CALL = (3 << 30) | (REQUEST.size << 16) | (0xA5 << 8)
DEFAULT_MASKS = (1, 2, 4, 8, 0x100, 0x200, 0x400, 0x800,
                 0x1000, 0x2000, 0x4000, 0x8000)
DEFAULT_SWEEP_VALUES = (0, 1, 2, 4, 8, 16, 32, 64, 128, 255)


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


def parse_mask(value: str) -> int:
    try:
        parsed = int(value, 0)
    except ValueError as exc:
        raise argparse.ArgumentTypeError("expected an integer mask") from exc
    if not 0 < parsed <= 0xFFFF:
        raise argparse.ArgumentTypeError("mask must be between 1 and 0xffff")
    return parsed


def parse_byte_list(value: str) -> tuple[int, ...]:
    try:
        values = tuple(parse_byte(item.strip()) for item in value.split(","))
    except argparse.ArgumentTypeError:
        raise
    if not values:
        raise argparse.ArgumentTypeError("list must contain at least one byte")
    return values


def parse_mask_list(value: str) -> tuple[int, ...]:
    try:
        values = tuple(parse_mask(item.strip()) for item in value.split(","))
    except argparse.ArgumentTypeError:
        raise
    if not values:
        raise argparse.ArgumentTypeError("list must contain at least one mask")
    return values


def parse_index_list(value: str) -> tuple[int, ...]:
    try:
        values = tuple(int(item.strip(), 0) for item in value.split(","))
    except ValueError as exc:
        raise argparse.ArgumentTypeError("expected comma-separated indexes") from exc
    if not values or any(index < 0 or index >= 16 for index in values):
        raise argparse.ArgumentTypeError("indexes must be between 0 and 15")
    return values


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
    state = call(args.device, METHOD_GET_KB_BACKLIGHT, struct.pack("<Q", 1))
    print(f"Readback ({len(state)} bytes): {state.hex(' ') if state else '<empty>'}")


def rgbkb_value(mask: int, color: tuple[int, int, int]) -> int:
    """Pack a SetGamingRgbKb value.

    This is deliberately kept separate from the method-20 frame: on the
    sibling PH16-71 capture, method 20 committed mode/brightness while this
    method carried the actual RGB value for one of three lightbar zones. The
    Windows service also constructs masks in the 0x100..0x8000 range for its
    shared EC-keyboard helper, so those masks are accepted for experiments.
    """
    if not 0 < mask <= 0xFFFF:
        raise ValueError("mask must be between 1 and 0xffff")
    red, green, blue = color
    return ((red << 8) | (green << 16) | (blue << 24) |
            (0x08 << 32) | (mask << 40))


def write_rgbkb(args: argparse.Namespace) -> None:
    require_arm(args)
    value = rgbkb_value(args.mask, args.color)
    payload = struct.pack("<Q", value)
    print(f"WMI method {METHOD_SET_RGB_KB}, mask {args.mask}, value 0x{value:016x}, "
          f"bytes: {payload.hex(' ')}")
    output = call(args.device, METHOD_SET_RGB_KB, payload)
    print(f"ACPI reply: {output.hex(' ') if output else '<empty>'}")


def read_rgbkb(args: argparse.Namespace) -> None:
    for mask in args.masks:
        output = call(args.device, METHOD_GET_RGB_KB,
                      struct.pack("<Q", mask), output_capacity=8)
        print(f"WMI method {METHOD_GET_RGB_KB}, mask {mask}, reply: "
              f"{output.hex(' ') if output else '<empty>'}")


def read_rgbkb_raw(device: str, mask: int) -> bytes:
    return call(device, METHOD_GET_RGB_KB, struct.pack("<Q", mask),
                output_capacity=8)


def read_backlight_raw(device: str) -> bytes:
    return call(device, METHOD_GET_KB_BACKLIGHT, struct.pack("<Q", 1),
                output_capacity=16)


def changed_offsets(before: bytes, after: bytes) -> list[int]:
    return [index for index, (old, new) in enumerate(zip(before, after))
            if old != new]


def auto_sweep(args: argparse.Namespace) -> None:
    """Run bounded differential tests and restore read-back baselines."""
    require_arm(args)

    baseline_frame = read_backlight_raw(args.device)
    if len(baseline_frame) != 16:
        raise SystemExit(
            f"method 21 returned {len(baseline_frame)} bytes; refusing to write "
            "because the baseline cannot be restored")

    baseline_rgb = {}
    for mask in args.masks:
        value = read_rgbkb_raw(args.device, mask)
        if len(value) != 8:
            raise SystemExit(
                f"method 7 mask 0x{mask:04x} returned {len(value)} bytes; "
                "refusing to write because the baseline cannot be restored")
        baseline_rgb[mask] = value

    def restore() -> None:
        call(args.device, METHOD_SET_KB_BACKLIGHT, baseline_frame)
        for raw in baseline_rgb.values():
            call(args.device, METHOD_SET_RGB_KB, raw)

    print(f"Baseline method-21: {baseline_frame.hex(' ')}")
    print("Baseline method-7: " + ", ".join(
        f"0x{mask:04x}={baseline_rgb[mask].hex(' ')}" for mask in args.masks))

    try:
        for mask in args.masks:
            restore()
            control = {tested: read_rgbkb_raw(args.device, tested)
                       for tested in args.masks}
            candidate = struct.pack("<Q", rgbkb_value(mask, args.color))
            call(args.device, METHOD_SET_RGB_KB, candidate)
            after = {tested: read_rgbkb_raw(args.device, tested)
                     for tested in args.masks}
            changed = [tested for tested in args.masks
                       if after[tested] != control[tested]]
            if changed:
                detail = ", ".join(
                    f"0x{tested:04x}={after[tested].hex(' ')}" for tested in changed)
                print(f"RGBKB mask 0x{mask:04x}: CHANGED {detail}")
            else:
                print(f"RGBKB mask 0x{mask:04x}: no readback change")
            restore()
            time.sleep(args.delay)

        for index in args.indices:
            for value in args.values:
                restore()
                control = read_backlight_raw(args.device)
                candidate = bytearray(baseline_frame)
                candidate[index] = value
                call(args.device, METHOD_SET_KB_BACKLIGHT, bytes(candidate))
                after = read_backlight_raw(args.device)
                changed = changed_offsets(control, after)
                if changed:
                    print(f"FRAME byte {index} = 0x{value:02x}: CHANGED "
                          f"offsets {changed}; readback {after.hex(' ')}")
                else:
                    print(f"FRAME byte {index} = 0x{value:02x}: no readback change")
                restore()
                time.sleep(args.delay)
    finally:
        print("Restoring complete baseline")
        restore()


def rear_static(args: argparse.Namespace) -> None:
    """Try the captured PH16-71 static-color commit sequence.

    This is an experiment for PH16-72, not a claim that the current Windows
    DLL uses this path.  The sequence is repeated three times because that is
    what the captured PredatorSense replacement did on the sibling chassis.
    """
    require_arm(args)
    arm = bytes((0x10, 0, 0, 0, 0, 0, 0xff, 0xff, 0xff, 0x15, 0, 0))
    commit = bytes((0, 0, args.brightness, 0, 0, 0, 0, 0, 0x03, 0x02,
                    0, 0, 0, 0, 0, 0))
    print(f"WMI method {METHOD_SET_LED}, arm: {arm.hex(' ')}")
    print(f"WMI method {METHOD_SET_KB_BACKLIGHT}, commit: {commit.hex(' ')}")
    for iteration in range(3):
        arm_reply = call(args.device, METHOD_SET_LED, arm)
        commit_reply = call(args.device, METHOD_SET_KB_BACKLIGHT, commit)
        print(f"commit {iteration + 1}: arm reply "
              f"{arm_reply.hex(' ') if arm_reply else '<empty>'}; "
              f"commit reply {commit_reply.hex(' ') if commit_reply else '<empty>'}")
        for mask in args.masks:
            value = rgbkb_value(mask, args.color)
            payload = struct.pack("<Q", value)
            reply = call(args.device, METHOD_SET_RGB_KB, payload)
            print(f"  method {METHOD_SET_RGB_KB}, mask {mask}, bytes "
                  f"{payload.hex(' ')}, reply {reply.hex(' ') if reply else '<empty>'}")
        if iteration != 2:
            time.sleep(args.delay)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--device", default=DEVICE)
    subparsers = parser.add_subparsers(dest="command", required=True)

    read = subparsers.add_parser("read", help="read the firmware's current 16-byte state")
    read.set_defaults(kind="read")

    for name, mode, help_text in (
        ("static", 0x00, "send a static-color frame"),
        ("breathing", 0x01, "send a breathing effect frame"),
        ("neon", 0x02, "send a neon effect frame"),
        ("wave", 0x03, "send a wave effect frame"),
        ("twinkling", 0x07, "send a twinkling effect frame"),
        ("direct", 0xFF, "send the unimplemented Windows Direct-mode candidate"),
    ):
        command = subparsers.add_parser(name, help=help_text)
        command.add_argument("--color", type=parse_color, default=(255, 0, 0))
        command.add_argument("--brightness", type=parse_byte, default=100)
        command.add_argument("--speed", type=parse_byte, default=5)
        command.add_argument("--direction", type=parse_byte, default=1)
        command.add_argument("--tail", type=parse_byte, default=0x02)
        command.add_argument("--arm", action="store_true")
        command.set_defaults(kind="write", mode=mode)

    off = subparsers.add_parser("off", help="send an armed black static frame")
    off.add_argument("--arm", action="store_true")
    off.set_defaults(kind="write", mode=0, color=(0, 0, 0), brightness=0,
                     speed=5, direction=1, tail=0x02)

    rgbkb = subparsers.add_parser(
        "rgbkb", help="send one captured SetGamingRgbKb zone-color value")
    rgbkb.add_argument("--mask", type=parse_mask, required=True,
                       help="zone bitmask, e.g. 1, 2, 4 or 0x100..0x8000")
    rgbkb.add_argument("--color", type=parse_color, default=(255, 0, 0))
    rgbkb.add_argument("--arm", action="store_true")
    rgbkb.set_defaults(kind="rgbkb")

    rgbkb_read = subparsers.add_parser(
        "rgbkb-read", help="read SetGamingRgbKb state for keyboard/lightbar masks")
    rgbkb_read.add_argument(
        "--masks", type=parse_mask_list,
        default=(1, 2, 4, 8, 0x100, 0x200, 0x400, 0x800,
                 0x1000, 0x2000, 0x4000, 0x8000),
        help="comma-separated masks; defaults include service zone bits 8..15")
    rgbkb_read.set_defaults(kind="rgbkb_read")

    rear = subparsers.add_parser(
        "rear-static",
        help="try the captured arm/commit/three-zone static-color sequence")
    rear.add_argument("--color", type=parse_color, default=(255, 0, 0))
    rear.add_argument("--brightness", type=parse_byte, default=100)
    rear.add_argument("--delay", type=float, default=0.065,
                      help="seconds between repeated commits (default: 0.065)")
    rear.add_argument("--masks", type=parse_mask_list, default=(1, 2, 4),
                      help="comma-separated method-6 masks (default: 1,2,4)")
    rear.add_argument("--arm", action="store_true")
    rear.set_defaults(kind="rear_static")

    sweep = subparsers.add_parser("sweep", help="test one candidate reserved/address byte")
    sweep.add_argument("index", type=int, choices=range(16))
    sweep.add_argument("--value", type=parse_byte, default=None,
                       help="single value to test (legacy form)")
    sweep.add_argument("--values", type=parse_byte_list,
                       help="comma-separated values to test in one run")
    sweep.add_argument("--mode", type=parse_byte, default=0xFF)
    sweep.add_argument("--color", type=parse_color, default=(255, 0, 0))
    sweep.add_argument("--brightness", type=parse_byte, default=100)
    sweep.add_argument("--speed", type=parse_byte, default=5)
    sweep.add_argument("--direction", type=parse_byte, default=1)
    sweep.add_argument("--tail", type=parse_byte, default=0x02)
    sweep.add_argument("--pause", type=float, default=0.10,
                       help="seconds between candidate writes")
    sweep.add_argument("--restore", action="store_true",
                       help="restore the baseline frame after testing")
    sweep.add_argument("--arm", action="store_true")
    sweep.set_defaults(kind="sweep")

    rgbkb_sweep = subparsers.add_parser(
        "rgbkb-sweep",
        help="write/read candidate SetGamingRgbKb masks one at a time")
    rgbkb_sweep.add_argument(
        "--masks", type=parse_mask_list,
        default=(1, 2, 4, 8, 0x100, 0x200, 0x400, 0x800,
                 0x1000, 0x2000, 0x4000, 0x8000),
        help="comma-separated masks; defaults include service zone bits 8..15")
    rgbkb_sweep.add_argument("--color", type=parse_color, default=(255, 0, 0))
    rgbkb_sweep.add_argument("--delay", type=float, default=0.10,
                             help="seconds between candidate writes")
    rgbkb_sweep.add_argument("--arm", action="store_true")
    rgbkb_sweep.set_defaults(kind="rgbkb_sweep")

    auto = subparsers.add_parser(
        "auto-sweep",
        help="test masks and frame bytes using readback-only change detection")
    auto.add_argument(
        "--masks", type=parse_mask_list, default=DEFAULT_MASKS,
        help="comma-separated method-7 masks (default: keyboard and service candidates)")
    auto.add_argument(
        "--indices", type=parse_index_list, default=range(10, 16),
        help="comma-separated frame byte indexes (default: 10..15)")
    auto.add_argument(
        "--values", type=parse_byte_list, default=DEFAULT_SWEEP_VALUES,
        help="comma-separated byte values (default: 0,1,2,4,...,255)")
    auto.add_argument("--color", type=parse_color, default=(255, 0, 0),
                      help="candidate RGB color for method-6 writes")
    auto.add_argument("--delay", type=float, default=0.10,
                      help="seconds between restored test cases")
    auto.add_argument("--arm", action="store_true")
    auto.set_defaults(kind="auto_sweep")

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
        if args.value is not None and args.values is not None:
            raise SystemExit("use either --value or --values, not both")
        values = args.values
        if values is None:
            values = (1 if args.value is None else args.value,)
        baseline = frame(args.mode, args.speed, args.brightness, args.direction,
                         args.color, args.tail)
        for offset, value in enumerate(values):
            payload = bytearray(baseline)
            payload[args.index] = value
            print(f"Sweep {offset + 1}/{len(values)}: byte {args.index} = 0x{value:02x}")
            write_frame(args, bytes(payload))
            if offset + 1 != len(values):
                time.sleep(args.pause)
        if args.restore:
            print("Restoring baseline frame")
            write_frame(args, baseline)
        return 0

    if args.kind == "rgbkb":
        write_rgbkb(args)
        return 0

    if args.kind == "rgbkb_read":
        read_rgbkb(args)
        return 0

    if args.kind == "rgbkb_sweep":
        require_arm(args)
        for offset, mask in enumerate(args.masks):
            value = rgbkb_value(mask, args.color)
            payload = struct.pack("<Q", value)
            print(f"Sweep {offset + 1}/{len(args.masks)}: mask 0x{mask:04x}, "
                  f"value 0x{value:016x}, bytes: {payload.hex(' ')}")
            output = call(args.device, METHOD_SET_RGB_KB, payload)
            print(f"  write reply: {output.hex(' ') if output else '<empty>'}")
            readback = call(args.device, METHOD_GET_RGB_KB,
                            struct.pack("<Q", mask), output_capacity=8)
            print(f"  readback: {readback.hex(' ') if readback else '<empty>'}")
            if offset + 1 != len(args.masks):
                time.sleep(args.delay)
        return 0

    if args.kind == "auto_sweep":
        auto_sweep(args)
        return 0

    if args.kind == "rear_static":
        rear_static(args)
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
