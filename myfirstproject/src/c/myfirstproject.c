#include <pebble.h>
static int s_target_score = 4;
static const int s_menu_scores[] = {4, 7, 10}; 
#define FONT_KEY_MAIN FONT_KEY_BITHAM_42_BOLD
typedef struct {
  int p1_round_score;
  int p2_round_score;
  int p1_game_score;
  int p2_game_score;
} AppState;
static AppState s_state;
static Window *s_main_window;
static TextLayer *s_p1_rs_layer;
static TextLayer *s_p2_rs_layer;
static TextLayer *s_p1_label;
static TextLayer *s_p2_label;
static TextLayer *s_target_label;
static Window *s_menu_window;
static MenuLayer *s_menu_layer;
static Window *s_instructions_window;
static ScrollLayer *s_instructions_scroll_layer;
static TextLayer *s_instructions_text_layer;
static void reset_all_scores() {
  s_state.p1_round_score = 0;
  s_state.p2_round_score = 0;
  s_state.p1_game_score = 0;
  s_state.p2_game_score = 0;
  APP_LOG(APP_LOG_LEVEL_INFO, "ACTION: Reset All Scores.");
}
static void reset_round_scores() {
  s_state.p1_round_score = 0;
  s_state.p2_round_score = 0;
  APP_LOG(APP_LOG_LEVEL_INFO, "ACTION: Reset Round Scores.");
}
static void reset_game_scores() {
  s_state.p1_game_score = 0;
  s_state.p2_game_score = 0;
  APP_LOG(APP_LOG_LEVEL_INFO, "ACTION: Reset Game Scores.");
}
static void update_display() {
  static char s_p1_rs_buffer[8];
  static char s_p2_rs_buffer[8];
  static char s_p1_gs_buffer[8];
  static char s_p2_gs_buffer[8];
  static char s_target_buffer[32];
  snprintf(s_p1_rs_buffer, sizeof(s_p1_rs_buffer), "%d", s_state.p1_round_score);
  snprintf(s_p2_rs_buffer, sizeof(s_p2_rs_buffer), "%d", s_state.p2_round_score);
  snprintf(s_p1_gs_buffer, sizeof(s_p1_gs_buffer), "W: %d", s_state.p1_game_score);
  snprintf(s_p2_gs_buffer, sizeof(s_p2_gs_buffer), "W: %d", s_state.p2_game_score);
  snprintf(s_target_buffer, sizeof(s_target_buffer), "Target: %d", s_target_score);
  text_layer_set_text(s_p1_rs_layer, s_p1_rs_buffer);
  text_layer_set_text(s_p2_rs_layer, s_p2_rs_buffer);
  text_layer_set_text(s_p1_label, s_p1_gs_buffer);
  text_layer_set_text(s_p2_label, s_p2_gs_buffer);
  text_layer_set_text(s_target_label, s_target_buffer);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Display Updated: P1 RS=%d, P2 RS=%d, GS=%d-%d",
          s_state.p1_round_score, s_state.p2_round_score, s_state.p1_game_score, s_state.p2_game_score);
}
static void check_for_game_win() {
  bool game_won = false;
  if (s_state.p1_round_score >= s_target_score) {
    s_state.p1_game_score++;
    game_won = true;
    APP_LOG(APP_LOG_LEVEL_INFO, "Player 1 Wins Game! Total: %d", s_state.p1_game_score);
    vibes_double_pulse();
  } else if (s_state.p2_round_score >= s_target_score) {
    s_state.p2_game_score++;
    game_won = true;
    APP_LOG(APP_LOG_LEVEL_INFO, "Player 2 Wins Game! Total: %d", s_state.p2_game_score);
    vibes_double_pulse();
  }
  if (game_won) {
    s_state.p1_round_score = 0;
    s_state.p2_round_score = 0;
  }
}
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  s_state.p1_round_score++;
  check_for_game_win();
  update_display();
}
static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  s_state.p2_round_score++;
  check_for_game_win();
  update_display();
}
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    reset_all_scores();
    update_display();
}
static void up_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  reset_round_scores();
  update_display();
  vibes_short_pulse();
}
static void down_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  reset_game_scores();
  update_display();
  vibes_short_pulse();
}
static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_long_click_subscribe(BUTTON_ID_UP, 700, up_long_click_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_long_click_subscribe(BUTTON_ID_DOWN, 700, down_long_click_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}
static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  int half_w = bounds.size.w / 2;
  int half_h = bounds.size.h / 2;
  s_p1_rs_layer = text_layer_create(GRect(0, 0, half_w, half_h));
  text_layer_set_background_color(s_p1_rs_layer, GColorClear);
  text_layer_set_text_color(s_p1_rs_layer, GColorRed);
  text_layer_set_font(s_p1_rs_layer, fonts_get_system_font(FONT_KEY_MAIN));
  text_layer_set_text_alignment(s_p1_rs_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_p1_rs_layer));
  s_p1_label = text_layer_create(GRect(half_w, 0, half_w, half_h));
  text_layer_set_background_color(s_p1_label, GColorClear);
  text_layer_set_text_color(s_p1_label, GColorBlack);
  text_layer_set_font(s_p1_label, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_p1_label, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_p1_label));
  s_p2_rs_layer = text_layer_create(GRect(0, half_h, half_w, half_h));
  text_layer_set_background_color(s_p2_rs_layer, GColorClear);
  text_layer_set_text_color(s_p2_rs_layer, GColorBlue);
  text_layer_set_font(s_p2_rs_layer, fonts_get_system_font(FONT_KEY_MAIN));
  text_layer_set_text_alignment(s_p2_rs_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_p2_rs_layer));
  s_p2_label = text_layer_create(GRect(half_w, half_h, half_w, half_h));
  text_layer_set_background_color(s_p2_label, GColorClear);
  text_layer_set_text_color(s_p2_label, GColorBlack);
  text_layer_set_font(s_p2_label, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_p2_label, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_p2_label));
  s_target_label = text_layer_create(GRect(0, bounds.size.h - 20, bounds.size.w, 20)); 
  text_layer_set_font(s_target_label, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_target_label, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_target_label));
  update_display();
}
static void main_window_unload(Window *window) {
  text_layer_destroy(s_p1_rs_layer);
  text_layer_destroy(s_p2_rs_layer);
  text_layer_destroy(s_p1_label);
  text_layer_destroy(s_p2_label);
  text_layer_destroy(s_target_label);
}
static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  return ARRAY_LENGTH(s_menu_scores) + 1;
}
static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  if (cell_index->row < ARRAY_LENGTH(s_menu_scores)) {
    static char s_buffer[32];
    snprintf(s_buffer, sizeof(s_buffer), "Play to %d points", s_menu_scores[cell_index->row]);
    menu_cell_basic_draw(ctx, cell_layer, s_buffer, "Target Score", NULL);
  } else {
    menu_cell_basic_draw(ctx, cell_layer, "How to Play", "Instructions", NULL);
  }
}
static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  if (cell_index->row < ARRAY_LENGTH(s_menu_scores)) {
    s_target_score = s_menu_scores[cell_index->row];
    window_stack_push(s_main_window, true);
    APP_LOG(APP_LOG_LEVEL_INFO, "Target Score set to %d. Starting game.", s_target_score);
  } else {
    window_stack_push(s_instructions_window, true);
  }
}
static void menu_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_rows = menu_get_num_rows_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
  });
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}
static void menu_window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
}
static void instructions_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  s_instructions_scroll_layer = scroll_layer_create(bounds);
  scroll_layer_set_click_config_onto_window(s_instructions_scroll_layer, window);
  layer_add_child(window_layer, scroll_layer_get_layer(s_instructions_scroll_layer));
  GSize content_size = GSize(bounds.size.w, 180); 
  scroll_layer_set_content_size(s_instructions_scroll_layer, content_size);
  s_instructions_text_layer = text_layer_create(GRect(5, 0, bounds.size.w - 10, content_size.h));
  text_layer_set_font(s_instructions_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_instructions_text_layer, GTextAlignmentLeft);
  text_layer_set_text(s_instructions_text_layer, 
      "UP: +1 Player 1\nDOWN: +1 Player 2\nLong UP: Reset Round\nLong DOWN: Reset Games\nSELECT: Reset ALL\nBACK: Go Back");
  scroll_layer_add_child(s_instructions_scroll_layer, text_layer_get_layer(s_instructions_text_layer));
}
static void instructions_window_unload(Window *window) {
  text_layer_destroy(s_instructions_text_layer);
  scroll_layer_destroy(s_instructions_scroll_layer);
}
static void init() {
  memset(&s_state, 0, sizeof(s_state));
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_set_click_config_provider(s_main_window, click_config_provider);
  s_menu_window = window_create();
  window_set_window_handlers(s_menu_window, (WindowHandlers) {
    .load = menu_window_load,
    .unload = menu_window_unload
  });
  window_set_background_color(s_menu_window, GColorWhite);
  s_instructions_window = window_create();
  window_set_window_handlers(s_instructions_window, (WindowHandlers) {
    .load = instructions_window_load,
    .unload = instructions_window_unload
  });
  window_set_background_color(s_instructions_window, GColorWhite);
  window_stack_push(s_menu_window, true);
}
static void deinit() {
  window_destroy(s_main_window);
  window_destroy(s_menu_window);
  window_destroy(s_instructions_window);
}
int main(void) {
  init();
  app_event_loop();
  deinit();
}
