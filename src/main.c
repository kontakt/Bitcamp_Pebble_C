#include <pebble.h>
  
struct stock {
  char handle[5];      // Stock trading tag
  char price[10];      // Last price
  int state;           // Down / Neutral / Up / Hot
  GBitmap  *graph;     
};

int stock_count = 0;
struct stock stocks [10];

static Window *window_main;      // Main window handle
static Window *window_detail;    // Detail window handle

static MenuLayer *main_menu;     // Menu for main window
static TextLayer *detail_text;   // Detail text layer

//// Prototypes ////
static void detail_window_unload(Window *window);
static void detail_window_load(Window *window);

// Generate stocks
struct stock generate(char* handle, char* price, int state){
  struct stock temp;
  strcpy( temp.handle, handle );
  strcpy( temp.price, price );
  temp.state = state;
  switch(state){
    case 0:
      temp.graph = gbitmap_create_with_resource(RESOURCE_ID_DOWN);
      break;
    case 1:
      temp.graph = gbitmap_create_with_resource(RESOURCE_ID_NEUTRAL);
      break;
    case 2:
      temp.graph = gbitmap_create_with_resource(RESOURCE_ID_UP);
      break;
    case 3:
      temp.graph = gbitmap_create_with_resource(RESOURCE_ID_HOT);
      break;
  }
  return temp;
}

void new_stock(){
  stocks[stock_count] = generate("GOOG", "$435.01", 0);
  stock_count++;
  stocks[stock_count] = generate("DONT", "$20.33", 2);
  stock_count++;
  stocks[stock_count] = generate("STOP", "$12.35", 1);
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

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  // Create detail window
  window_detail = window_create();
  // Set layer handling
  window_set_window_handlers(window_detail, (WindowHandlers) {
    .load = detail_window_load,
    .unload = detail_window_unload,
  });
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

// Layer constructor for detail window
static void detail_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
  
  detail_text = text_layer_create(GRect(5, 0, bounds.size.w - 5, bounds.size.h));
  text_layer_set_font(detail_text, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text(detail_text, "Example TextLayer!");
  text_layer_set_overflow_mode(detail_text, GTextOverflowModeWordWrap);
  layer_add_child(window_layer, text_layer_get_layer(detail_text));
}

// Layer destructor for detail window
static void detail_window_unload(Window *window) {
  text_layer_destroy(detail_text);
}

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
}

static void handle_deinit(void) {
  // Destroy main window
  window_destroy(window_main);
}

int main(void) {
  new_stock();
  handle_init();
  app_event_loop();
  handle_deinit();
}
