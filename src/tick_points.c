#include <pebble.h>

#include "tick_points.h"
#include "geometry.h"

#ifdef PBL_ROUND
static const GPoint tick_points[12][2] = {
    {{90, 0}, {90, 6}},
    {{135, 12}, {132, 18}},
    {{168, 45}, {162, 48}},
    {{180, 90}, {174, 90}},
    {{168, 135}, {162, 132}},
    {{135, 168}, {132, 162}},
    {{90, 180}, {90, 174}},
    {{45, 168}, {48, 162}},
    {{12, 135}, {18, 132}},
    {{0, 90}, {6, 90}},
    {{12, 45}, {18, 48}},
    {{45, 12}, {48, 18}}};
static  const GPoint time_points[12] = {
    {90, 17},
    {124, 28},
    {150, 50},
    {161, 86},
    {148, 124},
    {124, 146},
    {90, 159},
    {54, 147},
    {29, 124},
    {18, 87},
    {30, 52},
    {54, 28},
};

void tick_points_init(const GRect *area) {};
void tick_points_will_change(const GRect *new_area) {}
void tick_points_anim_change(AnimationProgress progress) {}
void tick_points_done_changing() {}

void get_tick_positions(int tick, AnimationProgress prog, GPoint *const out)
{
    memcpy(out, tick_points[tick], 2 * sizeof(GPoint));
}

GPoint get_time_position(int tick, AnimationProgress prog)
{
    return time_points[tick];
}
#else


#define TAN30 147
#define SCALING_FACTOR 0x100

#ifdef PBL_PLATFORM_EMERY
#define TICK_LENGTH 10
#define TICK_DX 4
#define TICK_DY 5
#define TIME_POINT_DX 23
#define TIME_POINT_DY 21
#define TIME_POINT_DY2 30
#define TIME_Y_OFFSET 2
#else
#define TICK_LENGTH 7
#define TICK_DX 3
#define TICK_DY 4
#define TIME_POINT_DX 18
#define TIME_POINT_DY 15
#define TIME_POINT_DY2 23
#define TIME_Y_OFFSET 1
#endif

static GPoint s_tick_points[12][2];
static GPoint s_time_points[12];

static GPoint s_new_tick_points[12][2];
static GPoint s_new_time_points[12];

