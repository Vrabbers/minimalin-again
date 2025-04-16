#pragma once

#include <pebble.h>

void tick_points_init(const GRect *area);
void tick_points_will_change(const GRect *new_area);
void tick_points_done_changing();
void get_tick_positions(int tick, AnimationProgress prog, GPoint *out);
GPoint get_time_position(int tick, AnimationProgress prog);
