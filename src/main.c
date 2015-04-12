#include <pebble.h>

// Parameters to be passed or evaluated
#define stock_new    0
#define stock_handle 1
#define stock_price  2
#define stock_state  3
#define stock_line_1 4
#define stock_line_2 5
#define stock_line_3 6
#define refresh      7


struct stock {
  char handle[5];      // Stock trading tag
  char price[20];      // Last price
  int state;           // VDown / Down / Neutral / Up / VUp / Hot
  char line_1[30];     // First line on details page
  char line_2[20];     // Second line
  char line_3[20];     // Third line
  GBitmap  *graph;     // The icon for the main menu
  GBitmap  *chart;     // Chart for details page
};

int current_focus = 0;
int stock_count = 0;
struct stock stocks [10];

//// MAIN SCREEN /////
static Window *window_main;      // Main window handle
static MenuLayer *main_menu;     // Menu for main window

//// FOCUS WINDOW ////
static Window *window_detail;    // Detail window handle
static TextLayer *stock_name;    // Stock name text layer
static Layer *chart;             // Chart for the stock
static TextLayer *line_1;        // Top Row
static TextLayer *line_2;        // Middle Row
static TextLayer *line_3;        // Bottom Row

//// Prototypes ////
static void detail_window_unload(Window *window);
static void detail_window_load(Window *window);

// Generate stocks
struct stock new_stock(char* handle, char* price, int state, char* line_1, char* line_2, char* line_3){
  struct stock temp;
  strcpy( temp.handle, handle );
  strcpy( temp.price, price );
  strcpy( temp.line_1, line_1 );
  strcpy( temp.line_2, line_2 );
  strcpy( temp.line_3, line_3 );
  temp.state = state;
  switch(state){
    case 0:
      temp.graph = gbitmap_create_with_resource(RESOURCE_ID_VDOWN);
      break;
    case 1:
      temp.graph = gbitmap_create_with_resource(RESOURCE_ID_DOWN);
      break;
    case 2:
      temp.graph = gbitmap_create_with_resource(RESOURCE_ID_NEUTRAL);
      break;
    case 3:
      temp.graph = gbitmap_create_with_resource(RESOURCE_ID_UP);
      break;
    case 4:
      temp.graph = gbitmap_create_with_resource(RESOURCE_ID_VUP);
      break;
    case 5:
      temp.graph = gbitmap_create_with_resource(RESOURCE_ID_FIRE);
      break;
  }
  return temp;
}

// Returns the index of a stock when given handle. Returns -1 if not found
static int get_index_by_handle(char* handle){
  for(int i=0; i<stock_count; i++){
    if(strncmp(handle, stocks[i].handle, 4) == 0)
      return i;
  }
  return -1;
}

// Generates dummy stocks
static void generate(){
  stocks[stock_count] = new_stock("GOOG", "$540.01 -0.14%", 1, "Price:$540.01 -0.14%", "H/L: 487.56/599.65", "V/dV: 1.4M/1.9M");
  stock_count++;
  stocks[stock_count] = new_stock("AAPL", "$127.10 +0.43%", 4, "Price: $127.10 +0.43%", "H/L: 133.60/73.05", "V/dV: 40M/50M");
  stock_count++;
  stocks[stock_count] = new_stock("FB", "$82.04 -0.16%", 0, "Price: $82.04 -0.16%", "H/L: 86.07/54.66", "V/dV: 12.5M/25.5M");
  stock_count++;
  stocks[stock_count] = new_stock("MSFT", "$41.72 +0.58%", 3, "Price: $41.72 +0.58%", "H/L: 50.05/38.51", "V/dV: 28M/39M");
  stock_count++;
}

// Only one section
static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return 1;
}

// Return the number of rows in the section
static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return stock_count;
}

// Confirm that the cells will be standard height
static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}

// Handle drawing the header text
static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
  menu_cell_basic_header_draw(ctx, cell_layer, "Your Stocks");
}

// Handle drawing for each layer
static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  menu_cell_basic_draw(ctx, cell_layer,
                       stocks[cell_index->row].handle,
                       stocks[cell_index->row].price,
                       stocks[cell_index->row].graph);
}

// Click handler
static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  // Set current focus index
  current_focus = cell_index->row;
  // Create detail window
  window_detail = window_create();
  // Set layer handling
  window_set_window_handlers(window_detail, (WindowHandlers) {
    .load = detail_window_load,
    .unload = detail_window_unload,
  });
  window_set_fullscreen(window_detail, true);
  // Display created window
  window_stack_push(window_detail, true);
}

// Layer constructor for main window
static void main_window_load(Window *window) {
  // Now we prepare to initialize the menu layer
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  // Create the menu layer
  main_menu = menu_layer_create(bounds);
  menu_layer_set_callbacks(main_menu, NULL, (MenuLayerCallbacks){
    .get_num_sections = menu_get_num_sections_callback,
    .get_num_rows = menu_get_num_rows_callback,
    .get_header_height = menu_get_header_height_callback,
    .draw_header = menu_draw_header_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
  });

  // Bind the menu layer's click config provider to the window for interactivity
  menu_layer_set_click_config_onto_window(main_menu, window);

  layer_add_child(window_layer, menu_layer_get_layer(main_menu));
}

