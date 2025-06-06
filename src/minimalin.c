#include <pebble.h>
#include "consts.h"
#include "config.h"
#include "text_block.h"
#include "quadrant.h"
#include "messenger.h"
#include "minimalin.h"
#include "geometry.h"
#include "globals.h"
#include "tick_points.h"

// #define d(string, ...) APP_LOG (APP_LOG_LEVEL_DEBUG, string, ##__VA_ARGS__)
// #define e(string, ...) APP_LOG (APP_LOG_LEVEL_ERROR, string, ##__VA_ARGS__)
#define i(string, ...) APP_LOG (APP_LOG_LEVEL_INFO, string, ##__VA_ARGS__)

typedef enum
{
    AppKeyMinuteHandColor = 0,
    AppKeyHourHandColor,
    AppKeyDateDisplayed,
    AppKeyBluetoothIcon,
    AppKeyRainbowMode,
    AppKeyBackgroundColor,
    AppKeyTimeColor,
    AppKeyInfoColor,
    AppKeyTemperatureUnit,
    AppKeyRefreshRate,
    AppKeyWeatherEnabled,
    AppKeyConfig,
    AppKeyWeatherTemperature,
    AppKeyWeatherIcon,
    AppKeyWeatherFailed,
    AppKeyWeatherRequest,
    AppKeyJsReady,
    AppKeyVibrateOnTheHour,
    AppKeyHealthEnabled,
    AppKeyBatteryDisplayedAt,
    AppKeyQuietTimeVisible,
    AppKeyAnimationEnabled
} AppKey;

typedef enum
{
    PersistKeyConfig = 0,
    PersistKeyWeather
} PersistKey;

typedef struct
{
    int32_t timestamp;
    int8_t icon;
    int8_t temperature;
    int8_t failed;
} Weather;

typedef struct
{
    Config *config;
    Weather weather;
    bool reset_weather;
    int steps;
    bool bluetooth_connected;
    BatteryChargeState charge_state;
    tm *time;
} Context;

static Context s_context;

static Window *s_main_window;
static Layer *s_root_layer;
static GRect s_root_layer_bounds;

static TextBlock *s_weather_info;
static TextBlock *s_date_info;
static TextBlock *s_steps_info;
static TextBlock *s_watch_info;
static TextBlock *s_hour_text;
static TextBlock *s_minute_text;

static Layer *s_tick_layer;

static GBitmap *s_rainbow_bitmap;
static Layer *s_minute_hand_layer;
static Layer *s_hour_hand_layer;
static RotBitmapLayer *s_rainbow_hand_layer;
static Layer *s_center_circle_layer;

static Quadrants *s_quadrants;

static Config *s_config;
static Messenger *s_messenger;

static AppTimer *s_weather_request_timer;
static int s_weather_request_timeout;

static int s_js_ready;

static GFont s_font;

static tm *s_current_time;

static AnimationProgress s_unob_area_anim_progress = ANIMATION_NORMALIZED_MIN;

static void schedule_weather_request(int timeout);
static void mark_dirty_minute_hand_layer();
static void fetch_step(Context *const context);

static const ConfValue CONF_DEFAULTS[CONF_SIZE] = {
    {.key = ConfigKeyMinuteHandColor, .value = 0xffffff},
    {.key = ConfigKeyHourHandColor, .value = PBL_IF_COLOR_ELSE(0xff0000, 0xffffff)},
    {.key = ConfigKeyBackgroundColor, .value = 0x000000},
    {.key = ConfigKeyDateColor, .value = PBL_IF_COLOR_ELSE(0x555555, 0xffffff)},
    {.key = ConfigKeyTimeColor, .value = PBL_IF_COLOR_ELSE(0xaaaaaa, 0xffffff)},
    {.key = ConfigKeyInfoColor, .value = PBL_IF_COLOR_ELSE(0x555555, 0xffffff)},
    {.key = ConfigKeyBluetoothIcon, .value = CONFIG_BLUETOOTH_ICON},
    {.key = ConfigKeyTemperatureUnit, .value = CONFIG_TEMPERATURE_UNIT},
    {.key = ConfigKeyRefreshRate, .value = 20},
    {.key = ConfigKeyDateDisplayed, .value = CONFIG_DATE_DISPLAYED},
    {.key = ConfigKeyRainbowMode, .value = PBL_IF_COLOR_ELSE(CONFIG_RAINBOW_MODE, false)},
    {.key = ConfigKeyWeatherEnabled, .value = CONFIG_WEATHER_ENABLED},
    {.key = ConfigKeyVibrateOnTheHour, .value = false},
    {.key = ConfigKeyVersion, .value = CONF_VERSION},
    {.key = ConfigKeyHealthEnabled, .value = false},
    {.key = ConfigKeyBatteryDisplayedAt, .value = -1},
    {.key = ConfigKeyQuietTimeVisible, .value = true},
    {.key = ConfigKeyAnimationEnabled, .value = true}};

