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

static const unsigned int predator_keyboard_matrix[KEY_GRID_HEIGHT][KEY_GRID_WIDTH] =
{
    {   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  15, 115, 102, 103, 104, 105 },
    {  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33, 114,  NA },
    {  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50, 101,  NA,  NA },
    {  51,  52,  53,  54,  55,  56,  61,  62,  63,  64,  65,  66,  67,  NA,  68,  69,  70,  NA,  NA,  NA },
    {  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  NA,  84,  85,  86,  87,  NA,  NA },
    {  88,  89,  90,  91,  92,  93,  95,  94,  96,  97,  98,  NA,  NA,  NA,  99, 100,  NA,  NA,  NA,  NA }
};

static const char* const PREDATOR_HW_KEY_NAMES[118] =
{
    KEY_EN_ESCAPE,           KEY_EN_F1,              KEY_EN_F2,              KEY_EN_F3,              KEY_EN_F4,
    KEY_EN_F5,               KEY_EN_F6,              KEY_EN_F7,              KEY_EN_F8,              KEY_EN_F9,
    KEY_EN_F10,              KEY_EN_F11,             KEY_EN_F12,             KEY_EN_PRINT_SCREEN,    KEY_EN_INSERT,
    KEY_EN_DELETE,           KEY_EN_BACK_TICK,       KEY_EN_1,               KEY_EN_2,               KEY_EN_3,
    KEY_EN_4,                KEY_EN_5,               KEY_EN_6,               KEY_EN_7,               KEY_EN_8,
    KEY_EN_9,                KEY_EN_0,               KEY_EN_MINUS,           KEY_EN_EQUALS,          KEY_EN_BACKSPACE,
    KEY_EN_NUMPAD_LOCK,      KEY_EN_NUMPAD_DIVIDE,   KEY_EN_NUMPAD_TIMES,    KEY_EN_NUMPAD_MINUS,    KEY_EN_TAB,
    KEY_EN_Q,                KEY_EN_W,               KEY_EN_E,               KEY_EN_R,               KEY_EN_T,
    KEY_EN_Y,                KEY_EN_U,               KEY_EN_I,               KEY_EN_O,               KEY_EN_P,
    KEY_EN_LEFT_BRACKET,     KEY_EN_RIGHT_BRACKET,   KEY_EN_ANSI_BACK_SLASH, KEY_EN_NUMPAD_7,        KEY_EN_NUMPAD_8,
    KEY_EN_NUMPAD_9,         KEY_EN_CAPS_LOCK,       KEY_EN_A,               KEY_EN_S,               KEY_EN_D,
    KEY_EN_F,                KEY_EN_G,               KEY_EN_ISO_BACK_SLASH,  KEY_EN_POUND,           "Key ISO Extra",
    KEY_JP_RO,               KEY_EN_H,               KEY_EN_J,               KEY_EN_K,               KEY_EN_L,
    KEY_EN_SEMICOLON,        KEY_EN_QUOTE,           KEY_EN_ANSI_ENTER,      KEY_EN_NUMPAD_4,        KEY_EN_NUMPAD_5,
    KEY_EN_NUMPAD_6,         KEY_EN_LEFT_SHIFT,      KEY_EN_Z,               KEY_EN_X,               KEY_EN_C,
    KEY_EN_V,                KEY_EN_B,               KEY_EN_N,               KEY_EN_M,               KEY_EN_COMMA,
    KEY_EN_PERIOD,           KEY_EN_FORWARD_SLASH,   KEY_EN_RIGHT_SHIFT,     KEY_EN_UP_ARROW,        KEY_EN_NUMPAD_1,
    KEY_EN_NUMPAD_2,         KEY_EN_NUMPAD_3,        KEY_EN_NUMPAD_ENTER,    KEY_EN_LEFT_CONTROL,    KEY_EN_LEFT_FUNCTION,
    KEY_EN_LEFT_WINDOWS,     KEY_EN_LEFT_ALT,        KEY_EN_SPACE,           KEY_EN_RIGHT_ALT,       KEY_EN_RIGHT_CONTROL,
    "Key PredatorSense",     KEY_EN_LEFT_ARROW,      KEY_EN_DOWN_ARROW,      KEY_EN_RIGHT_ARROW,     KEY_EN_NUMPAD_0,
    KEY_EN_NUMPAD_PERIOD,    KEY_EN_NUMPAD_PLUS,     KEY_EN_HOME,            KEY_EN_END,             KEY_EN_PAGE_UP,
    KEY_EN_PAGE_DOWN,        KEY_EN_MEDIA_PLAY_PAUSE,KEY_EN_MEDIA_STOP,      KEY_EN_MEDIA_PREVIOUS,  KEY_EN_MEDIA_NEXT,
    KEY_EN_MEDIA_MUTE,       KEY_EN_MEDIA_VOLUME_DOWN,KEY_EN_MEDIA_VOLUME_UP,"Key Mic Mute",        KEY_EN_POWER,
    "Key Turbo Mode",        "Indicator Caps Lock",  "Indicator Num Lock"
};

