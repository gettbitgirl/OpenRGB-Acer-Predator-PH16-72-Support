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
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <hidapi.h>
#include "RGBControllerInterface.h"

/*---------------------------------------------------------*\
| Sunrex USB Keyboard Protocol Constants                    |
|                                                           |
| The Sunrex MCU expects a 4-byte-per-LED buffer format:   |
|   [0x00, R, G, B] per LED position (32-bit aligned).     |
|                                                           |
| Main keyboard: 126 logical column positions x 4 bytes    |
|   = 504 bytes, transmitted in 8 x 64-byte chunks.        |
| MagKey WASD:   12 mini-LEDs x 4 bytes = 48 bytes,        |
|   transmitted in 1 x 64-byte chunk (16 zero-padded).     |
|                                                           |
| On Linux: no inter-chunk sleeps needed for hid_write().  |
| 15ms sleeps are only required before/after feature       |
| reports (hid_send_feature_report).                       |
\*---------------------------------------------------------*/
#define ACER_PREDATOR_NUM_LEDS          133
#define ACER_PREDATOR_KB_POSITIONS      126   /* Logical column positions (incl. gaps) */
#define ACER_PREDATOR_KB_PAYLOAD_SIZE   (ACER_PREDATOR_KB_POSITIONS * 4)  /* 504 bytes */
#define ACER_PREDATOR_KB_CHUNKS         8     /* ceil(504/64) = 8 chunks of 64 bytes   */
#define ACER_PREDATOR_MAG_LEDS          12    /* 12 mini-LEDs for WASD                 */
#define ACER_PREDATOR_MAG_PAYLOAD_SIZE  (ACER_PREDATOR_MAG_LEDS * 4)     /* 48 bytes  */
#define ACER_PREDATOR_PACKET_SIZE       65    /* Report ID (0x00) + 64 bytes data      */
#define ACER_PREDATOR_FEATURE_SIZE      9

/*---------------------------------------------------------*\
| Hardware Effect Opcodes (Main Keyboard)                   |
\*---------------------------------------------------------*/
enum
{
    ACER_PREDATOR_MODE_OFF          = 0x00,
    ACER_PREDATOR_MODE_STATIC       = 0x01,
    ACER_PREDATOR_MODE_BREATHING    = 0x02,
    ACER_PREDATOR_MODE_WAVE         = 0x03,
    ACER_PREDATOR_MODE_SNAKE        = 0x05,
    ACER_PREDATOR_MODE_RIPPLE       = 0x06,
    ACER_PREDATOR_MODE_NEON         = 0x08,
    ACER_PREDATOR_MODE_RAIN         = 0x0A,
    ACER_PREDATOR_MODE_LIGHTNING    = 0x12,
    ACER_PREDATOR_MODE_PHOSPHOR     = 0x25,
    ACER_PREDATOR_MODE_STARS        = 0x26,
    ACER_PREDATOR_MODE_FIREBALL     = 0x27,
    ACER_PREDATOR_MODE_SNOW         = 0x28,
    ACER_PREDATOR_MODE_HEARTBEAT    = 0x29,
    ACER_PREDATOR_MODE_ZOOM         = 0x2A,
    ACER_PREDATOR_MODE_METEOR       = 0x2B,
    ACER_PREDATOR_MODE_DIRECT       = 0x33, /* Direct streaming mode commit opcode */
};

/*---------------------------------------------------------*\
| MagKey WASD Subsystem Opcodes                             |
\*---------------------------------------------------------*/
enum
{
    ACER_PREDATOR_MAG_FOLLOW        = -1,   /* Internal: slave mini-LEDs to base key  */
    ACER_PREDATOR_MAG_OFF           = 0x40, /* MCU: slave mini-LEDs to base key color */
    ACER_PREDATOR_MAG_STATIC        = 0x41,
    ACER_PREDATOR_MAG_BREATHING     = 0x42,
    ACER_PREDATOR_MAG_WAVE          = 0x43,
    ACER_PREDATOR_MAG_SNAKE         = 0x44,
    ACER_PREDATOR_MAG_NEON          = 0x45,
    ACER_PREDATOR_MAG_SPOT          = 0x46,
    ACER_PREDATOR_MAG_STAR          = 0x47,
    ACER_PREDATOR_MAG_RAINBOW       = 0x48,
    ACER_PREDATOR_MAG_SLASH         = 0x49,
    ACER_PREDATOR_MAG_ZOOM          = 0x4A,
    ACER_PREDATOR_MAG_BLASTING      = 0x4E,
    ACER_PREDATOR_MAG_DIRECT        = 0x4F, /* Direct streaming mode for MagKey       */
};

struct AcerPredatorState
{
    uint8_t                     mode;
    uint8_t                     speed;
    uint8_t                     brightness;
    uint8_t                     direction;
    uint8_t                     color_preset; /* 0xE0 = rainbow/random, 0x00 = custom RGB */
    uint8_t                     red;
    uint8_t                     green;
    uint8_t                     blue;
    int                         magkey_mode;
    uint8_t                     magkey_speed;
    uint8_t                     magkey_brightness;
    uint8_t                     magkey_direction;
    uint8_t                     magkey_red;
    uint8_t                     magkey_green;
    uint8_t                     magkey_blue;

    /*---------------------------------------------------------*\
    | 4-byte-per-LED hardware buffers                           |
    | kb_buffer:  126 * 4 = 504 bytes (main keyboard)          |
    | mag_buffer: 12  * 4 = 48  bytes (MagKey mini-LEDs)       |
    \*---------------------------------------------------------*/
    std::vector<uint8_t>        kb_buffer;
    std::vector<uint8_t>        mag_buffer;
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
    void SendDirectFrame(const AcerPredatorState& state);
    void SendHardwareEffect(const AcerPredatorState& state);
    void SendMagKeyDirect(const AcerPredatorState& state);
    void SendMagKeyEffect(const AcerPredatorState& state);
    static uint8_t CalculateChecksum(const uint8_t* report, size_t len);
};
