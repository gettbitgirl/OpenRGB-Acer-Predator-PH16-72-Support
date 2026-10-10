/*---------------------------------------------------------*\
| RGBController_AcerPredatorLightbar_Linux.cpp              |
|                                                           |
|   Driver for Acer Predator Rear Lightbar (Linux sysfs)    |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "RGBController_AcerPredatorLightbar_Linux.h"
#include <fstream>
#include <sstream>
#include <iomanip>

/**------------------------------------------------------------------*\
    @name Acer Predator Rear Lightbar
    @category Laptop
    @type Linux Sysfs
    @save :x:
    @direct :white_check_mark:
    @effects :white_check_mark:
    @detectors DetectAcerPredatorLightbarLinuxControllers
    @comment Acer Predator Helios Addressable Rear Lightbar (acer-wmi)
\*-------------------------------------------------------------------*/

RGBController_AcerPredatorLightbarLinux::RGBController_AcerPredatorLightbarLinux(const std::string& path)
    : sysfs_base(path.empty() ? ACER_PREDATOR_LIGHTBAR_SYSFS_PATH : path)
{
    name        = "Acer Predator Rear Lightbar";
    vendor      = "Acer";
    type        = DEVICE_TYPE_LAPTOP;
    description = "7-Zone Addressable Rear Exhaust Lightbar";
    location    = sysfs_base;

    /* Mode 0: Direct (Per-LED static control) */
    mode Direct;
    Direct.name           = "Direct";
    Direct.value          = 0;
    Direct.flags          = MODE_FLAG_HAS_PER_LED_COLOR | MODE_FLAG_HAS_BRIGHTNESS;
    Direct.brightness_min = 0;
    Direct.brightness_max = 100;
    Direct.brightness     = 100;
    Direct.color_mode     = MODE_COLORS_PER_LED;
    modes.push_back(Direct);

    /* Mode 1: Static (Solid color all zones) */
    mode Static;
    Static.name           = "Static";
    Static.value          = 1;
    Static.flags          = MODE_FLAG_HAS_MODE_SPECIFIC_COLOR | MODE_FLAG_HAS_BRIGHTNESS;
    Static.brightness_min = 0;
    Static.brightness_max = 100;
    Static.brightness     = 100;
    Static.colors_min     = 1;
    Static.colors_max     = 1;
    Static.colors.resize(1);
    Static.color_mode     = MODE_COLORS_MODE_SPECIFIC;
    modes.push_back(Static);

    /* Mode 2: Breathing (Hardware effect) */
    mode Breathing;
    Breathing.name           = "Breathing";
    Breathing.value          = 2;
    Breathing.flags          = MODE_FLAG_HAS_SPEED | MODE_FLAG_HAS_BRIGHTNESS;
    Breathing.speed_min      = 1;
    Breathing.speed_max      = 5;
    Breathing.speed          = 3;
    Breathing.brightness_min = 0;
    Breathing.brightness_max = 100;
    Breathing.brightness     = 100;
    Breathing.color_mode     = MODE_COLORS_NONE;
    modes.push_back(Breathing);

    /* Mode 3: Neon (Hardware effect) */
    mode Neon;
    Neon.name           = "Neon";
    Neon.value          = 3;
    Neon.flags          = MODE_FLAG_HAS_SPEED | MODE_FLAG_HAS_BRIGHTNESS;
    Neon.speed_min      = 1;
    Neon.speed_max      = 5;
    Neon.speed          = 3;
    Neon.brightness_min = 0;
    Neon.brightness_max = 100;
    Neon.brightness     = 100;
    Neon.color_mode     = MODE_COLORS_NONE;
    modes.push_back(Neon);

    /* Mode 4: Wave (Hardware effect) */
    mode Wave;
    Wave.name           = "Wave";
    Wave.value          = 4;
    Wave.flags          = MODE_FLAG_HAS_SPEED | MODE_FLAG_HAS_BRIGHTNESS | MODE_FLAG_HAS_DIRECTION_LR;
    Wave.speed_min      = 1;
    Wave.speed_max      = 5;
    Wave.speed          = 3;
    Wave.direction      = MODE_DIRECTION_LEFT;
    Wave.brightness_min = 0;
    Wave.brightness_max = 100;
    Wave.brightness     = 100;
    Wave.color_mode     = MODE_COLORS_NONE;
    modes.push_back(Wave);

    /* Mode 5: Snake */
    mode Snake;
    Snake.name           = "Snake";
    Snake.value          = 5;
    Snake.flags          = MODE_FLAG_HAS_SPEED | MODE_FLAG_HAS_BRIGHTNESS;
    Snake.speed_min      = 1;
    Snake.speed_max      = 5;
    Snake.speed          = 3;
    Snake.brightness_min = 0;
    Snake.brightness_max = 100;
    Snake.brightness     = 100;
    Snake.color_mode     = MODE_COLORS_NONE;
    modes.push_back(Snake);

    /* Mode 6: Lightning */
    mode Lightning;
    Lightning.name           = "Lightning";
    Lightning.value          = 6;
    Lightning.flags          = MODE_FLAG_HAS_SPEED | MODE_FLAG_HAS_BRIGHTNESS;
    Lightning.speed_min      = 1;
    Lightning.speed_max      = 5;
    Lightning.speed          = 3;
    Lightning.brightness_min = 0;
    Lightning.brightness_max = 100;
    Lightning.brightness     = 100;
    Lightning.color_mode     = MODE_COLORS_NONE;
    modes.push_back(Lightning);

    /* Mode 7: Stack */
    mode Stack;
    Stack.name           = "Stack";
    Stack.value          = 7;
    Stack.flags          = MODE_FLAG_HAS_SPEED | MODE_FLAG_HAS_BRIGHTNESS;
    Stack.speed_min      = 1;
    Stack.speed_max      = 5;
    Stack.speed          = 3;
    Stack.brightness_min = 0;
    Stack.brightness_max = 100;
    Stack.brightness     = 100;
    Stack.color_mode     = MODE_COLORS_NONE;
    modes.push_back(Stack);

    /* Mode 8: Motion Point */
    mode MotionPoint;
    MotionPoint.name           = "Motion Point";
    MotionPoint.value          = 8;
    MotionPoint.flags          = MODE_FLAG_HAS_SPEED | MODE_FLAG_HAS_BRIGHTNESS | MODE_FLAG_HAS_DIRECTION_LR;
    MotionPoint.speed_min      = 1;
    MotionPoint.speed_max      = 5;
    MotionPoint.speed          = 3;
    MotionPoint.direction      = MODE_DIRECTION_LEFT;
    MotionPoint.brightness_min = 0;
    MotionPoint.brightness_max = 100;
    MotionPoint.brightness     = 100;
    MotionPoint.color_mode     = MODE_COLORS_NONE;
    modes.push_back(MotionPoint);

    /* Mode 9: Zoom */
    mode Zoom;
    Zoom.name           = "Zoom";
    Zoom.value          = 9;
    Zoom.flags          = MODE_FLAG_HAS_SPEED | MODE_FLAG_HAS_BRIGHTNESS | MODE_FLAG_HAS_DIRECTION_LR;
    Zoom.speed_min      = 1;
    Zoom.speed_max      = 5;
    Zoom.speed          = 3;
    Zoom.direction      = MODE_DIRECTION_LEFT;
    Zoom.brightness_min = 0;
    Zoom.brightness_max = 100;
    Zoom.brightness     = 100;
    Zoom.color_mode     = MODE_COLORS_NONE;
    modes.push_back(Zoom);

    /* Mode 10: Off */
    mode Off;
    Off.name           = "Off";
    Off.value          = 10;
    Off.flags          = 0;
    Off.color_mode     = MODE_COLORS_NONE;
    modes.push_back(Off);

    SetupZones();
}

