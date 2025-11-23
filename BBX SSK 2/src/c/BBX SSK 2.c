#include <pebble.h>

#ifndef PBL_DEBUG
#undef APP_LOG
#define APP_LOG(...) ((void)0)
#endif

// --- Adaptive Configuration (Fonts) ---
#define FONT_KEY_SCORE_MAIN FONT_KEY_GOTHIC_28_BOLD
#define FONT_KEY_LABEL FONT_KEY_GOTHIC_18_BOLD

// --- Layout Constants ---
#define V_PADDING 2
#define H_PADDING 4

// --- Menu Configuration and State ---
static int s_target_score = 4;
static const int s_menu_scores[] = {4, 7, 10};

#define FIXED_SCORE_OPTIONS (sizeof(s_menu_scores) / sizeof(s_menu_scores[0]))
#define CUSTOM_TARGET_ROW FIXED_SCORE_OPTIONS
#define INSTRUCTIONS_ROW (CUSTOM_TARGET_ROW + 1)
#define TOTAL_MENU_ROWS (INSTRUCTIONS_ROW + 1)

// --- App State ---
typedef struct {
  int p1_round_score;
  int p2_round_score;
  int p1_game_score;
  int p2_game_score;
} AppState;

static AppState s_state;

// --- Windows & Layers ---
static Window *s_main_window;
static Window *s_menu_window;
static Window *s_instructions_window;
static Window *s_custom_target_window;

static MenuLayer *s_menu_layer;
static ScrollLayer *s_instructions_scroll_layer;
static TextLayer *s_instructions_text_layer;

static TextLayer *s_custom_display_layer;

// Scoreboard layers
static TextLayer *s_clock_layer;   // top unobtrusive clock
static Layer *s_decor_layer;
static TextLayer *s_plr_header_layer;
static TextLayer *s_rnd_header_layer;
static TextLayer *s_win_header_layer;
static TextLayer *s_p1_label_layer;
static TextLayer *s_p2_label_layer;
static TextLayer *s_p1_round_layer;
static TextLayer *s_p2_round_layer;
static TextLayer *s_p1_win_layer;
static TextLayer *s_p2_win_layer;
static TextLayer *s_target_layer;
// Store original cell rectangles so we can vertically center text later
static GRect s_plr_header_rect;
static GRect s_rnd_header_rect;
static GRect s_win_header_rect;
static GRect s_p1_label_rect;
static GRect s_p1_round_rect;
static GRect s_p1_win_rect;
static GRect s_p2_label_rect;
static GRect s_p2_round_rect;
static GRect s_p2_win_rect;
static GRect s_target_rect;
static GRect s_clock_rect;

// --- Utility / Helpers ---
static int action_bar_width_always() {
  #ifdef PBL_RECT
    return 30;
  #else
    return 0;
  #endif
}

static TextLayer* create_layer(Layer *parent_layer, GRect rect, GColor text_color, GFont font, GTextAlignment alignment) {
  TextLayer *layer = text_layer_create(rect);
  text_layer_set_background_color(layer, GColorClear);
  text_layer_set_text_color(layer, text_color);
  text_layer_set_font(layer, font);
  // Prevent accidental clipping; prefer ellipsis for headers and center alignment
  text_layer_set_overflow_mode(layer, GTextOverflowModeTrailingEllipsis);
  text_layer_set_text_alignment(layer, alignment);
  if (parent_layer) {
    layer_add_child(parent_layer, text_layer_get_layer(layer));
  }
  return layer;
}

