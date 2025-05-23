#pragma once

#include <pebble.h>
#include "text_block.h"

#define FOUR 4

typedef enum
{
    North = 0,
    South,
    West,
    East
} Position;

typedef enum
{
    Tail,
    Low,
    Normal,
    High,
    Head
} Priority;

typedef enum
{
    First,
    Second,
    Third,
    Fourth
} Index;

typedef struct
{
    TextBlock *block;
    Priority priority;
    Position position;
} Quadrant;

typedef struct
{
    Quadrant *quadrants[4];
    Position free_positions[4];
    bool ready;
    int size;
    int hour_hand_radius;
    int minute_hand_radius;
} Quadrants;

Quadrants *quadrants_create(const GPoint center, const int hour_hand_radius, const int minute_hand_radius, const Layer *root_layer);
Quadrants *quadrants_destroy(Quadrants *const quadrants);
TextBlock *quadrants_add_text_block(Quadrants *const quadrants, Layer *const root_layer, const GFont font, const Priority priority, const tm *const time);
void quadrants_update(Quadrants *const quadrants, const tm *const time);

void quadrants_unobstructed_area_will_change(GRect new_unobstructed_area);
void quadrants_unobstructed_area_changing(AnimationProgress anim_progress);
void quadrants_unobstructed_area_done(void);
