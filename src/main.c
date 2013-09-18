#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"


#define MY_UUID { 0xBD, 0x1F, 0xCB, 0x2A, 0xBB, 0x06, 0x44, 0xC3, 0x8E, 0x9E, 0x55, 0xF2, 0x1F, 0xA6, 0xEB, 0xB8 }
PBL_APP_INFO(MY_UUID,
             "FlightTimer", "Banahogg",
             1, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_STANDARD_APP);

Window window;

#define NUM_TIMERS 2

uint8_t invert = 1;
int16_t gmtOffset = -7;
int16_t timerMinutes[NUM_TIMERS] = {30, 0};
int16_t timerRemainSeconds[NUM_TIMERS] = {-1, -1};

#define NUM_MENU_SECTIONS 1
#define NUM_MAIN_MENU_ITEMS 5
#define FIRST_TIMER_ITEM 1
	
// This is a simple menu layer
SimpleMenuLayer simple_menu_layer;

// A simple menu layer can have multiple sections
SimpleMenuSection menu_sections[NUM_MENU_SECTIONS];

// Each section is composed of a number of menu items
SimpleMenuItem main_menu_items[NUM_MAIN_MENU_ITEMS];

static char invertMenuText[32];
static char gmtMenuText[8];
static char timerMenuText[2][32];

void updateInvertMenuText() {
	strncpy(invertMenuText, invert ? "White on black" : "Black on white", sizeof(invertMenuText));
}

void updateGmtMenuText() {
  snprintf(gmtMenuText, sizeof(gmtMenuText), "%+d", gmtOffset);
}

void updateTimerMenuText() {
	for (int i=0; i<NUM_TIMERS; i++) {
		snprintf(timerMenuText[i], sizeof(timerMenuText[0]), "%d minutes", timerMinutes[i]);
	}
}

Window timerUpdateWindow;

TextLayer timerSelectHeader, timerSelectValue, timerSelectFooter;
char timerValueText[8];
int curTimer;

void updateTimerValueText() {
  snprintf(timerValueText, sizeof(timerValueText), "%d", timerMinutes[curTimer]);
  text_layer_set_text(&timerSelectValue, timerValueText);
}

void timer_select_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
	window_stack_pop(true);
}

void timer_up_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
  timerMinutes[curTimer]++;
  updateTimerValueText();
}

void timer_down_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
  timerMinutes[curTimer]--;
  updateTimerValueText();
}

void timer_click_config_provider(ClickConfig **config, Window *window) {
  config[BUTTON_ID_SELECT]->click.handler = (ClickHandler) timer_select_single_click_handler;
//  config[BUTTON_ID_SELECT]->long_click.handler = (ClickHandler) select_long_click_handler;

  config[BUTTON_ID_UP]->click.handler = (ClickHandler) timer_up_single_click_handler;
  config[BUTTON_ID_UP]->click.repeat_interval_ms = 100;

  config[BUTTON_ID_DOWN]->click.handler = (ClickHandler) timer_down_single_click_handler;
  config[BUTTON_ID_DOWN]->click.repeat_interval_ms = 100;
}

void timer_window_load(Window *me) {
  static char headerText[16];

  snprintf(headerText, sizeof(headerText), "Timer %d", curTimer+1);
  updateTimerValueText();

  text_layer_init(&timerSelectHeader, timerUpdateWindow.layer.frame);
  text_layer_set_text_color(&timerSelectHeader, GColorBlack);
  text_layer_set_background_color(&timerSelectHeader, GColorClear);
  layer_set_frame(&timerSelectHeader.layer, GRect(8, 8, 144-8, 168-68));
  text_layer_set_font(&timerSelectHeader, fonts_get_system_font(FONT_KEY_GOTHAM_30_BLACK));
  text_layer_set_text(&timerSelectHeader, headerText);
  layer_add_child(&timerUpdateWindow.layer, &timerSelectHeader.layer);

  text_layer_init(&timerSelectValue, timerUpdateWindow.layer.frame);
  text_layer_set_text_color(&timerSelectValue, GColorBlack);
  text_layer_set_background_color(&timerSelectValue, GColorClear);
  layer_set_frame(&timerSelectValue.layer, GRect(40, 50, 144-8, 168-68));
  text_layer_set_font(&timerSelectValue, fonts_get_system_font(FONT_KEY_GOTHAM_42_LIGHT));
  text_layer_set_text(&timerSelectValue, timerValueText);
  layer_add_child(&timerUpdateWindow.layer, &timerSelectValue.layer);

  text_layer_init(&timerSelectFooter, timerUpdateWindow.layer.frame);
  text_layer_set_text_color(&timerSelectFooter, GColorBlack);
  text_layer_set_background_color(&timerSelectFooter, GColorClear);
  layer_set_frame(&timerSelectFooter.layer, GRect(8, 110, 144-8, 168-68));
  text_layer_set_font(&timerSelectFooter, fonts_get_system_font(FONT_KEY_GOTHAM_30_BLACK));
  text_layer_set_text(&timerSelectFooter, "minutes");
  layer_add_child(&timerUpdateWindow.layer, &timerSelectFooter.layer);

  window_set_click_config_provider(&timerUpdateWindow, (ClickConfigProvider) timer_click_config_provider);
}