static void update_current_time()
{
#ifdef SCREENSHOT
    time_t screenshot_time = SCREENSHOT;
    s_current_time = gmtime(&screenshot_time);
#else
    const time_t temp = time(NULL);
    s_current_time = localtime(&temp);
#endif
    s_context.time = s_current_time;
}

// Messenger

static void config_info_color_updated(DictionaryIterator *iter, Tuple *tuple)
{
    config_set_int(s_config, ConfigKeyInfoColor, tuple->value->int32);
}

static void config_background_color_updated(DictionaryIterator *iter, Tuple *tuple)
{
    config_set_int(s_config, ConfigKeyBackgroundColor, tuple->value->int32);
    window_set_background_color(s_main_window, config_get_color(s_config, ConfigKeyBackgroundColor));
}

static void config_time_color_updated(DictionaryIterator *iter, Tuple *tuple)
{
    config_set_int(s_config, ConfigKeyTimeColor, tuple->value->int32);
    layer_mark_dirty(s_tick_layer);

    text_block_mark_dirty(s_hour_text);
    text_block_mark_dirty(s_minute_text);
}

static void config_hour_hand_color_updated(DictionaryIterator *iter, Tuple *tuple)
{
    config_set_int(s_config, ConfigKeyHourHandColor, tuple->value->int32);
    layer_mark_dirty(s_hour_hand_layer);
}

static void config_minute_hand_color_updated(DictionaryIterator *iter, Tuple *tuple)
{
    config_set_int(s_config, ConfigKeyMinuteHandColor, tuple->value->int32);
    mark_dirty_minute_hand_layer();
}

static void config_refresh_rate_updated(DictionaryIterator *iter, Tuple *tuple)
{
    config_set_int(s_config, ConfigKeyRefreshRate, tuple->value->int32);
}

static void config_temperature_unit_updated(DictionaryIterator *iter, Tuple *tuple)
{
    config_set_int(s_config, ConfigKeyTemperatureUnit, tuple->value->int32);
    text_block_mark_dirty(s_weather_info);
}

static void config_bluetooth_icon_updated(DictionaryIterator *iter, Tuple *tuple)
{
    config_set_int(s_config, ConfigKeyBluetoothIcon, tuple->value->int32);
    text_block_mark_dirty(s_watch_info);
}

static void config_battery_displayed_at_updated(DictionaryIterator *iter, Tuple *tuple)
{
    config_set_int(s_config, ConfigKeyBatteryDisplayedAt, tuple->value->int32);
    text_block_mark_dirty(s_watch_info);
}

static void config_date_displayed_updated(DictionaryIterator *iter, Tuple *tuple)
{
    config_set_bool(s_config, ConfigKeyDateDisplayed, tuple->value->int8);
    text_block_set_enabled(s_date_info, tuple->value->int8);
    text_block_mark_dirty(s_date_info);
}

static void config_rainbow_mode_updated(DictionaryIterator *iter, Tuple *tuple)
{
    config_set_bool(s_config, ConfigKeyRainbowMode, tuple->value->int8);
    mark_dirty_minute_hand_layer();
}

static void config_weather_enabled_updated(DictionaryIterator *iter, Tuple *tuple)
{
    const bool enabled = tuple->value->int8;
    config_set_bool(s_config, ConfigKeyWeatherEnabled, enabled);
    text_block_set_enabled(s_weather_info, enabled);
}

static void config_hourly_vibrate_updated(DictionaryIterator *iter, Tuple *tuple)
{
    config_set_bool(s_config, ConfigKeyVibrateOnTheHour, tuple->value->int8);
}

static void config_health_enabled_updated(DictionaryIterator *iter, Tuple *tuple)
{
    const bool enabled = tuple->value->int8;
    config_set_bool(s_config, ConfigKeyHealthEnabled, enabled);
    if (enabled)
    {
        fetch_step(&s_context);
    }
    text_block_set_enabled(s_steps_info, enabled);
}

static void config_quiet_time_visible_updated(DictionaryIterator *iter, Tuple *tuple)
{
    config_set_bool(s_config, ConfigKeyQuietTimeVisible, tuple->value->int8);
    text_block_mark_dirty(s_watch_info);
}