// Choose appropriate fonts based on available bounds and score area height.
static void choose_fonts_for_layout(GRect bounds, int score_h, GFont *out_score_font, GFont *out_label_font, GFont *out_clock_font) {
  // Default choices
  const char *score_key = FONT_KEY_GOTHIC_24;
  const char *label_key = FONT_KEY_GOTHIC_18_BOLD;
  const char *clock_key = FONT_KEY_GOTHIC_14;

  // Wider or taller displays get bigger fonts.
  if (bounds.size.w >= 200 || score_h >= 48) {
    score_key = FONT_KEY_GOTHIC_28_BOLD;
    label_key = FONT_KEY_GOTHIC_18_BOLD;
    clock_key = FONT_KEY_GOTHIC_14;
  } else if (bounds.size.w >= 144 && score_h >= 36) {
    score_key = FONT_KEY_GOTHIC_24;
    label_key = FONT_KEY_GOTHIC_18_BOLD;
    clock_key = FONT_KEY_GOTHIC_14;
  } else if (bounds.size.w < 144) {
    // Small screens (aplite narrow) use slightly smaller label font
    score_key = FONT_KEY_GOTHIC_24;
    label_key = FONT_KEY_GOTHIC_18_BOLD;
    clock_key = FONT_KEY_GOTHIC_14;
  }

  if (out_score_font) *out_score_font = fonts_get_system_font(score_key);
  if (out_label_font) *out_label_font = fonts_get_system_font(label_key);
  if (out_clock_font) *out_clock_font = fonts_get_system_font(clock_key);
}

// Vertically center the given TextLayer inside the provided cell rect.
static void center_text_layer_vertically(TextLayer *tl, GRect cell_rect) {
  if (!tl) return;
  GSize content = text_layer_get_content_size(tl);
  int content_h = content.h;
  if (content_h <= 0) return;
  int y = cell_rect.origin.y + (cell_rect.size.h - content_h) / 2;
  if (y < cell_rect.origin.y) y = cell_rect.origin.y;
  layer_set_frame(text_layer_get_layer(tl), GRect(cell_rect.origin.x, y, cell_rect.size.w, content_h));
}

// --- Score logic ---
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

static void update_display() {
  static char s_p1_round_buf[8];
  static char s_p2_round_buf[8];
  static char s_p1_win_buf[8];
  static char s_p2_win_buf[8];
  static char s_target_buf[32];

  static int last_p1_round = -1;
  static int last_p2_round = -1;
  static int last_p1_game = -1;
  static int last_p2_game = -1;
  static int last_target = -1;

  if (s_state.p1_round_score != last_p1_round) {
    if (s_p1_round_layer) {
      snprintf(s_p1_round_buf, sizeof(s_p1_round_buf), "%d", s_state.p1_round_score);
      text_layer_set_text(s_p1_round_layer, s_p1_round_buf);
    }
    last_p1_round = s_state.p1_round_score;
  }

  if (s_state.p2_round_score != last_p2_round) {
    if (s_p2_round_layer) {
      snprintf(s_p2_round_buf, sizeof(s_p2_round_buf), "%d", s_state.p2_round_score);
      text_layer_set_text(s_p2_round_layer, s_p2_round_buf);
    }
    last_p2_round = s_state.p2_round_score;
  }

  if (s_state.p1_game_score != last_p1_game) {
    if (s_p1_win_layer) {
      snprintf(s_p1_win_buf, sizeof(s_p1_win_buf), "%d", s_state.p1_game_score);
      text_layer_set_text(s_p1_win_layer, s_p1_win_buf);
    }
    last_p1_game = s_state.p1_game_score;
  }

  if (s_state.p2_game_score != last_p2_game) {
    if (s_p2_win_layer) {
      snprintf(s_p2_win_buf, sizeof(s_p2_win_buf), "%d", s_state.p2_game_score);
      text_layer_set_text(s_p2_win_layer, s_p2_win_buf);
    }
    last_p2_game = s_state.p2_game_score;
  }

  if (s_target_score != last_target) {
    if (s_target_layer) {
      snprintf(s_target_buf, sizeof(s_target_buf), "Target: %d", s_target_score);
      text_layer_set_text(s_target_layer, s_target_buf);
    }
    last_target = s_target_score;
  }
}

// --- Clock (always 24-hour) ---
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  static char s_time_buf[16];
  // Force 24-hour format
  strftime(s_time_buf, sizeof(s_time_buf), "%H:%M", tick_time);
  if (s_clock_layer) text_layer_set_text(s_clock_layer, s_time_buf);
}

