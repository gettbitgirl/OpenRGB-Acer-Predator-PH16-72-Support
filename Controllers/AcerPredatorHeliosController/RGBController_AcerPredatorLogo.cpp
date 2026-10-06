/*---------------------------------------------------------*\
| RGBController_AcerPredatorLogo.cpp                        |
|                                                           |
|   Driver for Acer Predator Cover/Lid Logo (Darfon)        |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "RGBController_AcerPredatorLogo.h"
#include <string>

/**------------------------------------------------------------------*\
    @name Acer Predator Lid Logo
    @category Laptop
    @type HID
    @save :x:
    @direct :white_check_mark:
    @effects :white_check_mark:
    @detectors DetectAcerPredatorHeliosLogoControllers
    @comment Acer Predator Helios Lid Cover Emblem (Darfon)
\*-------------------------------------------------------------------*/

RGBController_AcerPredatorLogo::RGBController_AcerPredatorLogo(std::shared_ptr<AcerPredatorLogoController> controller)
{
    this->controller    = controller;

    name                = "Acer Predator Lid Logo";
    vendor              = "Acer";
    type                = DEVICE_TYPE_LAPTOP;
    description         = "Acer Predator Lid Cover Logo (Darfon)";
    location            = controller->GetLocation();
    serial              = controller->GetSerial();

    mode Direct;
    Direct.name         = "Direct";
    Direct.value        = ACER_PREDATOR_LOGO_MODE_DIRECT;
    Direct.flags        = MODE_FLAG_HAS_PER_LED_COLOR | MODE_FLAG_HAS_BRIGHTNESS;
    Direct.color_mode   = MODE_COLORS_PER_LED;
    Direct.brightness_min = 0;
    Direct.brightness_max = 100;
    Direct.brightness   = 100;
    modes.push_back(Direct);

    mode Static;
    Static.name         = "Static";
    Static.value        = ACER_PREDATOR_LOGO_MODE_STATIC;
    Static.flags        = MODE_FLAG_HAS_MODE_SPECIFIC_COLOR | MODE_FLAG_HAS_BRIGHTNESS;
    Static.color_mode   = MODE_COLORS_MODE_SPECIFIC;
    Static.brightness_min = 0;
    Static.brightness_max = 100;
    Static.brightness   = 100;
    Static.colors_min   = 1;
    Static.colors_max   = 1;
    Static.colors.resize(1, 0x00AEC7);
    modes.push_back(Static);

    mode Breathing;
    Breathing.name      = "Breathing";
    Breathing.value     = ACER_PREDATOR_LOGO_MODE_BREATHING;
    Breathing.flags     = MODE_FLAG_HAS_MODE_SPECIFIC_COLOR | MODE_FLAG_HAS_BRIGHTNESS | MODE_FLAG_HAS_SPEED;
    Breathing.color_mode = MODE_COLORS_MODE_SPECIFIC;
    Breathing.brightness_min = 0;
    Breathing.brightness_max = 100;
    Breathing.brightness = 100;
    Breathing.speed_min = 1;
    Breathing.speed_max = 9;
    Breathing.speed     = 5;
    Breathing.colors_min = 1;
    Breathing.colors_max = 1;
    Breathing.colors.resize(1, 0x00AEC7);
    modes.push_back(Breathing);

    mode Neon;
    Neon.name           = "Neon";
    Neon.value          = ACER_PREDATOR_LOGO_MODE_NEON;
    Neon.flags          = MODE_FLAG_HAS_BRIGHTNESS | MODE_FLAG_HAS_SPEED;
    Neon.color_mode     = MODE_COLORS_NONE;
    Neon.brightness_min = 0;
    Neon.brightness_max = 100;
    Neon.brightness     = 100;
    Neon.speed_min      = 1;
    Neon.speed_max      = 9;
    Neon.speed          = 5;
    modes.push_back(Neon);

    SetupZones();
}

RGBController_AcerPredatorLogo::~RGBController_AcerPredatorLogo()
{
    Shutdown();
}

void RGBController_AcerPredatorLogo::SetupZones()
{
    zone logo_zone;
    logo_zone.name       = "Lid Logo";
    logo_zone.type       = ZONE_TYPE_LINEAR;
    logo_zone.leds_min   = 3;
    logo_zone.leds_max   = 3;
    logo_zone.leds_count = 3;
    zones.push_back(logo_zone);

    for(unsigned int i = 0; i < 3; i++)
    {
        led logo_led;
        logo_led.name  = "Logo Zone " + std::to_string(i + 1);
        logo_led.value = i;
        leds.push_back(logo_led);
    }

    SetupColors();
}

void RGBController_AcerPredatorLogo::DeviceUpdateLEDs()
{
    mode& current_mode = modes[active_mode];

    if(current_mode.value == ACER_PREDATOR_LOGO_MODE_DIRECT)
    {
        controller->SetDirect(colors.data(), colors.size(), (uint8_t)current_mode.brightness);
        return;
    }

    RGBColor color     = colors.size() > 0 ? colors[0] : 0;

    if(current_mode.color_mode == MODE_COLORS_MODE_SPECIFIC && current_mode.colors.size() > 0)
    {
        color = current_mode.colors[0];
    }

    controller->SetMode((uint8_t)current_mode.value, (uint8_t)current_mode.speed, (uint8_t)current_mode.brightness, color);
}

void RGBController_AcerPredatorLogo::DeviceUpdateZoneLEDs(int)
{
    DeviceUpdateLEDs();
}

void RGBController_AcerPredatorLogo::DeviceUpdateSingleLED(int)
{
    DeviceUpdateLEDs();
}

void RGBController_AcerPredatorLogo::DeviceUpdateMode()
{
    DeviceUpdateLEDs();
}
