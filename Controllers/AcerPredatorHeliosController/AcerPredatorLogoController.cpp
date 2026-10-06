/*---------------------------------------------------------*\
| AcerPredatorLogoController.cpp                            |
|                                                           |
|   Driver for Acer Predator Cover/Lid Logo (Darfon)        |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "AcerPredatorLogoController.h"
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

void AcerPredatorLogoController::SetLEDColor(RGBColor color, uint8_t brightness)
{
    SetMode(ACER_PREDATOR_LOGO_MODE_STATIC, 0, brightness, color);
}

void AcerPredatorLogoController::SetMode(uint8_t mode, uint8_t speed, uint8_t brightness, RGBColor color)
{
    std::lock_guard<std::mutex> lock(device_mutex);
    if(!dev)
    {
        return;
    }

    uint8_t report[ACER_PREDATOR_LOGO_FEATURE_SIZE];
    report[0] = 0x00;
    report[1] = mode;
    report[2] = 0x00;
    report[3] = RGBGetRValue(color);
    report[4] = RGBGetGValue(color);
    report[5] = RGBGetBValue(color);
    report[6] = (mode == ACER_PREDATOR_LOGO_MODE_STATIC) ? brightness : speed;
    report[7] = 0x00;
    report[8] = CalculateChecksum(report, ACER_PREDATOR_LOGO_FEATURE_SIZE);

    hid_send_feature_report(dev, report, ACER_PREDATOR_LOGO_FEATURE_SIZE);
    std::this_thread::sleep_for(std::chrono::milliseconds(15));
}
