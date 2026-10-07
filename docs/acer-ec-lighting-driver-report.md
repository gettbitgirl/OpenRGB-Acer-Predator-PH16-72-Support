# Acer PH-72 EC logo and lightbar driver report

This report records the Windows implementation found in
`PredatorSense_5.0.1463/AgentService/driver/AcerECLightbarController.dll`.
It covers the EC logo below the numpad (`AcerECLogoLED`) and the rear lightbar
(`AcerECRearLightbar`). Values below are from disassembly; no packet values are
guessed.

## Controller split

`RGBDevice.ini` identifies both devices as EC controllers rather than USB HID:

| Device | Windows class | Factory | Transport |
| --- | --- | --- | --- |
| Rear lightbar | `AcerECLightbar` | `CreateAcerECRearLightbarControllerClassInstance` | WMI `AcerGamingFunction` |
| Front/EC logo | `AcerECLogoLED` | `CreateAcerECLogoLEDControllerClassInstance` | WMI `AcerGamingFunction` |

The DLL imports COM/WMI (`CoCreateInstance`, `CoSetProxyBlanket`, OLE Automation
`VARIANT`/`SAFEARRAY` functions). It does not use HID for either device.

## LED topology

The EC-logo RGB controller constructor (`0x18000fa9d`) creates one LED entry
named `Logo LED`; its Direct mode has one color slot. The logo is therefore one
logical RGB LED, not an addressable matrix.

The rear-lightbar constructor creates the rear-lightbar device and then calls
`SetupZones()` at `0x18000f5e0`. That routine copies an LED count from the
factory/model object (`[this+0x1c0]+0x20`), stores it at `[this+0x1c8]`, and
creates one logical LED entry for each count. It labels a count of five or
more `Infinity Mirror LED`, otherwise `Rear LightBar LED`. The PH16-72 model
documentation identifies the physical bar as seven positions, but the DLL
itself is parameterized rather than hard-coding seven.

The mode/color API is group-based: the WMI payload selects the device group
and does not contain a per-LED frame buffer. The existence of multiple logical
LED entries therefore does not, by itself, prove that the EC accepts separate
colors for those entries.

The rear constructor also registers a `Direct` mode, but this is a controller
capability flag rather than proof of a working direct stream. Its mode record
contains value `0xff`, flags `0x30`, one color slot, brightness default `100`,
and a `50` default in the remaining mode parameter field (`0x18000d332`-
`0x18000d374`).

## WMI calls

The EC-logo `SetMode` routine is `0x180003230`. It issues two WMI method
queries against:

```
SELECT * FROM AcerGamingFunction
```

The color call is named `SetGamingLEDColor`; the behavior call is named
`SetGamingLEDBehavior`. The routine retries a failed method invocation after a
30 ms sleep.

### Color payload

At `0x1800034cf`, the DLL constructs the 64-bit value passed as `gmInput` to
`SetGamingLEDColor`. The assembly is equivalent to:

```c
uint64_t input = direction_or_speed_field;
input = (input << 8) | real_mode;
input = (input << 8) | blue;
input = (input << 8) | green;
input = (input << 8) | red;
input = (input << 8) | 0x01;
```

The final low byte is literally `0x01` (`or rbx, 1` at `0x1800034f9`). The
debug format immediately before packing is:

```
input mode = %d, real mode=%d, speed=%d, direction=%d,
brightness=%d, color=(0X%X,0X%X,0X%X)
```

The RGB byte order in the packed value is therefore unambiguously
`... blue, green, red, 0x01` (least-significant byte first when serialized as
little-endian WMI data). The exact interpretation of the two leading bytes is
the controller ABI's speed/direction/brightness field; the DLL passes the
values shown in its own format string and must be mirrored, not replaced by a
new OpenRGB convention.

### Behavior payload

At `0x180003692`, the behavior input is constructed as:

```c
uint64_t input = ((uint64_t)behavior_parameter << 16) | 0x01;
```

The low byte is again the literal group/device selector `0x01`. The behavior
parameter is the translated firmware mode byte (`real_mode`) selected from the
mode lookup table at `0x180016020`, indexed by the PredatorSense input mode.

The rear-lightbar path is different. Its mode updater calls
`0x180002ed0`, which builds a 16-byte frame and passes it to the common WMI
helper `0x180011500`. That helper invokes:

```
SELECT * FROM AcerGamingFunction
method: SetGamingKBBacklight
input: gmInput SAFEARRAY containing the 16-byte frame
```

The frame bytes recovered at `0x180003172`-`0x180003191` are:

| Offset | Value |
| ---: | --- |
| 0 | selected mode byte |
| 1 | speed low byte |
| 2 | brightness low byte |
| 3 | `0x00` |
| 4 | direction + 1 |
| 5 | red |
| 6 | green |
| 7 | blue |
| 8 | `0x03` |
| 9 | `0x02` |
| 10-15 | `0x00` (zero-initialized) |

The helper logs each byte as `input_data[%d]`, and retries a failed WMI call
after a 30 ms sleep. This is the payload to reproduce for rear effects; it is
not the logo's packed 64-bit LED-group ABI.

The parameter order is confirmed by the call site at `0x18000fa00`: it loads
the mode record's speed, direction, and brightness fields into the arguments
to `0x180002ed0`, whose byte stores emit speed at offset 1 and direction+1 at
offset 4. Earlier notes that swapped those two labels were incorrect.

## Mode table exposed by the EC-logo controller

The logo constructor registers these modes (the constructor writes the mode
values and defaults directly):

