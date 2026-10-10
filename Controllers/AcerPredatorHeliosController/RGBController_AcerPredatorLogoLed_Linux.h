/*---------------------------------------------------------*\
| RGBController_AcerPredatorLogoLed_Linux.h                 |
|                                                           |
|   Driver for Acer Predator EC Logo LED (Linux sysfs)      |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include "RGBController.h"
#include <string>

#define ACER_PREDATOR_LOGO_LED_SYSFS_PATH "/sys/devices/platform/acer-wmi/logo_led"

class RGBController_AcerPredatorLogoLedLinux : public RGBController
{
public:
    RGBController_AcerPredatorLogoLedLinux(const std::string& sysfs_path = ACER_PREDATOR_LOGO_LED_SYSFS_PATH);
    ~RGBController_AcerPredatorLogoLedLinux();

    void SetupZones();

    void DeviceUpdateLEDs() override;
    void DeviceUpdateZoneLEDs(int zone) override;
    void DeviceUpdateSingleLED(int led) override;

    void DeviceUpdateMode() override;

private:
    std::string sysfs_logo_led;
    void WriteLogo(const std::string& value);
    std::string ReadLogo();
};
