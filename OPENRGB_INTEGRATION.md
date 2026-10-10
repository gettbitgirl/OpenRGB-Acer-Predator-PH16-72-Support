# OpenRGB Integration Guide: Acer Predator Addressable Rear Lightbar

This specification provides all necessary details to implement an OpenRGB controller for the addressable rear exhaust RGB lightbar on Acer Predator laptops (e.g., Predator Helios 16 `PH16-71`, `PH16-72`, and related Predator platforms).

---

## 1. Device Overview & Topology

* **Device Name:** Acer Predator Rear Exhaust Lightbar
* **Device Type:** `DEVICE_TYPE_NOTEBOOK` or `DEVICE_TYPE_LEDSTRIP`
* **Physical Layout:** Linear strip of **7 individually addressable RGB LEDs** spanning the rear thermal exhaust vent.
* **Control Models:**
  1. **Direct / Static 7-Zone RGB Mode:** Full 24-bit per-LED color control across all 7 zones.
  2. **Hardware Dynamic Animation Engine:** Firmware-driven hardware animations (Breathing, Neon, Wave, Snake, Lightning, Stack, Motion Point, Zoom).
  3. **Off Mode:** Shuts off power and LED emission.

### Zone & LED Mapping
OpenRGB should define **1 Linear Zone** with **7 LEDs**:
* **Zone 0:** `"Rear Lightbar"` (`ZONE_TYPE_LINEAR`, 7 LEDs)
  * **LED 0:** `"Zone 1"` (Far Left / Port side)
  * **LED 1:** `"Zone 2"`
  * **LED 2:** `"Zone 3"`
  * **LED 3:** `"Zone 4"` (Center)
  * **LED 4:** `"Zone 5"`
  * **LED 5:** `"Zone 6"`
  * **LED 6:** `"Zone 7"` (Far Right / Starboard side)

---

## 2. Primary Integration Interface: Linux Kernel Sysfs

The `linuwu_sense` platform driver exports native sysfs attributes under:
```
/sys/devices/platform/acer-wmi/rear_lightbar/
```

This is the **recommended backend for Linux OpenRGB** because the driver handles kernel-level mutex locking, hardware handshake sequences, and ACPI execution safely without requiring raw WMI or root access (when udev rules are configured).

### 2.1 Sysfs Attributes Reference

| Attribute File | Permissions | Access | Description & Format |
| :--- | :---: | :---: | :--- |
| **`zones`** | `0644` | R/W | **Batch or Single Zone Control**<br>**Read:** `zone1=RRGGBB zone2=RRGGBB ... zone7=RRGGBB brightness=B`<br>**Write (All 7 Zones):** `RRGGBB,RRGGBB,RRGGBB,RRGGBB,RRGGBB,RRGGBB,RRGGBB[,brightness]`<br>**Write (Single Zone):** `zone_id,RRGGBB[,brightness]` (zone 1..7) |
| **`zone1` .. `zone7`** | `0644` | R/W | **Individual Zone Color**<br>**Read:** `RRGGBB\n`<br>**Write:** `RRGGBB` or `#RRGGBB` |
| **`static_color`** | `0644` | R/W | **Solid Color Across All Zones**<br>**Read:** `RRGGBB (all zones) brightness=B\n`<br>**Write:** `RRGGBB[,brightness]` |
| **`brightness`** | `0644` | R/W | **Master Lightbar Brightness (0-100)**<br>**Read:** `B\n`<br>**Write:** `B` (integer `0` to `100`) |
| **`animation`** | `0644` | R/W | **Hardware Animation Engine**<br>**Read:** `mode=M (name) speed=S brightness=B direction=D\n`<br>**Write:** `name_or_id[,speed,brightness,direction]`<br>*(Writing `"static"` returns `-EINVAL`; static color must use `zones`)* |

### 2.2 Non-Root Access (udev Configuration)

To allow OpenRGB running as a standard user to control the rear lightbar without root privileges, install this udev rule:

```udev
# /etc/udev/rules.d/99-acer-rear-lightbar.rules
ACTION=="add", SUBSYSTEM=="platform", KERNEL=="acer-wmi", RUN+="/bin/sh -c 'chmod -R 0666 /sys/devices/platform/acer-wmi/rear_lightbar/*'"
```
Or for systemd-based distributions with `uaccess`:
```udev
SUBSYSTEM=="platform", KERNEL=="acer-wmi", TAG+="uaccess"
```

---

## 3. OpenRGB Modes & Capabilities Specification

In OpenRGB, modes are configured on the `RGBController` instance with specific capability flags. Below is the mapping for each supported mode.

### 3.1 Mode 0: Direct (Per-LED Static Color)
* **OpenRGB Mode Name:** `"Direct"`
* **Mode Flags:**
  * `MODE_FLAG_HAS_PER_LED_COLOR`
  * `MODE_FLAG_HAS_BRIGHTNESS`
* **Color Capabilities:** Fully independent 24-bit RGB per zone (`colors[0..6]`).
* **Sysfs Write Action:**
  Format all 7 LED colors as uppercase hex and write to `zones`:
  ```
  RRGGBB,RRGGBB,RRGGBB,RRGGBB,RRGGBB,RRGGBB,RRGGBB,<brightness>
  ```
* **Performance:** Single atomic sysfs transaction; dispatches the 7 zone updates via kernel WMI in under 5ms. Ideal for OpenRGB software canvas / audio visualizers / ambient effects.

---

### 3.2 Mode 1: Static (All Zones Solid Color)
* **OpenRGB Mode Name:** `"Static"`
* **Mode Flags:**
  * `MODE_FLAG_HAS_MODE_SPECIFIC_COLOR`
  * `MODE_FLAG_HAS_BRIGHTNESS`
* **Sysfs Write Action:**
  Write to `static_color`:
  ```
  RRGGBB,<brightness>
  ```

---

### 3.3 Hardware Animation Modes

> [!IMPORTANT]
> **Hardware Firmware Animation Characteristic:**
> In the Acer EC firmware, all hardware animation patterns (Breathing, Neon, Wave, Snake, etc.) run **fixed internal rainbow sequences**. The EC ignores custom RGB parameters in dynamic modes.
> - Do **not** set `MODE_FLAG_HAS_PER_LED_COLOR` or `MODE_FLAG_HAS_MODE_SPECIFIC_COLOR` on hardware animation modes.
> - For custom-color animations (e.g. single-color pulse), OpenRGB should run the effect in software using **Mode 0 ("Direct")**.

#### Speed & Direction Parameters:
* **Speed Range:** `1` to `5` (`1` = Fastest, `5` = Slowest, Default = `3`).
* **Brightness Range:** `0` to `100%`.
* **Direction:**
  * `1` = `MODE_DIRECTION_RIGHT` (Flow Right)
  * `2` = `MODE_DIRECTION_LEFT` (Flow Left)

#### Effect Modes Matrix:

