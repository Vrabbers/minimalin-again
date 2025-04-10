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