| Mode | PredatorSense input value | Defaults recovered |
| --- | ---: | --- |
| Static | `0` | real behavior byte `1`; brightness `100`; one color |
| Breathing | `1` | real behavior byte `3`; speed `5`; brightness `100`; one color |
| Neon | `2` | real behavior byte `4`; speed `5`; brightness `100`; no color |
| Wave | `3` | real behavior byte `0`; speed `5`; brightness `100`; no color |
| Direct | `0xFF` | one direct color slot in metadata; updater skips it |

The constructor’s UI values are not the WMI behavior bytes. The four-byte table
at `0x180016020` is indexed by the UI mode and contains
`[1, 3, 4, 0, ...]`; the shared mode lookup translates the UI mode before
`SetGamingLEDBehavior` is called. A driver must use this translation rather
than assuming the UI values are raw firmware opcodes.

## Rear-lightbar differences

The rear-lightbar controller uses its own 16-byte `SetGamingKBBacklight`
payload helper, RGB controller factory, and mode table. The DLL contains
separate strings/factories for front, left, right, rear, and logo EC devices;
they must not be collapsed into the Darfon USB-logo implementation.

The rear-lightbar color path is group-based and the Windows documentation
identifies the rear zone as a seven-LED bar. The rear constructor registers
`STATIC` (value `0`), `BREATHING` (`1`), `NEON` (`2`), `WAVE` (`3`), and an
additional `TWINKLING` entry (value `7`) in addition to `Direct` (`0xff`). The
EC-logo path is one LED. A driver should therefore expose two separate
OpenRGB devices:

* `Acer EC Logo LED`: one RGB LED, one group selector, firmware modes plus
  a `Direct` entry in the Windows mode metadata. As with the rear controller,
  the logo mode updater returns before sending the WMI payload for `0xff`, so
  the DLL does not demonstrate a working Direct color update.
* `Acer EC Rear Lightbar`: seven physical positions represented by the one
  16-byte `SetGamingKBBacklight` group frame; no per-position streaming
  protocol has been recovered from this DLL.

### Direct-mode result

The rear controller's mode-update method is `0x18000fa00`. It loads the
selected mode value from the mode record and immediately returns when that
value is `0xff` (`0x18000fa1d`-`0x18000fa24`). Only non-`0xff` modes continue to
the rear payload builder at `0x180002ed0`, which is the only rear call site
that constructs the `SetGamingKBBacklight` frame. There is no second rear call
site that streams the RGB controller's color array when Direct is selected.

The virtual dispatch table makes this negative result explicit. The rear
vtable at `0x1800163e8` has these RGB-controller update slots:

| Vtable offset | Windows service name | Rear target | Effect |
| ---: | --- | --- | --- |
| `+0xd8` (`216`) | `UpdateLEDs` | `0x1800088b0` | immediate `ret` |
| `+0xe0` (`224`) | `UpdateZoneLEDs` | `0x18000af00` | thunk dispatching back to `+0xd8` |
| `+0xe8` (`232`) | `UpdateSingleLED` | `0x18000af10` | state write only; no WMI call |
| `+0xf0` (`240`) | `UpdateMode` | `0x18000fa00` | builds `SetGamingKBBacklight` frame |

The slot names are not inferred from generic C++ ordering: the companion
`AcerLightingService.exe` network controller contains the corresponding
`NetworkClient::SendRequest_RGBController_UpdateLEDs`, `...UpdateZoneLEDs`,
and `...UpdateSingleLED` strings at those virtual offsets. PredatorSense's
rear implementation therefore only transmits effect-mode changes. Its
`Direct` entry is metadata exposed by the common RGB-controller UI, not a
working per-LED transport in this Windows DLL.

Therefore Direct is *advertised* by the Windows RGB-controller metadata, but a
working rear direct-mode payload is not implemented by this DLL. The recovered
frame has RGB fields, so a group-level direct color remains a possible
undocumented EC feature, but PredatorSense never sends it for mode `0xff` and
supplies no per-LED index or frame-buffer protocol. Implementing Direct on
Linux would require an EC experiment: send the recovered 16-byte frame with
mode `0xff` and chosen RGB values through `SetGamingKBBacklight`, then verify
whether the firmware accepts it. That would be a new probe, not a payload
recovered from PredatorSense.

### Separate static-color protocol hypothesis

The PH16-72 tests showed that method 20 accepts every tested mode value but
leaves the rear color unchanged. This is consistent with a second protocol
reported for the closely related PH16-71: a fixed 12-byte `SetGamingLED`
priming write, a method-20 mode/brightness commit with zero RGB bytes, and
three `SetGamingRgbKb` u64 writes. The captured u64 packing is:

```c
value = (R << 8) | (G << 16) | (B << 24) |
        (0x08ULL << 32) | ((uint64_t)zone_mask << 40);
```

The masks were `1`, `2`, and `4` for left, center, and right on that unit;
the complete five-call group was repeated three times. This sequence is not
present in `AcerECLightbarController.dll`, and the PH16-71 capture is not
proof that the PH16-72 firmware has identical semantics. It is therefore
implemented only in the guarded probe as `rear-static`, so it can be tested
without silently changing the recovered Windows-effect path. A successful
PH16-72 test would establish the missing static/direct transport and should
then be promoted into the driver after confirming the zone count and cadence.

## Linux implementation requirement

Linux currently enumerates the WMI GUID but exposes no userspace API for
invoking `AcerGamingFunction` methods. OpenRGB cannot send these payloads over
the existing HID path. A complete implementation therefore needs:

1. a kernel-side WMI bridge for the logo's `SetGamingLEDColor`/
   `SetGamingLEDBehavior` calls and the rear `SetGamingKBBacklight` call,
   preserving each method's exact `gmInput` representation; and
2. OpenRGB EC controllers that call that bridge and present the separate logo
   and rear-lightbar devices.

Until that bridge exists, adding an OpenRGB class alone would only create a UI
device and would not reach the EC firmware.