// --- Decor drawing (lines / target box) ---
static void decor_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  #ifdef PBL_COLOR
    graphics_context_set_stroke_color(ctx, GColorLightGray);
  #else
    graphics_context_set_stroke_color(ctx, GColorDarkGray);
  #endif

  const int ab_width = action_bar_width_always();
  const int left_margin = H_PADDING;
  const int right_margin = ab_width + H_PADDING;
  const int inner_width = bounds.size.w - left_margin - right_margin;
  if (inner_width <= 0) return;

  // Estimate clock height consistent with main layout
  int clock_height = bounds.size.h / 12;
  if (clock_height < 14) clock_height = 14;
  if (clock_height > 28) clock_height = 28;

  int top_after_clock = clock_height + (2 * V_PADDING);
  int avail_h = bounds.size.h - top_after_clock - V_PADDING;
  if (avail_h <= 0) return;

  // Mirror the main window's layout algorithm so decorative lines align with content.
  int score_h = (avail_h * 35) / 100; // each player's area
  if (score_h < 20) score_h = 20;
  int header_h = avail_h - (2 * score_h);
  if (header_h < 18) {
    header_h = 18;
    score_h = (avail_h - header_h) / 2;
    if (score_h < 20) score_h = 20;
  }

  // Columns: draw only vertical separators and horizontal row separators
  int col_width = inner_width / 3;
  // Center the table horizontally (must match main_window_load)
  int table_w = inner_width;
  int table_left = (bounds.size.w - table_w) / 2;
  int x_col_1 = table_left + col_width;
  int x_col_2 = table_left + 2 * col_width;

  // Center the table vertically (must match main_window_load)
  int table_h = header_h + (2 * score_h);
  int top = top_after_clock + ((avail_h - table_h) / 2);
  int bottom = top + table_h;

  // Vertical lines between columns
  graphics_draw_line(ctx, GPoint(x_col_1, top), GPoint(x_col_1, bottom));
  graphics_draw_line(ctx, GPoint(x_col_2, top), GPoint(x_col_2, bottom));

  // Horizontal separators: header bottom and between player rows
  int header_bottom = top + header_h;
  graphics_draw_line(ctx, GPoint(table_left, header_bottom), GPoint(table_left + table_w, header_bottom));

  int row_sep = header_bottom + score_h;
  graphics_draw_line(ctx, GPoint(table_left, row_sep), GPoint(table_left + table_w, row_sep));
}

// --- Click handlers for main window ---
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
static void back_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Full reset when back is pressed from main window
  reset_all_scores();
  update_display();
  // pop this window to return to the previous menu
  window_stack_pop(true);
}
static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_long_click_subscribe(BUTTON_ID_UP, 700, up_long_click_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_long_click_subscribe(BUTTON_ID_DOWN, 700, down_long_click_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_BACK, back_click_handler);
}

// --- Menu callbacks ---
static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  return TOTAL_MENU_ROWS;
}
static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  const uint16_t index = cell_index->row;
  if (index < FIXED_SCORE_OPTIONS) {
    static char s_title_buffer[32];
    static char s_subtitle_buffer[32];
    snprintf(s_title_buffer, sizeof(s_title_buffer), "Target: %d points", s_menu_scores[index]);
    if (s_menu_scores[index] == s_target_score) {
      snprintf(s_subtitle_buffer, sizeof(s_subtitle_buffer), "Current Target");
    } else {
      snprintf(s_subtitle_buffer, sizeof(s_subtitle_buffer), "Set as Target");
    }
    menu_cell_basic_draw(ctx, cell_layer, s_title_buffer, s_subtitle_buffer, NULL);
  } else if (index == CUSTOM_TARGET_ROW) {
    static char s_buffer[32];
    snprintf(s_buffer, sizeof(s_buffer), "Current: %d", s_target_score);
    menu_cell_basic_draw(ctx, cell_layer, "Custom Target", s_buffer, NULL);
  } else {
    menu_cell_basic_draw(ctx, cell_layer, "Instructions", "How to use the app", NULL);
  }
}
static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  const uint16_t index = cell_index->row;
  if (index < FIXED_SCORE_OPTIONS) {
    s_target_score = s_menu_scores[index];
    reset_all_scores();
    window_stack_push(s_main_window, true);
  } else if (index == CUSTOM_TARGET_ROW) {
    window_stack_push(s_custom_target_window, true);
  } else {
    window_stack_push(s_instructions_window, true);
  }
}