static void config_animation_enabled(DictionaryIterator *iter, Tuple *tuple)
{
    config_set_bool(s_config, ConfigKeyAnimationEnabled, tuple->value->int8);
}

static void js_ready_callback(DictionaryIterator *iter, Tuple *tuple)
{
    s_js_ready = true;
    schedule_weather_request(NOW);
}

static void weather_requested_callback(DictionaryIterator *iter, Tuple *tuple)
{
    s_context.reset_weather = false;
    const Tuple *const icon_tuple = dict_find(iter, AppKeyWeatherIcon);
    const Tuple *const temp_tuple = dict_find(iter, AppKeyWeatherTemperature);
    if (icon_tuple && temp_tuple)
    {
        s_context.weather.failed = false;
        s_context.weather.timestamp = time(NULL);
        s_context.weather.icon = icon_tuple->value->int8;
        s_context.weather.temperature = temp_tuple->value->int8;
    }
    persist_write_data(PersistKeyWeather, &s_context.weather, sizeof(Weather));
    text_block_mark_dirty(s_weather_info);
    quadrants_update(s_quadrants, s_current_time);
}

static void weather_request_failed_callback(DictionaryIterator *iter, Tuple *tuple)
{
    s_context.weather.failed = true;
    s_context.weather.timestamp = time(NULL);
    persist_write_data(PersistKeyWeather, &s_context.weather, sizeof(Weather));
    text_block_mark_dirty(s_weather_info);
    quadrants_update(s_quadrants, s_current_time);
}

static void messenger_callback(DictionaryIterator *iter)
{
    if (dict_find(iter, AppKeyConfig))
    {
        config_set_int(s_config, ConfigKeyVersion, CONF_VERSION);
        config_save(s_config, PersistKeyConfig);
        s_context.reset_weather = true;
        schedule_weather_request(NOW);
        quadrants_update(s_quadrants, s_current_time);
        layer_mark_dirty(s_root_layer);
    }
}

// Time

static bool times_conflicting(const tm *const time)
{
    return time->tm_hour % 12 == time->tm_min / 5;
}

static bool times_conflicting_north_or_south(const tm *const time)
{
    if (!times_conflicting(time))
        return false;
    const int hour_mod_12 = time->tm_hour % 12;
    return hour_mod_12 <= 1 || hour_mod_12 >= 11 || (hour_mod_12 >= 5 && hour_mod_12 <= 7);
}

static void hour_time_update_proc(TextBlock *block)
{
    const Context *const context = (Context *)text_block_get_context(block);
    const GColor color = config_get_color(s_config, ConfigKeyTimeColor);
    char buffer[] = "00:00";
    const int hour = context->time->tm_hour;
    const int hour_mod_12 = hour % 12;
    const GPoint block_center = get_time_position(hour_mod_12, s_unob_area_anim_progress);
    const bool military_time = clock_is_24h_style();
    const int printed_hour = military_time ? hour : hour_mod_12 == 0 ? 12
                                                                     : hour_mod_12;
    if (times_conflicting_north_or_south(context->time))
    {
        const int min = context->time->tm_min;
        snprintf(buffer, sizeof(buffer), "%d:%02d", printed_hour, min);
        text_block_set_text(block, buffer, color);
        text_block_move(block, block_center);
    }
    else
    {
        snprintf(buffer, sizeof(buffer), "%d", printed_hour);
        text_block_set_text(block, buffer, color);
        if (times_conflicting(context->time))
        {
            text_block_move(block, GPoint(block_center.x, block_center.y - TIME_CONFLICT_OFFSET));
        }
        else
        {
            text_block_move(block, block_center);
        }
    }
}

static void minute_time_update_proc(TextBlock *block)
{
    const Context *const context = (Context *)text_block_get_context(block);
    const Config *const config = context->config;
    const GColor color = config_get_color(config, ConfigKeyTimeColor);
    char buffer[] = "00";
    const int min = context->time->tm_min;
    const GPoint block_center = get_time_position(min / 5, s_unob_area_anim_progress);
    if (times_conflicting_north_or_south(context->time))
    {
        text_block_set_text(s_minute_text, "", color);
    }
    else
    {
        text_block_set_enabled(s_minute_text, true);
        snprintf(buffer, sizeof(buffer), "%02d", min);
        if (times_conflicting(context->time))
        {
            text_block_move(s_minute_text, GPoint(block_center.x, block_center.y + TIME_CONFLICT_OFFSET));
        }
        else
        {
            text_block_move(s_minute_text, block_center);
        }
        text_block_set_text(s_minute_text, buffer, color);
    }
}

