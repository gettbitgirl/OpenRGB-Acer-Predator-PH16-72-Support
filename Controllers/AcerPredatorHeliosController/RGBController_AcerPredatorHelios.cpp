/*---------------------------------------------------------*\
| RGBController_AcerPredatorHelios.cpp                      |
|                                                           |
|   Driver for Acer Predator Helios 16/18 Keyboard (Sunrex) |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "RGBController_AcerPredatorHelios.h"
#include "RGBControllerKeyNames.h"
#include <cstring>

/**------------------------------------------------------------------*\
    @name Acer Predator Helios Keyboard
    @category Laptop
    @type HID
    @save :x:
    @direct :white_check_mark:
    @effects :white_check_mark:
    @detectors DetectAcerPredatorHeliosControllers
    @comment Acer Predator Helios 16 / 18 Per-Key RGB + MagKey WASD
\*-------------------------------------------------------------------*/

#define NA 0xFFFFFFFF

#define KEY_GRID_WIDTH  20
#define KEY_GRID_HEIGHT 6

static const unsigned int helios_matrix_map[KEY_GRID_HEIGHT][KEY_GRID_WIDTH] =
{
    // Row 0: Esc (0) .. Power (19)
    {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19
    },

    // Row 1: Backtick (20) .. Backspace (33), Backspace spacer, Gap, Predator Sense (34), NumLock (35), Num / (36), Num * (37)
    {
        20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, NA, NA, 34, 35, 36, 37
    },

    // Row 2: Tab (38), Tab spacer, Q (39) .. \ (51), Gap, Num 7 (52), Num 8 (53), Num 9 (54), Num - (55)
    {
        38, NA, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, NA, 52, 53, 54, 55
    },

    // Row 3: Caps Lock (56), Caps spacer, A (57) .. Enter (68), Enter spacer, Gap, Num 4 (69), Num 5 (70), Num 6 (71), Num + (72)
    {
        56, NA, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, NA, NA, 69, 70, 71, 72
    },

    // Row 4: Left Shift (73), Shift spacer, Z (74) .. / (83), Right Shift (84), Up Arrow (85), Gap, Num 1 (86), Num 2 (87), Num 3 (88), Num Enter (89)
    {
        73, NA, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, NA, NA, 86, 87, 88, 89
    },

    // Row 5: LCtrl (90)..LAlt (93), Space (94), Space spacers (4x), RAlt (95)..RCtrl (97), Left (98), Down (99), Right (100), Gap, Num 0 (101), Num 0 spacer, Num . (102), Num Enter vertical spacer
    {
        90, 91, 92, 93, 94, NA, NA, NA, NA, 95, 96, 97, 98, 99, 100, NA, 101, NA, 102, NA
    },
};

