#include "pebble.h"

#define ACCEL_STEP_MS 10
#define MIN_TAP_PERIOD_MS 100

#define TAP_HAPPENED 1


static Window *s_main_window;
static InverterLayer *s_inverter_layer;
static TextLayer *s_time_layer;

static BitmapLayer *s_image_layer;
static GBitmap *s_image_bitmap;


uint64_t lastTapTime = 0;

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  // Create time TextLayer
  s_time_layer = text_layer_create(GRect(0, 108, 144, 50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));

  s_image_bitmap = gbitmap_create_with_resource(RESOURCE_ID_PEBBLE_MAGNET);
  s_image_layer = bitmap_layer_create(GRect(0, 0, 144, 110));
  bitmap_layer_set_bitmap(s_image_layer, s_image_bitmap);
  bitmap_layer_set_alignment(s_image_layer, GAlignCenter);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_image_layer));
  
  s_inverter_layer = inverter_layer_create(GRect(0, -60, bounds.size.w, bounds.size.h));
  layer_add_child(window_layer, inverter_layer_get_layer(s_inverter_layer));
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char buffer[] = "00:00";

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    // Use 12 hour format
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, buffer);
}

static void flipLayer(){
  Layer * inverter = inverter_layer_get_layer(s_inverter_layer);
  bool hidden = layer_get_hidden(inverter);
  layer_set_hidden(inverter, !hidden);
}

static void actuateNow(){
  flipLayer();

  static const uint32_t const segments[] = { 50, 1};
  VibePattern pat = {
    .durations = segments,
    .num_segments = ARRAY_LENGTH(segments),
  };
  vibes_cancel();
  vibes_enqueue_custom_pattern(pat);
}

// Write message to buffer & send
void send_message(void){
  DictionaryIterator *iter;
  
  app_message_outbox_begin(&iter);
  dict_write_uint8(iter, TAP_HAPPENED, 0x1);
  
  dict_write_end(iter);
  app_message_outbox_send();
}

static void perceiveNow(){
  flipLayer();
  send_message();
}

static void processDataPoint(AccelData dataPoint){
  if (abs(dataPoint.z) > 1500){
    if (abs(dataPoint.timestamp - lastTapTime) > MIN_TAP_PERIOD_MS){
      lastTapTime = dataPoint.timestamp;

      perceiveNow();

      // APP_LOG(APP_LOG_LEVEL_DEBUG, "x %i y %i z %i, time %lu", dataPoint.x,dataPoint.y,dataPoint.z, (unsigned long)dataPoint.timestamp);
    }
  }
}

static void timer_callback(void *data) {
  AccelData accel = (AccelData) { .x = 0, .y = 0, .z = 0 };
  accel_service_peek(&accel);

  processDataPoint(accel);

  app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);
}
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    // Get the first pair
  Tuple *t = dict_read_first(iterator);

  // Process all pairs present
  while(t != NULL) {
    // Process this pair's key
    switch (t->key) {
      case TAP_HAPPENED:
        // APP_LOG(APP_LOG_LEVEL_INFO, "TAP_HAPPENED received with value %d", (int)t->value->int32);
        actuateNow();
        break;
    }

    // Get next pair, if any
    t = dict_read_next(iterator);
  }
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  // APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  inverter_layer_destroy(s_inverter_layer);
  bitmap_layer_destroy(s_image_layer);
  gbitmap_destroy(s_image_bitmap);
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
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  update_time();
  
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
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
