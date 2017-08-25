#include <pebble.h>
#include "battery_gauge.h"
#include "bluetooth_indicator.h"

// Define this during development to make it easier to see animations
// in a timely fashion.
//#define FAST_TIME 1

#define SCREEN_WIDTH 144
#define SCREEN_HEIGHT 168

Window *window;

GBitmap *current_face;
Layer *face_layer;
TextLayer *time_layer, *date_layer, *day_layer;

int hour_value, minute_value, second_value, day_value, date_value;
bool shaken = false;

#define NUM_FACES 26
int face_resource_ids[NUM_FACES] = {
  RESOURCE_ID_ROSE,
  RESOURCE_ID_ONE,
  RESOURCE_ID_TWO,
  RESOURCE_ID_THREE,
  RESOURCE_ID_FOUR,
  RESOURCE_ID_FIVE,
  RESOURCE_ID_SIX,
  RESOURCE_ID_SEVEN,
  RESOURCE_ID_EIGHT,
  RESOURCE_ID_WAR,
  RESOURCE_ID_NINE,
  RESOURCE_ID_TEN,
  RESOURCE_ID_ELEVEN,
  RESOURCE_ID_TWELVE,
  RESOURCE_ID_TARDIS,
  RESOURCE_ID_DALEK,
  RESOURCE_ID_ANGEL,
  RESOURCE_ID_CYBERMAN,
  RESOURCE_ID_MASTER,
  RESOURCE_ID_DONNA,
  RESOURCE_ID_AMY,
  RESOURCE_ID_CLARA,
  RESOURCE_ID_HANDLES,
  RESOURCE_ID_MARTHA,
  RESOURCE_ID_RIVER,
  RESOURCE_ID_RORY
};

void face_layer_update_callback(Layer *me, GContext* ctx) {
  time_t now = time(NULL);
  struct tm *now_struct = localtime(&now);
  if(now_struct->tm_min % 15 == 0 || current_face==NULL || shaken) {
    // Update face with new bitmap, chosen randomly.
    int index = rand() % NUM_FACES;
    gbitmap_destroy(current_face);
    current_face = gbitmap_create_with_resource(face_resource_ids[index]);
  }
  GRect destination = layer_get_frame(me);
  destination.origin.x = 0;
  destination.origin.y = 0;

  // Draw black box at bottom for date and time.
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);
  graphics_draw_bitmap_in_rect(ctx, current_face, destination);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, 118, 144, 50), 8, GCornersTop);
  shaken = false;
}

void time_update_callback(struct tm *time, TimeUnits units_changed) {
  // Update time
  static char buffer[] = "00:00 AM";
  strftime(buffer, sizeof(buffer), "%l:%M%p", time);
  text_layer_set_text(time_layer, buffer);
  
  // Update date
  static char date_buffer[] = "DAY MON 00";
  strftime(date_buffer, sizeof(date_buffer), "%a %b %d", time);
  text_layer_set_text(date_layer, date_buffer);
  
  layer_mark_dirty(face_layer);
}

void tap_handler(AccelAxisType axis, int32_t direction) {
  shaken = true;
  layer_mark_dirty(face_layer);
}


void handle_init() {
  time_t now = time(NULL);
  struct tm *startup_time = localtime(&now);
  srand(now);
  
  hour_value = startup_time->tm_hour % 12;
  //int index = rand() % NUM_FACES;
  //current_face = gbitmap_create_with_resource(face_resource_ids[index]);
  
  minute_value = startup_time->tm_min;
  second_value = startup_time->tm_sec;
  day_value = startup_time->tm_wday;
  date_value = startup_time->tm_mday;
  
  window = window_create();
  window_set_background_color(window, GColorWhite);
  struct Layer *root_layer = window_get_root_layer(window);

  window_stack_push(window, true);

  // Init face layer.
  face_layer = layer_create(layer_get_bounds(root_layer));
  layer_set_update_proc(face_layer, &face_layer_update_callback);
  layer_add_child(root_layer, face_layer);
  
  // Init time layer.
  time_layer = text_layer_create(GRect(10, 122, 124, 26));
  text_layer_set_background_color(time_layer, GColorBlack);
  text_layer_set_text_color(time_layer, GColorWhite);
  text_layer_set_text(time_layer, "00:00AM");
  text_layer_set_font(time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_TIMELORD_SOLID_24)));
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  layer_add_child(root_layer, text_layer_get_layer(time_layer));

  // Init date layer.
  date_layer = text_layer_create(GRect(10, 150, 124, 16));
  text_layer_set_background_color(date_layer, GColorBlack);
  text_layer_set_text_color(date_layer, GColorWhite);
  text_layer_set_text(date_layer, "Day Mon 00");
  text_layer_set_font(date_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_TIMELORD_SOLID_12)));
  text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
  layer_add_child(root_layer, text_layer_get_layer(date_layer));
  
  
  // Subscribe to minute ticks to update time, date, and day.
  tick_timer_service_subscribe(MINUTE_UNIT, &time_update_callback);

  init_battery_gauge(root_layer, 125, 0, false, true);

  init_bluetooth_indicator(root_layer, 0, 0, false, true);
    APP_LOG(APP_LOG_LEVEL_INFO, "bluetooth setup");
  
  // Subscribe to shake event to update face.
  accel_tap_service_subscribe(&tap_handler);

}

void handle_deinit() {
  tick_timer_service_unsubscribe();
  accel_tap_service_unsubscribe();

  window_stack_pop_all(false);  // Not sure if this is needed?
  deinit_battery_gauge();
  deinit_bluetooth_indicator();
  gbitmap_destroy(current_face);
  text_layer_destroy(time_layer);
  text_layer_destroy(date_layer);
  text_layer_destroy(day_layer);
  layer_destroy(face_layer);
  window_destroy(window);

}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}