// Date

static void date_info_update_proc(TextBlock *block)
{
    const Context *const context = (Context *)text_block_get_context(block);
    const Config *const config = context->config;
    if (config_get_bool(config, ConfigKeyDateDisplayed))
    {
        const GColor date_color = config_get_color(config, ConfigKeyInfoColor);
        char buffer[] = "00";
        snprintf(buffer, sizeof(buffer), "%d", context->time->tm_mday);
        text_block_set_text(block, buffer, date_color);
    }
}

// Hands
static AnimationProgress s_animation_progress;

static void mark_dirty_minute_hand_layer()
{
    layer_mark_dirty(s_minute_hand_layer);
    const bool rainbow_mode = config_get_bool(s_config, ConfigKeyRainbowMode);
    if (rainbow_mode)
    {
        const int minute_angle = angle_minute(s_current_time);
        rot_bitmap_layer_set_angle(s_rainbow_hand_layer, minute_angle);
    }
    layer_set_hidden((Layer *)s_rainbow_hand_layer, !rainbow_mode);
}

static void update_minute_hand_layer(Layer *layer, GContext *ctx)
{
    if (!config_get_bool(s_config, ConfigKeyRainbowMode))
    {
        const int start_angle = angle(270, 360);
        const int minute_angle = angle_minute(s_current_time);
        const int hand_angle = minute_angle - start_angle * (ANIMATION_NORMALIZED_MAX - s_animation_progress) / (ANIMATION_NORMALIZED_MAX + 1);
        const GPoint hand_end = gpoint_on_circle(g_center, hand_angle, MINUTE_HAND_RADIUS);
        graphics_context_set_stroke_width(ctx, MINUTE_HAND_WIDTH);
        graphics_context_set_stroke_color(ctx, config_get_color(s_config, ConfigKeyMinuteHandColor));
        graphics_draw_line(ctx, g_center, hand_end);
    }
}

static void update_hour_hand_layer(Layer *layer, GContext *ctx)
{
    const int hour_angle = angle_hour(s_current_time, true);
    const int start_angle = angle(90, 360);
    const bool rainbow_mode = config_get_bool(s_config, ConfigKeyRainbowMode);
    const int hand_angle = rainbow_mode ? hour_angle : hour_angle - start_angle * (ANIMATION_NORMALIZED_MAX - s_animation_progress) / (ANIMATION_NORMALIZED_MAX + 1);
    const GPoint hand_end = gpoint_on_circle(g_center, hand_angle, HOUR_HAND_RADIUS);
    graphics_context_set_stroke_width(ctx, HOUR_HAND_WIDTH);
    graphics_context_set_stroke_color(ctx, config_get_color(s_config, ConfigKeyHourHandColor));
    graphics_draw_line(ctx, g_center, hand_end);
}

static void update_center_circle_layer(Layer *layer, GContext *ctx)
{
    const GColor color = config_get_bool(s_config, ConfigKeyRainbowMode) ? GColorVividViolet : config_get_color(s_config, ConfigKeyHourHandColor);
    graphics_context_set_fill_color(ctx, color);
    graphics_fill_circle(ctx, g_center, CENTER_CIRCLE_RADIUS);
}

// Ticks

static void draw_tick(GContext *ctx, const int index)
{
    GPoint points[2]; 
    get_tick_positions(index, s_unob_area_anim_progress, points);
    graphics_draw_line(ctx, points[0], points[1]);
}

static void tick_layer_update_callback(Layer *layer, GContext *graphic_ctx)
{
    const Context *const context = *(Context **)layer_get_data(layer);
    graphics_context_set_stroke_color(graphic_ctx, config_get_color(context->config, ConfigKeyTimeColor));
    graphics_context_set_stroke_width(graphic_ctx, TICK_WIDTH);
    const tm *const time = context->time;
    draw_tick(graphic_ctx, time->tm_hour % 12);
    if (times_conflicting(time))
    {
        return;
    }
    draw_tick(graphic_ctx, time->tm_min / 5);
}

// Weather

#define FAILED_TIMEOUT 2*60
#define WEATHER_VISIBLE_TOLERANCE 5*60

static time_t weather_timeout(const Context *const context)
{
    const Weather *const weather = &(context->weather);
    const Config *const config = context->config;
    return (weather->failed ? FAILED_TIMEOUT : (config_get_int(config, ConfigKeyRefreshRate)) * 60);
}