RGBController_AcerPredatorLightbarLinux::~RGBController_AcerPredatorLightbarLinux()
{
}

void RGBController_AcerPredatorLightbarLinux::SetupZones()
{
    zone lightbar_zone;
    lightbar_zone.name       = "Rear Lightbar";
    lightbar_zone.type       = ZONE_TYPE_LINEAR;
    lightbar_zone.leds_min   = 7;
    lightbar_zone.leds_max   = 7;
    lightbar_zone.leds_count = 7;
    zones.push_back(lightbar_zone);

    for (int i = 0; i < 7; i++)
    {
        led new_led;
        new_led.name  = "Zone " + std::to_string(i + 1);
        new_led.value = i;
        leds.push_back(new_led);
    }

    SetupColors();
}

void RGBController_AcerPredatorLightbarLinux::WriteSysfs(const std::string& attr, const std::string& value)
{
    std::ofstream file(sysfs_base + attr);
    if (file.is_open())
    {
        file << value;
        file.close();
    }
}

std::string RGBController_AcerPredatorLightbarLinux::ReadSysfs(const std::string& attr)
{
    std::ifstream file(sysfs_base + attr);
    std::string val;
    if (file.is_open())
    {
        std::getline(file, val);
        file.close();
    }
    return val;
}

void RGBController_AcerPredatorLightbarLinux::DeviceUpdateLEDs()
{
    /* If currently in Direct mode, push all 7 colors */
    if (modes[active_mode].name == "Direct")
    {
        std::ostringstream ss;
        for (int i = 0; i < 7; i++)
        {
            RGBColor c = colors[i];
            ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)RGBGetRValue(c)
               << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)RGBGetGValue(c)
               << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)RGBGetBValue(c);
            if (i < 6)
            {
                ss << ",";
            }
        }
        ss << "," << std::dec << modes[active_mode].brightness;
        WriteSysfs("zones", ss.str());
    }
    else if (modes[active_mode].name == "Static")
    {
        RGBColor c = modes[active_mode].colors.size() > 0 ? modes[active_mode].colors[0] : 0;
        std::ostringstream ss;
        ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)RGBGetRValue(c)
           << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)RGBGetGValue(c)
           << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)RGBGetBValue(c)
           << "," << std::dec << modes[active_mode].brightness;
        WriteSysfs("static_color", ss.str());
    }
}

