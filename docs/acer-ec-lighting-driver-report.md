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

The rear-lightbar constructor creates the rear-lightbar device and its LED
entry. The model documentation identifies this as a seven-LED linear bar. The
EC DLL's mode/color API is group-based, however: the WMI payload selects the
device group and does not provide a per-LED frame buffer. A Linux driver should
expose the seven LEDs as one linear zone only if the hardware behavior is
confirmed; the Windows controller itself sends group color/effect commands.

## WMI calls

The shared EC `SetMode` routine is `0x180003230`. It issues two WMI method
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

## Mode table exposed by the EC-logo controller

The logo constructor registers these modes (the constructor writes the mode
values and defaults directly):

| Mode | PredatorSense input value | Defaults recovered |
| --- | ---: | --- |
| Static | `0` | real behavior byte `1`; brightness `100`; one color |
| Breathing | `1` | real behavior byte `3`; speed `5`; brightness `100`; one color |
| Neon | `2` | real behavior byte `4`; speed `5`; brightness `100`; no color |
| Wave | `3` | real behavior byte `0`; speed `5`; brightness `100`; no color |
| Direct | `0xFF` | one direct color slot |

The constructor’s UI values are not the WMI behavior bytes. The four-byte table
at `0x180016020` is indexed by the UI mode and contains
`[1, 3, 4, 0, ...]`; the shared mode lookup translates the UI mode before
`SetGamingLEDBehavior` is called. A driver must use this translation rather
than assuming the UI values are raw firmware opcodes.

## Rear-lightbar differences

The rear-lightbar controller uses the same WMI method names and packing helper,
but has its own RGB controller factory and mode table. The DLL contains
separate strings/factories for front, left, right, rear, and logo EC devices;
they must not be collapsed into the Darfon USB-logo implementation.

The rear-lightbar color path is group-based and the Windows documentation
identifies the rear zone as a seven-LED bar. The EC-logo path is one LED. A
driver should therefore expose two separate OpenRGB devices:

* `Acer EC Logo LED`: one RGB LED, one group selector, firmware modes plus
  Direct color.
* `Acer EC Rear Lightbar`: seven physical positions represented by the one EC
  group command; no per-position streaming protocol has been recovered from
  this DLL.

## Linux implementation requirement

Linux currently enumerates the WMI GUID but exposes no userspace API for
invoking `AcerGamingFunction` methods. OpenRGB cannot send these payloads over
the existing HID path. A complete implementation therefore needs:

1. a kernel-side WMI bridge for `SetGamingLEDColor` and
   `SetGamingLEDBehavior`, preserving the 64-bit little-endian `gmInput`; and
2. OpenRGB EC controllers that call that bridge and present the separate logo
   and rear-lightbar devices.

Until that bridge exists, adding an OpenRGB class alone would only create a UI
device and would not reach the EC firmware.