static void weather_info_update_proc(TextBlock *block)
{
    const Context *const context = (Context *)text_block_get_context(block);
    const Config *const config = context->config;
    const Weather *const weather = &context->weather;
    const int timeout = weather_timeout(context);
    const int expiration = weather->timestamp + timeout + WEATHER_VISIBLE_TOLERANCE;
    const bool weather_valid = time(NULL) < expiration;
    char info_buffer[6] = {0};
    if (weather_valid && !weather->failed)
    {
        const int temp = weather->temperature;
        const bool is_farhrenheit = config_get_int(config, ConfigKeyTemperatureUnit) == Fahrenheit;
        const int converted_temp = is_farhrenheit ? (temp * 9 + 2) / 5 + 32 : temp;
        snprintf(info_buffer, 6, "%c%d°", weather->icon, converted_temp);
    }
    else if (weather->failed)
    {
        info_buffer[0] = 'x';
    }
    const GColor info_color = config_get_color(s_config, ConfigKeyInfoColor);
    text_block_set_text(block, info_buffer, info_color);
}

#define WEATHER_TIMEOUT 5000

static void send_weather_request_callback(void *context)
{
    s_weather_request_timer = NULL;
    const int timeout = weather_timeout(&s_context);
    const int expiration = s_context.weather.timestamp + timeout;
    const bool almost_expired = time(NULL) > expiration;
    const bool can_update_weather = (s_context.reset_weather || almost_expired) && s_js_ready;
    if (can_update_weather)
    {
        if (config_get_bool(s_config, ConfigKeyWeatherEnabled))
        {
            DictionaryIterator *out_iter;
            AppMessageResult result = app_message_outbox_begin(&out_iter);
            if (result == APP_MSG_OK)
            {
                const int value = 1;
                dict_write_int(out_iter, AppKeyWeatherRequest, &value, sizeof(int), true);
                result = app_message_outbox_send();
                if (result != APP_MSG_OK)
                {
                    schedule_weather_request(WEATHER_TIMEOUT);
                }
            }
            else
            {
                schedule_weather_request(WEATHER_TIMEOUT);
            }
        }
    }
}

static void schedule_weather_request(const int timeout)
{
    const int expiration = time(NULL) + timeout;
    if (s_weather_request_timer)
    {
        if (expiration < s_weather_request_timeout)
        {
            s_weather_request_timeout = expiration;
            app_timer_reschedule(s_weather_request_timer, timeout);
        }
    }
    else
    {
        s_weather_request_timeout = expiration;
        s_weather_request_timer = app_timer_register(timeout, send_weather_request_callback, NULL);
    }
}

// Battery + Bluetooth + Quiet Time

static void watch_info_update_proc(TextBlock *block)
{
    const Context *const context = (Context *)text_block_get_context(block);
    const Config *const config = context->config;
    char info_buffer[4] = {0};
    int buf_pos = 0;
    const BluetoothIcon bluetooth_icon = config_get_int(config, ConfigKeyBluetoothIcon);
    const bool bluetooth_disconneted = !context->bluetooth_connected;
    const bool bluetooth_icon_set = bluetooth_icon != NoIcon;
    if (bluetooth_disconneted && bluetooth_icon_set)
    {
        info_buffer[buf_pos++] = bluetooth_icon == Bluetooth ? 'z' : 'Z';
    }
    const int battery_threshold = config_get_int(config, ConfigKeyBatteryDisplayedAt);
    const BatteryChargeState charge_state = context->charge_state;
    const bool battery_below_threshold = charge_state.charge_percent < battery_threshold;
    if (battery_below_threshold)
    {
        info_buffer[buf_pos++] = 'w';
    }
    if (quiet_time_is_active() && config_get_bool(config, ConfigKeyQuietTimeVisible))
    {
        info_buffer[buf_pos++] = 'q';
    }

    const GColor info_color = config_get_color(config, ConfigKeyInfoColor);
    text_block_set_text(block, info_buffer, info_color);
}

// Steps

static void steps_info_update_proc(TextBlock *block)
{
    const Context *const context = (Context *)text_block_get_context(block);
    const Config *const config = context->config;
    const int steps = context->steps;
    char step_text[8] = {0};
    const GColor info_color = config_get_color(config, ConfigKeyInfoColor);
    if (steps > 10000)
    {
        snprintf(step_text, sizeof(step_text), "y%dk", steps / 1000);
    }
    else if (steps > 1000)
    {
        snprintf(step_text, sizeof(step_text), "y%d.%dk", steps / 1000, (steps % 1000) / 100);
    }
    else
    {
        snprintf(step_text, sizeof(step_text), "y%d", steps);
    }
    text_block_set_text(block, step_text, info_color);
}

