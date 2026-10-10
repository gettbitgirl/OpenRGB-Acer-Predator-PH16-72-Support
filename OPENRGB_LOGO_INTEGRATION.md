# OpenRGB Integration Guide: Acer Predator Logo LED

This specification describes the Linux OpenRGB integration for the EC-managed
logo LED on Acer Predator laptops such as the Predator Helios 16 `PH16-72`.
The logo is a separate device from the rear seven-zone lightbar, keyboard, and
USB HID lid-logo controllers.

## 1. Device Overview

* **Device name:** Acer Predator Logo LED
* **Device type:** `DEVICE_TYPE_NOTEBOOK`
* **Physical layout:** One RGB LED zone
* **OpenRGB layout:** One zone (`ZONE_TYPE_SINGLE`) containing one LED
* **Hardware effects:** Static (`Direct` in OpenRGB), Breathing, and Neon

The Linux kernel driver owns the ACPI-WMI transaction and exposes one atomic
sysfs attribute. OpenRGB should use that interface instead of issuing raw ACPI
WMI calls.

## 2. Primary Integration Interface: Linux Kernel Sysfs

The `linuwu_sense` driver exports:

```text
/sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/logo_led
```

The exact path should be discovered at runtime rather than assumed. A device
may be rejected unless `logo_led` exists and is both readable and writable.

### 2.1 Attribute format

| Attribute | Access | Format |
| :--- | :---: | :--- |
| `logo_led` | R/W | `RRGGBB,brightness,speed,effect` |

* `RRGGBB`: six hexadecimal digits, with optional `#` accepted by the kernel
  interface
* `brightness`: `0..100`
* `speed`: `1..5`
* `effect`: `off`, `static`, `breathing`, or `neon`

Example writes:

```text
ff0000,100,5,static
00ff00,100,3,breathing
0000ff,100,3,neon
000000,0,5,off
```

One write performs the complete color/effect update. OpenRGB should not split a
single user change into separate color and effect writes.

### 2.2 Non-root access

The installed module package may create a tmpfiles entry for `logo_led`. If
OpenRGB cannot open the file as the normal user, use a narrowly scoped udev
rule or tmpfiles rule for this one attribute; do not grant access to all ACPI
or WMI devices.

## 3. OpenRGB Modes and Capabilities

Register one zone and one LED:

```text
Zone name: Rear/Logo LED (or "Logo LED")
Zone type: ZONE_TYPE_SINGLE
LED name: Logo LED
```

Recommended modes:

| OpenRGB mode | Sysfs effect | Flags | Color input |
| :--- | :--- | :--- | :--- |
| `Direct` | `static` | `MODE_FLAG_HAS_PER_LED_COLOR`, `MODE_FLAG_HAS_BRIGHTNESS` | One RGB color |
| `Breathing` | `breathing` | `MODE_FLAG_HAS_SPEED`, `MODE_FLAG_HAS_BRIGHTNESS` | None; firmware effect |
| `Neon` | `neon` | `MODE_FLAG_HAS_SPEED`, `MODE_FLAG_HAS_BRIGHTNESS` | None; firmware effect |
| `Off` | `off` | none | None |

Hardware effects should not advertise a color picker unless testing proves
that the firmware uses the color field in that mode. For custom-color effects,
OpenRGB should use software effects and repeatedly update the `Direct` mode.

## 4. Reference OpenRGB Implementation

### 4.1 Header

```cpp
#pragma once

#include "RGBController.h"
#include <string>

class RGBController_AcerPredatorLogo : public RGBController
{
public:
    explicit RGBController_AcerPredatorLogo(const std::string& sysfs_path);

    void SetupZones();
    void ResizeZone(int zone, int new_size);
    void DeviceUpdateLEDs();
    void UpdateZoneLEDs(int zone);
    void UpdateSingleLED(int led);
    void DeviceUpdateMode();

private:
    std::string logo_led;
    void WriteLogo(const std::string& value);
};
```

### 4.2 Implementation pattern

