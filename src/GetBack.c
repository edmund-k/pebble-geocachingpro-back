#include <pebble.h>
  
#undef APP_LOG
#define APP_LOG(...)

#define MENU_LENGTH 22

static Window *window, *menu_window;
static TextLayer *distance_layer;
static TextLayer *unit_layer;
static TextLayer *hint_layer;
static Layer *head_layer;
static TextLayer *star_layer;
static TextLayer *n_layer;
static TextLayer *e_layer;
static TextLayer *s_layer;
static TextLayer *w_layer;
static TextLayer *heading_layer;
static TextLayer *direction_layer;
static TextLayer *bearing_layer;
static TextLayer *accuracy_layer;
static TextLayer *speed_layer;
static TextLayer *dialog_layer;
static MenuLayer *menu_layer;
static char hint_text[80];
int32_t distance = 0;
int16_t bearing = 0;
int16_t heading = -1;
int16_t orientation = 0;
bool orientToHeading = false;
int16_t nextcall = 5;
static const uint32_t CMD_KEY = 0x1;
static const uint32_t BEARING_KEY = 0x2;
static const uint32_t DISTANCE_KEY = 0x3;
static const uint32_t UNIT_KEY = 0x4;
static const uint32_t NEXTCALL_KEY = 0x5;
static const uint32_t SPEED_KEY = 0x6;
static const uint32_t ACCURACY_KEY = 0x7;
static const uint32_t HEADING_KEY = 0x8;
static const uint32_t MESSAGE_KEY = 0x9;
static const uint32_t LOCATION_KEY = 0xA;
static char command[12] = "get";
static GPath *head_path;
static GRect hint_layer_size;
static AppTimer *locationtimer;
static char locations[MENU_LENGTH][40];
static int last_menu_item = 1;
const bool animated = true;

#ifdef PBL_SDK_3
static bool pinID_needs_sending;
#endif
  
GPoint center;

const GPathInfo HEAD_PATH_POINTS = {
  3,
  (GPoint []) {
    {0, -77},
    {-11, -48},
    {11,  -48},
  }
};

static void tick_handler(void *data) {
  if (nextcall == 0) return;
  text_layer_set_text(speed_layer, "?");
  strcpy(command,"get");
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  if (iter == NULL) {
    vibes_short_pulse();
    APP_LOG(APP_LOG_LEVEL_WARNING, "Can not send command '%s' to phone!", command);
  } else {
    dict_write_cstring(iter, CMD_KEY, command);
    const uint32_t final_size = dict_write_end(iter);
    app_message_outbox_send();
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Sent command '%s' to phone! (%d bytes)", command, (int) final_size);
  }
  locationtimer = app_timer_register(1000 * (nextcall + 10), (AppTimerCallback) tick_handler, NULL);
}
  
static void draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "I'm drawing.");
  menu_cell_title_draw(ctx, cell_layer, locations[cell_index->row]);
}

static uint16_t num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
	return last_menu_item + 1;
}

static int16_t cell_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	return 32;
}

static void select_click_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Got a click for row %d", cell_index->row);
  if (hint_layer) {
    text_layer_destroy(hint_layer);
    hint_layer = NULL;
  }
  if (cell_index->row > 0) {
    app_timer_reschedule(locationtimer, 1000 * (nextcall + 10));
    if (cell_index->row == 1) {
      strcpy(command, "set");
      text_layer_set_text(distance_layer, "0");
    } else {
      snprintf(command, sizeof(command), "set%d", (cell_index->row)-2);
      text_layer_set_text(distance_layer, "?");
    }
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    if (iter == NULL) {
      vibes_short_pulse();
      APP_LOG(APP_LOG_LEVEL_WARNING, "Can not send command %s to phone!", command);
      return;
    }
    dict_write_cstring(iter, CMD_KEY, command);
    const uint32_t final_size = dict_write_end(iter);
    app_message_outbox_send();
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Sent command '%s' to phone! (%d bytes)", command, (int) final_size);
  }
  window_stack_pop(animated);
}
 
