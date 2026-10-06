/*---------------------------------------------------------*\
| AcerPredatorHeliosDetect.cpp                              |
|                                                           |
|   Detector for Acer Predator Helios 16/18 Devices         |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include <vector>
#include "DetectionManager.h"
#include "AcerPredatorHeliosController.h"
#include "RGBController_AcerPredatorHelios.h"
#include "AcerPredatorLogoController.h"
#include "RGBController_AcerPredatorLogo.h"

#define ACER_SUNREX_VID         0x05AF
#define ACER_SUNREX_PID         0x666A
#define ACER_SUNREX_USAGE_PAGE  0xFF02
#define ACER_SUNREX_USAGE       0x0001

#define ACER_DARFON_VID         0x0D62
#define ACER_DARFON_PID         0xBA51
#define ACER_DARFON_USAGE_PAGE  0xFF01
#define ACER_DARFON_USAGE       0x0001

DetectedControllers DetectAcerPredatorHeliosControllers(hid_device_info* info, const std::string&)
{
    DetectedControllers detected_controllers;
    hid_device*         dev;

    dev = hid_open_path(info->path);
    if(dev)
    {
        std::shared_ptr<AcerPredatorHeliosController> controller = std::make_shared<AcerPredatorHeliosController>(dev, info->path);
        RGBController_AcerPredatorHelios* rgb_controller         = new RGBController_AcerPredatorHelios(controller);

        detected_controllers.push_back(rgb_controller);
    }

    return(detected_controllers);
}

DetectedControllers DetectAcerPredatorHeliosLogoControllers(hid_device_info* info, const std::string&)
{
    DetectedControllers detected_controllers;
    hid_device*         dev;

    dev = hid_open_path(info->path);
    if(dev)
    {
        std::shared_ptr<AcerPredatorLogoController> controller = std::make_shared<AcerPredatorLogoController>(dev, info->path);
        RGBController_AcerPredatorLogo* rgb_controller         = new RGBController_AcerPredatorLogo(controller);

        detected_controllers.push_back(rgb_controller);
    }

    return(detected_controllers);
}

REGISTER_HID_DETECTOR_I("Acer Predator Helios Keyboard", DetectAcerPredatorHeliosControllers, ACER_SUNREX_VID, ACER_SUNREX_PID, 3);
REGISTER_HID_DETECTOR_I("Acer Predator Lid Logo", DetectAcerPredatorHeliosLogoControllers, ACER_DARFON_VID, ACER_DARFON_PID, 0);