// --- Menu window load/unload ---
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
  if (s_menu_layer) {
    menu_layer_destroy(s_menu_layer);
    s_menu_layer = NULL;
  }
}

// --- Instructions window ---
static void instructions_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  const int h_margin = 4;
  const int content_width = bounds.size.w - (2 * h_margin);

  const char *instructions_text =
    "How to Use Scoreboard:\n\n"
    "Score Round Points:\n"
    " - UP button: Player 1 scores +1\n"
    " - DOWN button: Player 2 scores +1\n\n"
    "Reset Actions:\n"
    " - SELECT button: Reset ALL scores (Round & Game Wins)\n"
    " - Long UP: Reset only Round Scores\n"
    " - Long DOWN: Reset only Game Wins\n\n"
    "Target Score:\n"
    "When a player reaches the Target score, they win the Game, and the Round Score resets.\n\n"
    "Target Change:\n"
    "Restart the app to return to the Menu, or use the 'Custom Target' option.";

  s_instructions_text_layer = text_layer_create(GRect(h_margin, 0, content_width, 2000));
  text_layer_set_text(s_instructions_text_layer, instructions_text);
  text_layer_set_font(s_instructions_text_layer, fonts_get_system_font(FONT_KEY_LABEL));
  text_layer_set_text_color(s_instructions_text_layer, GColorBlack);

  GSize content_size = text_layer_get_content_size(s_instructions_text_layer);
  layer_set_frame(text_layer_get_layer(s_instructions_text_layer), GRect(h_margin, 0, content_width, content_size.h + 8));

  s_instructions_scroll_layer = scroll_layer_create(bounds);
  scroll_layer_set_content_size(s_instructions_scroll_layer, GSize(bounds.size.w, content_size.h + 8));
  scroll_layer_add_child(s_instructions_scroll_layer, text_layer_get_layer(s_instructions_text_layer));
  scroll_layer_set_click_config_onto_window(s_instructions_scroll_layer, window);
  layer_add_child(window_layer, scroll_layer_get_layer(s_instructions_scroll_layer));
}
static void instructions_window_unload(Window *window) {
  if (s_instructions_scroll_layer) {
    scroll_layer_destroy(s_instructions_scroll_layer);
    s_instructions_scroll_layer = NULL;
  }
  if (s_instructions_text_layer) {
    text_layer_destroy(s_instructions_text_layer);
    s_instructions_text_layer = NULL;
  }
}

// --- Custom target window logic ---
static void update_custom_display() {
  static char s_buffer[128];
  snprintf(s_buffer, sizeof(s_buffer),
           "Set Win Target\n%d\n(1 - 10)\nUP/DOWN to change\nSELECT to confirm",
           s_target_score);
  if (s_custom_display_layer) text_layer_set_text(s_custom_display_layer, s_buffer);
}
static void custom_up_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_target_score < 10) {
    s_target_score++;
    update_custom_display();
  } else {
    vibes_short_pulse();
  }
}
static void custom_down_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_target_score > 1) {
    s_target_score--;
    update_custom_display();
  } else {
    vibes_short_pulse();
  }
}
static void custom_select_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "ACTION: Custom Target Score set to %d", s_target_score);
  reset_all_scores();
  window_stack_pop(true);
  window_stack_push(s_main_window, true);
}
static void custom_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, custom_up_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, custom_down_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, custom_select_handler);
}
static void custom_target_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  window_set_background_color(window, GColorWhite);

  GFont label_font = fonts_get_system_font(FONT_KEY_LABEL);
  s_custom_display_layer = create_layer(window_layer, bounds, GColorBlack, label_font, GTextAlignmentCenter);
  if (s_target_score < 1) s_target_score = 1;
  if (s_target_score > 10) s_target_score = 10;

  update_custom_display();

  GSize content_size = text_layer_get_content_size(s_custom_display_layer);
  const int content_height_buffered = content_size.h + 4;
  int y_centered = (bounds.size.h - content_height_buffered) / 2;
  layer_set_frame(text_layer_get_layer(s_custom_display_layer), GRect(0, y_centered, bounds.size.w, content_height_buffered));
}
static void custom_target_window_unload(Window *window) {
  if (s_custom_display_layer) {
    text_layer_destroy(s_custom_display_layer);
    s_custom_display_layer = NULL;
  }
}