void RGBController_AcerPredatorLightbarLinux::DeviceUpdateZoneLEDs(int /*zone*/)
{
    DeviceUpdateLEDs();
}

void RGBController_AcerPredatorLightbarLinux::DeviceUpdateSingleLED(int led)
{
    if (led >= 0 && led < 7)
    {
        RGBColor c = colors[led];
        std::ostringstream ss;
        ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)RGBGetRValue(c)
           << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)RGBGetGValue(c)
           << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)RGBGetBValue(c);
        WriteSysfs("zone" + std::to_string(led + 1), ss.str());
    }
}

void RGBController_AcerPredatorLightbarLinux::DeviceUpdateMode()
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
    if (cur.name == "Breathing")        effect_name = "breathing";
    else if (cur.name == "Neon")        effect_name = "neon";
    else if (cur.name == "Wave")        effect_name = "wave";
    else if (cur.name == "Snake")       effect_name = "snake";
    else if (cur.name == "Lightning")   effect_name = "lightning";
    else if (cur.name == "Stack")       effect_name = "stack";
    else if (cur.name == "Motion Point") effect_name = "motion_point";
    else if (cur.name == "Zoom")        effect_name = "zoom";

    int direction = (cur.direction == MODE_DIRECTION_RIGHT) ? 1 : 2;

    std::ostringstream ss;
    ss << effect_name << ","
       << cur.speed << ","
       << cur.brightness << ","
       << direction;

    WriteSysfs("animation", ss.str());
}
