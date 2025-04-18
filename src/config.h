#pragma once

#include <stdint.h>

typedef struct
{
    int32_t key;
    int32_t value;
} __attribute__((packed)) ConfValue;

typedef struct
{
    ConfValue *data;
    int32_t size;
} Config;

int8_t config_get_bool(const Config *conf, const int32_t key);
void config_set_bool(Config *conf, const int32_t key, const int8_t value);
GColor config_get_color(const Config *conf, const int32_t key);
int32_t config_get_int(const Config *conf, const int32_t key);
void config_set_int(Config *conf, const int32_t key, const int32_t value);
Config *config_load(const int32_t persist_key, int32_t size, const ConfValue *defaults);
void config_save(Config *conf, const int32_t persist_key);
Config *config_destroy(Config *conf);

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
    ConfigKeyVersion,
    ConfigKeyHealthEnabled,
    ConfigKeyBatteryDisplayedAt,
    ConfigKeyQuietTimeVisible,
    ConfigKeyAnimationEnabled
} ConfigKey;

#define CONF_SIZE 18

#define CONF_VERSION 2

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
