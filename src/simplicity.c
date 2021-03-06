/*
 * SimplicityPro by HBTeicher
 *
 * Copyright (c) 2014 HBTeicher/Christoffer Martinsson/Max Baeumle
 *
 * SimplicityPro is entirely based on code by:
 * Simplicity by CM /Christoffer Martinsson
 * Max Baeumle https://github.com/maxbaeumle/smartwatchface
 * James Fowler https://github.com/JamesFowler42/diaryface20
 * Martin Norland https://github.com/cynorg/PebbleTimely
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "common.h"

Window *window;

TextLayer *text_date_layer;
TextLayer *text_time_layer;
TextLayer *text_week_layer;

Layer *line_layer;

GBitmap *icon_battery;
Layer *battery_layer;

TextLayer *text_event_title_layer;
TextLayer *text_event_start_date_layer;
TextLayer *text_event_location_layer;

// connected info
static bool bluetooth_state_changed = true;
static bool app_state_changed = true;
static bool bluetooth_connected = true;
static bool app_connected = true;

static int current_day_number = 0; 

// Event array for storing two events
Event event[2];
static int event_received_rows;
static int event_count;

BatteryStatus battery_status;

// Vibe generator 
void generate_vibe(uint32_t vibe_pattern_number) {
  vibes_cancel();
  switch ( vibe_pattern_number ) {
  case 0: // No Vibration
    return;
  case 1: // Single short
    vibes_short_pulse();
    break;
  case 2: // Double short
    vibes_double_pulse();
    break;
  case 3: // Triple
    vibes_double_pulse();
  case 4: // Long
    vibes_double_pulse();
    break;
  case 5: // Subtle
    vibes_double_pulse();
    break;
  case 6: // Less Subtle
    vibes_double_pulse();
    break;
  case 7: // Not Subtle
    vibes_double_pulse();
    break;
  default: // No Vibration
    return;
  }
}

void line_layer_update_callback(Layer *layer, GContext *ctx) {
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_line(ctx, GPoint(8, 94), GPoint(131, 94));
  graphics_draw_line(ctx, GPoint(8, 95), GPoint(131, 95));
}

void battery_layer_update_callback(Layer *layer, GContext *ctx) {
  graphics_context_set_compositing_mode(ctx, GCompOpAssignInverted);
  graphics_draw_bitmap_in_rect(ctx, icon_battery, GRect(35, 0, 24, 12));

  if (battery_status.state != 0 && battery_status.level >= 0 && battery_status.level <= 100) {
    int num = snprintf(NULL, 0, "%hhd %%", battery_status.level);
    char *str = malloc(num + 1);
    snprintf(str, num + 1, "%hhd %%", battery_status.level);

    graphics_draw_text(ctx, str, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(-2, -3, 35-3, 20), 0, GTextAlignmentRight, NULL);

    free(str);

    if (battery_status.level > 0) {
      graphics_context_set_stroke_color(ctx, GColorBlack);
      graphics_context_set_fill_color(ctx, GColorWhite);
      graphics_fill_rect(ctx, GRect(38, 3, (uint8_t)((battery_status.level / 100.0) * 16.0), 6), 0, GCornerNone);
    }
  }
}

void update_connection(){
  // Only run when changed
  if(bluetooth_state_changed != bluetooth_connected){
    bluetooth_state_changed = bluetooth_connected;
    // Check BT status
    if(!bluetooth_connected){
      // Display text in calendar view
      text_layer_set_text(text_event_start_date_layer, "BT disconnected");
      text_layer_set_text(text_event_title_layer, "WARNING!");
      text_layer_set_text(text_event_location_layer, "");
      // Vibrate hard 5 times
      generate_vibe(7);
    }else{
      // Vibrate 3 times
      generate_vibe(3);
    }
  }
  // Only run when changed
  if((app_state_changed != app_connected) && (bluetooth_connected)){
    app_state_changed = app_connected;
    // Check smartwatch pro app status
    if(!app_connected){
      // Display text in calendar view
      text_layer_set_text(text_event_start_date_layer, "App disconnected");
      text_layer_set_text(text_event_title_layer, "WARNING!");
      text_layer_set_text(text_event_location_layer, "");
      // Vibrate hard 5 times
      generate_vibe(7);
    }else{
      // Vibrate 3 times
      generate_vibe(3);
    }
  }
}

void handle_timer(void *data) {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (!iter) {
    app_timer_register(1000, &handle_timer, NULL);
    return;
  }

  dict_write_uint8(iter, REQUEST_BATTERY_KEY, 1);
  app_message_outbox_send();
}

void handle_show_event_timer(void *data) {
  uint8_t event_index;
  // Select what event to show 
  if(event_received_rows > 1){ 
    // Check if current event is of type ALL DAY 
    if(event[0].all_day){
      // Check if next event is of type ALL DAY 
      if(event[1].all_day){
        // If so use current event
        event_index = 0;
      }else{
        // Else use next event
        event_index = 1;
      }
    }else{
        // If current event not of type ALL DAY use current event
      event_index = 0;
    }
    // If only current event present use current event
  }else{
    event_index = 0;
}
  // Update event text on watch
  text_layer_set_text(text_event_title_layer, event[event_index].title);
  text_layer_set_text(text_event_start_date_layer, event[event_index].start_date);
  text_layer_set_text(text_event_location_layer, event[event_index].has_location ? event[event_index].location : "");
}

void handle_bluetooth_connection(bool connected) {
   if(connected){
     // Check one extra time
     if(bluetooth_connection_service_peek()){
       // Connection to BT OK
       bluetooth_connected = true;
     }
   }else{
     // Connection to BT NOT OK!
     bluetooth_connected = false;
   }
   update_connection(); 
}

void handle_message_fail(DictionaryIterator *failed, AppMessageResult reason, void *context) {
    if(reason == APP_MSG_NOT_CONNECTED){
        // Connection to smartwatch pro app NOT OK!
        app_connected = false;
        update_connection();
    }
}

void handle_message_receive(DictionaryIterator *received, void *context) {
  Event temp_event;
  Tuple *tuple = dict_find(received, RECONNECT_KEY);

  if (tuple) {
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    if (!iter) return;
    dict_write_int8(iter, REQUEST_CALENDAR_KEY, -1);
    uint8_t clock_style = clock_is_24h_style() ? CLOCK_STYLE_24H : CLOCK_STYLE_12H;
    dict_write_uint8(iter, CLOCK_STYLE_KEY, clock_style);
    event_received_rows = 0;
    app_message_outbox_send();

    app_timer_register(1000, &handle_timer, NULL);
  } else {
    Tuple *tuple = dict_find(received, CALENDAR_RESPONSE_KEY);

    if (tuple) {
        // Get the next two events from phone
        uint8_t i, j;
        // Init indexing for iterating tuple
        if (event_count > event_received_rows) {
      		i = event_received_rows;
      		j = 0;
        } else {
      	    event_count = tuple->value->data[0];
      	    i = 0;
      	    j = 1;
        }
        // Copy data from tuple to event array
        while (i < event_count && j < tuple->length) {
    	    memcpy(&temp_event, &tuple->value->data[j], sizeof(Event));
			if (temp_event.index < 2)
      	        memcpy(&event[temp_event.index], &temp_event, sizeof(Event));

      	    i++;
      	    j += sizeof(Event);
        }

        event_received_rows = i;
       
        // Run/Reset timer to show event text on watch 
        app_timer_register(1000, &handle_show_event_timer, NULL);
    
    } else {
      Tuple *tuple = dict_find(received, BATTERY_RESPONSE_KEY);

      if (tuple) {
        memcpy(&battery_status, &tuple->value->data[0], sizeof(BatteryStatus));
        layer_mark_dirty(battery_layer);
      }
    }
  }
  // Connection to smartwatch pro app OK
  app_connected = true;
  update_connection();
}

void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  // Need to be static because they're used by the system later.
  static char time_text[] = "00:00";
  static char date_text[] = "Xxxxxxxxx xxx 00 xxx";
  static char week_text[] = "W 00";

  char *time_format;

  if (!tick_time) {
    time_t now = time(NULL);
    tick_time = localtime(&now);
  }

  // Only update if date changed
  if(tick_time->tm_mday != current_day_number){
    current_day_number = tick_time->tm_mday;
    // Update date and week
    strftime(week_text, sizeof(date_text), "W%V", tick_time);
    text_layer_set_text(text_week_layer, week_text);
    strftime(date_text, sizeof(date_text), "%A • %b %e", tick_time);
    text_layer_set_text(text_date_layer, date_text);
  }

  if (clock_is_24h_style()) {
    time_format = "%R";
  } else {
    time_format = "%I:%M";
  }

  strftime(time_text, sizeof(time_text), time_format, tick_time);

  // Kludge to handle lack of non-padded hour format string
  // for twelve hour clock.
  if (!clock_is_24h_style() && (time_text[0] == '0')) {
    memmove(time_text, &time_text[1], sizeof(time_text) - 1);
  }

  text_layer_set_text(text_time_layer, time_text);

  // Update calendar data
  if (tick_time->tm_min % 10 == 0) {
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    if (!iter) return;
	  //     dict_write_int8(iter, REQUEST_CALENDAR_KEY, 0);
    dict_write_int8(iter, REQUEST_CALENDAR_KEY, -1);
    uint8_t clock_style = clock_is_24h_style() ? CLOCK_STYLE_24H : CLOCK_STYLE_12H;
    dict_write_uint8(iter, CLOCK_STYLE_KEY, clock_style);
    app_message_outbox_send();

    app_timer_register(1000, &handle_timer, NULL);
  }
}

void init() {
  window = window_create();
  window_stack_push(window, true /* Animated */);
  window_set_background_color(window, GColorBlack);

  text_date_layer = text_layer_create(GRect(0, 94, 144, 168-94));
  text_layer_set_text_color(text_date_layer, GColorWhite);
  text_layer_set_background_color(text_date_layer, GColorClear);
  text_layer_set_font(text_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(text_date_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_date_layer));

  text_time_layer = text_layer_create(GRect(2, 112, 144-2, 168-112));
  text_layer_set_text_color(text_time_layer, GColorWhite);
  text_layer_set_background_color(text_time_layer, GColorClear);
  text_layer_set_font(text_time_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
  text_layer_set_text_alignment(text_time_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_time_layer));
  
  text_week_layer = text_layer_create(GRect(0, -5, 50, 28));
  text_layer_set_text_color(text_week_layer, GColorWhite);
  text_layer_set_background_color(text_week_layer, GColorClear);
  text_layer_set_font(text_week_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(text_week_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_week_layer));

  line_layer = layer_create(layer_get_bounds(window_get_root_layer(window)));
  layer_set_update_proc(line_layer, line_layer_update_callback);
  layer_add_child(window_get_root_layer(window), line_layer);

  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);

  text_event_title_layer = text_layer_create(GRect(5, 42, layer_get_bounds(window_get_root_layer(window)).size.w - 5, 31));
  text_layer_set_text_color(text_event_title_layer, GColorWhite);
  text_layer_set_background_color(text_event_title_layer, GColorClear);
  text_layer_set_font(text_event_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_event_title_layer));

  text_event_start_date_layer = text_layer_create(GRect(5, 64, layer_get_bounds(window_get_root_layer(window)).size.w - 5, 31));
  text_layer_set_text_color(text_event_start_date_layer, GColorWhite);
  text_layer_set_background_color(text_event_start_date_layer, GColorClear);
  text_layer_set_font(text_event_start_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_event_start_date_layer));

  text_event_location_layer = text_layer_create(GRect(5, 22, layer_get_bounds(window_get_root_layer(window)).size.w - 5, 31));
  text_layer_set_text_color(text_event_location_layer, GColorWhite);
  text_layer_set_background_color(text_event_location_layer, GColorClear);
  text_layer_set_font(text_event_location_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_event_location_layer));

  // Init bluetooth connection handles
  bluetooth_connection_service_subscribe(&handle_bluetooth_connection);
  handle_bluetooth_connection(bluetooth_connection_service_peek());
  
  icon_battery = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_ICON);

  GRect frame;
  frame.origin.x = layer_get_bounds(window_get_root_layer(window)).size.w - 66;
  frame.origin.y = 6;
  frame.size.w = 59;
  frame.size.h = 12;

  battery_layer = layer_create(frame);
  layer_set_update_proc(battery_layer, battery_layer_update_callback);
  layer_add_child(window_get_root_layer(window), battery_layer);

  battery_status.state = 0;
  battery_status.level = -1;

  app_message_open(124, 256);
  app_message_register_inbox_received(handle_message_receive);
  app_message_register_outbox_failed(handle_message_fail);

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  if (!iter) return;
  dict_write_int8(iter, REQUEST_CALENDAR_KEY, -1);
  uint8_t clock_style = clock_is_24h_style() ? CLOCK_STYLE_24H : CLOCK_STYLE_12H;
  dict_write_uint8(iter, CLOCK_STYLE_KEY, clock_style);
  event_received_rows = 0;
  app_message_outbox_send();

  app_timer_register(1000, &handle_timer, NULL);
  
}

void deinit() {
  app_message_deregister_callbacks();
  layer_destroy(battery_layer);
  gbitmap_destroy(icon_battery);
  text_layer_destroy(text_event_location_layer);
  text_layer_destroy(text_event_start_date_layer);
  text_layer_destroy(text_event_title_layer);
  tick_timer_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  layer_destroy(line_layer);
  text_layer_destroy(text_time_layer);
  text_layer_destroy(text_date_layer);
  text_layer_destroy(text_week_layer);
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
