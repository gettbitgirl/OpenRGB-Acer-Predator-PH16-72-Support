# Acer EC hidden-path audit

This is a static audit of the shipped PredatorSense 5.0.1463 binaries. It is
intended to identify software paths worth probing without requiring a Windows
installation. Addresses are image virtual addresses in the original PE files.

## Rear controller DLL

`AcerECLightbarController.dll` contains these WMI method strings and callsites:

| Method | Relevant function | Meaning |
| --- | --- | --- |
| `SetGamingLEDColor` | `0x180003230` | EC-logo packed 64-bit color path |
| `SetGamingLEDBehavior` | `0x180003230` | EC-logo behavior path |
| `SetGamingKBBacklight` | `0x180011500`, called by rear `0x18000fa00` | 16-byte rear effect frame |

There are no `SetGamingRgbKb` references in this DLL. The rear controller's
`UpdateLEDs`, `UpdateZoneLEDs`, and `UpdateSingleLED` virtual slots do not issue
an alternate color-array write; only `UpdateMode` reaches
`SetGamingKBBacklight`. This rules out a dormant per-LED path in this DLL, but
not an undocumented WMI command implemented by the EC firmware.

## Lighting service paths

`AcerLightingService.exe` has a separate shared `ECKeyboard` implementation.
Its relevant functions are:

| Function | Static address | WMI method | Recovered behavior |
| --- | ---: | --- | --- |
| `ECKeyboard::SetKeyboardBacklightEffect` | `0x14000b190` | `SetGamingKBBacklight` | Builds and logs a 10-byte keyboard-effect payload, then calls the common WMI helper at `0x14001e8a0`. |
| `ECKeyboard::SetGamingLEDBehavior` | `0x14000b3c0` | `SetGamingRgbKb` | Iterates color entries, packs RGB bytes, and calls the common helper at `0x14001e6f0`. |
| common `SetGamingRgbKb` helper | `0x14001e6f0` | `SetGamingRgbKb` | Sends a 64-bit `gmInput`. |
| common `SetGamingKBBacklight` helper | `0x14001e8a0` | `SetGamingKBBacklight` | Sends a SAFEARRAY payload and retries after a 30 ms failure delay. |

The service's `SetGamingLEDBehavior` packer explicitly sets mask bits in the
`0x100` through `0x8000` range. It selects those bits from a zone/device
index, then packs RGB bytes before calling `SetGamingRgbKb`. This is the reason
the probe now tests those masks. The code is labelled `ECKeyboard`, so these
bits are candidates only; the binary does not establish that they address the
rear bar.

The same function logs `deviceIndex`, `ledcount`, `brightness`,
`lightbarSytle`, and RGB values before building behavior data. That makes it a
useful target for future binary emulation or Windows-side API hooking, but it
does not prove that PredatorSense routes the rear-lightbar object through this
keyboard helper.

## Probe coverage

The guarded probe now supports two bounded experiments:

* `sweep INDEX --values ... --restore` changes one byte of the 16-byte
  `SetGamingKBBacklight` frame at a time and reads method 21 after every write.
* `rgbkb-sweep --masks ...` sends one `SetGamingRgbKb` value per candidate mask
  and immediately reads method 7 for that mask. The default mask set includes
  both the PH16-71 captures (`1`, `2`, `4`, `8`) and the service-derived
  `0x100..0x8000` bits.

All writes still require `--arm`; reads do not. A response is meaningful only
if a single physical segment changes while the others remain unchanged, and
the final state should be restored with the `off` command.