static void fetch_step(Context *const context)
{
    if (config_get_bool(context->config, ConfigKeyHealthEnabled))
    {
        context->steps = (int)health_service_sum_today(HealthMetricStepCount);
    }
}

// Event handlers

static void update_watch_info_layer_visibility()
{
    const Config *const config = s_context.config;
    const bool battery_icon_visible = s_context.charge_state.charge_percent < config_get_int(config, ConfigKeyBatteryDisplayedAt);
    const bool bt_icon_visible = !s_context.bluetooth_connected && config_get_int(config, ConfigKeyBluetoothIcon) != NoIcon;
    const bool quiet_time_visible = quiet_time_is_active() && config_get_bool(config, ConfigKeyQuietTimeVisible);
    text_block_set_enabled(s_watch_info, battery_icon_visible || bt_icon_visible || quiet_time_visible);
}

static void bt_handler(bool connected)
{
    if (connected)
    {
        schedule_weather_request(NOW);
    }
    s_context.bluetooth_connected = connected;
    update_watch_info_layer_visibility();
    text_block_mark_dirty(s_watch_info);
}

static void battery_handler(BatteryChargeState charge)
{
    s_context.charge_state = charge;
    update_watch_info_layer_visibility();
    text_block_mark_dirty(s_watch_info);
}

static void step_handler(HealthEventType event, void *context)
{
    if (event == HealthEventSignificantUpdate)
    {
        fetch_step((Context *)context);
        text_block_mark_dirty(s_steps_info);
    }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
    if (HOUR_UNIT & units_changed)
    {
        const bool vibrate_on_the_hour = config_get_bool(s_config, ConfigKeyVibrateOnTheHour);
        const bool quiet_time = quiet_time_is_active();
        if (vibrate_on_the_hour && !quiet_time)
        {
            if (PBL_IF_HEALTH_ELSE(config_get_bool(s_config, ConfigKeyHealthEnabled), false) ||
                !(health_service_peek_current_activities() &
                  (HealthActivitySleep | HealthActivityRestfulSleep)))
            {
                vibes_short_pulse();
            }
        }
    }
    schedule_weather_request(10000);
    update_current_time();
    fetch_step(&s_context);

    layer_mark_dirty(s_hour_hand_layer);
    layer_mark_dirty(s_tick_layer);
    mark_dirty_minute_hand_layer();

    text_block_mark_dirty(s_hour_text);
    text_block_mark_dirty(s_minute_text);
    text_block_mark_dirty(s_date_info);
    text_block_mark_dirty(s_steps_info);

    quadrants_update(s_quadrants, s_current_time);
}

static void implementation_update(Animation *animation,
                                  const AnimationProgress progress)
{
    s_animation_progress = progress;
    layer_mark_dirty(s_hour_hand_layer);
    mark_dirty_minute_hand_layer();
}

static const AnimationImplementation implementation = {
    .update = implementation_update,
};

static GPoint s_old_center;
static GPoint s_new_center;

static void unobstructed_area_will_change_handler(GRect final_unobstructed_screen_area, void *context)
{
    s_old_center = g_center;
    s_new_center = grect_center_point(&final_unobstructed_screen_area);
    tick_points_will_change(&final_unobstructed_screen_area);
    quadrants_unobstructed_area_will_change(final_unobstructed_screen_area);
}

static void unobstructed_area_change_handler(AnimationProgress progress, void *context)
{
    g_center = gpoint_lerp_anim(s_old_center, s_new_center, progress);
    quadrants_unobstructed_area_changing(progress);
    quadrants_update(s_quadrants, s_current_time);
    layer_mark_dirty(s_hour_hand_layer);
    mark_dirty_minute_hand_layer();
    s_unob_area_anim_progress = progress;
}

static void unobstructed_area_did_change_handler(void *context)
{
    g_center = s_new_center;
    quadrants_unobstructed_area_done();
    quadrants_update(s_quadrants, s_current_time);
    tick_points_done_changing();
    s_unob_area_anim_progress = ANIMATION_NORMALIZED_MIN;
}

