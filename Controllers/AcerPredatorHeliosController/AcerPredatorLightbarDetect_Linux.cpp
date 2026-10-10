/*---------------------------------------------------------*\
| AcerPredatorLightbarDetect_Linux.cpp                      |
|                                                           |
|   Detector for Acer Predator Rear Lightbar (Linux sysfs)  |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include <unistd.h>
#include "DetectionManager.h"
#include "RGBController_AcerPredatorLightbar_Linux.h"

DetectedControllers DetectAcerPredatorLightbarLinuxControllers()
{
    DetectedControllers detected_controllers;

    if(!access(ACER_PREDATOR_LIGHTBAR_SYSFS_PATH "zones", F_OK))
    {
        RGBController_AcerPredatorLightbarLinux* controller = new RGBController_AcerPredatorLightbarLinux();
        detected_controllers.push_back(controller);
    }

    return(detected_controllers);
}

REGISTER_DETECTOR("Acer Predator Rear Lightbar", DetectAcerPredatorLightbarLinuxControllers);

REGISTER_CUSTOM_UDEV_RULE(acer_rear_lightbar, "Acer Predator Rear Lightbar (acer-wmi)",
    "ACTION==\"add\", SUBSYSTEM==\"platform\", KERNEL==\"acer-wmi\", RUN+=\"/bin/sh -c 'chmod -R 0666 /sys/devices/platform/acer-wmi/rear_lightbar/*'\"");