// Layer destructor for main window
static void main_window_unload(Window *window) {
  // Destroy the menu layer
  menu_layer_destroy(main_menu);
}

// Overlay the chart over the handle
static void chart_proc(struct Layer *layer, GContext *ctx){
  GBitmap* overlay = gbitmap_create_with_resource(RESOURCE_ID_GRAPH);
  GBitmap* buffer = graphics_capture_frame_buffer(ctx);
  uint16_t stride = buffer->row_size_bytes;
  for(int y=0; y<84; y++){
    for(int x=0; x<144; x+=8){
      char val1 = ((uint8_t*)buffer->addr)[(y * stride) + x / 8];
      char val2 = ((uint8_t*)overlay->addr)[(y * stride) + x / 8];
      ((uint8_t*)buffer->addr)[(y * stride) + x / 8] = ~(val1 ^ val2);
    }
  }
  graphics_release_frame_buffer(ctx, buffer);
}

// Layer constructor for detail window
static void detail_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);

  // Stock name configuration
  stock_name = text_layer_create(GRect(0, 12, 144, 60));
  text_layer_set_font(stock_name, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text(stock_name, stocks[current_focus].handle);
  text_layer_set_text_alignment(stock_name, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(stock_name));

  // Create generic layer
  chart = layer_create(GRect(0, 0, 144, 84));
  // Add in bitmap
  layer_set_update_proc(chart, chart_proc);
  layer_add_child(window_layer, chart);

  // Lines configuration
  // Line 1
  line_1 = text_layer_create(GRect(2, 84, 140, 28));
  text_layer_set_background_color(line_1, GColorBlack);
  text_layer_set_text_color(line_1, GColorWhite);
  text_layer_set_text_alignment(line_1, GTextAlignmentCenter);
  text_layer_set_font(line_1, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text(line_1, stocks[current_focus].line_1);
  layer_add_child(window_layer, text_layer_get_layer(line_1));
  // Line 2
  line_2 = text_layer_create(GRect(2, 112, 140, 28));
  text_layer_set_text_alignment(line_2, GTextAlignmentCenter);
  text_layer_set_font(line_2, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text(line_2, stocks[current_focus].line_2);
  layer_add_child(window_layer, text_layer_get_layer(line_2));
  // Line 3
  line_3 = text_layer_create(GRect(2, 140, 140, 28));
  text_layer_set_background_color(line_3, GColorBlack);
  text_layer_set_text_color(line_3, GColorWhite);
  text_layer_set_text_alignment(line_3, GTextAlignmentCenter);
  text_layer_set_font(line_3, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text(line_3, stocks[current_focus].line_3);
  layer_add_child(window_layer, text_layer_get_layer(line_3));
}

// Layer destructor for detail window
static void detail_window_unload(Window *window) {
  text_layer_destroy(stock_name);
  layer_destroy(chart);
  text_layer_destroy(line_1);
  text_layer_destroy(line_2);
  text_layer_destroy(line_3);
}

// Handle the taps from the accelerometer
static void tap_handler(AccelAxisType axis, int32_t direction) {
  vibes_short_pulse();
}

// Communications
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Get the first pair
  Tuple *t = dict_read_first(iterator);

  // Temp value for recieved stock
  struct stock temp;

  // Process all pairs present
  while(t != NULL) {
    // Determine the pair we have
    switch (t->key) {
      case stock_new:
        APP_LOG(APP_LOG_LEVEL_INFO, "stock_new received with value %d", (int)t->value->int32);
        break;
      case stock_handle:
        strcpy( temp.handle, t->value);
        break;
      case stock_price:
        strcpy( temp.price, t->value);
        break;
      case stock_state:
        temp.state = (int)t->value->int32;
        break;
      case stock_line_1:
        strcpy( temp.line_1, t->value );
        break;
      case stock_line_2:
        strcpy( temp.line_2, t->value );
        break;
      case stock_line_3:
        strcpy( temp.line_3, t->value );
        break;
    }

    // Get next pair, if any
    t = dict_read_next(iterator);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void sync_changed_handler(const uint32_t key, const Tuple *new_tuple, const Tuple *old_tuple, void *context) {
  // Update UI here

}

static void sync_error_handler(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  // An error occured!

}

// Initialization
static void handle_init(void) {
  // Create main window
  window_main = window_create();
  // Set layer handling
  window_set_window_handlers(window_main, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  // Display created window
  window_stack_push(window_main, true);
  // Engage tap handler
  accel_tap_service_subscribe(tap_handler);
  // Register communication callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  // Open messenger
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

}

// Deinitilization
static void handle_deinit(void) {
  // Disengage tap handler
  accel_tap_service_unsubscribe();
  // Destroy main window
  window_destroy(window_main);
}

// Entry point
int main(void) {
  generate();
  handle_init();
  app_event_loop();
  handle_deinit();
}