static void tick_point_change_positions(GPoint tick_points[][2], GPoint time_points[], const GRect *const area)
{
    const GPoint center = grect_center_point(area);
    const GPoint origin = area->origin;
    const GPoint end = GPoint(area->origin.x + area->size.w, area->origin.y + area->size.h - 1);
    // TAN30 is represented * 0x100
    // multiply it by the dimension to get the height difference on the rectangle
    // "radius" would be area->size / 2
    // so divide by 0x200
    const int one_dx_edge = (TAN30 * area->size.h + SCALING_FACTOR / 2) / (SCALING_FACTOR * 2);
    const int two_dy_edge = (TAN30 * area->size.w + SCALING_FACTOR / 2) / (SCALING_FACTOR * 2);

    tick_points[0][0] = GPoint(center.x, origin.y);
    tick_points[1][0] = GPoint(center.x + one_dx_edge, origin.y);
    tick_points[2][0] = GPoint(end.x, center.y - two_dy_edge);
    tick_points[3][0] = GPoint(end.x, center.y);
    tick_points[4][0] = GPoint(end.x, center.y + two_dy_edge);
    tick_points[5][0] = GPoint(center.x + one_dx_edge, end.y);
    tick_points[6][0] = GPoint(center.x, end.y);
    tick_points[7][0] = GPoint(center.x - one_dx_edge, end.y);
    tick_points[8][0] = GPoint(origin.x, center.y + two_dy_edge);
    tick_points[9][0] = GPoint(origin.x, center.y);
    tick_points[10][0] = GPoint(origin.x, center.y - two_dy_edge);
    tick_points[11][0] = GPoint(center.x - one_dx_edge, origin.y);

    const int one_dx_tick = one_dx_edge - TICK_DX;
    const int two_dy_tick = two_dy_edge - TICK_DY; 
    
    tick_points[0][1] = GPoint(center.x, origin.y + TICK_LENGTH);
    tick_points[1][1] = GPoint(center.x + one_dx_tick, origin.y + TICK_LENGTH);
    tick_points[2][1] = GPoint(end.x - TICK_LENGTH, center.y - two_dy_tick);
    tick_points[3][1] = GPoint(end.x - TICK_LENGTH, center.y);
    tick_points[4][1] = GPoint(end.x - TICK_LENGTH, center.y + two_dy_tick);
    tick_points[5][1] = GPoint(center.x + one_dx_tick, end.y - TICK_LENGTH);
    tick_points[6][1] = GPoint(center.x, end.y - TICK_LENGTH);
    tick_points[7][1] = GPoint(center.x - one_dx_tick, end.y - TICK_LENGTH);
    tick_points[8][1] = GPoint(origin.x + TICK_LENGTH, center.y + two_dy_tick);
    tick_points[9][1] = GPoint(origin.x + TICK_LENGTH, center.y);
    tick_points[10][1] = GPoint(origin.x + TICK_LENGTH, center.y - two_dy_tick);
    tick_points[11][1] = GPoint(center.x - one_dx_tick, origin.y + TICK_LENGTH);

    const int one_dx_time = (TAN30 * (area->size.h - TIME_POINT_DY)) / (SCALING_FACTOR * 2);
    const int two_dy_time = (TAN30 * (area->size.w - TIME_POINT_DX)) / (SCALING_FACTOR * 2);

    time_points[0] = GPoint(center.x, origin.y + TIME_POINT_DY);
    time_points[1] = GPoint(center.x + one_dx_time, origin.y + TIME_POINT_DY);
    time_points[2] = GPoint(end.x - TIME_POINT_DX, center.y - two_dy_time);
    time_points[3] = GPoint(end.x - TIME_POINT_DX, center.y - TIME_Y_OFFSET);
    time_points[4] = GPoint(end.x - TIME_POINT_DX, center.y + two_dy_time);
    time_points[5] = GPoint(center.x + one_dx_time, end.y - TIME_POINT_DY2);
    time_points[6] = GPoint(center.x, end.y - TIME_POINT_DY2);
    time_points[7] = GPoint(center.x - one_dx_time, end.y - TIME_POINT_DY2);
    time_points[8] = GPoint(origin.x + TIME_POINT_DX, center.y + two_dy_time);
    time_points[9] = GPoint(origin.x + TIME_POINT_DX, center.y - TIME_Y_OFFSET);
    time_points[10] = GPoint(origin.x + TIME_POINT_DX, center.y - two_dy_time);
    time_points[11] = GPoint(center.x - one_dx_time, origin.y + TIME_POINT_DY);
}

void tick_points_init(const GRect *area)
{
    tick_point_change_positions(s_tick_points, s_time_points, area);
}

void tick_points_will_change(const GRect *new_area) 
{
    tick_point_change_positions(s_new_tick_points, s_new_time_points, new_area);
}

void tick_points_done_changing() 
{
    memcpy(s_tick_points, s_new_tick_points, 12 * 2 * sizeof(GPoint));
    memcpy(s_time_points, s_new_time_points, 12 * sizeof(GPoint));
}

void get_tick_positions(int tick, AnimationProgress prog, GPoint *const out)
{
    if (prog == ANIMATION_NORMALIZED_MIN)
    {
        memcpy(out,s_tick_points[tick], 2 * sizeof(GPoint));
    }
    else
    {
        out[0] = gpoint_lerp_anim(s_tick_points[tick][0], s_new_tick_points[tick][0], prog);
        out[1] = gpoint_lerp_anim(s_tick_points[tick][1], s_new_tick_points[tick][1], prog);
    }
}

GPoint get_time_position(int tick, AnimationProgress prog)
{
    if (prog == ANIMATION_NORMALIZED_MIN)
    {
        return s_time_points[tick];
    }
    else
    {
        return gpoint_lerp_anim(s_time_points[tick], s_new_time_points[tick], prog);
    }
}
#endif