static void reset_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Reset");
  if (hint_layer) {
    text_layer_destroy(hint_layer);
    hint_layer = NULL;
  } else {
    window_stack_push(menu_window, animated);
    menu_layer_reload_data(menu_layer);
    layer_mark_dirty(menu_layer_get_layer(menu_layer));  
  }
}

#ifdef PBL_SDK_3
static void pin_action_handler(uint32_t pinID) {
  snprintf(command, sizeof command, "%i", (int) pinID);
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  if (iter == NULL) {
    vibes_short_pulse();
    APP_LOG(APP_LOG_LEVEL_WARNING, "Can not send command %s to phone!", command);
    return;
  }
  dict_write_cstring(iter, CMD_KEY, command);
  const uint32_t final_size = dict_write_end(iter);
  app_message_outbox_send();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Sent command '%s' to phone! (%d bytes)", command, (int) final_size);
  if (!hint_layer) {
    Layer *window_layer = window_get_root_layer(window);
    hint_layer = text_layer_create(hint_layer_size);
    text_layer_set_font(hint_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_text_alignment(hint_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(hint_layer));
  }
  snprintf(hint_text, sizeof hint_text, "Setting\nlocation\ninformation\nfrom pin\n%i", (int) pinID);
  text_layer_set_text(hint_layer, hint_text);
  pinID_needs_sending = false;
}
#endif
  
static void get_info_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Get location info.");
  if (hint_layer) {
    text_layer_destroy(hint_layer);
    hint_layer = NULL;
  } 
  else {
    app_timer_reschedule(locationtimer, 1000 * (nextcall + 10));
    strcpy(command, "inf");
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    if (iter == NULL) {
      vibes_short_pulse();
      APP_LOG(APP_LOG_LEVEL_WARNING, "Can not send command %s to phone!", command);
      return;
    }
    dict_write_cstring(iter, CMD_KEY, command);
    const uint32_t final_size = dict_write_end(iter);
    app_message_outbox_send();
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Sent command '%s' to phone! (%d bytes)", command, (int) final_size);
    if (!hint_layer) {
      Layer *window_layer = window_get_root_layer(window);
      hint_layer = text_layer_create(hint_layer_size);
      text_layer_set_font(hint_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
      text_layer_set_text_alignment(hint_layer, GTextAlignmentCenter);
      layer_add_child(window_layer, text_layer_get_layer(hint_layer));
    }
    text_layer_set_text(hint_layer, "\nGetting\nlocation\ninformation...");
  }
}

static void pin_set_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Pin set");
  if (hint_layer) {
    text_layer_destroy(hint_layer);
    hint_layer = NULL;
  } 
  else {
    app_timer_reschedule(locationtimer, 1000 * (nextcall + 10));
    strcpy(command, "pin");
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    if (iter == NULL) {
      vibes_short_pulse();
      APP_LOG(APP_LOG_LEVEL_WARNING, "Can not send command %s to phone!", command);
      return;
    }
    dict_write_cstring(iter, CMD_KEY, command);
    const uint32_t final_size = dict_write_end(iter);
    app_message_outbox_send();
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Sent command '%s' to phone! (%d bytes)", command, (int) final_size);
    if (!hint_layer) {
      Layer *window_layer = window_get_root_layer(window);
      hint_layer = text_layer_create(hint_layer_size);
      text_layer_set_font(hint_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
      text_layer_set_text_alignment(hint_layer, GTextAlignmentCenter);
      layer_add_child(window_layer, text_layer_get_layer(hint_layer));
    }
    text_layer_set_text(hint_layer, "\nSetting\ntimeline\npin...");
  }
}

static void hint_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Hint");
  if (hint_layer) {
    text_layer_destroy(hint_layer);
    hint_layer = NULL;
  } else {
    app_timer_cancel(locationtimer);
    tick_handler(NULL);
  }  
}  

