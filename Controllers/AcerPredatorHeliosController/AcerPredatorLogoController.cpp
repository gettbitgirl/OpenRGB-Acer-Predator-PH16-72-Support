/*---------------------------------------------------------*\
| AcerPredatorLogoController.cpp                            |
|                                                           |
|   Driver for Acer Predator Cover/Lid Logo (Darfon)        |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "AcerPredatorLogoController.h"
#include <algorithm>
#include <chrono>
#include <thread>

AcerPredatorLogoController::AcerPredatorLogoController(hid_device* dev_handle, const char* path)
{
    this->dev         = dev_handle;
    this->location    = path;
    this->device_name = "Acer Predator Lid Logo";
    this->serial      = "";

    wchar_t serial_buf[256];
    if(dev && hid_get_serial_number_string(dev, serial_buf, 256) >= 0)
    {
        std::wstring ws(serial_buf);
        serial = std::string(ws.begin(), ws.end());
    }
}

AcerPredatorLogoController::~AcerPredatorLogoController()
{
    if(dev)
    {
        hid_close(dev);
    }
}

std::string AcerPredatorLogoController::GetDeviceName() const
{
    return(device_name);
}

std::string AcerPredatorLogoController::GetLocation() const
{
    return("HID: " + location);
}

std::string AcerPredatorLogoController::GetSerial() const
{
    return(serial);
}

uint8_t AcerPredatorLogoController::CalculateChecksum(const uint8_t* report, size_t len)
{
    uint8_t sum = 0;
    for(size_t i = 1; i < len - 1; i++)
    {
        sum += report[i];
    }
    return (uint8_t)(~sum);
}

void AcerPredatorLogoController::SendFeatureReport(const uint8_t* report)
{
    hid_send_feature_report(dev, report, ACER_PREDATOR_LOGO_FEATURE_SIZE);

    /*
       DarfonUSBLogo.dll waits 15 ms after every feature report.  The
       controller drops later reports if they arrive before it has processed
       the preceding one.
    */
    std::this_thread::sleep_for(std::chrono::milliseconds(15));
}

void AcerPredatorLogoController::SetLEDColor(RGBColor color, uint8_t brightness)
{
    SetMode(ACER_PREDATOR_LOGO_MODE_STATIC, 5, brightness, color);
}

void AcerPredatorLogoController::SetMode(uint8_t mode, uint8_t speed, uint8_t brightness, RGBColor color)
{
    std::lock_guard<std::mutex> lock(device_mutex);
    if(!dev)
    {
        return;
    }

    /*
       This is the transaction used by DarfonUSBLogo.dll's logo-update
       routine.  A single colour/effect report (the old implementation) is
       accepted by hidapi but ignored by the firmware because the controller
       has not been woken and reset first.
    */
    const uint8_t wake[ACER_PREDATOR_LOGO_FEATURE_SIZE] =
    {
        0x00, 0xB1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4E
    };
    const uint8_t reset[ACER_PREDATOR_LOGO_FEATURE_SIZE] =
    {
        0x00, 0x08, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF5
    };

    /* Firmware speed is inverted: OpenRGB 1..9 maps to 9..1. */
    const uint8_t firmware_speed = (uint8_t)(10 - std::min<uint8_t>(std::max<uint8_t>(speed, 1), 9));
    uint8_t effect[ACER_PREDATOR_LOGO_FEATURE_SIZE] =
    {
        0x00, 0x08, 0x02, mode, firmware_speed, brightness, 0x08, 0x01, 0x00
    };
    effect[8] = CalculateChecksum(effect, sizeof(effect));

    uint8_t colour[ACER_PREDATOR_LOGO_FEATURE_SIZE] =
    {
        0x00, 0x14, 0x00, 0x00,
        RGBGetRValue(color), RGBGetGValue(color), RGBGetBValue(color), 0x00, 0x00
    };
    colour[8] = CalculateChecksum(colour, sizeof(colour));

    SendFeatureReport(wake);
    SendFeatureReport(reset);
    SendFeatureReport(effect);
    SendFeatureReport(colour);
}