void timer_window_disappear(Window *me) {
  updateTimerMenuText();
  layer_mark_dirty(simple_menu_layer_get_layer(&simple_menu_layer));
}

void timer_select_callback(int index, void *ctx) {
  curTimer = index - FIRST_TIMER_ITEM;
	
  window_init(&timerUpdateWindow, "Update Timer");
  window_stack_push(&timerUpdateWindow, true /* Animated */);	

  window_set_window_handlers(&timerUpdateWindow, (WindowHandlers){
    .disappear = timer_window_disappear,
	.load = timer_window_load,
  });
}

NumberWindow numberWindow;
	
void gmt_callback(NumberWindow *me, void *ctx) {
	gmtOffset = number_window_get_value(me);
	updateGmtMenuText();
	window_stack_pop(true);
}

void gmt_select_callback(int index, void *ctx) {
	number_window_init(&numberWindow, "Select GMT offset", (NumberWindowCallbacks){
		.selected = gmt_callback,
	}, NULL);
	number_window_set_label(&numberWindow, "Select GMT offset");
	number_window_set_min(&numberWindow, -11);
	number_window_set_max(&numberWindow, 12);
	number_window_set_value(&numberWindow, gmtOffset);
	window_stack_push((Window *)&numberWindow, true);
}

void invert_select_callback(int index, void *ctx) {
  invert = !invert;
  updateInvertMenuText();
  layer_mark_dirty(simple_menu_layer_get_layer(&simple_menu_layer));
}

void notify() {
  static const uint32_t const segments[] = { 100, 100, 100 };
  VibePattern pat = {
    .durations = segments,
    .num_segments = ARRAY_LENGTH(segments),
  };
  vibes_enqueue_custom_pattern(pat);
	
  light_enable_interaction();
}
void resetTimer(int i) {
  timerRemainSeconds[i] = 60*timerMinutes[i];
}

Window clockWindow;
TextLayer localLayer, zuluLayer, t1Layer, t2Layer; 

static void formatTime(char *text, size_t len, const char *name, int16_t time) {
  char sign = ' ';
  if (time < 0) { time = -time; sign = '-'; }
	
  snprintf(text, len, "%c%3d:%.2d %s", sign, time/60, time%60, name);	
}

void handle_second_tick(AppContextRef ctx, PebbleTickEvent *t) {

  if (window_stack_get_top_window() != &clockWindow) return;
	
  // Need to be static because they're used by the system later.
  static char local_text[] = "00:00:00L";
  static char zulu_text[] = "00:00:00Z";
  static char t1_text[] = "s000:00 T1";
  static char t2_text[] = "s000:00 T2";

  string_format_time(local_text, sizeof(local_text), "%TL", t->tick_time);
  text_layer_set_text(&localLayer, local_text);

  PblTm tm = *(t->tick_time);
  tm.tm_hour -= gmtOffset;
  if (tm.tm_hour > 24) tm.tm_hour -= 24;
  else if (tm.tm_hour < 0) tm.tm_hour += 24;
  string_format_time(zulu_text, sizeof(zulu_text), "%TZ", &tm);
  text_layer_set_text(&zuluLayer, zulu_text);

  formatTime(t1_text, sizeof(t1_text), "T1", timerRemainSeconds[0]);
  text_layer_set_text(&t1Layer, t1_text);
  formatTime(t2_text, sizeof(t2_text), "T2", timerRemainSeconds[1]);
  text_layer_set_text(&t2Layer, t2_text);
	
  for (int i=0; i<NUM_TIMERS; i++) {
    if (timerMinutes[i] > 0) {
      timerRemainSeconds[i]--;
		if (timerRemainSeconds[i] < 0) {
			notify();
		}
	}
  }
}

InverterLayer inverter_layer;

void clock_up_long_click_handler(ClickRecognizerRef recognizer, Window *window) {
  resetTimer(0);
}

void clock_down_long_click_handler(ClickRecognizerRef recognizer, Window *window) {
  resetTimer(1);
}