static const char* const helios_led_names[103] =
{
    KEY_EN_ESCAPE,           // 0
    KEY_EN_F1,               // 1
    KEY_EN_F2,               // 2
    KEY_EN_F3,               // 3
    KEY_EN_F4,               // 4
    KEY_EN_F5,               // 5
    KEY_EN_F6,               // 6
    KEY_EN_F7,               // 7
    KEY_EN_F8,               // 8
    KEY_EN_F9,               // 9
    KEY_EN_F10,              // 10
    KEY_EN_F11,              // 11
    KEY_EN_F12,              // 12
    KEY_EN_PRINT_SCREEN,     // 13
    KEY_EN_INSERT,           // 14
    KEY_EN_DELETE,           // 15
    KEY_EN_MEDIA_PREVIOUS,   // 16
    KEY_EN_MEDIA_PLAY_PAUSE, // 17
    KEY_EN_MEDIA_NEXT,       // 18
    KEY_EN_POWER,            // 19
    KEY_EN_BACK_TICK,        // 20
    KEY_EN_1,                // 21
    KEY_EN_2,                // 22
    KEY_EN_3,                // 23
    KEY_EN_4,                // 24
    KEY_EN_5,                // 25
    KEY_EN_6,                // 26
    KEY_EN_7,                // 27
    KEY_EN_8,                // 28
    KEY_EN_9,                // 29
    KEY_EN_0,                // 30
    KEY_EN_MINUS,            // 31
    KEY_EN_EQUALS,           // 32
    KEY_EN_BACKSPACE,        // 33
    "PredatorSense",         // 34
    KEY_EN_NUMPAD_LOCK,      // 35
    KEY_EN_NUMPAD_DIVIDE,    // 36
    KEY_EN_NUMPAD_TIMES,     // 37
    KEY_EN_TAB,              // 38
    KEY_EN_Q,                // 39
    KEY_EN_W,                // 40
    KEY_EN_E,                // 41
    KEY_EN_R,                // 42
    KEY_EN_T,                // 43
    KEY_EN_Y,                // 44
    KEY_EN_U,                // 45
    KEY_EN_I,                // 46
    KEY_EN_O,                // 47
    KEY_EN_P,                // 48
    KEY_EN_LEFT_BRACKET,     // 49
    KEY_EN_RIGHT_BRACKET,    // 50
    KEY_EN_ANSI_BACK_SLASH,  // 51
    KEY_EN_NUMPAD_7,         // 52
    KEY_EN_NUMPAD_8,         // 53
    KEY_EN_NUMPAD_9,         // 54
    KEY_EN_NUMPAD_MINUS,     // 55
    KEY_EN_CAPS_LOCK,        // 56
    KEY_EN_A,                // 57
    KEY_EN_S,                // 58
    KEY_EN_D,                // 59
    KEY_EN_F,                // 60
    KEY_EN_G,                // 61
    KEY_EN_H,                // 62
    KEY_EN_J,                // 63
    KEY_EN_K,                // 64
    KEY_EN_L,                // 65
    KEY_EN_SEMICOLON,        // 66
    KEY_EN_QUOTE,            // 67
    KEY_EN_ANSI_ENTER,       // 68
    KEY_EN_NUMPAD_4,         // 69
    KEY_EN_NUMPAD_5,         // 70
    KEY_EN_NUMPAD_6,         // 71
    KEY_EN_NUMPAD_PLUS,      // 72
    KEY_EN_LEFT_SHIFT,       // 73
    KEY_EN_Z,                // 74
    KEY_EN_X,                // 75
    KEY_EN_C,                // 76
    KEY_EN_V,                // 77
    KEY_EN_B,                // 78
    KEY_EN_N,                // 79
    KEY_EN_M,                // 80
    KEY_EN_COMMA,            // 81
    KEY_EN_PERIOD,           // 82
    KEY_EN_FORWARD_SLASH,    // 83
    KEY_EN_RIGHT_SHIFT,      // 84
    KEY_EN_UP_ARROW,         // 85
    KEY_EN_NUMPAD_1,         // 86
    KEY_EN_NUMPAD_2,         // 87
    KEY_EN_NUMPAD_3,         // 88
    KEY_EN_NUMPAD_ENTER,     // 89
    KEY_EN_LEFT_CONTROL,     // 90
    KEY_EN_LEFT_FUNCTION,    // 91
    KEY_EN_LEFT_WINDOWS,     // 92
    KEY_EN_LEFT_ALT,         // 93
    KEY_EN_SPACE,            // 94
    KEY_EN_RIGHT_ALT,        // 95
    KEY_EN_MENU,             // 96
    KEY_EN_RIGHT_CONTROL,    // 97
    KEY_EN_LEFT_ARROW,       // 98
    KEY_EN_DOWN_ARROW,       // 99
    KEY_EN_RIGHT_ARROW,      // 100
    KEY_EN_NUMPAD_0,         // 101
    KEY_EN_NUMPAD_PERIOD,    // 102
};

