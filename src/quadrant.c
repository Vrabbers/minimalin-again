#include "text_block.h"
#include "geometry.h"
#include "pebble.h"
#include "quadrant.h"

#define QUADRANT_COUNT 4
#define POSTIONS_COUNT 4
#define BLOCK_SIZE GSize(38, 20)
#define QUADRANT(blck, prio, pos) \
    (Quadrant) { .block = block, .priority = prio, .position = pos }
#define BLOCK(quadrants, index) quadrants->quadrants[index]->block
#define PRIORITY(quadrants, index) quadrants->quadrants[index]->priority
#define POS(quadrants, index) quadrants->quadrants[index]->position

static GPoint *info_centers = NULL;

static GPoint *new_info_centers;

static AnimationProgress animation_progess = ANIMATION_NORMALIZED_MIN;

static GRect rect_translate(const GRect rect, const int x, const int y)
{
    const GPoint origin = rect.origin;
    return (GRect){.origin = GPoint(origin.x + x, origin.y + y), .size = rect.size};
}

static GPoint center_for_position(const Position position)
{
    GPoint center = info_centers[position];
    if (new_info_centers != NULL && animation_progess != ANIMATION_NORMALIZED_MIN)
    {
        const GPoint new_center = new_info_centers[position];
        const int32_t dx = ((new_center.x - center.x) * ANIMATION_NORMALIZED_MAX) / (ANIMATION_NORMALIZED_MAX + 1);
        const int32_t dy = ((new_center.y - center.y) * ANIMATION_NORMALIZED_MAX) / (ANIMATION_NORMALIZED_MAX + 1);
        center.x += dx;
        center.y += dy;
    }
    return center;
}

static bool segment_intersect_with_position(const Segment segment, const Position position)
{
    GPoint center = center_for_position(position);
    const GRect rect = grect_from_center_and_size(center, BLOCK_SIZE);
    return intersect(segment, rect_translate(rect, 0, 4));
}

static void quadrants_move_quadrant(Quadrants *const quadrants, const Index index, const Position position)
{
    if (index >= POSTIONS_COUNT)
    {
        return;
    }
    Quadrant *const quadrant = quadrants->quadrants[index];
    quadrant->position = position;
    text_block_move(quadrant->block, center_for_position(position));
}

static void quadrants_swap(Quadrants *quadrants, const Index first, const Index second)
{
    Quadrant *const first_quadrant = quadrants->quadrants[first];
    Quadrant *const second_quadrant = quadrants->quadrants[second];
    const Position first_position = first_quadrant->position;
    quadrants_move_quadrant(quadrants, first, second_quadrant->position);
    quadrants_move_quadrant(quadrants, second, first_position);
}

static bool quadrants_takeover_quadrant(Quadrants *const quadrants, const Index index, const Position position)
{
    if (index >= QUADRANT_COUNT)
    {
        return false;
    }
    for (int index_to_takeover = 0; index_to_takeover < quadrants->size; index_to_takeover++)
    {
        if (index_to_takeover == index || POS(quadrants, index_to_takeover) != position)
        {
            continue;
        }
        const bool has_higher_priority = PRIORITY(quadrants, index_to_takeover) >= PRIORITY(quadrants, index);
        const TextBlock *const block = BLOCK(quadrants, index_to_takeover);
        if (has_higher_priority && ((text_block_get_ready(block) && text_block_get_visible(block)) || text_block_get_enabled(block)))
        {
            return false;
        }
        quadrants_swap(quadrants, index_to_takeover, index);
        return true;
    }
    quadrants_move_quadrant(quadrants, index, position);
    return true;
}

static bool time_intersect_with_position(Quadrants *const quadrants, const tm *const time, const Position pos)
{
    const GPoint center = quadrants->center;
    const int hour_handle = angle_hour(time, true);
    const Segment hour_hand = SEGMENT(quadrants->center, gpoint_on_circle(center, hour_handle, quadrants->hour_hand_radius));
    const int minute_angle = angle_minute(time);
    const Segment minute_hand = SEGMENT(quadrants->center, gpoint_on_circle(center, minute_angle, quadrants->minute_hand_radius));
    return segment_intersect_with_position(hour_hand, pos) || segment_intersect_with_position(minute_hand, pos);
}

static bool quadrants_try_takeover_quadrant_in_order(Quadrants *const quadrants, const Index index, const tm *const time, const Position order[POSTIONS_COUNT], const bool check_intersect)
{
    for (int index_pos = 0; index_pos < POSTIONS_COUNT; index_pos++)
    {
        const Position pos = order[index_pos];
        if (check_intersect && time_intersect_with_position(quadrants, time, pos))
        {
            continue;
        }
        if (quadrants_takeover_quadrant(quadrants, index, pos))
        {
            return true;
        }
    }
    return false;
}