void clock_click_config_provider(ClickConfig **config, Window *window) {
  config[BUTTON_ID_UP]->long_click.handler = (ClickHandler) clock_up_long_click_handler;
  config[BUTTON_ID_DOWN]->long_click.handler = (ClickHandler) clock_down_long_click_handler;
}

void clock_select_callback(int index, void *ctx) {
  window_init(&clockWindow, "Timer clock");
  window_stack_push(&clockWindow, true /* Animated */);	

  for (int i=0; i<NUM_TIMERS; i++) {
	 resetTimer(i);
  }

  text_layer_init(&localLayer, GRect(8, 8, 144-8, 168-68));
  text_layer_set_text_color(&localLayer, GColorBlack);
  text_layer_set_background_color(&localLayer, GColorClear);
  text_layer_set_font(&localLayer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
  layer_add_child(&clockWindow.layer, &localLayer.layer);

  text_layer_init(&zuluLayer, GRect(8, 32, 144-8, 168-68));
  text_layer_set_text_color(&zuluLayer, GColorBlack);
  text_layer_set_background_color(&zuluLayer, GColorClear);
  text_layer_set_font(&zuluLayer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
  layer_add_child(&clockWindow.layer, &zuluLayer.layer);

  text_layer_init(&t1Layer, GRect(8, 56, 144-8, 168-68));
  text_layer_set_text_color(&t1Layer, GColorBlack);
  text_layer_set_background_color(&t1Layer, GColorClear);
  text_layer_set_font(&t1Layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
  layer_add_child(&clockWindow.layer, &t1Layer.layer);

  text_layer_init(&t2Layer, GRect(8, 80, 144-8, 168-68));
  text_layer_set_text_color(&t2Layer, GColorBlack);
  text_layer_set_background_color(&t2Layer, GColorClear);
  text_layer_set_font(&t2Layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
  layer_add_child(&clockWindow.layer, &t2Layer.layer);
	
  if (invert) {
    inverter_layer_init(&inverter_layer, GRect(0, 0, 144, 168));
    layer_add_child(&clockWindow.layer, &inverter_layer.layer);
  }

  window_set_click_config_provider(&clockWindow, (ClickConfigProvider) clock_click_config_provider);
}

// This initializes the menu upon window load
void window_load(Window *me) {
  // This is an example of how you'd set a simple menu item
  main_menu_items[0] = (SimpleMenuItem){
    // You should give each menu item a title and callback
    .title = "Go",
    .callback = clock_select_callback,
  };
  // The menu items appear in the order saved in the menu items array
  main_menu_items[FIRST_TIMER_ITEM+0] = (SimpleMenuItem){
    .title = "Set Timer 1",
    .subtitle = timerMenuText[0],
    .callback = timer_select_callback,
  };
  main_menu_items[FIRST_TIMER_ITEM+1] = (SimpleMenuItem){
    .title = "Set Timer 2",
    .subtitle = timerMenuText[1],
    .callback = timer_select_callback,
  };
  main_menu_items[FIRST_TIMER_ITEM+2] = (SimpleMenuItem){
    .title = "Set GMT Offset",
    .subtitle = gmtMenuText,
    .callback = gmt_select_callback,
  };
  main_menu_items[FIRST_TIMER_ITEM+3] = (SimpleMenuItem){
    .title = "Color scheme",
    .subtitle = invertMenuText,
    .callback = invert_select_callback,
  };

  // Bind the menu items to the corresponding menu sections
  menu_sections[0] = (SimpleMenuSection){
    .num_items = NUM_MAIN_MENU_ITEMS,
    .items = main_menu_items,
  };
	
  updateTimerMenuText();
  updateGmtMenuText();
  updateInvertMenuText();
  
  // Now we prepare to initialize the simple menu layer
  // We need the bounds to specify the simple menu layer's viewport size
  // In this case, it'll be the same as the window's
  GRect bounds = me->layer.bounds;

  // Initialize the simple menu layer
  simple_menu_layer_init(&simple_menu_layer, bounds, me, menu_sections, NUM_MENU_SECTIONS, NULL);

  // Add it to the window for display
  layer_add_child(&me->layer, simple_menu_layer_get_layer(&simple_menu_layer));
}

void handle_init(AppContextRef ctx) {
  (void)ctx;

  window_init(&window, "Plane Timer");
  window_stack_push(&window, true /* Animated */);

  // Setup the window handlers
  window_set_window_handlers(&window, (WindowHandlers){
    .load = window_load,
 //   .unload = window_unload,
  });
}


void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,

	.tick_info = {
      .tick_handler = &handle_second_tick,
      .tick_units = SECOND_UNIT
    }
  };
  app_event_loop(params, &handlers);
}
