/*---------------------------------------------------------*\
| AcerPredatorHeliosController.cpp                          |
|                                                           |
|   Driver for Acer Predator Helios 16/18 Keyboard (Sunrex) |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "AcerPredatorHeliosController.h"
#include <cstring>

static const uint8_t PREAMBLE_STANDARD[ACER_PREDATOR_FEATURE_SIZE] =
{
    0x00, 0x12, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0xE5
};

static const uint8_t PREAMBLE_MAGKEY[ACER_PREDATOR_FEATURE_SIZE] =
{
    0x00, 0x13, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0xE4
};

AcerPredatorHeliosController::AcerPredatorHeliosController(hid_device* dev_handle, const char* path)
{
    this->dev           = dev_handle;
    this->location      = path;
    this->device_name   = "Acer Predator Helios Keyboard";
    this->serial        = "";
    this->state_pending = false;
    this->thread_run    = true;

    wchar_t serial_buf[256];
    if(dev && hid_get_serial_number_string(dev, serial_buf, 256) >= 0)
    {
        std::wstring ws(serial_buf);
        serial = std::string(ws.begin(), ws.end());
    }

    worker_thread = std::thread(&AcerPredatorHeliosController::WorkerLoop, this);
}

AcerPredatorHeliosController::~AcerPredatorHeliosController()
{
    thread_run = false;
    cv.notify_all();

    if(worker_thread.joinable())
    {
        worker_thread.join();
    }

    if(dev)
    {
        hid_close(dev);
    }
}

std::string AcerPredatorHeliosController::GetLocation() const
{
    return("HID: " + location);
}

std::string AcerPredatorHeliosController::GetDeviceName() const
{
    return(device_name);
}

std::string AcerPredatorHeliosController::GetSerial() const
{
    return(serial);
}

void AcerPredatorHeliosController::SetHardwareState(const AcerPredatorState& new_state)
{
    {
        std::lock_guard<std::mutex> lock(state_mutex);
        pending_state = new_state;
        state_pending = true;
    }
    cv.notify_one();
}

uint8_t AcerPredatorHeliosController::CalculateChecksum(const uint8_t* report, size_t len)
{
    uint8_t sum = 0;
    for(size_t i = 1; i < len - 1; i++)
    {
        sum += report[i];
    }
    return (uint8_t)(~sum);
}

void AcerPredatorHeliosController::WorkerLoop()
{
    while(thread_run)
    {
        AcerPredatorState current_state;

        {
            std::unique_lock<std::mutex> lock(state_mutex);
            cv.wait(lock, [this]() { return state_pending || !thread_run; });

            if(!thread_run)
            {
                break;
            }

            current_state = pending_state;
            state_pending = false;
        }

        SendFrame(current_state);
    }
}

void AcerPredatorHeliosController::SendFrame(const AcerPredatorState& state)
{
    if(!dev)
    {
        return;
    }

    /*---------------------------------------------------------*\
    | Phase 1: Send standard keyboard preamble                  |
    \*---------------------------------------------------------*/
    hid_send_feature_report(dev, PREAMBLE_STANDARD, ACER_PREDATOR_FEATURE_SIZE);
    std::this_thread::sleep_for(std::chrono::milliseconds(15));

    /*---------------------------------------------------------*\
    | Phase 2: Stream payload data (7 packets of 64 bytes)      |
    \*---------------------------------------------------------*/
    uint8_t payload[448];
    std::memset(payload, 0, sizeof(payload));

    size_t copy_len = std::min(state.rgb_buffer.size(), (size_t)ACER_PREDATOR_PAYLOAD_SIZE);
    if(copy_len > 0)
    {
        std::memcpy(payload, state.rgb_buffer.data(), copy_len);
    }

    for(size_t packet_idx = 0; packet_idx < 7; packet_idx++)
    {
        uint8_t packet[ACER_PREDATOR_PACKET_SIZE];
        packet[0] = 0x00; // Report ID
        std::memcpy(&packet[1], &payload[packet_idx * 64], 64);

        hid_write(dev, packet, ACER_PREDATOR_PACKET_SIZE);
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }

    /*---------------------------------------------------------*\
    | Phase 3: Send primary keyboard commit feature report      |
    \*---------------------------------------------------------*/
    uint8_t commit[ACER_PREDATOR_FEATURE_SIZE];
    commit[0] = 0x00; // Report ID
    commit[1] = 0x08; // Length / Window
    commit[2] = 0x02; // Target Subsystem
    commit[3] = (state.mode == (uint8_t)ACER_PREDATOR_MODE_DIRECT) ? (uint8_t)ACER_PREDATOR_MODE_STATIC : state.mode;
    commit[4] = state.speed;
    commit[5] = state.brightness;
    commit[6] = state.direction;
    commit[7] = state.palette;
    commit[8] = CalculateChecksum(commit, ACER_PREDATOR_FEATURE_SIZE);

    hid_send_feature_report(dev, commit, ACER_PREDATOR_FEATURE_SIZE);
    std::this_thread::sleep_for(std::chrono::milliseconds(15));

    /*---------------------------------------------------------*\
    | Phase 4: Handle MagKey subsystem transaction              |
    \*---------------------------------------------------------*/
    uint8_t mag_opcode = ACER_PREDATOR_MAG_OFF;
    if(state.magkey_mode != ACER_PREDATOR_MAG_FOLLOW && state.magkey_mode >= 0)
    {
        mag_opcode = (uint8_t)state.magkey_mode;
    }

    hid_send_feature_report(dev, PREAMBLE_MAGKEY, ACER_PREDATOR_FEATURE_SIZE);
    std::this_thread::sleep_for(std::chrono::milliseconds(15));

    uint8_t mag_commit[ACER_PREDATOR_FEATURE_SIZE];
    mag_commit[0] = 0x00;
    mag_commit[1] = 0x08;
    mag_commit[2] = 0x02;
    mag_commit[3] = mag_opcode;
    mag_commit[4] = state.magkey_speed;
    mag_commit[5] = state.magkey_brightness;
    mag_commit[6] = 0x01;
    mag_commit[7] = 0x00;
    mag_commit[8] = CalculateChecksum(mag_commit, ACER_PREDATOR_FEATURE_SIZE);

    hid_send_feature_report(dev, mag_commit, ACER_PREDATOR_FEATURE_SIZE);
    std::this_thread::sleep_for(std::chrono::milliseconds(15));
}