static const unsigned int helios_led_payload_indices[103] =
{
    // Row 0: Function & Top Media Keys (0-19)
    0,   //   0: KEY_EN_ESCAPE
    6,   //   1: KEY_EN_F1
    12,  //   2: KEY_EN_F2
    18,  //   3: KEY_EN_F3
    24,  //   4: KEY_EN_F4
    30,  //   5: KEY_EN_F5
    36,  //   6: KEY_EN_F6
    42,  //   7: KEY_EN_F7
    48,  //   8: KEY_EN_F8
    54,  //   9: KEY_EN_F9
    60,  //  10: KEY_EN_F10
    66,  //  11: KEY_EN_F11
    72,  //  12: KEY_EN_F12
    78,  //  13: KEY_EN_PRINT_SCREEN
    84,  //  14: KEY_EN_INSERT
    90,  //  15: KEY_EN_DELETE
    96,  //  16: KEY_EN_MEDIA_PREVIOUS
    102, //  17: KEY_EN_MEDIA_PLAY_PAUSE
    108, //  18: KEY_EN_MEDIA_NEXT
    114, //  19: KEY_EN_POWER

    // Row 1: Number Row & Top Numpad (20-37)
    1,   //  20: KEY_EN_BACK_TICK
    7,   //  21: KEY_EN_1
    13,  //  22: KEY_EN_2
    19,  //  23: KEY_EN_3
    25,  //  24: KEY_EN_4
    31,  //  25: KEY_EN_5
    37,  //  26: KEY_EN_6
    43,  //  27: KEY_EN_7
    49,  //  28: KEY_EN_8
    55,  //  29: KEY_EN_9
    61,  //  30: KEY_EN_0
    67,  //  31: KEY_EN_MINUS
    73,  //  32: KEY_EN_EQUALS
    91,  //  33: KEY_EN_BACKSPACE
    97,  //  34: "PredatorSense"
    103, //  35: KEY_EN_NUMPAD_LOCK
    109, //  36: KEY_EN_NUMPAD_DIVIDE
    115, //  37: KEY_EN_NUMPAD_TIMES

    // Row 2: QWERTY Row & Numpad 7-9, - (38-55)
    2,   //  38: KEY_EN_TAB
    8,   //  39: KEY_EN_Q
    14,  //  40: KEY_EN_W
    20,  //  41: KEY_EN_E
    26,  //  42: KEY_EN_R
    32,  //  43: KEY_EN_T
    38,  //  44: KEY_EN_Y
    44,  //  45: KEY_EN_U
    50,  //  46: KEY_EN_I
    56,  //  47: KEY_EN_O
    62,  //  48: KEY_EN_P
    68,  //  49: KEY_EN_LEFT_BRACKET
    74,  //  50: KEY_EN_RIGHT_BRACKET
    92,  //  51: KEY_EN_ANSI_BACK_SLASH
    98,  //  52: KEY_EN_NUMPAD_7
    104, //  53: KEY_EN_NUMPAD_8
    110, //  54: KEY_EN_NUMPAD_9
    116, //  55: KEY_EN_NUMPAD_MINUS

    // Row 3: Home Row & Numpad 4-6, + (56-72)
    3,   //  56: KEY_EN_CAPS_LOCK
    9,   //  57: KEY_EN_A
    15,  //  58: KEY_EN_S
    21,  //  59: KEY_EN_D
    27,  //  60: KEY_EN_F
    33,  //  61: KEY_EN_G
    39,  //  62: KEY_EN_H
    45,  //  63: KEY_EN_J
    51,  //  64: KEY_EN_K
    57,  //  65: KEY_EN_L
    63,  //  66: KEY_EN_SEMICOLON
    69,  //  67: KEY_EN_QUOTE
    93,  //  68: KEY_EN_ANSI_ENTER
    99,  //  69: KEY_EN_NUMPAD_4
    105, //  70: KEY_EN_NUMPAD_5
    111, //  71: KEY_EN_NUMPAD_6
    117, //  72: KEY_EN_NUMPAD_PLUS

    // Row 4: ZXCV Row & Numpad 1-3, Enter (73-89)
    4,   //  73: KEY_EN_LEFT_SHIFT
    16,  //  74: KEY_EN_Z
    22,  //  75: KEY_EN_X
    28,  //  76: KEY_EN_C
    34,  //  77: KEY_EN_V
    40,  //  78: KEY_EN_B
    46,  //  79: KEY_EN_N
    52,  //  80: KEY_EN_M
    58,  //  81: KEY_EN_COMMA
    64,  //  82: KEY_EN_PERIOD
    70,  //  83: KEY_EN_FORWARD_SLASH
    88,  //  84: KEY_EN_RIGHT_SHIFT
    94,  //  85: KEY_EN_UP_ARROW
    100, //  86: KEY_EN_NUMPAD_1
    106, //  87: KEY_EN_NUMPAD_2
    112, //  88: KEY_EN_NUMPAD_3
    119, //  89: KEY_EN_NUMPAD_ENTER

    // Row 5: Bottom Row & Numpad 0, . (90-102)
    5,   //  90: KEY_EN_LEFT_CONTROL
    11,  //  91: KEY_EN_LEFT_FUNCTION
    17,  //  92: KEY_EN_LEFT_WINDOWS
    23,  //  93: KEY_EN_LEFT_ALT
    41,  //  94: KEY_EN_SPACE
    59,  //  95: KEY_EN_RIGHT_ALT
    65,  //  96: KEY_EN_MENU
    83,  //  97: KEY_EN_RIGHT_CONTROL
    89,  //  98: KEY_EN_LEFT_ARROW
    95,  //  99: KEY_EN_DOWN_ARROW
    101, // 100: KEY_EN_RIGHT_ARROW
    107, // 101: KEY_EN_NUMPAD_0
    113, // 102: KEY_EN_NUMPAD_PERIOD
};

