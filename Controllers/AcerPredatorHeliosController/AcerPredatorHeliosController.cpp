/*---------------------------------------------------------*\
| AcerPredatorHeliosController.cpp                          |
|                                                           |
|   Driver for Acer Predator Helios 16/18 Keyboard (Sunrex) |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "AcerPredatorHeliosController.h"
#include <chrono>
#include <cstring>
#include <thread>

/*---------------------------------------------------------*\
| Preamble Feature Reports (9 bytes)                        |
|   Opcode 0x12: Begin main keyboard stream                 |
|   Opcode 0x13: Begin MagKey WASD stream                   |
| Checksum = ~(sum of bytes[1..7]) & 0xFF                   |
|   0x12 preamble: ~(0x12 + 0x08) & 0xFF = 0xE5            |
|   0x13 preamble: ~(0x13 + 0x08) & 0xFF = 0xE4            |
\*---------------------------------------------------------*/
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
    /*---------------------------------------------------------*\
    | One's-complement checksum: ~(sum of bytes[1..len-2])     |
    | For a 9-byte report this covers bytes [1..7].            |
    \*---------------------------------------------------------*/
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

        if(current_state.mode == ACER_PREDATOR_MODE_DIRECT)
        {
            SendDirectFrame(current_state);
        }
        else
        {
            SendHardwareEffect(current_state);
        }

        if(current_state.magkey_mode == ACER_PREDATOR_MAG_DIRECT)
        {
            SendMagKeyDirect(current_state);
        }
        else
        {
            SendMagKeyEffect(current_state);
        }
    }
}

void AcerPredatorHeliosController::SendDirectFrame(const AcerPredatorState& state)
{
    if(!dev)
    {
        return;
    }

    /*---------------------------------------------------------*\
    | Phase 1: Send standard keyboard preamble                  |
    | On Linux: USB xHCI handles pacing at wire speed           |
    \*---------------------------------------------------------*/
    hid_send_feature_report(dev, PREAMBLE_STANDARD, ACER_PREDATOR_FEATURE_SIZE);

    /*---------------------------------------------------------*\
    | Phase 2: Stream payload (8 chunks of 64 bytes)            |
    | Buffer format: [0x00, R, G, B] per LED (4 bytes/LED).    |
    | Total: 126 positions * 4 = 504 bytes -> 512 bytes padded  |
    \*---------------------------------------------------------*/
    uint8_t payload[ACER_PREDATOR_KB_CHUNKS * 64];
    std::memset(payload, 0, sizeof(payload));

    size_t copy_len = std::min(state.kb_buffer.size(), (size_t)ACER_PREDATOR_KB_PAYLOAD_SIZE);
    if(copy_len > 0)
    {
        std::memcpy(payload, state.kb_buffer.data(), copy_len);
    }

    for(size_t chunk_idx = 0; chunk_idx < ACER_PREDATOR_KB_CHUNKS; chunk_idx++)
    {
        uint8_t packet[ACER_PREDATOR_PACKET_SIZE];
        packet[0] = 0x00; // Report ID
        std::memcpy(&packet[1], &payload[chunk_idx * 64], 64);
        hid_write(dev, packet, ACER_PREDATOR_PACKET_SIZE);
    }

    /*---------------------------------------------------------*\
    | Phase 3: Direct mode commit feature report                |
    | Opcode 0x33 = ACER_PREDATOR_MODE_DIRECT                  |
    | Checksum = ~(0x08 + 0x02 + 0x33 + 0x05 + bright + 0x08  |
    |             + direction) & 0xFF                           |
    | Simplified: ~(0x4B + brightness) & 0xFF                  |
    \*---------------------------------------------------------*/
    uint8_t commit[ACER_PREDATOR_FEATURE_SIZE];
    commit[0] = 0x00;                   // Report ID
    commit[1] = 0x08;                   // Subsystem Command
    commit[2] = 0x02;                   // Target
    commit[3] = ACER_PREDATOR_MODE_DIRECT; // 0x33: Direct streaming commit
    commit[4] = 0x05;                   // Speed (fixed at default 5 for Direct)
    commit[5] = state.brightness;       // Brightness
    commit[6] = 0x08;                   // Mode flag for Direct
    commit[7] = state.direction;        // Direction
    commit[8] = CalculateChecksum(commit, ACER_PREDATOR_FEATURE_SIZE);

    hid_send_feature_report(dev, commit, ACER_PREDATOR_FEATURE_SIZE);
}

