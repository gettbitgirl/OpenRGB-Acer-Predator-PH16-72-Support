/*---------------------------------------------------------*\
| RGBController_AcerPredatorHelios.h                        |
|                                                           |
|   Driver for Acer Predator Helios 16/18 Keyboard (Sunrex) |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <memory>
#include "RGBController.h"
#include "AcerPredatorHeliosController.h"

class RGBController_AcerPredatorHelios : public RGBController
{
public:
    RGBController_AcerPredatorHelios(std::shared_ptr<AcerPredatorHeliosController> controller);
    ~RGBController_AcerPredatorHelios();

    void SetupZones();

    void DeviceUpdateLEDs() override;
    void DeviceUpdateZoneLEDs(int zone) override;
    void DeviceUpdateSingleLED(int led) override;
    void DeviceUpdateMode() override;

    void InitDeviceSpecificConfiguration();
    void DeviceConfigureDevice() override;
    void DeviceUpdateDeviceSpecificConfiguration() override;

private:
    std::shared_ptr<AcerPredatorHeliosController> controller;
    bool                                          slave_magkeys_in_direct;
};