| Mode ID | OpenRGB Mode Name | Sysfs Identifier | OpenRGB Mode Flags | Directions Supported | Description |
| :---: | :--- | :--- | :--- | :---: | :--- |
| **`0`** | `"Off"` | `off` | *(None)* | None | Turns off rear lightbar emission and power gate. |
| **`1`** | `"Breathing"` | `breathing` | `MODE_FLAG_HAS_SPEED`<br>`MODE_FLAG_HAS_BRIGHTNESS` | None | Firmware multi-color fading breathing cycle. |
| **`2`** | `"Neon"` | `neon` | `MODE_FLAG_HAS_SPEED`<br>`MODE_FLAG_HAS_BRIGHTNESS` | None | Smooth full-spectrum rainbow color cycling. |
| **`3`** | `"Wave"` | `wave` | `MODE_FLAG_HAS_SPEED`<br>`MODE_FLAG_HAS_BRIGHTNESS`<br>`MODE_FLAG_HAS_DIRECTION_LR` | Left (`2`), Right (`1`) | Continuous multi-color flowing wave across 7 zones. |
| **`9`** | `"Snake"` | `snake` | `MODE_FLAG_HAS_SPEED`<br>`MODE_FLAG_HAS_BRIGHTNESS` | None | Chase pattern moving across zones. |
| **`10`** | `"Lightning"` | `lightning` | `MODE_FLAG_HAS_SPEED`<br>`MODE_FLAG_HAS_BRIGHTNESS` | None | Strobe/flash simulation pattern. |
| **`11`** | `"Stack"` | `stack` | `MODE_FLAG_HAS_SPEED`<br>`MODE_FLAG_HAS_BRIGHTNESS` | None | Zones light up sequentially and stack to the side. |
| **`12`** | `"Motion Point"` | `motion_point` | `MODE_FLAG_HAS_SPEED`<br>`MODE_FLAG_HAS_BRIGHTNESS`<br>`MODE_FLAG_HAS_DIRECTION_LR` | Left (`2`), Right (`1`) | A bouncing single light point travelling back and forth. |
| **`13`** | `"Zoom"` | `zoom` | `MODE_FLAG_HAS_SPEED`<br>`MODE_FLAG_HAS_BRIGHTNESS`<br>`MODE_FLAG_HAS_DIRECTION_LR` | Expand / Contract | Light emits outward from center zone or contracts inward. |

#### Sysfs Write Action for Animations:
Write to `animation`:
```
<effect_name>,<speed>,<brightness>,<direction>
```
*Example:* `wave,3,100,2` (Wave, Speed 3, 100% Brightness, Flow Left).

---

## 4. Alternative Direct ACPI WMI Specification

If OpenRGB implements a direct kernel-bypass driver or Windows port using direct WMI:

* **WMI GUID:** `7A4D4140-9924-11DE-A8A3-0800200C9A66` (`WMID_GUID4`)

### 4.1 Method 2: Controller Unlock (`SetGamingLEDBehavior`)
Executed before setting colors to unlock the EC register multiplexer.
* **Method ID:** `2`
* **Payload (16 bytes):**
  ```c
  uint8_t unlock[16] = {
      0x10, 0x00, 0x00, 0x00,
      0x00, 0x00, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF,
      0x55, 0x15, 0x00, 0x00
  };
  ```

### 4.2 Method 20: Animation / Static Arm (`SetGamingKBBacklight`)
Used to configure hardware animations OR arm the static engine.
* **Method ID:** `20`
* **Static Arm Payload (16 bytes):**
  ```c
  uint8_t static_arm[16] = {
      0x00, 0x00, 0x64, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0x03, 0x02, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00
  };
  ```
* **Hardware Animation Payload (16 bytes):**
  ```
  Byte [0]: Mode ID (1, 2, 3, 9, 10, 11, 12, 13, or 0)
  Byte [1]: Speed (1..5)
  Byte [2]: Brightness (0..100)
  Byte [4]: Target / Flow Direction (0x02 = Left / Default, 0x01 = Right)
  Byte [8]: 0x03
  Byte [9]: 0x02
  ```

### 4.3 Method 6: 7-Zone RGB Commit (`SetGamingRgbKb`)
Commits an individual zone color to the EC latch.
* **Method ID:** `6`
* **Payload:** 64-bit integer (`uint64_t`)
* **Bitfield Layout:**
  ```
  Bit 40..46: Zone Mask (1 << zone_idx) -> 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40
  Bit 32..35: Hardware Flag (0x08)
  Bit 24..31: Blue (0..255)
  Bit 16..23: Green (0..255)
  Bit  8..15: Red (0..255)
  Bit  0..7 : 0x00
  ```
* **Packing Formula:**
  ```c
  uint64_t payload = ((uint64_t)(1 << zone_idx) << 40) |
                     ((uint64_t)0x08            << 32) |
                     ((uint64_t)b               << 24) |
                     ((uint64_t)g               << 16) |
                     ((uint64_t)r               << 8);
  ```

---

## 5. Reference OpenRGB C++ Implementation

Below is a complete reference design for an OpenRGB `RGBController` class interacting via the sysfs interface.

### 5.1 Header: `RGBController_AcerPredatorLightbar.h`