static const char* const helios_magkey_names[12] =
{
    "Key: W (Top)",
    "Key: W (Bottom-Left)",
    "Key: W (Bottom-Right)",
    "Key: A (Top)",
    "Key: A (Bottom-Left)",
    "Key: A (Bottom-Right)",
    "Key: S (Top)",
    "Key: S (Bottom-Left)",
    "Key: S (Bottom-Right)",
    "Key: D (Top)",
    "Key: D (Bottom-Left)",
    "Key: D (Bottom-Right)"
};

RGBController_AcerPredatorHelios::RGBController_AcerPredatorHelios(std::shared_ptr<AcerPredatorHeliosController> controller)
{
    this->controller                = controller;

    name                            = "Acer Predator Helios Keyboard";
    vendor                          = "Acer";
    type                            = DEVICE_TYPE_LAPTOP;
    description                     = "Acer Predator Helios 16/18 Sunrex RGB Keyboard";
    location                        = controller->GetLocation();
    serial                          = controller->GetSerial();

    flags                          |= CONTROLLER_FLAG_MANUALLY_CONFIGURABLE_DEVICE_SPECIFIC;
    slave_magkeys_in_direct         = true;

    auto add_mode = [this](const char* m_name, int m_val, bool has_speed, bool has_dir, bool has_color, bool has_random, RGBColor default_color)
    {
        mode m;
        m.name                 = m_name;
        m.value                = m_val;
        m.flags                = MODE_FLAG_HAS_BRIGHTNESS;
        if(has_color)  m.flags |= MODE_FLAG_HAS_MODE_SPECIFIC_COLOR;
        if(has_random) m.flags |= MODE_FLAG_HAS_RANDOM_COLOR;
        if(has_speed)
        {
            m.flags           |= MODE_FLAG_HAS_SPEED;
            m.speed_min        = 1;
            m.speed_max        = 9;
            m.speed            = 5;
        }
        if(has_dir)
        {
            m.flags           |= MODE_FLAG_HAS_DIRECTION_LR;
            m.direction        = MODE_DIRECTION_RIGHT;
        }
        m.color_mode           = has_color ? MODE_COLORS_MODE_SPECIFIC : (has_random ? MODE_COLORS_RANDOM : MODE_COLORS_NONE);
        m.colors_min           = has_color ? 1 : 0;
        m.colors_max           = has_color ? 1 : 0;
        if(has_color)
        {
            m.colors.resize(1);
            m.colors[0]        = default_color;
        }
        m.brightness_min       = 0;
        m.brightness_max       = 100;
        m.brightness           = 100;
        modes.push_back(m);
    };

    /*---------------------------------------------------------*\
    | Mode 0: Direct (Per-LED streaming)                        |
    \*---------------------------------------------------------*/
    mode Direct;
    Direct.name                 = "Direct";
    Direct.value                = ACER_PREDATOR_MODE_DIRECT;
    Direct.flags                = MODE_FLAG_HAS_PER_LED_COLOR | MODE_FLAG_HAS_BRIGHTNESS;
    Direct.color_mode           = MODE_COLORS_PER_LED;
    Direct.brightness_min       = 0;
    Direct.brightness_max       = 100;
    Direct.brightness           = 100;
    modes.push_back(Direct);

    /*---------------------------------------------------------*\
    | Main Keyboard Hardware Effects                            |
    \*---------------------------------------------------------*/
    add_mode("Static",        ACER_PREDATOR_MODE_STATIC,    false, false, true,  false, ToRGBColor(0, 255, 255));
    add_mode("Breathing",     ACER_PREDATOR_MODE_BREATHING, true,  false, true,  true,  ToRGBColor(0, 255, 255));
    add_mode("Wave",          ACER_PREDATOR_MODE_WAVE,      true,  true,  false, true,  0);
    add_mode("Snake",         ACER_PREDATOR_MODE_SNAKE,     true,  true,  false, true,  0);
    add_mode("Ripple",        ACER_PREDATOR_MODE_RIPPLE,    true,  false, true,  true,  ToRGBColor(0, 255, 255));
    add_mode("Neon",          ACER_PREDATOR_MODE_NEON,      true,  false, false, true,  0);
    add_mode("Rain",          ACER_PREDATOR_MODE_RAIN,      true,  false, true,  true,  ToRGBColor(0, 255, 255));
    add_mode("Lightning",     ACER_PREDATOR_MODE_LIGHTNING, true,  false, true,  true,  ToRGBColor(0, 255, 255));
    add_mode("Stars",         ACER_PREDATOR_MODE_STARS,     true,  false, true,  true,  ToRGBColor(255, 255, 255));
    add_mode("Fireball",      ACER_PREDATOR_MODE_FIREBALL,  true,  true,  true,  true,  ToRGBColor(255, 100, 0));
    add_mode("Snow",          ACER_PREDATOR_MODE_SNOW,      true,  false, true,  true,  ToRGBColor(200, 220, 255));
    add_mode("Heartbeat",     ACER_PREDATOR_MODE_HEARTBEAT, true,  false, true,  true,  ToRGBColor(255, 0, 50));
    add_mode("Off",           ACER_PREDATOR_MODE_OFF,       false, false, false, false, 0);

    SetupZones();
    InitDeviceSpecificConfiguration();
}

