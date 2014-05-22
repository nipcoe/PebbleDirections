#include <pebble.h>

static Window *window;
static TextLayer *text_layer;
static TextLayer *text_layer_north;
static Layer *layer;
static Layer *circle_layer;
static GPoint p0;
static GPoint p1;
static GPoint center;
static int compassLength;

// Pebble app to show directions

// Draws a line between p0 and p1. 
static void point_layer_update_callback(Layer *layer, GContext *ctx) {
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_draw_line(ctx, p0, p1);
}

// Draws a circle
static void circle_layer_update_callback(Layer *layer, GContext *ctx) {
  graphics_draw_circle(ctx, center, compassLength + 10);
}

// Handlers for the up, down, and select button. They do nothing
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
}

// Sets handlers for buttons
static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

// Loads the window, by initializing the layers. Draws when we first load
static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Writes waiting at the start
  text_layer = text_layer_create((GRect) { .origin = { 0, 130 }, .size = { bounds.size.w, 100 } });
  text_layer_set_text(text_layer, "Waiting for directions");
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);

  // Writes north in compass, and draws circle
  text_layer_north = text_layer_create((GRect) { .origin = { center.x + 10, 10 }, .size = { 20, 20 } });
  text_layer_set_text(text_layer_north, "N");
  text_layer_set_text_alignment(text_layer_north, GTextAlignmentCenter);
  
  circle_layer = layer_create(GRect(20, 10, 120, 120));
  layer = layer_create(GRect(20, 10, 120, 120));
  layer_set_update_proc(circle_layer, circle_layer_update_callback);

  layer_add_child(window_layer, text_layer_get_layer(text_layer_north));
  layer_add_child(window_layer, circle_layer);
  layer_add_child(window_layer, layer);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
}


static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
}


void out_sent_handler(DictionaryIterator *sent, void *context) {
  // outgoing message was delivered
}


void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  // outgoing message failed
}

// Handles when we receive a message/dictionary from the app
void in_received_handler(DictionaryIterator *received, void *context) {
  Tuple *in_tuple1 = dict_find(received, 1);
  Tuple *in_tuple2 = dict_find(received, 2);
  if(in_tuple1){
    text_layer_set_text(text_layer, in_tuple1->value->cstring);
  }
   if(in_tuple2){
     int32_t angle = TRIG_MAX_ANGLE* (in_tuple2->value->uint32) /360;
     //if(angle != -1){
     int32_t c_angle =  cos_lookup(angle )*compassLength/ TRIG_MAX_RATIO;
     int32_t s_angle =  sin_lookup(angle )*compassLength/ TRIG_MAX_RATIO;
     p0 = center;
     p1 = GPoint(center.x + c_angle, center.y - s_angle);
     
     layer_set_update_proc(layer, point_layer_update_callback);
     layer_mark_dirty(layer);
   }  
 }


void in_dropped_handler(AppMessageResult reason, void *context) {
  // incoming message dropped
}


static void init(void) {
  center = GPoint(52, 50);
  compassLength = 40;
  
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_register_outbox_sent(out_sent_handler);
  app_message_register_outbox_failed(out_failed_handler);

  const uint32_t inbound_size = 64;
  const uint32_t outbound_size = 64;
  app_message_open(inbound_size, outbound_size);

  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);
  
  app_event_loop();
  deinit();
}

