/*---------------------------------------------------------*\
| RGBController_AcerPredatorLogoLed_Linux.cpp               |
|                                                           |
|   Driver for Acer Predator EC Logo LED (Linux sysfs)      |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "RGBController_AcerPredatorLogoLed_Linux.h"
#include <fstream>
#include <sstream>
#include <iomanip>

/**------------------------------------------------------------------*\
    @name Acer Predator Logo LED
    @category Laptop
    @type Linux Sysfs
    @save :x:
    @direct :white_check_mark:
    @effects :white_check_mark:
    @detectors DetectAcerPredatorLogoLedLinuxControllers
    @comment Acer Predator Helios EC-managed Logo LED (acer-wmi)
\*-------------------------------------------------------------------*/

RGBController_AcerPredatorLogoLedLinux::RGBController_AcerPredatorLogoLedLinux(const std::string& path)
    : sysfs_logo_led(path.empty() ? ACER_PREDATOR_LOGO_LED_SYSFS_PATH : path)
{
    name        = "Acer Predator Logo LED";
    vendor      = "Acer";
    type        = DEVICE_TYPE_LAPTOP;
    description = "EC-managed Predator logo RGB LED";
    location    = sysfs_logo_led;

    /* Mode 0: Direct (Static RGB) */
    mode Direct;
    Direct.name           = "Direct";
    Direct.value          = 0;
    Direct.flags          = MODE_FLAG_HAS_PER_LED_COLOR | MODE_FLAG_HAS_BRIGHTNESS;
    Direct.brightness_min = 0;
    Direct.brightness_max = 100;
    Direct.brightness     = 100;
    Direct.color_mode     = MODE_COLORS_PER_LED;
    modes.push_back(Direct);

    /* Mode 1: Breathing */
    mode Breathing;
    Breathing.name           = "Breathing";
    Breathing.value          = 1;
    Breathing.flags          = MODE_FLAG_HAS_SPEED | MODE_FLAG_HAS_BRIGHTNESS | MODE_FLAG_HAS_MODE_SPECIFIC_COLOR;
    Breathing.speed_min      = 1;
    Breathing.speed_max      = 5;
    Breathing.speed          = 3;
    Breathing.brightness_min = 0;
    Breathing.brightness_max = 100;
    Breathing.brightness     = 100;
    Breathing.colors_min     = 1;
    Breathing.colors_max     = 1;
    Breathing.colors.resize(1);
    Breathing.color_mode     = MODE_COLORS_MODE_SPECIFIC;
    modes.push_back(Breathing);

    /* Mode 2: Neon */
    mode Neon;
    Neon.name           = "Neon";
    Neon.value          = 2;
    Neon.flags          = MODE_FLAG_HAS_SPEED | MODE_FLAG_HAS_BRIGHTNESS;
    Neon.speed_min      = 1;
    Neon.speed_max      = 5;
    Neon.speed          = 3;
    Neon.brightness_min = 0;
    Neon.brightness_max = 100;
    Neon.brightness     = 100;
    Neon.color_mode     = MODE_COLORS_NONE;
    modes.push_back(Neon);

    /* Mode 3: Off */
    mode Off;
    Off.name           = "Off";
    Off.value          = 3;
    Off.flags          = 0;
    Off.color_mode     = MODE_COLORS_NONE;
    modes.push_back(Off);

    SetupZones();
}

RGBController_AcerPredatorLogoLedLinux::~RGBController_AcerPredatorLogoLedLinux()
{
    Shutdown();
}

void RGBController_AcerPredatorLogoLedLinux::SetupZones()
{
    zone logo_zone;
    logo_zone.name       = "Logo LED";
    logo_zone.type       = ZONE_TYPE_SINGLE;
    logo_zone.leds_min   = 1;
    logo_zone.leds_max   = 1;
    logo_zone.leds_count = 1;
    zones.push_back(logo_zone);

    led logo_led_item;
    logo_led_item.name  = "Logo LED";
    logo_led_item.value = 0;
    leds.push_back(logo_led_item);

    SetupColors();
}

void RGBController_AcerPredatorLogoLedLinux::WriteLogo(const std::string& value)
{
    std::ofstream file(sysfs_logo_led);
    if(file.is_open())
    {
        file << value;
        file.close();
    }
}

std::string RGBController_AcerPredatorLogoLedLinux::ReadLogo()
{
    std::ifstream file(sysfs_logo_led);
    std::string val;
    if(file.is_open())
    {
        std::getline(file, val);
        file.close();
    }
    return val;
}

void RGBController_AcerPredatorLogoLedLinux::DeviceUpdateLEDs()
{
    const mode& current = modes[active_mode];
    if(current.name != "Direct")
    {
        return;
    }

    RGBColor color = colors.size() > 0 ? colors[0] : 0;
    std::ostringstream ss;
    ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)RGBGetRValue(color)
       << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)RGBGetGValue(color)
       << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)RGBGetBValue(color)
       << "," << std::dec << current.brightness
       << ",5,static";

    WriteLogo(ss.str());
}

void RGBController_AcerPredatorLogoLedLinux::DeviceUpdateZoneLEDs(int /*zone*/)
{
    DeviceUpdateLEDs();
}

void RGBController_AcerPredatorLogoLedLinux::DeviceUpdateSingleLED(int /*led*/)
{
    DeviceUpdateLEDs();
}

void RGBController_AcerPredatorLogoLedLinux::DeviceUpdateMode()
{
    const mode& current = modes[active_mode];

    if(current.name == "Direct")
    {
        DeviceUpdateLEDs();
        return;
    }

    if(current.name == "Off")
    {
        WriteLogo("000000,0,5,off");
        return;
    }

    std::string effect = "static";
    if(current.name == "Breathing") effect = "breathing";
    else if(current.name == "Neon") effect = "neon";

    RGBColor color = 0;
    if(current.color_mode == MODE_COLORS_MODE_SPECIFIC && current.colors.size() > 0)
    {
        color = current.colors[0];
    }
    else if(colors.size() > 0)
    {
        color = colors[0];
    }

    std::ostringstream ss;
    ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)RGBGetRValue(color)
       << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)RGBGetGValue(color)
       << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)RGBGetBValue(color)
       << "," << std::dec << current.brightness << ","
       << current.speed << ","
       << effect;

    WriteLogo(ss.str());
}
