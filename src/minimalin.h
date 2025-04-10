#pragma once

typedef enum
{
    NoIcon = 0,
    Bluetooth,
    Heart
} BluetoothIcon;
typedef enum
{
    Celsius = 0,
    Fahrenheit
} TemperatureUnit;

typedef enum
{
    ConfigKeyMinuteHandColor = 0,
    ConfigKeyHourHandColor,
    ConfigKeyBackgroundColor,
    ConfigKeyDateColor,
    ConfigKeyTimeColor,
    ConfigKeyInfoColor,
    ConfigKeyRefreshRate,
    ConfigKeyTemperatureUnit,
    ConfigKeyBluetoothIcon,
    ConfigKeyWeatherEnabled,
    ConfigKeyRainbowMode,
    ConfigKeyDateDisplayed,
    ConfigKeyVibrateOnTheHour,
    ConfigKeyMilitaryTime,
    ConfigKeyHealthEnabled,
    ConfigKeyBatteryDisplayedAt
} ConfigKey;

#define CONF_SIZE 16

#ifndef CONFIG_BLUETOOTH_ICON
#define CONFIG_BLUETOOTH_ICON Bluetooth
#endif
#ifndef CONFIG_TEMPERATURE_UNIT
#define CONFIG_TEMPERATURE_UNIT Celsius
#endif
#ifndef CONFIG_DATE_DISPLAYED
#define CONFIG_DATE_DISPLAYED true
#endif
#ifndef CONFIG_RAINBOW_MODE
#define CONFIG_RAINBOW_MODE false
#endif
#ifndef CONFIG_WEATHER_ENABLED
#define CONFIG_WEATHER_ENABLED true
#endif
#ifndef CONFIG_MILITARY_TIME
#define CONFIG_MILITARY_TIME false
#endif

ConfValue CONF_DEFAULTS[CONF_SIZE] = {
    {.key = ConfigKeyMinuteHandColor, .type = ColorConf, .value = 0xffffff},
    {.key = ConfigKeyHourHandColor, .type = ColorConf, .value = PBL_IF_COLOR_ELSE(0xff0000, 0xffffff)},
    {.key = ConfigKeyBackgroundColor, .type = ColorConf, .value = 0x000000},
    {.key = ConfigKeyDateColor, .type = ColorConf, .value = PBL_IF_COLOR_ELSE(0x555555, 0xffffff)},
    {.key = ConfigKeyTimeColor, .type = ColorConf, .value = PBL_IF_COLOR_ELSE(0xaaaaaa, 0xffffff)},
    {.key = ConfigKeyInfoColor, .type = ColorConf, .value = PBL_IF_COLOR_ELSE(0x555555, 0xffffff)},
    {.key = ConfigKeyBluetoothIcon, .type = IntConf, .value = CONFIG_BLUETOOTH_ICON},
    {.key = ConfigKeyTemperatureUnit, .type = IntConf, .value = CONFIG_TEMPERATURE_UNIT},
    {.key = ConfigKeyRefreshRate, .type = IntConf, .value = 20},
    {.key = ConfigKeyDateDisplayed, .type = BoolConf, .value = CONFIG_DATE_DISPLAYED},
    {.key = ConfigKeyRainbowMode, .type = BoolConf, .value = PBL_IF_COLOR_ELSE(CONFIG_RAINBOW_MODE, false)},
    {.key = ConfigKeyWeatherEnabled, .type = BoolConf, .value = CONFIG_WEATHER_ENABLED},
    {.key = ConfigKeyVibrateOnTheHour, .type = BoolConf, .value = false},
    {.key = ConfigKeyMilitaryTime, .type = BoolConf, .value = CONFIG_MILITARY_TIME},
    {.key = ConfigKeyHealthEnabled, .type = BoolConf, .value = false},
    {.key = ConfigKeyBatteryDisplayedAt, .type = IntConf, .value = -1}};