static const char* const MAGKEY_ACCENT_NAMES[12] =
{
    "MagKey W Top", "MagKey W Bottom-Left", "MagKey W Bottom-Right",
    "MagKey A Top", "MagKey A Bottom-Left", "MagKey A Bottom-Right",
    "MagKey S Top", "MagKey S Bottom-Left", "MagKey S Bottom-Right",
    "MagKey D Top", "MagKey D Bottom-Left", "MagKey D Bottom-Right"
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

    /*---------------------------------------------------------*\
    | Global Device Modes                                       |
    \*---------------------------------------------------------*/
    mode Direct;
    Direct.name                     = "Direct";
    Direct.value                    = ACER_PREDATOR_MODE_DIRECT;
    Direct.flags                    = MODE_FLAG_HAS_PER_LED_COLOR | MODE_FLAG_HAS_BRIGHTNESS;
    Direct.color_mode               = MODE_COLORS_PER_LED;
    Direct.brightness_min           = 0;
    Direct.brightness_max           = 100;
    Direct.brightness               = 100;
    modes.push_back(Direct);

    mode Static;
    Static.name                     = "Static";
    Static.value                    = ACER_PREDATOR_MODE_STATIC;
    Static.flags                    = MODE_FLAG_HAS_MODE_SPECIFIC_COLOR | MODE_FLAG_HAS_BRIGHTNESS;
    Static.color_mode               = MODE_COLORS_MODE_SPECIFIC;
    Static.brightness_min           = 0;
    Static.brightness_max           = 100;
    Static.brightness               = 100;
    Static.colors_min               = 1;
    Static.colors_max               = 1;
    Static.colors.resize(1, 0x0000FF);
    modes.push_back(Static);

    mode Breathing;
    Breathing.name                  = "Breathing";
    Breathing.value                 = ACER_PREDATOR_MODE_BREATHING;
    Breathing.flags                 = MODE_FLAG_HAS_MODE_SPECIFIC_COLOR | MODE_FLAG_HAS_BRIGHTNESS | MODE_FLAG_HAS_SPEED;
    Breathing.color_mode            = MODE_COLORS_MODE_SPECIFIC;
    Breathing.brightness_min        = 0;
    Breathing.brightness_max        = 100;
    Breathing.brightness            = 100;
    Breathing.speed_min             = 1;
    Breathing.speed_max             = 9;
    Breathing.speed                 = 5;
    Breathing.colors_min            = 1;
    Breathing.colors_max            = 1;
    Breathing.colors.resize(1, 0x00FFFF);
    modes.push_back(Breathing);

    mode Wave;
    Wave.name                       = "Wave";
    Wave.value                      = ACER_PREDATOR_MODE_WAVE;
    Wave.flags                      = MODE_FLAG_HAS_BRIGHTNESS | MODE_FLAG_HAS_SPEED | MODE_FLAG_HAS_DIRECTION_LR;
    Wave.color_mode                 = MODE_COLORS_NONE;
    Wave.brightness_min             = 0;
    Wave.brightness_max             = 100;
    Wave.brightness                 = 100;
    Wave.speed_min                  = 1;
    Wave.speed_max                  = 9;
    Wave.speed                      = 5;
    Wave.direction                  = MODE_DIRECTION_RIGHT;
    modes.push_back(Wave);

    mode Snake;
    Snake.name                      = "Snake";
    Snake.value                     = ACER_PREDATOR_MODE_SNAKE;
    Snake.flags                     = MODE_FLAG_HAS_BRIGHTNESS | MODE_FLAG_HAS_SPEED | MODE_FLAG_HAS_DIRECTION_LR;
    Snake.color_mode                = MODE_COLORS_NONE;
    Snake.brightness_min            = 0;
    Snake.brightness_max            = 100;
    Snake.brightness                = 100;
    Snake.speed_min                 = 1;
    Snake.speed_max                 = 9;
    Snake.speed                     = 5;
    Snake.direction                 = MODE_DIRECTION_RIGHT;
    modes.push_back(Snake);

    mode Ripple;
    Ripple.name                     = "Ripple";
    Ripple.value                    = ACER_PREDATOR_MODE_RIPPLE;
    Ripple.flags                    = MODE_FLAG_HAS_MODE_SPECIFIC_COLOR | MODE_FLAG_HAS_BRIGHTNESS | MODE_FLAG_HAS_SPEED;
    Ripple.color_mode               = MODE_COLORS_MODE_SPECIFIC;
    Ripple.brightness_min           = 0;
    Ripple.brightness_max           = 100;
    Ripple.brightness               = 100;
    Ripple.speed_min                = 1;
    Ripple.speed_max                = 9;
    Ripple.speed                    = 5;
    Ripple.colors_min               = 1;
    Ripple.colors_max               = 1;
    Ripple.colors.resize(1, 0xFF0055);
    modes.push_back(Ripple);

    mode Neon;
    Neon.name                       = "Neon";
    Neon.value                      = ACER_PREDATOR_MODE_NEON;
    Neon.flags                      = MODE_FLAG_HAS_BRIGHTNESS | MODE_FLAG_HAS_SPEED;
    Neon.color_mode                 = MODE_COLORS_NONE;
    Neon.brightness_min             = 0;
    Neon.brightness_max             = 100;
    Neon.brightness                 = 100;
    Neon.speed_min                  = 1;
    Neon.speed_max                  = 9;
    Neon.speed                      = 5;
    modes.push_back(Neon);

    mode Rain;
    Rain.name                       = "Rain";
    Rain.value                      = ACER_PREDATOR_MODE_RAIN;
    Rain.flags                      = MODE_FLAG_HAS_MODE_SPECIFIC_COLOR | MODE_FLAG_HAS_BRIGHTNESS | MODE_FLAG_HAS_SPEED;
    Rain.color_mode                 = MODE_COLORS_MODE_SPECIFIC;
    Rain.brightness_min             = 0;
    Rain.brightness_max             = 100;
    Rain.brightness                 = 100;
    Rain.speed_min                  = 1;
    Rain.speed_max                  = 9;
    Rain.speed                      = 5;
    Rain.colors_min                 = 1;
    Rain.colors_max                 = 1;
    Rain.colors.resize(1, 0x00FF88);
    modes.push_back(Rain);

    mode Twinkle;
    Twinkle.name                    = "Twinkle";
    Twinkle.value                   = ACER_PREDATOR_MODE_STARS;
    Twinkle.flags                   = MODE_FLAG_HAS_MODE_SPECIFIC_COLOR | MODE_FLAG_HAS_BRIGHTNESS | MODE_FLAG_HAS_SPEED;
    Twinkle.color_mode              = MODE_COLORS_MODE_SPECIFIC;
    Twinkle.brightness_min          = 0;
    Twinkle.brightness_max          = 100;
    Twinkle.brightness              = 100;
    Twinkle.speed_min               = 1;
    Twinkle.speed_max               = 9;
    Twinkle.speed                   = 5;
    Twinkle.colors_min              = 1;
    Twinkle.colors_max              = 1;
    Twinkle.colors.resize(1, 0xFFFFFF);
    modes.push_back(Twinkle);

    mode Off;
    Off.name                        = "Off";
    Off.value                       = ACER_PREDATOR_MODE_OFF;
    Off.flags                       = 0;
    Off.color_mode                  = MODE_COLORS_NONE;
    modes.push_back(Off);

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
    | Zone 0: Primary Keyboard (Matrix)                         |
    \*---------------------------------------------------------*/
    zone keyboard_zone;
    keyboard_zone.name              = "Keyboard";
    keyboard_zone.type              = ZONE_TYPE_MATRIX;
    keyboard_zone.leds_min          = 118;
    keyboard_zone.leds_max          = 118;
    keyboard_zone.leds_count        = 118;

    keyboard_zone.matrix_map.height = KEY_GRID_HEIGHT;
    keyboard_zone.matrix_map.width  = KEY_GRID_WIDTH;
    keyboard_zone.matrix_map.map.resize(KEY_GRID_HEIGHT * KEY_GRID_WIDTH);

    for(unsigned int row = 0; row < KEY_GRID_HEIGHT; row++)
    {
        for(unsigned int col = 0; col < KEY_GRID_WIDTH; col++)
        {
            keyboard_zone.matrix_map.map[row * KEY_GRID_WIDTH + col] = predator_keyboard_matrix[row][col];
        }
    }

    for(unsigned int i = 0; i < 118; i++)
    {
        led new_led;
        new_led.name                = PREDATOR_HW_KEY_NAMES[i];
        new_led.value               = i;
        leds.push_back(new_led);
    }
    zones.push_back(keyboard_zone);

    /*---------------------------------------------------------*\
    | Zone 1: MagKey WASD Accents (Linear with Per-Zone Modes)  |
    \*---------------------------------------------------------*/
    zone magkey_zone;
    magkey_zone.name                = "MagKey WASD Accents";
    magkey_zone.type                = ZONE_TYPE_LINEAR;
    magkey_zone.leds_min            = 12;
    magkey_zone.leds_max            = 12;
    magkey_zone.leds_count          = 12;
    magkey_zone.active_mode         = -1; // -1 = Follow Device Mode!

    /*---------------------------------------------------------*\
    | Define dedicated MagKey hardware modes                    |
    \*---------------------------------------------------------*/
    mode MagStatic;
    MagStatic.name                  = "Static";
    MagStatic.value                 = ACER_PREDATOR_MAG_STATIC;
    MagStatic.flags                 = MODE_FLAG_HAS_MODE_SPECIFIC_COLOR | MODE_FLAG_HAS_BRIGHTNESS;
    MagStatic.color_mode            = MODE_COLORS_MODE_SPECIFIC;
    MagStatic.brightness_min        = 0;
    MagStatic.brightness_max        = 100;
    MagStatic.brightness            = 100;
    MagStatic.colors_min            = 1;
    MagStatic.colors_max            = 1;
    MagStatic.colors.resize(1, 0xFF7700);
    magkey_zone.modes.push_back(MagStatic);

    mode MagBreathing;
    MagBreathing.name               = "Breathing";
    MagBreathing.value              = ACER_PREDATOR_MAG_BREATHING;
    MagBreathing.flags              = MODE_FLAG_HAS_MODE_SPECIFIC_COLOR | MODE_FLAG_HAS_BRIGHTNESS | MODE_FLAG_HAS_SPEED;
    MagBreathing.color_mode         = MODE_COLORS_MODE_SPECIFIC;
    MagBreathing.brightness_min     = 0;
    MagBreathing.brightness_max     = 100;
    MagBreathing.brightness         = 100;
    MagBreathing.speed_min          = 1;
    MagBreathing.speed_max          = 9;
    MagBreathing.speed              = 5;
    MagBreathing.colors_min         = 1;
    MagBreathing.colors_max         = 1;
    MagBreathing.colors.resize(1, 0xFF7700);
    magkey_zone.modes.push_back(MagBreathing);

    mode MagWave;
    MagWave.name                    = "Wave";
    MagWave.value                   = ACER_PREDATOR_MAG_WAVE;
    MagWave.flags                   = MODE_FLAG_HAS_BRIGHTNESS | MODE_FLAG_HAS_SPEED;
    MagWave.color_mode              = MODE_COLORS_NONE;
    MagWave.brightness_min          = 0;
    MagWave.brightness_max          = 100;
    MagWave.brightness              = 100;
    MagWave.speed_min               = 1;
    MagWave.speed_max               = 9;
    MagWave.speed                   = 5;
    magkey_zone.modes.push_back(MagWave);

    mode MagNeon;
    MagNeon.name                    = "Neon";
    MagNeon.value                   = ACER_PREDATOR_MAG_NEON;
    MagNeon.flags                   = MODE_FLAG_HAS_BRIGHTNESS | MODE_FLAG_HAS_SPEED;
    MagNeon.color_mode              = MODE_COLORS_NONE;
    MagNeon.brightness_min          = 0;
    MagNeon.brightness_max          = 100;
    MagNeon.brightness              = 100;
    MagNeon.speed_min               = 1;
    MagNeon.speed_max               = 9;
    MagNeon.speed                   = 5;
    magkey_zone.modes.push_back(MagNeon);

    mode MagRainbow;
    MagRainbow.name                 = "Rainbow";
    MagRainbow.value                = ACER_PREDATOR_MAG_RAINBOW;
    MagRainbow.flags                = MODE_FLAG_HAS_BRIGHTNESS | MODE_FLAG_HAS_SPEED;
    MagRainbow.color_mode           = MODE_COLORS_NONE;
    MagRainbow.brightness_min       = 0;
    MagRainbow.brightness_max       = 100;
    MagRainbow.brightness           = 100;
    MagRainbow.speed_min            = 1;
    MagRainbow.speed_max            = 9;
    MagRainbow.speed                = 5;
    magkey_zone.modes.push_back(MagRainbow);

    for(unsigned int i = 0; i < 12; i++)
    {
        led new_led;
        new_led.name                = MAGKEY_ACCENT_NAMES[i];
        new_led.value               = 121 + i;
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
    InitDeviceSpecificConfiguration();
}

void RGBController_AcerPredatorHelios::DeviceUpdateDeviceSpecificConfiguration()
{
    nlohmann::json config = GetDeviceSpecificConfiguration();
    if(config.contains("slave_magkeys_in_direct"))
    {
        slave_magkeys_in_direct = config["slave_magkeys_in_direct"].get<bool>();
    }
    DeviceUpdateLEDs();
}

void RGBController_AcerPredatorHelios::DeviceUpdateLEDs()
{
    AcerPredatorState state;
    mode& current_mode      = modes[active_mode];

    state.mode              = (uint8_t)current_mode.value;
    state.speed             = (uint8_t)current_mode.speed;
    state.brightness        = (uint8_t)current_mode.brightness;
    state.direction         = (current_mode.direction == MODE_DIRECTION_LEFT) ? 2 : 1;
    state.palette           = (current_mode.color_mode == MODE_COLORS_NONE) ? 1 : 0;

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
    }
    else
    {
        state.magkey_mode       = ACER_PREDATOR_MAG_FOLLOW;
        state.magkey_speed      = state.speed;
        state.magkey_brightness = state.brightness;
    }

    /*---------------------------------------------------------*\
    | Prepare 133-LED (399 byte) hardware RGB buffer            |
    \*---------------------------------------------------------*/
    state.rgb_buffer.resize(ACER_PREDATOR_PAYLOAD_SIZE, 0);

    /*---------------------------------------------------------*\
    | Populate Zone 0 (Keyboard keys 0..117)                    |
    \*---------------------------------------------------------*/
    for(size_t led_idx = 0; led_idx < 118 && led_idx < colors.size(); led_idx++)
    {
        RGBColor color = colors[led_idx];
        if(current_mode.color_mode == MODE_COLORS_MODE_SPECIFIC && current_mode.colors.size() > 0)
        {
            color = current_mode.colors[0];
        }

        state.rgb_buffer[led_idx * 3 + 0] = RGBGetRValue(color);
        state.rgb_buffer[led_idx * 3 + 1] = RGBGetGValue(color);
        state.rgb_buffer[led_idx * 3 + 2] = RGBGetBValue(color);
    }

    /*---------------------------------------------------------*\
    | Populate Zone 1 (MagKey halos at indices 121..132)        |
    \*---------------------------------------------------------*/
    bool should_slave = (state.magkey_mode == ACER_PREDATOR_MAG_FOLLOW);
    if(current_mode.value == ACER_PREDATOR_MODE_DIRECT && !slave_magkeys_in_direct && mag_zone.active_mode < 0)
    {
        should_slave = false;
    }

    if(should_slave)
    {
        /* Mirror base W (idx 36) */
        RGBColor color_w = colors[36];
        for(size_t i = 121; i <= 123; i++)
        {
            state.rgb_buffer[i * 3 + 0] = RGBGetRValue(color_w);
            state.rgb_buffer[i * 3 + 1] = RGBGetGValue(color_w);
            state.rgb_buffer[i * 3 + 2] = RGBGetBValue(color_w);
        }

        /* Mirror base A (idx 52) */
        RGBColor color_a = colors[52];
        for(size_t i = 124; i <= 126; i++)
        {
            state.rgb_buffer[i * 3 + 0] = RGBGetRValue(color_a);
            state.rgb_buffer[i * 3 + 1] = RGBGetGValue(color_a);
            state.rgb_buffer[i * 3 + 2] = RGBGetBValue(color_a);
        }

        /* Mirror base S (idx 53) */
        RGBColor color_s = colors[53];
        for(size_t i = 127; i <= 129; i++)
        {
            state.rgb_buffer[i * 3 + 0] = RGBGetRValue(color_s);
            state.rgb_buffer[i * 3 + 1] = RGBGetGValue(color_s);
            state.rgb_buffer[i * 3 + 2] = RGBGetBValue(color_s);
        }

        /* Mirror base D (idx 54) */
        RGBColor color_d = colors[54];
        for(size_t i = 130; i <= 132; i++)
        {
            state.rgb_buffer[i * 3 + 0] = RGBGetRValue(color_d);
            state.rgb_buffer[i * 3 + 1] = RGBGetGValue(color_d);
            state.rgb_buffer[i * 3 + 2] = RGBGetBValue(color_d);
        }
    }
    else
    {
        /* Use independent colors assigned to Zone 1 LEDs */
        for(size_t i = 0; i < 12 && (118 + i) < colors.size(); i++)
        {
            RGBColor color = colors[118 + i];
            size_t hw_idx  = 121 + i;

            state.rgb_buffer[hw_idx * 3 + 0] = RGBGetRValue(color);
            state.rgb_buffer[hw_idx * 3 + 1] = RGBGetGValue(color);
            state.rgb_buffer[hw_idx * 3 + 2] = RGBGetBValue(color);
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