RGBController_AcerPredatorHelios::~RGBController_AcerPredatorHelios()
{
    Shutdown();
}

void RGBController_AcerPredatorHelios::SetupZones()
{
    /*---------------------------------------------------------*\
    | Zone 0: Primary Keyboard (Matrix, 103 Keys)               |
    \*---------------------------------------------------------*/
    zone keyboard_zone;
    keyboard_zone.name              = ZONE_EN_KEYBOARD;
    keyboard_zone.type              = ZONE_TYPE_MATRIX;
    keyboard_zone.leds_min          = 103;
    keyboard_zone.leds_max          = 103;
    keyboard_zone.leds_count        = 103;

    keyboard_zone.matrix_map.height = KEY_GRID_HEIGHT;
    keyboard_zone.matrix_map.width  = KEY_GRID_WIDTH;
    keyboard_zone.matrix_map.map.resize(KEY_GRID_HEIGHT * KEY_GRID_WIDTH);

    for(unsigned int row = 0; row < KEY_GRID_HEIGHT; row++)
    {
        for(unsigned int col = 0; col < KEY_GRID_WIDTH; col++)
        {
            keyboard_zone.matrix_map.map[row * KEY_GRID_WIDTH + col] = helios_matrix_map[row][col];
        }
    }

    for(unsigned int i = 0; i < 103; i++)
    {
        led new_led;
        new_led.name                = helios_led_names[i];
        new_led.value               = helios_led_payload_indices[i];
        leds.push_back(new_led);
    }
    zones.push_back(keyboard_zone);

    /*---------------------------------------------------------*\
    | Zone 1: MagKey WASD Accents (Linear, 12 LEDs)             |
    | With independent per-zone modes                           |
    \*---------------------------------------------------------*/
    zone magkey_zone;
    magkey_zone.name                = "MagKey WASD Accents";
    magkey_zone.type                = ZONE_TYPE_LINEAR;
    magkey_zone.leds_min            = 12;
    magkey_zone.leds_max            = 12;
    magkey_zone.leds_count          = 12;
    magkey_zone.matrix_map.height   = 0;
    magkey_zone.matrix_map.width    = 0;
    magkey_zone.active_mode         = -1; // -1 = Follow Global Keyboard Mode!

    auto add_mag_mode = [&](const char* m_name, int m_val, bool has_speed, bool has_dir, bool has_color)
    {
        mode m;
        m.name                 = m_name;
        m.value                = m_val;
        m.flags                = MODE_FLAG_HAS_BRIGHTNESS;
        if(has_color) m.flags |= MODE_FLAG_HAS_MODE_SPECIFIC_COLOR;
        if(has_speed)
        {
            m.flags           |= MODE_FLAG_HAS_SPEED;
            m.speed_min        = 1;
            m.speed_max        = 9;
            m.speed            = 5;
        }
        if(has_dir)
        {
            m.flags           |= MODE_FLAG_HAS_DIRECTION_LR;
            m.direction        = MODE_DIRECTION_RIGHT;
        }
        m.color_mode           = has_color ? MODE_COLORS_MODE_SPECIFIC : MODE_COLORS_NONE;
        m.colors_min           = has_color ? 1 : 0;
        m.colors_max           = has_color ? 1 : 0;
        if(has_color)
        {
            m.colors.resize(1);
            m.colors[0]        = ToRGBColor(255, 100, 0);
        }
        m.brightness_min       = 0;
        m.brightness_max       = 100;
        m.brightness           = 100;
        magkey_zone.modes.push_back(m);
    };

    add_mag_mode("Static",    ACER_PREDATOR_MAG_STATIC,    false, false, true);
    add_mag_mode("Breathing", ACER_PREDATOR_MAG_BREATHING, true,  false, true);
    add_mag_mode("Wave",      ACER_PREDATOR_MAG_WAVE,      true,  true,  false);
    add_mag_mode("Snake",     ACER_PREDATOR_MAG_SNAKE,     true,  true,  false);
    add_mag_mode("Neon",      ACER_PREDATOR_MAG_NEON,      true,  false, false);
    add_mag_mode("Rainbow",   ACER_PREDATOR_MAG_RAINBOW,   true,  false, false);

    for(unsigned int i = 0; i < 12; i++)
    {
        led new_led;
        new_led.name                = helios_magkey_names[i];
        new_led.value               = 103 + i;
        leds.push_back(new_led);
    }
    zones.push_back(magkey_zone);

    SetupColors();
}