static void quadrants_try_takeover_quadrant(Quadrants *const quadrants, const Index index, const tm *const time)
{
    Position order[POSTIONS_COUNT] = {North, South, East, West};
    const int hour_mod_12 = time->tm_hour % 12;
    const int min_fifth = time->tm_min / 5;
    const bool hour_at_3 = hour_mod_12 == 3;
    const bool hour_at_9 = hour_mod_12 == 9;
    const bool min_at_3 = min_fifth == 3;
    const bool min_at_9 = min_fifth == 9;
    if (hour_at_3 || hour_at_9 || min_at_3 || min_at_9)
    {
        if ((hour_at_3 && !min_at_9) || (!hour_at_9 && min_at_3))
        {
            order[2] = West;
            order[3] = East;
        }
        else if (!((hour_at_9 && !min_at_3) || (!hour_at_3 && min_at_9)))
        {
            quadrants_try_takeover_quadrant_in_order(quadrants, index, time, order, false);
            return;
        }
    }
    if (quadrants_try_takeover_quadrant_in_order(quadrants, index, time, order, true))
    {
        return;
    }
    quadrants_try_takeover_quadrant_in_order(quadrants, index, time, order, false);
}

static GPoint *create_centers_for_rect(const GRect area)
{
    GPoint *centers = (GPoint *)malloc(sizeof(GPoint) * POSTIONS_COUNT);
    const int quarter_width = area.size.w / 4;
    const int quarter_height = area.size.h / 4;
    centers[North] = GPoint(area.origin.x + 2 * quarter_width, area.origin.y + quarter_height);
    centers[South] = GPoint(area.origin.x + 2 * quarter_width, area.origin.y + 3 * quarter_height);
    centers[West] = GPoint(area.origin.x + quarter_width, area.origin.y + 2 * quarter_height);
    centers[East] = GPoint(area.origin.x + 3 * quarter_width, area.origin.y + 2 * quarter_height);
    return centers;
}

Quadrants *quadrants_create(const GPoint center, const int hour_hand_radius, const int minute_hand_radius, const Layer *const root_layer)
{
    info_centers = create_centers_for_rect(layer_get_unobstructed_bounds(root_layer));
    Quadrants *const quadrants = (Quadrants *)malloc(sizeof(Quadrants));
    quadrants->ready = false;
    quadrants->center = center;
    quadrants->size = 0;
    for (int i = 0; i < QUADRANT_COUNT; i++)
    {
        quadrants->free_positions[i] = true;
    }
    quadrants->hour_hand_radius = hour_hand_radius;
    quadrants->minute_hand_radius = minute_hand_radius;
    for (int i = 0; i < QUADRANT_COUNT; i++)
    {
        quadrants->quadrants[i] = NULL;
    }
    return quadrants;
}

Quadrants *quadrants_destroy(Quadrants *const quadrants)
{
    for (int i = 0; i < quadrants->size; i++)
    {
        Quadrant *const quadrant = quadrants->quadrants[i];
        if (quadrant != NULL)
        {
            free(quadrant);
        }
    }
    free(quadrants);
    return NULL;
}

TextBlock *quadrants_add_text_block(Quadrants *const quadrants, Layer *const root_layer, const GFont font, const Priority priority, const tm *const time)
{
    Position position = North;
    for (int pos = 0; pos < POSTIONS_COUNT; pos++)
    {
        if (quadrants->free_positions[pos])
        {
            quadrants->free_positions[pos] = false;
            position = pos;
            break;
        }
    }

    TextBlock *const block = text_block_create(root_layer, center_for_position(position), font);
    text_block_set_ready(block, false);
    const int size = quadrants->size;
    if (size >= QUADRANT_COUNT)
    {
        return NULL;
    }
    Quadrant *const quadrant = (Quadrant *)malloc(sizeof(Quadrant));
    *quadrant = QUADRANT(block, priority, position);
    int i = size;
    while (i > 0 && quadrants->quadrants[i - 1] != NULL && PRIORITY(quadrants, i - 1) < priority)
    {
        quadrants->quadrants[i] = quadrants->quadrants[i - 1];
        i--;
    }
    quadrants->quadrants[i] = quadrant;
    quadrants->size++;
    return block;
}

void quadrants_update(Quadrants *const quadrants, const tm *const time)
{
    for (int index = 0; index < quadrants->size; index++)
    {
        const TextBlock *const block = BLOCK(quadrants, index);
        if ((text_block_get_ready(block) && text_block_get_visible(block)) || text_block_get_enabled(block))
        {
            quadrants_try_takeover_quadrant(quadrants, index, time);
        }
    }
    if (!quadrants->ready)
    {
        quadrants->ready = true;
        for (int index = 0; index < quadrants->size; index++)
        {
            text_block_set_ready(BLOCK(quadrants, index), true);
        }
    }
}

void quadrants_unobstructed_area_will_change(GRect new_unobstructed_area)
{
    new_info_centers = create_centers_for_rect(new_unobstructed_area);
}

void quadrants_unobstructed_area_changing(AnimationProgress anim_progress)
{
    animation_progess = anim_progress;
}

void quadrants_unobstructed_area_done(void)
{
    GPoint *const old_info_centers = info_centers;
    info_centers = new_info_centers;
    free(old_info_centers);
    new_info_centers = NULL;
    animation_progess = ANIMATION_NORMALIZED_MIN;
}
