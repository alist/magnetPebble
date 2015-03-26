#include "pebble.h"

#define ACCEL_STEP_MS 10
#define MIN_TAP_PERIOD_MS 100

#define TAP_HAPPENED 0


static Window *s_main_window;
static TextLayer *s_text_below, *s_text_above;
static InverterLayer *s_inverter_layer;
uint64_t lastTapTime = 0;

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  s_text_below = text_layer_create(GRect(0, 54, bounds.size.w, bounds.size.h-54 /* height */));
  text_layer_set_text(s_text_below, "below");
  text_layer_set_font(s_text_below, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  text_layer_set_text_alignment(s_text_below, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_text_below));

  s_inverter_layer = inverter_layer_create(GRect(bounds.size.w/2, 0, bounds.size.w/2, bounds.size.h));
  layer_add_child(window_layer, inverter_layer_get_layer(s_inverter_layer));

  s_text_above = text_layer_create(GRect(0, 84, bounds.size.w, 168-84 /* height */));
  text_layer_set_text(s_text_above, "above");
  text_layer_set_font(s_text_above, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  text_layer_set_text_alignment(s_text_above, GTextAlignmentCenter);
  text_layer_set_background_color(s_text_above, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(s_text_above));
}

static void processDataPoint(AccelData dataPoint){
  if (abs(dataPoint.z) > 1500){
    if (abs(dataPoint.timestamp - lastTapTime) > MIN_TAP_PERIOD_MS){
      lastTapTime = dataPoint.timestamp;

      // APP_LOG(APP_LOG_LEVEL_DEBUG, "x %i y %i z %i, time %lu", dataPoint.x,dataPoint.y,dataPoint.z, (unsigned long)dataPoint.timestamp);
      Layer * inverter = inverter_layer_get_layer(s_inverter_layer);
      bool hidden = layer_get_hidden(inverter);
      layer_set_hidden(inverter, !hidden);

      static const uint32_t const segments[] = { 50, 1};
      VibePattern pat = {
        .durations = segments,
        .num_segments = ARRAY_LENGTH(segments),
      };
      vibes_enqueue_custom_pattern(pat);

    }
  }
}

static void timer_callback(void *data) {
  AccelData accel = (AccelData) { .x = 0, .y = 0, .z = 0 };
  accel_service_peek(&accel);

  processDataPoint(accel);

  app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_text_above);
  text_layer_destroy(s_text_below);
  inverter_layer_destroy(s_inverter_layer);
}

static void init() {
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);
  
  accel_data_service_subscribe(0, NULL);
  app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);
}

static void deinit() {
  accel_tap_service_unsubscribe();
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
