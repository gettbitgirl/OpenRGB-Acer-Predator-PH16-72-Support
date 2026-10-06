/*---------------------------------------------------------*\
| AcerPredatorHeliosController.h                            |
|                                                           |
|   Driver for Acer Predator Helios 16/18 Keyboard (Sunrex) |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <hidapi.h>
#include "RGBControllerInterface.h"

#define ACER_PREDATOR_NUM_LEDS          133
#define ACER_PREDATOR_PAYLOAD_SIZE      (ACER_PREDATOR_NUM_LEDS * 3) // 399 bytes
#define ACER_PREDATOR_PACKET_SIZE       65                          // Report ID (0x00) + 64 bytes
#define ACER_PREDATOR_FEATURE_SIZE      9

enum
{
    ACER_PREDATOR_MODE_OFF          = 0x00,
    ACER_PREDATOR_MODE_STATIC       = 0x01,
    ACER_PREDATOR_MODE_BREATHING    = 0x03,
    ACER_PREDATOR_MODE_WAVE         = 0x05,
    ACER_PREDATOR_MODE_SNAKE        = 0x06,
    ACER_PREDATOR_MODE_RIPPLE       = 0x08,
    ACER_PREDATOR_MODE_NEON         = 0x0A,
    ACER_PREDATOR_MODE_RAIN         = 0x12,
    ACER_PREDATOR_MODE_LIGHTNING    = 0x22,
    ACER_PREDATOR_MODE_STARS        = 0x25,
    ACER_PREDATOR_MODE_FIREBALL     = 0x26,
    ACER_PREDATOR_MODE_SNOW         = 0x27,
    ACER_PREDATOR_MODE_HEARTBEAT    = 0x28,
    ACER_PREDATOR_MODE_SHIFTING     = 0x29,
    ACER_PREDATOR_MODE_ZOOM         = 0x2A,
    ACER_PREDATOR_MODE_METEOR       = 0x2B,
    ACER_PREDATOR_MODE_DIRECT       = 0xFF,
};

enum
{
    ACER_PREDATOR_MAG_FOLLOW        = -1,   // Internal OpenRGB: Follow global keyboard
    ACER_PREDATOR_MAG_OFF           = 0x40, // MCU hardware: slave mini-LEDs to base key
    ACER_PREDATOR_MAG_STATIC        = 0x41,
    ACER_PREDATOR_MAG_BREATHING     = 0x42,
    ACER_PREDATOR_MAG_WAVE          = 0x43,
    ACER_PREDATOR_MAG_SNAKE         = 0x44,
    ACER_PREDATOR_MAG_NEON          = 0x45,
    ACER_PREDATOR_MAG_SPOT          = 0x46,
    ACER_PREDATOR_MAG_STAR          = 0x47,
    ACER_PREDATOR_MAG_RAINBOW       = 0x48,
    ACER_PREDATOR_MAG_SLASH         = 0x49,
    ACER_PREDATOR_MAG_BLASTING      = 0x4E,
    ACER_PREDATOR_MAG_ZOOM          = 0x4F,
};

struct AcerPredatorState
{
    uint8_t                     mode;
    uint8_t                     speed;
    uint8_t                     brightness;
    uint8_t                     direction;
    uint8_t                     palette;
    int                         magkey_mode;
    uint8_t                     magkey_speed;
    uint8_t                     magkey_brightness;
    std::vector<uint8_t>        rgb_buffer;
};

class AcerPredatorHeliosController
{
public:
    AcerPredatorHeliosController(hid_device* dev_handle, const char* path);
    ~AcerPredatorHeliosController();

    std::string GetDeviceName() const;
    std::string GetLocation()   const;
    std::string GetSerial()     const;

    void SetHardwareState(const AcerPredatorState& new_state);

private:
    hid_device*                 dev;
    std::string                 device_name;
    std::string                 location;
    std::string                 serial;

    std::mutex                  state_mutex;
    std::condition_variable     cv;
    std::atomic<bool>           thread_run;
    bool                        state_pending;
    AcerPredatorState           pending_state;
    std::thread                 worker_thread;

    void WorkerLoop();
    void SendFrame(const AcerPredatorState& state);
    static uint8_t CalculateChecksum(const uint8_t* report, size_t len);
};