void AcerPredatorHeliosController::SendHardwareEffect(const AcerPredatorState& state)
{
    if(!dev)
    {
        return;
    }

    /*---------------------------------------------------------*\
    | Mode OFF (0x00): Send power-down feature report           |
    \*---------------------------------------------------------*/
    if(state.mode == ACER_PREDATOR_MODE_OFF)
    {
        uint8_t off_pkt[ACER_PREDATOR_FEATURE_SIZE] = { 0x00, 0x08, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF6 };
        hid_send_feature_report(dev, off_pkt, ACER_PREDATOR_FEATURE_SIZE);
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
        return;
    }

    /*---------------------------------------------------------*\
    | Hardware Onboard Effects 4-Packet Handshake:              |
    |   Packet 1: Wake/Init (0xB1)                              |
    |   Packet 2: Reset/Base ({0x08, 0x02, ..., 0xF5})          |
    |   Packet 3: Mode & Animation Parameters (0x08)            |
    |   Packet 4: Custom Zone Color (0x14)                      |
    | Each packet is a 9-byte hid_send_feature_report with      |
    | a mandatory 15ms sleep between packets.                   |
    \*---------------------------------------------------------*/

    /* Packet 1: Wake/Init */
    uint8_t wake[ACER_PREDATOR_FEATURE_SIZE] = { 0x00, 0xB1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4E };
    hid_send_feature_report(dev, wake, ACER_PREDATOR_FEATURE_SIZE);
    std::this_thread::sleep_for(std::chrono::milliseconds(15));

    /* Packet 2: Reset/Base */
    uint8_t reset[ACER_PREDATOR_FEATURE_SIZE] = { 0x00, 0x08, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF5 };
    hid_send_feature_report(dev, reset, ACER_PREDATOR_FEATURE_SIZE);
    std::this_thread::sleep_for(std::chrono::milliseconds(15));

    /* Packet 3: Mode & Animation Parameters                    */
    /* Speed is inverted: sp_val = 10 - speed (1..9 -> 9..1)  */
    uint8_t sp_val = (uint8_t)(10 - state.speed);
    uint8_t effect[ACER_PREDATOR_FEATURE_SIZE];
    effect[0] = 0x00;
    effect[1] = 0x08;
    effect[2] = 0x02;
    effect[3] = state.mode;
    effect[4] = sp_val;
    effect[5] = state.brightness;
    effect[6] = state.color_preset;     /* 0xE0 = rainbow/random, 0x00 = custom RGB */
    effect[7] = state.direction;
    effect[8] = CalculateChecksum(effect, ACER_PREDATOR_FEATURE_SIZE);
    hid_send_feature_report(dev, effect, ACER_PREDATOR_FEATURE_SIZE);
    std::this_thread::sleep_for(std::chrono::milliseconds(15));

    /* Packet 4: Custom Zone Color (main keyboard zone = 0x00) */
    uint8_t color[ACER_PREDATOR_FEATURE_SIZE];
    color[0] = 0x00;
    color[1] = 0x14;                    // Command: Set Base Color
    color[2] = 0x00;                    // Reserved
    color[3] = 0x00;                    // Zone: 0x00 = Main Keyboard
    color[4] = state.red;
    color[5] = state.green;
    color[6] = state.blue;
    color[7] = 0x00;
    color[8] = CalculateChecksum(color, ACER_PREDATOR_FEATURE_SIZE);
    hid_send_feature_report(dev, color, ACER_PREDATOR_FEATURE_SIZE);
    std::this_thread::sleep_for(std::chrono::milliseconds(15));
}