// --- Main window (dynamic / responsive) ---
static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  window_set_background_color(window, GColorWhite);

  // Clock height proportional to screen height
  int clock_height = bounds.size.h / 12;
  if (clock_height < 14) clock_height = 14;
  if (clock_height > 28) clock_height = 28;

  // Create clock (top, full width)
  GFont clock_font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  s_clock_rect = GRect(0, V_PADDING, bounds.size.w, clock_height);
  s_clock_layer = create_layer(window_layer, s_clock_rect, GColorBlack, clock_font, GTextAlignmentCenter);
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  tick_handler(t, MINUTE_UNIT);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  // Layout math for scoreboard area below the clock
  const int ab_width = action_bar_width_always();
  const int left_margin = H_PADDING;
  const int right_margin = ab_width + H_PADDING;
  const int inner_width = bounds.size.w - left_margin - right_margin;
  const int col_width = (inner_width > 0) ? (inner_width / 3) : 0;

  int top_after_clock = clock_height + (2 * V_PADDING);
  int avail_h = bounds.size.h - top_after_clock - V_PADDING;
  if (avail_h < 40) avail_h = bounds.size.h - top_after_clock;

  // Allocate about 35% of the available area to each player score region (P1, P2)
  int score_h = (avail_h * 35) / 100; // each player's area
  if (score_h < 20) score_h = 20;

  // Header gets the remaining space; enforce a sensible minimum so headers don't clip
  int header_h = avail_h - (2 * score_h);
  if (header_h < 18) {
    header_h = 18;
    score_h = (avail_h - header_h) / 2;
    if (score_h < 20) score_h = 20;
  }

  // Center the score table vertically and horizontally within available area
  const int table_h = header_h + (2 * score_h);
  const int table_top = top_after_clock + ((avail_h - table_h) / 2);
  const int header_y = table_top;
  const int p1_y = header_y + header_h;
  const int p2_y = p1_y + score_h;
  const int target_y = p2_y + score_h + 4;

  // Center horizontally: use inner_width as table width and center it in the full bounds
  const int table_w = inner_width;
  const int table_left = (bounds.size.w - table_w) / 2;
  const int left_margin_for_table = table_left;

  // Choose fonts based on width for scaling (cache GFont handles)
  const char *score_font_key;
  const char *label_font_key;
  if (bounds.size.w >= 200) {
    score_font_key = FONT_KEY_GOTHIC_28_BOLD;
    label_font_key = FONT_KEY_GOTHIC_18_BOLD;
  } else if (bounds.size.w >= 180) {
    score_font_key = FONT_KEY_GOTHIC_24;
    label_font_key = FONT_KEY_GOTHIC_18_BOLD;
  } else {
    score_font_key = FONT_KEY_GOTHIC_24;
    label_font_key = FONT_KEY_GOTHIC_18_BOLD;
  }
  GFont score_font = fonts_get_system_font(score_font_key);
  GFont label_font = fonts_get_system_font(label_font_key);
  // Choose more appropriate fonts based on computed score height
  choose_fonts_for_layout(bounds, score_h, &score_font, &label_font, NULL);

  #ifdef PBL_COLOR
    const GColor p1_color = GColorRed;
    const GColor p2_color = GColorBlue;
    const GColor header_color = GColorBlack;
  #else
    const GColor p1_color = GColorBlack;
    const GColor p2_color = GColorBlack;
    const GColor header_color = GColorBlack;
  #endif

  // Decorative layer behind text (lines between columns/rows)
  s_decor_layer = layer_create(bounds);
  layer_set_update_proc(s_decor_layer, decor_update_proc);
  layer_add_child(window_layer, s_decor_layer);

  // Headers (store rects then create layers and vertically center text)
  s_plr_header_rect = GRect(left_margin_for_table, header_y, col_width, header_h);
  s_plr_header_layer = create_layer(window_layer, s_plr_header_rect, header_color, label_font, GTextAlignmentCenter);
  text_layer_set_text(s_plr_header_layer, "PLR");
  center_text_layer_vertically(s_plr_header_layer, s_plr_header_rect);

  s_rnd_header_rect = GRect(left_margin_for_table + col_width, header_y, col_width, header_h);
  s_rnd_header_layer = create_layer(window_layer, s_rnd_header_rect, header_color, label_font, GTextAlignmentCenter);
  text_layer_set_text(s_rnd_header_layer, "RND");
  center_text_layer_vertically(s_rnd_header_layer, s_rnd_header_rect);

  s_win_header_rect = GRect(left_margin_for_table + 2 * col_width, header_y, col_width, header_h);
  s_win_header_layer = create_layer(window_layer, s_win_header_rect, header_color, label_font, GTextAlignmentCenter);
  text_layer_set_text(s_win_header_layer, "WIN");
  center_text_layer_vertically(s_win_header_layer, s_win_header_rect);

  // Player 1 (store rects and center)
  s_p1_label_rect = GRect(left_margin_for_table, p1_y, col_width, score_h);
  s_p1_label_layer = create_layer(window_layer, s_p1_label_rect, p1_color, score_font, GTextAlignmentCenter);
  text_layer_set_text(s_p1_label_layer, "P1");
  center_text_layer_vertically(s_p1_label_layer, s_p1_label_rect);

  s_p1_round_rect = GRect(left_margin_for_table + col_width, p1_y, col_width, score_h);
  s_p1_round_layer = create_layer(window_layer, s_p1_round_rect, p1_color, score_font, GTextAlignmentCenter);
  { char buf[8]; snprintf(buf, sizeof(buf), "%d", s_state.p1_round_score); text_layer_set_text(s_p1_round_layer, buf); }
  center_text_layer_vertically(s_p1_round_layer, s_p1_round_rect);

  s_p1_win_rect = GRect(left_margin_for_table + 2 * col_width, p1_y, col_width, score_h);
  s_p1_win_layer  = create_layer(window_layer, s_p1_win_rect, p1_color, score_font, GTextAlignmentCenter);
  { char buf[8]; snprintf(buf, sizeof(buf), "%d", s_state.p1_game_score); text_layer_set_text(s_p1_win_layer, buf); }
  center_text_layer_vertically(s_p1_win_layer, s_p1_win_rect);

  // Player 2 (store rects and center)
  s_p2_label_rect = GRect(left_margin_for_table, p2_y, col_width, score_h);
  s_p2_label_layer = create_layer(window_layer, s_p2_label_rect, p2_color, score_font, GTextAlignmentCenter);
  text_layer_set_text(s_p2_label_layer, "P2");
  center_text_layer_vertically(s_p2_label_layer, s_p2_label_rect);

  s_p2_round_rect = GRect(left_margin_for_table + col_width, p2_y, col_width, score_h);
  s_p2_round_layer = create_layer(window_layer, s_p2_round_rect, p2_color, score_font, GTextAlignmentCenter);
  { char buf[8]; snprintf(buf, sizeof(buf), "%d", s_state.p2_round_score); text_layer_set_text(s_p2_round_layer, buf); }
  center_text_layer_vertically(s_p2_round_layer, s_p2_round_rect);

  s_p2_win_rect = GRect(left_margin_for_table + 2 * col_width, p2_y, col_width, score_h);
  s_p2_win_layer   = create_layer(window_layer, s_p2_win_rect, p2_color, score_font, GTextAlignmentCenter);
  { char buf[8]; snprintf(buf, sizeof(buf), "%d", s_state.p2_game_score); text_layer_set_text(s_p2_win_layer, buf); }
  center_text_layer_vertically(s_p2_win_layer, s_p2_win_rect);

  // Target (store rect and center)
  s_target_rect = GRect(left_margin_for_table, target_y, table_w, header_h);
  s_target_layer = create_layer(window_layer, s_target_rect, header_color, label_font, GTextAlignmentCenter);
  { char buf[32]; snprintf(buf, sizeof(buf), "Target: %d", s_target_score); text_layer_set_text(s_target_layer, buf); }
  center_text_layer_vertically(s_target_layer, s_target_rect);

  update_display();
}

