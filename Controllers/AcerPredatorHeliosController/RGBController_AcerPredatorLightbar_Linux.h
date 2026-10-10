/*---------------------------------------------------------*\
| RGBController_AcerPredatorLightbar_Linux.h                |
|                                                           |
|   Driver for Acer Predator Rear Lightbar (Linux sysfs)    |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include "RGBController.h"
#include <string>

#define ACER_PREDATOR_LIGHTBAR_SYSFS_PATH "/sys/devices/platform/acer-wmi/rear_lightbar/"

class RGBController_AcerPredatorLightbarLinux : public RGBController
{
public:
    RGBController_AcerPredatorLightbarLinux(const std::string& sysfs_path = ACER_PREDATOR_LIGHTBAR_SYSFS_PATH);
    ~RGBController_AcerPredatorLightbarLinux();

    void SetupZones();

    void DeviceUpdateLEDs() override;
    void DeviceUpdateZoneLEDs(int zone) override;
    void DeviceUpdateSingleLED(int led) override;

    void DeviceUpdateMode() override;

private:
    std::string sysfs_base;
    void WriteSysfs(const std::string& attr, const std::string& value);
    std::string ReadSysfs(const std::string& attr);
};
