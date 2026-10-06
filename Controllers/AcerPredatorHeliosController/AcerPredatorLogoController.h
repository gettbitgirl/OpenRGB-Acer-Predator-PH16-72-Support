/*---------------------------------------------------------*\
| AcerPredatorLogoController.h                              |
|                                                           |
|   Driver for Acer Predator Cover/Lid Logo (Darfon)        |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <mutex>
#include <string>
#include <hidapi.h>
#include "RGBControllerInterface.h"

#define ACER_PREDATOR_LOGO_FEATURE_SIZE 9

enum
{
    ACER_PREDATOR_LOGO_MODE_STATIC    = 0x00,
    ACER_PREDATOR_LOGO_MODE_BREATHING = 0x01,
    ACER_PREDATOR_LOGO_MODE_NEON      = 0x04,
    ACER_PREDATOR_LOGO_MODE_RAIN      = 0x03,
};

class AcerPredatorLogoController
{
public:
    AcerPredatorLogoController(hid_device* dev_handle, const char* path);
    ~AcerPredatorLogoController();

    std::string GetDeviceName() const;
    std::string GetLocation()   const;
    std::string GetSerial()     const;

    void SetLEDColor(RGBColor color, uint8_t brightness);
    void SetMode(uint8_t mode, uint8_t speed, uint8_t brightness, RGBColor color);

private:
    hid_device* dev;
    std::string device_name;
    std::string location;
    std::string serial;
    std::mutex  device_mutex;

    static uint8_t CalculateChecksum(const uint8_t* report, size_t len);
};