static void main_window_unload(Window *window) {
  if (s_decor_layer) {
    layer_destroy(s_decor_layer);
    s_decor_layer = NULL;
  }

  if (s_plr_header_layer) { text_layer_destroy(s_plr_header_layer); s_plr_header_layer = NULL; }
  if (s_rnd_header_layer) { text_layer_destroy(s_rnd_header_layer); s_rnd_header_layer = NULL; }
  if (s_win_header_layer) { text_layer_destroy(s_win_header_layer); s_win_header_layer = NULL; }

  if (s_p1_label_layer) { text_layer_destroy(s_p1_label_layer); s_p1_label_layer = NULL; }
  if (s_p1_round_layer) { text_layer_destroy(s_p1_round_layer); s_p1_round_layer = NULL; }
  if (s_p1_win_layer)   { text_layer_destroy(s_p1_win_layer);   s_p1_win_layer = NULL; }
  if (s_p2_label_layer) { text_layer_destroy(s_p2_label_layer); s_p2_label_layer = NULL; }
  if (s_p2_round_layer) { text_layer_destroy(s_p2_round_layer); s_p2_round_layer = NULL; }
  if (s_p2_win_layer)   { text_layer_destroy(s_p2_win_layer);   s_p2_win_layer = NULL; }
  if (s_target_layer)   { text_layer_destroy(s_target_layer);   s_target_layer = NULL; }

  if (s_clock_layer) {
    text_layer_destroy(s_clock_layer);
    s_clock_layer = NULL;
  }
  tick_timer_service_unsubscribe();
}