```cpp
#include "RGBController_AcerPredatorLogo.h"
#include <fstream>
#include <iomanip>
#include <sstream>

RGBController_AcerPredatorLogo::RGBController_AcerPredatorLogo(
    const std::string& sysfs_path)
    : logo_led(sysfs_path)
{
    name        = "Acer Predator Logo LED";
    vendor      = "Acer";
    type        = DEVICE_TYPE_NOTEBOOK;
    description = "EC-managed Predator logo RGB LED";
    location    = logo_led;

    mode direct;
    direct.name = "Direct";
    direct.value = 0;
    direct.flags = MODE_FLAG_HAS_PER_LED_COLOR |
                   MODE_FLAG_HAS_BRIGHTNESS;
    direct.brightness = 100;
    direct.color_mode = MODE_COLORS_PER_LED;
    modes.push_back(direct);

    mode breathing;
    breathing.name = "Breathing";
    breathing.value = 1;
    breathing.flags = MODE_FLAG_HAS_SPEED | MODE_FLAG_HAS_BRIGHTNESS;
    breathing.speed_min = 1;
    breathing.speed_max = 5;
    breathing.speed = 5;
    breathing.brightness = 100;
    breathing.color_mode = MODE_COLORS_NONE;
    modes.push_back(breathing);

    mode neon;
    neon.name = "Neon";
    neon.value = 2;
    neon.flags = MODE_FLAG_HAS_SPEED | MODE_FLAG_HAS_BRIGHTNESS;
    neon.speed_min = 1;
    neon.speed_max = 5;
    neon.speed = 5;
    neon.brightness = 100;
    neon.color_mode = MODE_COLORS_NONE;
    modes.push_back(neon);

    mode off;
    off.name = "Off";
    off.value = 3;
    off.flags = 0;
    off.color_mode = MODE_COLORS_NONE;
    modes.push_back(off);

    SetupZones();
}

void RGBController_AcerPredatorLogo::SetupZones()
{
    zone logo_zone;
    logo_zone.name = "Logo LED";
    logo_zone.type = ZONE_TYPE_SINGLE;
    logo_zone.leds_min = 1;
    logo_zone.leds_max = 1;
    logo_zone.leds_count = 1;
    logo_zone.matrix_map = nullptr;
    zones.push_back(logo_zone);

    led logo;
    logo.name = "Logo LED";
    logo.value = 0;
    leds.push_back(logo);
    colors.push_back(ToRGBColor(0, 0, 0));
    SetupColors();
}

void RGBController_AcerPredatorLogo::ResizeZone(int, int)
{
    /* The logo is a fixed one-LED zone. */
}

void RGBController_AcerPredatorLogo::WriteLogo(const std::string& value)
{
    std::ofstream file(logo_led);
    if (file)
        file << value;
}

void RGBController_AcerPredatorLogo::DeviceUpdateLEDs()
{
    const mode& current = modes[active_mode];
    if (current.name != "Direct")
        return;

    RGBColor color = colors[0];
    std::ostringstream value;
    value << std::hex << std::setw(2) << std::setfill('0')
          << static_cast<int>(RGBGetRValue(color))
          << std::setw(2) << static_cast<int>(RGBGetGValue(color))
          << std::setw(2) << static_cast<int>(RGBGetBValue(color))
          << std::dec << "," << current.brightness << ",5,static";
    WriteLogo(value.str());
}

void RGBController_AcerPredatorLogo::UpdateZoneLEDs(int)
{
    DeviceUpdateLEDs();
}

void RGBController_AcerPredatorLogo::UpdateSingleLED(int)
{
    DeviceUpdateLEDs();
}

void RGBController_AcerPredatorLogo::DeviceUpdateMode()
{
    const mode& current = modes[active_mode];
    std::string effect = "static";
    if (current.name == "Breathing") effect = "breathing";
    if (current.name == "Neon") effect = "neon";
    if (current.name == "Off") effect = "off";

    RGBColor color = ToRGBColor(0, 0, 0);
    if (current.name == "Direct")
        color = colors[0];

    std::ostringstream value;
    value << std::hex << std::setw(2) << std::setfill('0')
          << static_cast<int>(RGBGetRValue(color))
          << std::setw(2) << static_cast<int>(RGBGetGValue(color))
          << std::setw(2) << static_cast<int>(RGBGetBValue(color))
          << std::dec << "," << current.brightness << ","
          << current.speed << "," << effect;
    WriteLogo(value.str());
}
```

The exact OpenRGB class and helper names can vary by OpenRGB branch. The
important properties are the one-LED model, the single atomic sysfs write, and
the mode/capability mapping above.

## 5. Direct WMI Reference (Not the Preferred Linux Backend)

If a userspace implementation intentionally bypasses the kernel driver, the
firmware metadata for this machine is:

```text
GUID: 7A4DDFE7-5B5D-40B4-8595-4408E0CC7F56
Class: AcerGamingFunction
Instance: 0
```

The relevant method IDs are:

| Method | ID | Input type |
| :--- | :---: | :--- |
| `SetGamingLEDBehavior` | `10` | `uint64` |
| `SetGamingLEDColor` | `12` | `uint64` |

Color packing is:

```text
bits  0..7   zone = 0x01
bits  8..15  red
bits 16..23  green
bits 24..31  blue
bits 32..39  brightness
bits 40..47  speed
```

Behavior packing is:

```text
bits  0..7   zone = 0x01
bits 16..23  effect: static=1, breathing=3, neon=4
```

Do not use method IDs `1` and `2` for the logo protocol. Those are different
`AcerGamingFunction` methods (`SetGamingProfile` and the 16-byte
`SetGamingLED` interface). Do not send behavior as a 32-bit input; this
firmware's Binary MOF declares it as `uint64`.

## 6. Validation Checklist

1. Detect `logo_led` and verify it is writable.
2. Register exactly one zone with one LED.
3. Verify static red, green, and blue writes.
4. Verify `breathing` and `neon` at speeds `1`, `3`, and `5`.
5. Verify `off`.
6. Confirm OpenRGB software effects update `Direct`, not the raw WMI path.
7. Treat this as validated on PH16-72 only unless another model's firmware
   Binary MOF and hardware behavior are checked.