void RGBController_AcerPredatorHelios::InitDeviceSpecificConfiguration()
{
    nlohmann::json configuration_json;
    configuration_json["schema"]        = nlohmann::json::object();
    configuration_json["configuration"] = nlohmann::json::object();

    nlohmann::json& schema = configuration_json["schema"];
    nlohmann::json& config = configuration_json["configuration"];

    schema["slave_magkeys_in_direct"]["title"]       = "Mirror MagKeys in Direct Mode";
    schema["slave_magkeys_in_direct"]["description"] = "Mirror W, A, S, D colors to perimeter MagKey halos when in Direct mode";
    schema["slave_magkeys_in_direct"]["type"]        = "bool";
    schema["slave_magkeys_in_direct"]["default"]     = true;
    schema["slave_magkeys_in_direct"]["order"]       = 1;
    config["slave_magkeys_in_direct"]                = true;

    SetDeviceSpecificConfiguration(configuration_json);
}

void RGBController_AcerPredatorHelios::DeviceConfigureDevice()
{
    /*---------------------------------------------------------*\
    | ConfigureDevice() already holds AccessMutex, so parse     |
    | the protected configuration string directly               |
    \*---------------------------------------------------------*/
    try
    {
        nlohmann::json config_json = nlohmann::json::parse(configuration);
        if(config_json.contains("configuration") && config_json["configuration"].contains("slave_magkeys_in_direct"))
        {
            slave_magkeys_in_direct = config_json["configuration"]["slave_magkeys_in_direct"].get<bool>();
        }
    }
    catch(...)
    {
    }
}