static void go_back_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Go back.");
  if (hint_layer) {
    text_layer_destroy(hint_layer);
    hint_layer = NULL;
  } else
    window_stack_pop_all(animated);  // Exit the app.  
}  

void out_sent_handler(DictionaryIterator *sent, void *context) {
   // outgoing message was delivered
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Message accepted by phone: %s", command);
  if (strncmp(command,"set",3) == 0) {
    if (!hint_layer) {
      Layer *window_layer = window_get_root_layer(window);
      hint_layer = text_layer_create(hint_layer_size);
      text_layer_set_font(hint_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
      text_layer_set_text_alignment(hint_layer, GTextAlignmentCenter);
      layer_add_child(window_layer, text_layer_get_layer(hint_layer));
    }
    if (strlen(command) == 3)
      strncpy(hint_text, "\nSetting\nreference\nlocation...", sizeof(hint_text));
    else if (strlen(command) == 4)
      snprintf(hint_text, sizeof(hint_text), "Selected\n%s.", locations[command[3] - '0' + 2]);
    else /* command is set1x */
      snprintf(hint_text, sizeof(hint_text), "Selected\n%s.", locations[command[4] - '0' + 12]);
    text_layer_set_text(hint_layer, hint_text);
  };
}

void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
   // outgoing message failed
  APP_LOG(APP_LOG_LEVEL_WARNING, "\nMessage rejected by phone: %s %d", command, reason);
  if (reason == APP_MSG_SEND_TIMEOUT) {
    if (strncmp(command,"set",3) == 0) {
      if (!hint_layer) {
        Layer *window_layer = window_get_root_layer(window);
        hint_layer = text_layer_create(hint_layer_size);
        text_layer_set_font(hint_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
        text_layer_set_text_alignment(hint_layer, GTextAlignmentCenter);
        layer_add_child(window_layer, text_layer_get_layer(hint_layer));
      }
      text_layer_set_text(hint_layer, "Cannot set target.\nPlease restart the app.");
      vibes_double_pulse();
    };
  }
}

void draw_compass_face(void) {
  int32_t starx = 0;
  int32_t stary = 0;
  if (orientToHeading) {
    starx = center.x;
    stary = center.y-63;
  } else if (heading>=0) {
    starx = center.x + 63 * sin_lookup(TRIG_MAX_ANGLE * heading / 360 + orientation)/TRIG_MAX_RATIO;
    stary = center.y - 63 * cos_lookup(TRIG_MAX_ANGLE * heading / 360 + orientation)/TRIG_MAX_RATIO;
  }    
  layer_set_frame((Layer *) star_layer, GRect(starx - 14, stary - 18, 28, 28));
  int32_t nx = center.x + 63 * sin_lookup(orientation)/TRIG_MAX_RATIO;
  int32_t ny = center.y - 63 * cos_lookup(orientation)/TRIG_MAX_RATIO;
#ifndef PBL_COLOR
  if (orientToHeading && (abs(nx-starx)<10) && (abs(ny-stary)<10))
    text_layer_set_text(n_layer, "");
  else
#endif
    text_layer_set_text(n_layer, "N");
  layer_set_frame((Layer *) n_layer, GRect(nx - 14, ny - 18, 28, 28));
  int32_t ex = center.x + 63 * sin_lookup(orientation + TRIG_MAX_ANGLE/4)/TRIG_MAX_RATIO;
  int32_t ey = center.y - 63 * cos_lookup(orientation + TRIG_MAX_ANGLE/4)/TRIG_MAX_RATIO;
#ifndef PBL_COLOR
  if (orientToHeading && (abs(ex-starx)<10) && (abs(ey-stary)<10))
    text_layer_set_text(e_layer, "");
  else
#endif
    text_layer_set_text(e_layer, "E");
  layer_set_frame((Layer *) e_layer, GRect(ex - 14, ey - 18, 28, 28));
  int32_t sx = center.x + 63 * sin_lookup(orientation + TRIG_MAX_ANGLE/2)/TRIG_MAX_RATIO;
  int32_t sy = center.y - 63 * cos_lookup(orientation + TRIG_MAX_ANGLE/2)/TRIG_MAX_RATIO;
#ifndef PBL_COLOR
  if (orientToHeading && (abs(sx-starx)<10) &&  (abs(sy-stary)<10))
    text_layer_set_text(s_layer, "");
  else
#endif
    text_layer_set_text(s_layer, "S");
  layer_set_frame((Layer *) s_layer, GRect(sx - 14, sy - 18, 28, 28));
  int32_t wx = center.x + 63 * sin_lookup(orientation + TRIG_MAX_ANGLE*3/4)/TRIG_MAX_RATIO;
  int32_t wy = center.y - 63 * cos_lookup(orientation + TRIG_MAX_ANGLE*3/4)/TRIG_MAX_RATIO;
#ifndef PBL_COLOR
  if (orientToHeading && (abs(wx-starx)<10) &&  (abs(wy-stary)<10))
    text_layer_set_text(w_layer, "");
  else
#endif
    text_layer_set_text(w_layer, "W");
  layer_set_frame((Layer *) w_layer, GRect(wx - 14, wy - 18, 28, 28));
  layer_mark_dirty(head_layer);
}
  
void in_received_handler(DictionaryIterator *iter, void *context) {
  // incoming message received
  static char unit_text[9];
  static char distance_text[9];
  static char bearing_text[9];
  static char heading_text[9];
  static char speed_text[9];
  static char accuracy_text[12];

  Tuple *bearing_tuple = dict_find(iter, BEARING_KEY);
  if (bearing_tuple) {
    bearing = bearing_tuple->value->int16;
    snprintf(bearing_text, sizeof(bearing_text), "%d", bearing);
    text_layer_set_text(bearing_layer, bearing_text);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Updated bearing to %d", bearing);
    layer_mark_dirty(head_layer);
  }

  Tuple *unit_tuple = dict_find(iter, UNIT_KEY);
  if (unit_tuple) {
    strncpy(unit_text, unit_tuple->value->cstring, sizeof unit_text);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Unit: %s", unit_text);
    text_layer_set_text(unit_layer, unit_text);
  }

  Tuple *distance_tuple = dict_find(iter, DISTANCE_KEY);
  if (distance_tuple) {
    if ((distance_text[0] != '!') && (distance_tuple->value->cstring[0] == '!')) vibes_double_pulse();
    strcpy(distance_text, distance_tuple->value->cstring);
    if (distance_text[0] == '!') {
      text_layer_set_text(distance_layer, &distance_text[1]);
      #ifdef PBL_COLOR
      text_layer_set_text_color(distance_layer, GColorBlue);
      text_layer_set_text_color(unit_layer, GColorBlue);
      #endif
    } else {
      text_layer_set_text(distance_layer, distance_text);
      #ifdef PBL_COLOR
      text_layer_set_text_color(distance_layer, GColorBlack);
      text_layer_set_text_color(unit_layer, GColorBlack);
      #endif
    }
  }
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Distance updated: %s %s", distance_text, unit_text);
  
  Tuple *nextcall_tuple = dict_find(iter, NEXTCALL_KEY);
  if (nextcall_tuple) {
    nextcall = nextcall_tuple->value->int32;
    app_timer_reschedule(locationtimer, 1000 * (nextcall + 10));
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Next call is %d.", nextcall);
  }

  Tuple *speed_tuple = dict_find(iter, SPEED_KEY);
  if (speed_tuple) {
    strncpy(speed_text, speed_tuple->value->cstring, sizeof speed_text);
    text_layer_set_text(speed_layer, speed_text);
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Speed is %s.", speed_text);
  }
  
  Tuple *accuracy_tuple = dict_find(iter, ACCURACY_KEY);
  if (accuracy_tuple) {
    strncpy(accuracy_text, accuracy_tuple->value->cstring, sizeof accuracy_text);
    text_layer_set_text(accuracy_layer, accuracy_text);
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Accuracy is %s.", accuracy_text);
  }
  
  Tuple *heading_tuple = dict_find(iter, HEADING_KEY);
  if (heading_tuple) {
    heading = heading_tuple->value->int16;
    if (heading >= 0) {
      snprintf(heading_text, sizeof(heading_text), "%d", heading);
      text_layer_set_text(heading_layer, heading_text);
      if (orientToHeading) {
        text_layer_set_text(star_layer, "^");
        orientation = -TRIG_MAX_ANGLE * heading / 360;
      } else {
        text_layer_set_text(star_layer, "*");
      }
      draw_compass_face();
    } else /* heading <0 (not valid) */ {
      text_layer_set_text(heading_layer, "");
      if (orientToHeading)
        text_layer_set_text(star_layer, "?");
      else
        text_layer_set_text(star_layer, "");
    }
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Updated heading to %d", heading);
  }
  
  Tuple *message_tuple = dict_find(iter, MESSAGE_KEY);
  if (message_tuple && (strlen(message_tuple->value->cstring) > 0)) {    
    if (!hint_layer) {
      Layer *window_layer = window_get_root_layer(window);
      hint_layer = text_layer_create(hint_layer_size);
      text_layer_set_font(hint_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
      text_layer_set_text_alignment(hint_layer, GTextAlignmentCenter);
      layer_add_child(window_layer, text_layer_get_layer(hint_layer));
    }
    strncpy(hint_text, message_tuple->value->cstring, sizeof hint_text);
    text_layer_set_text(hint_layer, hint_text);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Got message: %s", hint_text);
  }

  for (int menuItemNumber=2; menuItemNumber <= MENU_LENGTH; menuItemNumber++) {
    Tuple *location_tuple = dict_find(iter, LOCATION_KEY+menuItemNumber-2);
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Finding location %d.", menuItemNumber);
    if ((location_tuple) && (strlen(location_tuple->value->cstring) > 0)) {
      strncpy(locations[menuItemNumber], location_tuple->value->cstring, sizeof locations[0]);
      last_menu_item = menuItemNumber;
      APP_LOG(APP_LOG_LEVEL_DEBUG,"Got location %d = %s.", menuItemNumber, locations[menuItemNumber]);
    }
  }
#ifdef PBL_SDK_3
  if (pinID_needs_sending)
    pin_action_handler(launch_get_args());
#endif
}

void compass_direction_handler(CompassHeadingData direction_data){
  static char direction_buf[]="~~~";
  switch (direction_data.compass_status) {
    case CompassStatusDataInvalid:
      snprintf(direction_buf, sizeof(direction_buf), "%s", "!!!");
      if (!orientToHeading) return;
    case CompassStatusCalibrating:
      snprintf(direction_buf, sizeof(direction_buf), "%s", "???");
      break;
    case CompassStatusCalibrated:
      snprintf(direction_buf, sizeof(direction_buf), "%d", (int) ((360-TRIGANGLE_TO_DEG(direction_data.true_heading)) % 360));
  }
  text_layer_set_text(direction_layer, direction_buf);
  if (!orientToHeading) orientation = direction_data.true_heading;
  draw_compass_face();
}

static void head_layer_update_callback(Layer *layer, GContext *ctx) {
  gpath_rotate_to(head_path, (TRIG_MAX_ANGLE / 360) * (bearing + TRIGANGLE_TO_DEG(orientation)));
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, 77);
#ifdef PBL_COLOR
  if (heading >= 0) {
    int angle = bearing - heading;
    if (angle < 0) angle += 360;
    if (angle < 16)
      graphics_context_set_fill_color(ctx, GColorYellow);
    else if (angle < 46)
      graphics_context_set_fill_color(ctx, GColorSpringBud);
    else if (angle < 76)
      graphics_context_set_fill_color(ctx, GColorBrightGreen);
    else if (angle < 106)
      graphics_context_set_fill_color(ctx, GColorGreen);
    else if (angle < 136)
      graphics_context_set_fill_color(ctx, GColorScreaminGreen);
    else if (angle < 166)
      graphics_context_set_fill_color(ctx, GColorMintGreen);
    else if (angle < 196)
      graphics_context_set_fill_color(ctx, GColorBabyBlueEyes);
    else if (angle < 226)
      graphics_context_set_fill_color(ctx, GColorMelon);
    else if (angle < 256)
      graphics_context_set_fill_color(ctx, GColorSunsetOrange);
    else if (angle < 286)
      graphics_context_set_fill_color(ctx, GColorRed);
    else if (angle < 316)
      graphics_context_set_fill_color(ctx, GColorOrange);
    else if (angle < 346)
      graphics_context_set_fill_color(ctx, GColorChromeYellow);
    else
      graphics_context_set_fill_color(ctx, GColorYellow);
  } else /* heading undefined */
#endif
  graphics_context_set_fill_color(ctx, GColorWhite);
  gpath_draw_filled(ctx, head_path);
  graphics_fill_circle(ctx, center, 49);
  graphics_context_set_fill_color(ctx, GColorBlack);
}

static void toggleorienttoheading(ClickRecognizerRef recognizer, void *context) {
  if (hint_layer) {
    text_layer_destroy(hint_layer);
    hint_layer = NULL;
  } else {
    orientToHeading = !orientToHeading;
    if (orientToHeading) {
      compass_service_unsubscribe();
      text_layer_set_text(direction_layer, "---");
      if (heading >= 0) { 
        text_layer_set_text(star_layer, "^");
        orientation = -TRIG_MAX_ANGLE * heading / 360;
        draw_compass_face();
      } else 
        text_layer_set_text(star_layer, "?");
    } else {
      compass_service_subscribe(&compass_direction_handler);
      text_layer_set_text(direction_layer, "!!!");
      if (heading >= 0) {
        text_layer_set_text(star_layer, "*");
      } else 
        text_layer_set_text(star_layer, "");
    }
  }
}

void in_dropped_handler(AppMessageResult reason, void *context) {
   // incoming message dropped
  APP_LOG(APP_LOG_LEVEL_WARNING, "Could not handle message from watch: %d", reason);
}
 
static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, hint_handler);
//  window_single_click_subscribe(BUTTON_ID_UP, toggleorienttoheading);  Do nothing an just let it turn the light on.
  window_single_click_subscribe(BUTTON_ID_DOWN, get_info_handler);
  window_single_click_subscribe(BUTTON_ID_BACK, go_back_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 0, reset_handler, NULL);
  window_long_click_subscribe(BUTTON_ID_UP, 0, toggleorienttoheading, NULL);
  window_long_click_subscribe(BUTTON_ID_DOWN, 0, pin_set_handler, NULL);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
  center = grect_center_point(&bounds);

  head_layer = layer_create(bounds);
  layer_set_update_proc(head_layer, head_layer_update_callback);
  head_path = gpath_create(&HEAD_PATH_POINTS);
  gpath_move_to(head_path, grect_center_point(&bounds));  
  layer_add_child(window_layer, head_layer);

  n_layer = text_layer_create(GRect(center.x-14, center.y-80, 28, 28));
  text_layer_set_background_color(n_layer, GColorClear);
  #ifdef PBL_COLOR 
    text_layer_set_text_color(n_layer, GColorBabyBlueEyes);
  #else
    text_layer_set_text_color(n_layer, GColorWhite);
  #endif  
  text_layer_set_font(n_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(n_layer, GTextAlignmentCenter);
  text_layer_set_text(n_layer, "N");
  layer_add_child(head_layer, (Layer *) n_layer);

  e_layer = text_layer_create(GRect(center.x+48, center.y-19, 28, 28));
  text_layer_set_background_color(e_layer, GColorClear);
  #ifdef PBL_COLOR 
    text_layer_set_text_color(e_layer, GColorMintGreen);
  #else
    text_layer_set_text_color(e_layer, GColorWhite);
  #endif  
  text_layer_set_font(e_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(e_layer, GTextAlignmentCenter);
  text_layer_set_text(e_layer, "E");
  layer_add_child(head_layer, (Layer *) e_layer);

  s_layer = text_layer_create(GRect(center.x-14, center.y+48, 28, 28));
  text_layer_set_background_color(s_layer, GColorClear);
  #ifdef PBL_COLOR 
    text_layer_set_text_color(s_layer, GColorPastelYellow);
  #else
    text_layer_set_text_color(s_layer, GColorWhite);
  #endif  
  text_layer_set_font(s_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_layer, GTextAlignmentCenter);
  text_layer_set_text(s_layer, "S");
  layer_add_child(head_layer, (Layer *) s_layer);

  w_layer = text_layer_create(GRect(center.x-76, center.y-18, 28, 28));
  text_layer_set_background_color(w_layer, GColorClear);
  #ifdef PBL_COLOR 
    text_layer_set_text_color(w_layer, GColorMelon);
  #else
    text_layer_set_text_color(w_layer, GColorWhite);
  #endif  
  text_layer_set_font(w_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(w_layer, GTextAlignmentCenter);
  text_layer_set_text(w_layer, "W");
  layer_add_child(head_layer, text_layer_get_layer(w_layer));

  star_layer = text_layer_create(GRect(center.x-14, center.y-80, 28, 28));
  text_layer_set_background_color(star_layer, GColorClear);
  #ifdef PBL_COLOR 
    text_layer_set_text_color(star_layer, GColorFashionMagenta);
  #else
    text_layer_set_text_color(star_layer, GColorWhite);
  #endif  
  text_layer_set_font(star_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(star_layer, GTextAlignmentCenter);
  layer_add_child(head_layer, (Layer *) star_layer);

  distance_layer = text_layer_create(GRect(0, 60, 144, 42));
  text_layer_set_background_color(distance_layer, GColorClear);
  text_layer_set_font(distance_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(distance_layer, GTextAlignmentCenter);
  text_layer_set_text(distance_layer, "...");
  layer_add_child(window_layer, text_layer_get_layer(distance_layer));
  
  unit_layer = text_layer_create(GRect(50, 96, 44, 30));
  text_layer_set_background_color(unit_layer, GColorClear);
  text_layer_set_font(unit_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(unit_layer, GTextAlignmentCenter);
  text_layer_set_text(unit_layer, "...");
  layer_add_child(window_layer, text_layer_get_layer(unit_layer));

  direction_layer = text_layer_create(GRect(90, 142, 53, 24));
  text_layer_set_background_color(direction_layer, GColorClear);
  text_layer_set_font(direction_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(direction_layer, GTextAlignmentRight);
  text_layer_set_text(direction_layer, "!!!");
  layer_add_child(window_layer, text_layer_get_layer(direction_layer));

  bearing_layer = text_layer_create(GRect(0, 142, 53, 24));
  text_layer_set_background_color(bearing_layer, GColorClear);
  text_layer_set_font(bearing_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(bearing_layer, GTextAlignmentLeft);
  text_layer_set_text(direction_layer, "...");
  layer_add_child(window_layer, text_layer_get_layer(bearing_layer));

  speed_layer = text_layer_create(GRect(40, 38, 64, 30));
  text_layer_set_background_color(speed_layer, GColorClear);
  text_layer_set_font(speed_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(speed_layer, GTextAlignmentCenter);
  text_layer_set_text(speed_layer, "...");
  layer_add_child(window_layer, text_layer_get_layer(speed_layer));

  accuracy_layer = text_layer_create(GRect(90, -8, 53, 24));
  text_layer_set_background_color(accuracy_layer, GColorClear);
  text_layer_set_font(accuracy_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(accuracy_layer, GTextAlignmentRight);
  text_layer_set_text(accuracy_layer, "...");
  layer_add_child(window_layer, text_layer_get_layer(accuracy_layer));

  heading_layer = text_layer_create(GRect(0, -8, 53, 24));
  text_layer_set_background_color(heading_layer, GColorClear);
  text_layer_set_font(heading_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(heading_layer, GTextAlignmentLeft);
  text_layer_set_text(heading_layer, "...");
  layer_add_child(window_layer, text_layer_get_layer(heading_layer));
}

static void menu_window_load(Window *window) {
  strcpy(locations[0],"No change");
  strcpy(locations[1],"This Location");
  
  dialog_layer = text_layer_create(GRect(0, 0, 144, 36));
  text_layer_set_background_color(dialog_layer, GColorClear);
  text_layer_set_font(dialog_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text(dialog_layer, "Set target to:");
  layer_add_child(window_get_root_layer(menu_window), text_layer_get_layer(dialog_layer));
  
  menu_layer = menu_layer_create(GRect(0, 36, 144, 132));
	menu_layer_set_click_config_onto_window(menu_layer, menu_window);
  menu_layer_set_callbacks(menu_layer, NULL, (MenuLayerCallbacks) {
	  .get_num_rows = num_rows_callback,
	  .draw_row = draw_row_callback,
	  .get_cell_height = cell_height_callback,
	  .select_click = select_click_callback
  });
	layer_add_child(window_get_root_layer(menu_window), menu_layer_get_layer(menu_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(heading_layer);
  text_layer_destroy(speed_layer);
  text_layer_destroy(accuracy_layer);
  text_layer_destroy(bearing_layer);
  text_layer_destroy(direction_layer);
  text_layer_destroy(unit_layer);
  text_layer_destroy(distance_layer);
  text_layer_destroy(w_layer);
  text_layer_destroy(s_layer);
  text_layer_destroy(e_layer);
  text_layer_destroy(n_layer);
  text_layer_destroy(star_layer);
}

static void menu_window_unload(Window *window) {
  if (hint_layer) text_layer_destroy(hint_layer);
	menu_layer_destroy(menu_layer);
  text_layer_destroy(dialog_layer);
}

static void init(void) {
  // update screen only when heading changes at least 6 degrees
  compass_service_set_heading_filter(TRIG_MAX_ANGLE*6/360);
  compass_service_subscribe(&compass_direction_handler);
  
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_register_outbox_sent(out_sent_handler);
  app_message_register_outbox_failed(out_failed_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  
  hint_layer_size = GRect(6, 18, 132, 132);
  
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
#ifndef PBL_SDK_3
  window_set_fullscreen(window,true);
#endif

  window_stack_push(window, animated);

  menu_window = window_create();
  window_set_window_handlers(menu_window, (WindowHandlers) {
    .load = menu_window_load,
    .unload = menu_window_unload,
  });
#ifndef PBL_SDK_3
  window_set_fullscreen(menu_window,true);
#endif

#ifdef PBL_SDK_3
  pinID_needs_sending = (launch_reason() == APP_LAUNCH_TIMELINE_ACTION);
#endif
  
  locationtimer = app_timer_register(15000, (AppTimerCallback) tick_handler, NULL);
}

static void deinit(void) {
  app_timer_cancel(locationtimer);
  compass_service_unsubscribe();
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  if (iter == NULL) {

    APP_LOG(APP_LOG_LEVEL_WARNING, "Can not send quit command to phone!");
    return;
  }
  strcpy(command,"quit");
  dict_write_cstring(iter, CMD_KEY, command);
  const uint32_t final_size = dict_write_end(iter);
  app_message_outbox_send();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Sent command '%s' to phone! (%d bytes)", command, (int) final_size);
  window_destroy(menu_window);
  window_destroy(window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
