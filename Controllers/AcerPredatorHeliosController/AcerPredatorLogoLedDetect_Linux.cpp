/*---------------------------------------------------------*\
| AcerPredatorLogoLedDetect_Linux.cpp                       |
|                                                           |
|   Detector for Acer Predator EC Logo LED (Linux sysfs)    |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include <unistd.h>
#include "DetectionManager.h"
#include "RGBController_AcerPredatorLogoLed_Linux.h"

static const char* candidate_paths[] =
{
    "/sys/devices/platform/acer-wmi/logo_led",
    "/sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/logo_led"
};

DetectedControllers DetectAcerPredatorLogoLedLinuxControllers()
{
    DetectedControllers detected_controllers;

    for(const char* path : candidate_paths)
    {
        if(!access(path, F_OK))
        {
            RGBController_AcerPredatorLogoLedLinux* controller = new RGBController_AcerPredatorLogoLedLinux(path);
            detected_controllers.push_back(controller);
            break;
        }
    }

    return(detected_controllers);
}

REGISTER_DETECTOR("Acer Predator Logo LED", DetectAcerPredatorLogoLedLinuxControllers);

REGISTER_CUSTOM_UDEV_RULE(acer_logo_led, "Acer Predator Logo LED (acer-wmi)",
    "ACTION==\"add\", SUBSYSTEM==\"platform\", KERNEL==\"acer-wmi\", RUN+=\"/bin/sh -c 'chmod 0666 /sys/devices/platform/acer-wmi/logo_led 2>/dev/null || true'\"");