// --- App lifecycle ---
static void init(void) {
  s_state = (AppState){.p1_round_score = 0, .p2_round_score = 0, .p1_game_score = 0, .p2_game_score = 0};

  s_main_window = window_create();
  s_menu_window = window_create();
  s_instructions_window = window_create();
  s_custom_target_window = window_create();

  window_set_click_config_provider(s_main_window, click_config_provider);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  window_set_window_handlers(s_menu_window, (WindowHandlers) {
    .load = menu_window_load,
    .unload = menu_window_unload
  });

  window_set_window_handlers(s_instructions_window, (WindowHandlers) {
    .load = instructions_window_load,
    .unload = instructions_window_unload
  });

  window_set_click_config_provider(s_custom_target_window, custom_click_config_provider);
  window_set_window_handlers(s_custom_target_window, (WindowHandlers) {
    .load = custom_target_window_load,
    .unload = custom_target_window_unload
  });

  window_stack_push(s_menu_window, true);
}

static void deinit(void) {
  if (s_main_window) { window_destroy(s_main_window); s_main_window = NULL; }
  if (s_menu_window) { window_destroy(s_menu_window); s_menu_window = NULL; }
  if (s_instructions_window) { window_destroy(s_instructions_window); s_instructions_window = NULL; }
  if (s_custom_target_window) { window_destroy(s_custom_target_window); s_custom_target_window = NULL; }
}

int main(void) {
  init();
  app_event_loop();
  deinit();
  return 0;
}