void RGBController_AcerPredatorHelios::DeviceUpdateDeviceSpecificConfiguration()
{
    /*---------------------------------------------------------*\
    | SetDeviceSpecificConfiguration() already holds AccessMutex|
    | here, so parse the protected configuration string directly|
    \*---------------------------------------------------------*/
    try
    {
        nlohmann::json config_json = nlohmann::json::parse(configuration);
        if(config_json.contains("configuration") && config_json["configuration"].contains("slave_magkeys_in_direct"))
        {
            slave_magkeys_in_direct = config_json["configuration"]["slave_magkeys_in_direct"].get<bool>();
        }
    }
    catch(...)
    {
    }

    DeviceUpdateLEDs();
}

void RGBController_AcerPredatorHelios::DeviceUpdateLEDs()
{
    AcerPredatorState state;
    mode& current_mode      = modes[active_mode];
    bool is_direct          = (current_mode.value == ACER_PREDATOR_MODE_DIRECT);

    state.mode              = (uint8_t)current_mode.value;
    state.speed             = (uint8_t)current_mode.speed;
    state.brightness        = (uint8_t)current_mode.brightness;
    state.direction         = (current_mode.direction == MODE_DIRECTION_LEFT) ? 2 : 1;
    state.color_preset      = (current_mode.color_mode == MODE_COLORS_NONE || current_mode.color_mode == MODE_COLORS_RANDOM) ? 0x00 : 0x08;

    RGBColor mode_col = current_mode.colors.empty() ? ToRGBColor(0, 255, 255) : current_mode.colors[0];
    state.red   = RGBGetRValue(mode_col);
    state.green = RGBGetGValue(mode_col);
    state.blue  = RGBGetBValue(mode_col);

    /*---------------------------------------------------------*\
    | Check MagKey per-zone mode state                          |
    \*---------------------------------------------------------*/
    zone& mag_zone = zones[1];
    if(mag_zone.active_mode >= 0 && (size_t)mag_zone.active_mode < mag_zone.modes.size())
    {
        mode& mag_mode          = mag_zone.modes[mag_zone.active_mode];
        state.magkey_mode       = mag_mode.value;
        state.magkey_speed      = (uint8_t)mag_mode.speed;
        state.magkey_brightness = (uint8_t)mag_mode.brightness;

        RGBColor mag_col = mag_mode.colors.empty() ? ToRGBColor(255, 100, 0) : mag_mode.colors[0];
        state.magkey_red   = RGBGetRValue(mag_col);
        state.magkey_green = RGBGetGValue(mag_col);
        state.magkey_blue  = RGBGetBValue(mag_col);
    }
    else
    {
        state.magkey_mode       = is_direct ? ACER_PREDATOR_MAG_DIRECT : ACER_PREDATOR_MAG_FOLLOW;
        state.magkey_speed      = state.speed;
        state.magkey_brightness = state.brightness;
        state.magkey_red        = state.red;
        state.magkey_green      = state.green;
        state.magkey_blue       = state.blue;
    }

    if(is_direct)
    {
        /*---------------------------------------------------------*\
        | Stream 103 standard keys to 126-column hardware matrix    |
        \*---------------------------------------------------------*/
        state.kb_buffer.resize(ACER_PREDATOR_KB_PAYLOAD_SIZE, 0);

        for(size_t i = 0; i < 103 && i < colors.size(); i++)
        {
            unsigned int payload_idx = leds[i].value;
            if(payload_idx < ACER_PREDATOR_KB_POSITIONS)
            {
                state.kb_buffer[payload_idx * 4 + 0] = 0x00;
                state.kb_buffer[payload_idx * 4 + 1] = RGBGetRValue(colors[i]);
                state.kb_buffer[payload_idx * 4 + 2] = RGBGetGValue(colors[i]);
                state.kb_buffer[payload_idx * 4 + 3] = RGBGetBValue(colors[i]);
            }

            if(i == 84) // Right Shift (wide keycap: mirror across cols 12 & 14)
            {
                for(unsigned int sp : { 76, 88 })
                {
                    state.kb_buffer[sp * 4 + 0] = 0x00;
                    state.kb_buffer[sp * 4 + 1] = RGBGetRValue(colors[i]);
                    state.kb_buffer[sp * 4 + 2] = RGBGetGValue(colors[i]);
                    state.kb_buffer[sp * 4 + 3] = RGBGetBValue(colors[i]);
                }
            }
            else if(i == 94) // Spacebar: mirror across its multi-LED diffuse zone
            {
                for(unsigned int sp : { 29, 35, 47, 53 })
                {
                    state.kb_buffer[sp * 4 + 0] = 0x00;
                    state.kb_buffer[sp * 4 + 1] = RGBGetRValue(colors[i]);
                    state.kb_buffer[sp * 4 + 2] = RGBGetGValue(colors[i]);
                    state.kb_buffer[sp * 4 + 3] = RGBGetBValue(colors[i]);
                }
            }
        }

        /*---------------------------------------------------------*\
        | Stream 12 MagKey WASD LEDs (48 bytes)                     |
        \*---------------------------------------------------------*/
        state.mag_buffer.resize(ACER_PREDATOR_MAG_PAYLOAD_SIZE, 0);

        bool should_slave = (state.magkey_mode == ACER_PREDATOR_MAG_FOLLOW ||
                            (state.magkey_mode == ACER_PREDATOR_MAG_DIRECT && slave_magkeys_in_direct && mag_zone.active_mode < 0));

        if(should_slave)
        {
            /* Mirror base W (idx 40) */
            RGBColor color_w = (colors.size() > 40) ? colors[40] : 0;
            for(size_t i = 0; i < 3; i++)
            {
                state.mag_buffer[i * 4 + 0] = 0x00;
                state.mag_buffer[i * 4 + 1] = RGBGetRValue(color_w);
                state.mag_buffer[i * 4 + 2] = RGBGetGValue(color_w);
                state.mag_buffer[i * 4 + 3] = RGBGetBValue(color_w);
            }

            /* Mirror base A (idx 57) */
            RGBColor color_a = (colors.size() > 57) ? colors[57] : 0;
            for(size_t i = 3; i < 6; i++)
            {
                state.mag_buffer[i * 4 + 0] = 0x00;
                state.mag_buffer[i * 4 + 1] = RGBGetRValue(color_a);
                state.mag_buffer[i * 4 + 2] = RGBGetGValue(color_a);
                state.mag_buffer[i * 4 + 3] = RGBGetBValue(color_a);
            }

            /* Mirror base S (idx 58) */
            RGBColor color_s = (colors.size() > 58) ? colors[58] : 0;
            for(size_t i = 6; i < 9; i++)
            {
                state.mag_buffer[i * 4 + 0] = 0x00;
                state.mag_buffer[i * 4 + 1] = RGBGetRValue(color_s);
                state.mag_buffer[i * 4 + 2] = RGBGetGValue(color_s);
                state.mag_buffer[i * 4 + 3] = RGBGetBValue(color_s);
            }

            /* Mirror base D (idx 59) */
            RGBColor color_d = (colors.size() > 59) ? colors[59] : 0;
            for(size_t i = 9; i < 12; i++)
            {
                state.mag_buffer[i * 4 + 0] = 0x00;
                state.mag_buffer[i * 4 + 1] = RGBGetRValue(color_d);
                state.mag_buffer[i * 4 + 2] = RGBGetGValue(color_d);
                state.mag_buffer[i * 4 + 3] = RGBGetBValue(color_d);
            }
        }
        else
        {
            for(size_t i = 0; i < 12 && (103 + i) < colors.size(); i++)
            {
                RGBColor color = colors[103 + i];
                state.mag_buffer[i * 4 + 0] = 0x00;
                state.mag_buffer[i * 4 + 1] = RGBGetRValue(color);
                state.mag_buffer[i * 4 + 2] = RGBGetGValue(color);
                state.mag_buffer[i * 4 + 3] = RGBGetBValue(color);
            }
        }
    }

    controller->SetHardwareState(state);
}

void RGBController_AcerPredatorHelios::DeviceUpdateZoneLEDs(int)
{
    DeviceUpdateLEDs();
}

void RGBController_AcerPredatorHelios::DeviceUpdateSingleLED(int)
{
    DeviceUpdateLEDs();
}

void RGBController_AcerPredatorHelios::DeviceUpdateMode()
{
    DeviceUpdateLEDs();
}