```cpp
#pragma once

#include "RGBController.h"
#include <string>

class RGBController_AcerPredatorLightbar : public RGBController
{
public:
    RGBController_AcerPredatorLightbar(const std::string& sysfs_path);
    ~RGBController_AcerPredatorLightbar();

    void SetupZones();
    void ResizeZone(int zone, int new_size);

    void DeviceUpdateLEDs();
    void UpdateZoneLEDs(int zone);
    void UpdateSingleLED(int led);

    void DeviceUpdateMode();

private:
    std::string sysfs_base;
    void WriteSysfs(const std::string& attr, const std::string& value);
    std::string ReadSysfs(const std::string& attr);
};
```

### 5.2 Implementation: `RGBController_AcerPredatorLightbar.cpp`

```cpp
#include "RGBController_AcerPredatorLightbar.h"
#include <fstream>
#include <sstream>
#include <iomanip>

static const std::string SYSFS_PATH = "/sys/devices/platform/acer-wmi/rear_lightbar/";

RGBController_AcerPredatorLightbar::RGBController_AcerPredatorLightbar(const std::string& path)
    : sysfs_base(path.empty() ? SYSFS_PATH : path)
{
    name        = "Acer Predator Rear Lightbar";
    vendor      = "Acer";
    type        = DEVICE_TYPE_NOTEBOOK;
    description = "7-Zone Addressable Rear Exhaust Lightbar";
    location    = sysfs_base;

    /* Mode 0: Direct (Per-LED static control) */
    mode Direct;
    Direct.name       = "Direct";
    Direct.value      = 0;
    Direct.flags      = MODE_FLAG_HAS_PER_LED_COLOR | MODE_FLAG_HAS_BRIGHTNESS;
    Direct.brightness = 100;
    Direct.color_mode = MODE_COLORS_PER_LED;
    modes.push_back(Direct);

    /* Mode 1: Static (Solid color all zones) */
    mode Static;
    Static.name       = "Static";
    Static.value      = 1;
    Static.flags      = MODE_FLAG_HAS_MODE_SPECIFIC_COLOR | MODE_FLAG_HAS_BRIGHTNESS;
    Static.colors.resize(1);
    Static.brightness = 100;
    Static.color_mode = MODE_COLORS_MODE_SPECIFIC;
    modes.push_back(Static);

    /* Mode 2: Breathing (Hardware effect) */
    mode Breathing;
    Breathing.name       = "Breathing";
    Breathing.value      = 2;
    Breathing.flags      = MODE_FLAG_HAS_SPEED | MODE_FLAG_HAS_BRIGHTNESS;
    Breathing.speed_min  = 1;
    Breathing.speed_max  = 5;
    Breathing.speed      = 3;
    Breathing.brightness = 100;
    Breathing.color_mode = MODE_COLORS_NONE;
    modes.push_back(Breathing);

    /* Mode 3: Neon (Hardware effect) */
    mode Neon;
    Neon.name       = "Neon";
    Neon.value      = 3;
    Neon.flags      = MODE_FLAG_HAS_SPEED | MODE_FLAG_HAS_BRIGHTNESS;
    Neon.speed_min  = 1;
    Neon.speed_max  = 5;
    Neon.speed      = 3;
    Neon.brightness = 100;
    Neon.color_mode = MODE_COLORS_NONE;
    modes.push_back(Neon);

    /* Mode 4: Wave (Hardware effect) */
    mode Wave;
    Wave.name       = "Wave";
    Wave.value      = 4;
    Wave.flags      = MODE_FLAG_HAS_SPEED | MODE_FLAG_HAS_BRIGHTNESS | MODE_FLAG_HAS_DIRECTION_LR;
    Wave.speed_min  = 1;
    Wave.speed_max  = 5;
    Wave.speed      = 3;
    Wave.direction  = MODE_DIRECTION_LEFT;
    Wave.brightness = 100;
    Wave.color_mode = MODE_COLORS_NONE;
    modes.push_back(Wave);

    /* Mode 5: Snake */
    mode Snake;
    Snake.name       = "Snake";
    Snake.value      = 5;
    Snake.flags      = MODE_FLAG_HAS_SPEED | MODE_FLAG_HAS_BRIGHTNESS;
    Snake.speed_min  = 1;
    Snake.speed_max  = 5;
    Snake.speed      = 3;
    Snake.brightness = 100;
    Snake.color_mode = MODE_COLORS_NONE;
    modes.push_back(Snake);

    /* Mode 6: Lightning */
    mode Lightning;
    Lightning.name       = "Lightning";
    Lightning.value      = 6;
    Lightning.flags      = MODE_FLAG_HAS_SPEED | MODE_FLAG_HAS_BRIGHTNESS;
    Lightning.speed_min  = 1;
    Lightning.speed_max  = 5;
    Lightning.speed      = 3;
    Lightning.brightness = 100;
    Lightning.color_mode = MODE_COLORS_NONE;
    modes.push_back(Lightning);

    /* Mode 7: Stack */
    mode Stack;
    Stack.name       = "Stack";
    Stack.value      = 7;
    Stack.flags      = MODE_FLAG_HAS_SPEED | MODE_FLAG_HAS_BRIGHTNESS;
    Stack.speed_min  = 1;
    Stack.speed_max  = 5;
    Stack.speed      = 3;
    Stack.brightness = 100;
    Stack.color_mode = MODE_COLORS_NONE;
    modes.push_back(Stack);

    /* Mode 8: Motion Point */
    mode MotionPoint;
    MotionPoint.name       = "Motion Point";
    MotionPoint.value      = 8;
    MotionPoint.flags      = MODE_FLAG_HAS_SPEED | MODE_FLAG_HAS_BRIGHTNESS | MODE_FLAG_HAS_DIRECTION_LR;
    MotionPoint.speed_min  = 1;
    MotionPoint.speed_max  = 5;
    MotionPoint.speed      = 3;
    MotionPoint.direction  = MODE_DIRECTION_LEFT;
    MotionPoint.brightness = 100;
    MotionPoint.color_mode = MODE_COLORS_NONE;
    modes.push_back(MotionPoint);

    /* Mode 9: Zoom */
    mode Zoom;
    Zoom.name       = "Zoom";
    Zoom.value      = 9;
    Zoom.flags      = MODE_FLAG_HAS_SPEED | MODE_FLAG_HAS_BRIGHTNESS | MODE_FLAG_HAS_DIRECTION_LR;
    Zoom.speed_min  = 1;
    Zoom.speed_max  = 5;
    Zoom.speed      = 3;
    Zoom.direction  = MODE_DIRECTION_LEFT;
    Zoom.brightness = 100;
    Zoom.color_mode = MODE_COLORS_NONE;
    modes.push_back(Zoom);

    /* Mode 10: Off */
    mode Off;
    Off.name       = "Off";
    Off.value      = 10;
    Off.flags      = 0;
    Off.color_mode = MODE_COLORS_NONE;
    modes.push_back(Off);

    SetupZones();
}

RGBController_AcerPredatorLightbar::~RGBController_AcerPredatorLightbar()
{
}

void RGBController_AcerPredatorLightbar::SetupZones()
{
    zone lightbar_zone;
    lightbar_zone.name       = "Rear Lightbar";
    lightbar_zone.type       = ZONE_TYPE_LINEAR;
    lightbar_zone.leds_min   = 7;
    lightbar_zone.leds_max   = 7;
    lightbar_zone.leds_count = 7;
    lightbar_zone.matrix_map = nullptr;
    zones.push_back(lightbar_zone);

    for (int i = 0; i < 7; i++)
    {
        led new_led;
        new_led.name  = "Zone " + std::to_string(i + 1);
        new_led.value = i;
        leds.push_back(new_led);
        colors.push_back(ToRGBColor(0, 0, 0));
    }

    SetupColors();
}

void RGBController_AcerPredatorLightbar::ResizeZone(int /*zone*/, int /*new_size*/)
{
    /* Lightbar has a fixed hardware length of 7 zones */
}

void RGBController_AcerPredatorLightbar::WriteSysfs(const std::string& attr, const std::string& value)
{
    std::ofstream file(sysfs_base + attr);
    if (file.is_open())
    {
        file << value;
        file.close();
    }
}

void RGBController_AcerPredatorLightbar::DeviceUpdateLEDs()
{
    /* If currently in Direct mode, push all 7 colors */
    if (modes[active_mode].name == "Direct")
    {
        std::ostringstream ss;
        for (int i = 0; i < 7; i++)
        {
            RGBColor c = colors[i];
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)RGBGetRValue(c)
               << std::hex << std::setw(2) << std::setfill('0') << (int)RGBGetGValue(c)
               << std::hex << std::setw(2) << std::setfill('0') << (int)RGBGetBValue(c);
            if (i < 6)
                ss << ",";
        }
        ss << "," << std::dec << modes[active_mode].brightness;
        WriteSysfs("zones", ss.str());
    }
    else if (modes[active_mode].name == "Static")
    {
        RGBColor c = modes[active_mode].colors[0];
        std::ostringstream ss;
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)RGBGetRValue(c)
           << std::hex << std::setw(2) << std::setfill('0') << (int)RGBGetGValue(c)
           << std::hex << std::setw(2) << std::setfill('0') << (int)RGBGetBValue(c)
           << "," << std::dec << modes[active_mode].brightness;
        WriteSysfs("static_color", ss.str());
    }
}

void RGBController_AcerPredatorLightbar::UpdateZoneLEDs(int /*zone*/)
{
    DeviceUpdateLEDs();
}

void RGBController_AcerPredatorLightbar::UpdateSingleLED(int led)
{
    if (led >= 0 && led < 7)
    {
        RGBColor c = colors[led];
        std::ostringstream ss;
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)RGBGetRValue(c)
           << std::hex << std::setw(2) << std::setfill('0') << (int)RGBGetGValue(c)
           << std::hex << std::setw(2) << std::setfill('0') << (int)RGBGetBValue(c);
        WriteSysfs("zone" + std::to_string(led + 1), ss.str());
    }
}

void RGBController_AcerPredatorLightbar::DeviceUpdateMode()
{
    const mode& cur = modes[active_mode];

    if (cur.name == "Direct" || cur.name == "Static")
    {
        DeviceUpdateLEDs();
        return;
    }

    if (cur.name == "Off")
    {
        WriteSysfs("animation", "off");
        return;
    }

    std::string effect_name;
    if (cur.name == "Breathing")    effect_name = "breathing";
    else if (cur.name == "Neon")    effect_name = "neon";
    else if (cur.name == "Wave")    effect_name = "wave";
    else if (cur.name == "Snake")   effect_name = "snake";
    else if (cur.name == "Lightning") effect_name = "lightning";
    else if (cur.name == "Stack")   effect_name = "stack";
    else if (cur.name == "Motion Point") effect_name = "motion_point";
    else if (cur.name == "Zoom")    effect_name = "zoom";

    int direction = (cur.direction == MODE_DIRECTION_RIGHT) ? 1 : 2;

    std::ostringstream ss;
    ss << effect_name << ","
       << cur.speed << ","
       << cur.brightness << ","
       << direction;

    WriteSysfs("animation", ss.str());
}
```

---

## 6. Summary Checklist for OpenRGB Plugin Developers

1. **Detection:** Check if `/sys/devices/platform/acer-wmi/rear_lightbar/zones` exists and is accessible.
2. **Zones:** Register 1 linear zone with 7 LEDs.
3. **Colors:** Support full Direct RGB using the comma-separated hex batch payload to `zones`.
4. **Hardware Effects:** Map the 8 hardware effects (`breathing`, `neon`, `wave`, `snake`, `lightning`, `stack`, `motion_point`, `zoom`) to `animation`. Do not present color pickers for hardware effects since EC firmware ignores them.
5. **Software Effects:** Use OpenRGB's built-in Effects Engine on the **Direct** mode for custom-color animations (e.g. single-color pulse, audio visualizer, gradient shifts).
