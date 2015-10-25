#include <pebble.h>
#include "bluetooth_layer.h"

#define ICN_BT_HEIGHT 20
#define ICN_BT_WIDTH 13
#define ICN_BT_RADIUS 18 + ICN_BT_HEIGHT / 2

static GBitmap * s_icn_bt_black;
static BitmapLayer * s_bluetooth_layer;

static void bt_handler(bool connected){
  APP_LOG(APP_LOG_LEVEL_DEBUG,"connected: %d", connected);
  if(connected){
    bitmap_layer_set_bitmap(s_bluetooth_layer, NULL);
  }else{
    bitmap_layer_set_bitmap(s_bluetooth_layer, s_icn_bt_black);
  }
}

void init_bluetooth_layer(Layer * root_layer){
  s_icn_bt_black = gbitmap_create_with_resource(RESOURCE_ID_ICN_BT_BLACK);
  GRect root_layer_bounds = layer_get_bounds(root_layer);
  GPoint layer_origin = grect_center_point(&root_layer_bounds);
  layer_origin.x -= ICN_BT_WIDTH / 2;
  layer_origin.y -= ICN_BT_HEIGHT / 2 + ICN_BT_RADIUS;
  GRect layer_bounds = (GRect) { .origin = layer_origin, .size = GSize(ICN_BT_WIDTH, ICN_BT_HEIGHT) };
  s_bluetooth_layer = bitmap_layer_create(layer_bounds);
  bt_handler(connection_service_peek_pebble_app_connection());
  layer_add_child(root_layer, bitmap_layer_get_layer(s_bluetooth_layer));
  bluetooth_connection_service_subscribe(bt_handler);
}

void deinit_bluetooth_layer(){
  bitmap_layer_destroy(s_bluetooth_layer); 
  gbitmap_destroy(s_icn_bt_black);
  bluetooth_connection_service_unsubscribe();
}