static void main_window_load(Window *window)
{
    s_root_layer = window_get_root_layer(window);
    s_root_layer_bounds = layer_get_bounds(s_root_layer);
    GRect unob_bounds = layer_get_unobstructed_bounds(s_root_layer);
    g_center = grect_center_point(&unob_bounds);
    tick_points_init(&unob_bounds);
    update_current_time();
    window_set_background_color(window, config_get_color(s_config, ConfigKeyBackgroundColor));
    
    s_quadrants = quadrants_create(g_center, HOUR_HAND_RADIUS, MINUTE_HAND_RADIUS, s_root_layer);
    s_date_info = quadrants_add_text_block(s_quadrants, s_root_layer, s_font, Low, s_current_time);
    text_block_set_enabled(s_date_info, config_get_bool(s_config, ConfigKeyDateDisplayed));
    text_block_set_context(s_date_info, &s_context);
    text_block_set_update_proc(s_date_info, date_info_update_proc);

    s_steps_info = quadrants_add_text_block(s_quadrants, s_root_layer, s_font, High, s_current_time);
    text_block_set_enabled(s_steps_info, config_get_bool(s_config, ConfigKeyHealthEnabled));
    text_block_set_context(s_steps_info, &s_context);
    text_block_set_update_proc(s_steps_info, steps_info_update_proc);
    health_service_events_subscribe(step_handler, &s_context);
    fetch_step(&s_context);

    s_weather_info = quadrants_add_text_block(s_quadrants, s_root_layer, s_font, Head, s_current_time);
    text_block_set_enabled(s_weather_info, config_get_bool(s_config, ConfigKeyWeatherEnabled));
    text_block_mark_dirty(s_weather_info);
    text_block_set_context(s_weather_info, &s_context);
    text_block_set_update_proc(s_weather_info, weather_info_update_proc);

    s_watch_info = quadrants_add_text_block(s_quadrants, s_root_layer, s_font, Tail, s_current_time);
    text_block_set_context(s_watch_info, &s_context);
    text_block_set_update_proc(s_watch_info, watch_info_update_proc);
    bluetooth_connection_service_subscribe(bt_handler);
    bt_handler(connection_service_peek_pebble_app_connection());
    battery_state_service_subscribe(battery_handler);
    battery_handler(battery_state_service_peek());
    update_watch_info_layer_visibility();

    s_hour_text = text_block_create(s_root_layer, get_time_position(6, ANIMATION_NORMALIZED_MIN), s_font);
    text_block_set_context(s_hour_text, &s_context);
    text_block_set_update_proc(s_hour_text, hour_time_update_proc);

    s_minute_text = text_block_create(s_root_layer, get_time_position(0, ANIMATION_NORMALIZED_MIN), s_font);
    text_block_set_context(s_minute_text, &s_context);
    text_block_set_update_proc(s_minute_text, minute_time_update_proc);

    s_tick_layer = layer_create(s_root_layer_bounds);
    s_tick_layer = layer_create_with_data(s_root_layer_bounds, sizeof(Context *));
    Context **data = (Context **)layer_get_data(s_tick_layer);
    *data = &s_context;
    layer_set_update_proc(s_tick_layer, tick_layer_update_callback);
    layer_add_child(s_root_layer, s_tick_layer);

    s_rainbow_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMG_RAINBOW_HAND);
    s_minute_hand_layer = layer_create(s_root_layer_bounds);
    s_hour_hand_layer = layer_create(s_root_layer_bounds);
    s_center_circle_layer = layer_create(s_root_layer_bounds);
    s_rainbow_hand_layer = rot_bitmap_layer_create(s_rainbow_bitmap);
    rot_bitmap_set_compositing_mode(s_rainbow_hand_layer, GCompOpSet);
    const GPoint png_center = GPoint(RAINBOW_HAND_OFFSET_X, RAINBOW_HAND_OFFSET_Y);
    rot_bitmap_set_src_ic(s_rainbow_hand_layer, png_center);
    GRect frame = layer_get_frame((Layer *)s_rainbow_hand_layer);
    frame.origin.x = g_center.x - frame.size.w / 2;
    frame.origin.y = g_center.y - frame.size.h / 2;
    layer_set_frame((Layer *)s_rainbow_hand_layer, frame);
    layer_set_update_proc(s_hour_hand_layer, update_hour_hand_layer);
    layer_set_update_proc(s_minute_hand_layer, update_minute_hand_layer);
    layer_set_update_proc(s_center_circle_layer, update_center_circle_layer);
    layer_add_child(s_root_layer, s_minute_hand_layer);
    layer_add_child(s_root_layer, (Layer *)s_rainbow_hand_layer);
    layer_add_child(s_root_layer, s_hour_hand_layer);
    layer_add_child(s_root_layer, s_center_circle_layer);
    mark_dirty_minute_hand_layer();

    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

    quadrants_update(s_quadrants, s_current_time);

    if (config_get_bool(s_config, ConfigKeyAnimationEnabled))
    {
        s_animation_progress = 0;

        Animation *animation = animation_create();
        animation_set_curve(animation, AnimationCurveEaseInOut);

        animation_set_delay(animation, 0);
        animation_set_duration(animation, 1000);

        animation_set_implementation(animation, &implementation);

        animation_schedule(animation);
    }
    else 
    {
        s_animation_progress = ANIMATION_NORMALIZED_MAX;
    }

    UnobstructedAreaHandlers unobstructed_area_handlers = {
        .will_change = unobstructed_area_will_change_handler,
        .change = unobstructed_area_change_handler,
        .did_change = unobstructed_area_did_change_handler
    };

    unobstructed_area_service_subscribe(unobstructed_area_handlers, NULL);
}