void AcerPredatorHeliosController::SendMagKeyDirect(const AcerPredatorState& state)
{
    if(!dev)
    {
        return;
    }

    /*---------------------------------------------------------*\
    | MagKey Direct Mode: Stream 48-byte buffer                 |
    | Format: [0x00, R, G, B] per mini-LED (4 bytes/LED)       |
    | 12 mini-LEDs * 4 bytes = 48 bytes -> padded to 64 bytes   |
    | Commit opcode: 0x4F (MAG_DIRECT)                         |
    \*---------------------------------------------------------*/
    hid_send_feature_report(dev, PREAMBLE_MAGKEY, ACER_PREDATOR_FEATURE_SIZE);

    uint8_t packet[ACER_PREDATOR_PACKET_SIZE];
    std::memset(packet, 0, sizeof(packet));
    packet[0] = 0x00; // Report ID

    size_t copy_len = std::min(state.mag_buffer.size(), (size_t)ACER_PREDATOR_MAG_PAYLOAD_SIZE);
    if(copy_len > 0)
    {
        std::memcpy(&packet[1], state.mag_buffer.data(), copy_len);
    }
    hid_write(dev, packet, ACER_PREDATOR_PACKET_SIZE);

    /* MagKey Direct commit: ~(0x67 + brightness) & 0xFF */
    uint8_t mag_commit[ACER_PREDATOR_FEATURE_SIZE];
    mag_commit[0] = 0x00;
    mag_commit[1] = 0x08;
    mag_commit[2] = 0x02;
    mag_commit[3] = ACER_PREDATOR_MAG_DIRECT; // 0x4F
    mag_commit[4] = 0x05;               // Speed (fixed at default 5 for Direct)
    mag_commit[5] = state.magkey_brightness;
    mag_commit[6] = 0x08;               // Mode flag for Direct
    mag_commit[7] = 0x01;
    mag_commit[8] = CalculateChecksum(mag_commit, ACER_PREDATOR_FEATURE_SIZE);
    hid_send_feature_report(dev, mag_commit, ACER_PREDATOR_FEATURE_SIZE);
}

void AcerPredatorHeliosController::SendMagKeyEffect(const AcerPredatorState& state)
{
    if(!dev)
    {
        return;
    }

    /*---------------------------------------------------------*\
    | MagKey hardware effect: 4-Packet Handshake                |
    | MAG_OFF (0x40): slave mini-LEDs to base WASD key colors  |
    | Other opcodes: dedicated MagKey animation                 |
    \*---------------------------------------------------------*/
    uint8_t mag_opcode = ACER_PREDATOR_MAG_OFF;
    if(state.magkey_mode != ACER_PREDATOR_MAG_FOLLOW && state.magkey_mode >= 0)
    {
        mag_opcode = (uint8_t)state.magkey_mode;
    }

    /* Packet 1: Wake/Init */
    uint8_t wake[ACER_PREDATOR_FEATURE_SIZE] = { 0x00, 0xB1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4E };
    hid_send_feature_report(dev, wake, ACER_PREDATOR_FEATURE_SIZE);
    std::this_thread::sleep_for(std::chrono::milliseconds(15));

    /* Packet 2: Reset/Base */
    uint8_t reset[ACER_PREDATOR_FEATURE_SIZE] = { 0x00, 0x08, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF5 };
    hid_send_feature_report(dev, reset, ACER_PREDATOR_FEATURE_SIZE);
    std::this_thread::sleep_for(std::chrono::milliseconds(15));

    /* Packet 3: MagKey WASD Mode Packet */
    uint8_t sp_val = (uint8_t)(10 - state.magkey_speed);
    uint8_t mag_commit[ACER_PREDATOR_FEATURE_SIZE];
    mag_commit[0] = 0x00;
    mag_commit[1] = 0x08;
    mag_commit[2] = 0x02;
    mag_commit[3] = mag_opcode;
    mag_commit[4] = sp_val;
    mag_commit[5] = state.magkey_brightness;
    mag_commit[6] = (mag_opcode == ACER_PREDATOR_MAG_RAINBOW || mag_opcode == ACER_PREDATOR_MAG_NEON) ? 0xE0 : 0x00;
    mag_commit[7] = state.direction;
    mag_commit[8] = CalculateChecksum(mag_commit, ACER_PREDATOR_FEATURE_SIZE);
    hid_send_feature_report(dev, mag_commit, ACER_PREDATOR_FEATURE_SIZE);
    std::this_thread::sleep_for(std::chrono::milliseconds(15));

    /* Packet 4: Custom Zone Color for Zone 1 (MagKey WASD) */
    uint8_t color[ACER_PREDATOR_FEATURE_SIZE];
    color[0] = 0x00;
    color[1] = 0x14;
    color[2] = 0x00;
    color[3] = 0x01;                    // Zone: 0x01 = MagKey WASD
    color[4] = state.magkey_red;
    color[5] = state.magkey_green;
    color[6] = state.magkey_blue;
    color[7] = 0x00;
    color[8] = CalculateChecksum(color, ACER_PREDATOR_FEATURE_SIZE);
    hid_send_feature_report(dev, color, ACER_PREDATOR_FEATURE_SIZE);
    std::this_thread::sleep_for(std::chrono::milliseconds(15));
}
