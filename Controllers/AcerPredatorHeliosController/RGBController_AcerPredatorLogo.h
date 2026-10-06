/*---------------------------------------------------------*\
| RGBController_AcerPredatorLogo.h                          |
|                                                           |
|   Driver for Acer Predator Cover/Lid Logo (Darfon)        |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <memory>
#include "RGBController.h"
#include "AcerPredatorLogoController.h"

class RGBController_AcerPredatorLogo : public RGBController
{
public:
    RGBController_AcerPredatorLogo(std::shared_ptr<AcerPredatorLogoController> controller);
    ~RGBController_AcerPredatorLogo();

    void SetupZones();

    void DeviceUpdateLEDs() override;
    void DeviceUpdateZoneLEDs(int zone) override;
    void DeviceUpdateSingleLED(int led) override;
    void DeviceUpdateMode() override;

private:
    std::shared_ptr<AcerPredatorLogoController> controller;
};