static void main_window_unload(Window *window)
{
    layer_destroy(s_hour_hand_layer);
    rot_bitmap_layer_destroy(s_rainbow_hand_layer);
    layer_destroy(s_minute_hand_layer);
    layer_destroy(s_center_circle_layer);
    gbitmap_destroy(s_rainbow_bitmap);

    text_block_destroy(s_hour_text);
    text_block_destroy(s_minute_text);

    layer_destroy(s_tick_layer);

    if (config_get_bool(s_config, ConfigKeyHealthEnabled))
    {
        health_service_events_unsubscribe();
    }
    bluetooth_connection_service_unsubscribe();

    s_quadrants = quadrants_destroy(s_quadrants);

    text_block_destroy(s_weather_info);
    text_block_destroy(s_date_info);
    text_block_destroy(s_steps_info);
    text_block_destroy(s_watch_info);
}

static void init()
{
    static const Message messages[] = {
        {AppKeyJsReady, js_ready_callback},
        {AppKeyWeatherTemperature, weather_requested_callback},
        {AppKeyWeatherFailed, weather_request_failed_callback},
        {AppKeyBackgroundColor, config_background_color_updated},
        {AppKeyHourHandColor, config_hour_hand_color_updated},
        {AppKeyInfoColor, config_info_color_updated},
        {AppKeyMinuteHandColor, config_minute_hand_color_updated},
        {AppKeyTimeColor, config_time_color_updated},
        {AppKeyDateDisplayed, config_date_displayed_updated},
        {AppKeyRainbowMode, config_rainbow_mode_updated},
        {AppKeyBluetoothIcon, config_bluetooth_icon_updated},
        {AppKeyRefreshRate, config_refresh_rate_updated},
        {AppKeyTemperatureUnit, config_temperature_unit_updated},
        {AppKeyWeatherEnabled, config_weather_enabled_updated},
        {AppKeyVibrateOnTheHour, config_hourly_vibrate_updated},
        {AppKeyHealthEnabled, config_health_enabled_updated},
        {AppKeyBatteryDisplayedAt, config_battery_displayed_at_updated},
        {AppKeyQuietTimeVisible, config_quiet_time_visible_updated},
        {AppKeyAnimationEnabled, config_animation_enabled}};
    s_messenger = messenger_create(sizeof(messages) / sizeof(Message), messenger_callback, messages);
    s_weather_request_timeout = 0;
    s_js_ready = false;
    s_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_NUPE_23));
    s_config = config_load(PersistKeyConfig, CONF_SIZE, CONF_DEFAULTS);
    s_context = (Context){
        .config = s_config,
        .steps = 0,
        .reset_weather = false,
        .bluetooth_connected = false,
        .charge_state = (BatteryChargeState){
            .charge_percent = 100,
            .is_charging = false,
            .is_plugged = false},
        .time = NULL};
    if (persist_exists(PersistKeyWeather))
    {
        persist_read_data(PersistKeyWeather, &s_context.weather, sizeof(Weather));
    }
    s_main_window = window_create();
    window_set_window_handlers(s_main_window, (WindowHandlers){
                                                  .load = main_window_load,
                                                  .unload = main_window_unload});
    window_stack_push(s_main_window, true);
}

static void deinit()
{
    tick_timer_service_unsubscribe();
    app_message_deregister_callbacks();
    window_stack_remove(s_main_window, true);
    window_destroy(s_main_window);
    s_config = config_destroy(s_config);
    fonts_unload_custom_font(s_font);
    s_messenger = messenger_destroy(s_messenger);
}

int main(void)
{
    init();
    app_event_loop();
    deinit();
}
