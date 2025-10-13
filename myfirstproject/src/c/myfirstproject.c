// =================================================================================
// Simple Score Keeper for Pebble 2 Duo
// C Implementation based on user's HTML/JS logic.
// The app tracks round points and total games won for two players.
// Target Score is now selectable on startup.
// =================================================================================

#include <pebble.h>

// --- Configuration Constants & State ---
// This variable holds the dynamically chosen target score for winning a game.
static int s_target_score = 4;
static const int s_menu_scores[] = {4, 7, 10}; // Score options for the menu

#define FONT_KEY_MAIN FONT_KEY_LECO_42_NUMBERS // Large font for scores

// --- Data Structure for Game State ---
// This struct holds all the variable data for our score keeper.
typedef struct {
  int p1_round_score;
  int p2_round_score;
  int p1_game_score;
  int p2_game_score;
} AppState;

// Global instance of the game state
static AppState s_state;

// --- UI Elements ---
static Window *s_main_window;
static TextLayer *s_p1_rs_layer; // Player 1 Round Score
static TextLayer *s_p2_rs_layer; // Player 2 Round Score
static TextLayer *s_gs_layer;    // Games Score (P1 - P2)
static TextLayer *s_p1_label;    // Label for Player 1
static TextLayer *s_p2_label;    // Label for Player 2
static TextLayer *s_target_label; // Label for Target Score info

// Menu Window Elements
static Window *s_menu_window;
static MenuLayer *s_menu_layer;


// --- Utility Functions ---

/**
 * @brief Resets all scores (Round Points and Games Won) to zero.
 */
static void reset_all_scores() {
  s_state.p1_round_score = 0;
  s_state.p2_round_score = 0;
  s_state.p1_game_score = 0;
  s_state.p2_game_score = 0;
  APP_LOG(APP_LOG_LEVEL_INFO, "ACTION: Reset All Scores.");
}

/**
 * @brief Resets only the current round scores to zero.
 */
static void reset_round_scores() {
  s_state.p1_round_score = 0;
  s_state.p2_round_score = 0;
  APP_LOG(APP_LOG_LEVEL_INFO, "ACTION: Reset Round Scores.");
}

/**
 * @brief Resets only the games won scores to zero.
 */
static void reset_game_scores() {
  s_state.p1_game_score = 0;
  s_state.p2_game_score = 0;
  APP_LOG(APP_LOG_LEVEL_INFO, "ACTION: Reset Game Scores.");
}


/**
 * @brief Updates all TextLayers with the current score values.
 */
static void update_display() {
  // Use static buffers to avoid stack overflow in the update function
  static char s_p1_rs_buffer[8];
  static char s_p2_rs_buffer[8];
  static char s_gs_buffer[16];
  static char s_target_buffer[32]; // Buffer for instructions/target

  // Format Round Scores
  snprintf(s_p1_rs_buffer, sizeof(s_p1_rs_buffer), "%d", s_state.p1_round_score);
  snprintf(s_p2_rs_buffer, sizeof(s_p2_rs_buffer), "%d", s_state.p2_round_score);

  // Format Game Scores (e.g., "Games: 3 - 2")
  snprintf(s_gs_buffer, sizeof(s_gs_buffer), "Games: %d - %d", s_state.p1_game_score, s_state.p2_game_score);
  
  // Format Target/Instructions
  snprintf(s_target_buffer, sizeof(s_target_buffer), "Target: %d. UP/DOWN to score.", s_target_score);


  // Set text on the layers
  text_layer_set_text(s_p1_rs_layer, s_p1_rs_buffer);
  text_layer_set_text(s_p2_rs_layer, s_p2_rs_buffer);
  text_layer_set_text(s_gs_layer, s_gs_buffer);
  text_layer_set_text(s_target_label, s_target_buffer); // Update instruction text

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Display Updated: P1 RS=%d, P2 RS=%d, GS=%d-%d",
          s_state.p1_round_score, s_state.p2_round_score, s_state.p1_game_score, s_state.p2_game_score);
}

/**
 * @brief Checks if a player has won the current game (round).
 * If a player reaches the s_target_score, their game win count is incremented,
 * and the round scores are reset.
 */
static void check_for_game_win() {
  bool game_won = false;

  if (s_state.p1_round_score >= s_target_score) {
    s_state.p1_game_score++;
    game_won = true;
    APP_LOG(APP_LOG_LEVEL_INFO, "Player 1 Wins Game! Total: %d", s_state.p1_game_score);
    // Vibrate to notify the user of a win
    vibes_double_pulse();
  } else if (s_state.p2_round_score >= s_target_score) {
    s_state.p2_game_score++;
    game_won = true;
    APP_LOG(APP_LOG_LEVEL_INFO, "Player 2 Wins Game! Total: %d", s_state.p2_game_score);
    // Vibrate to notify the user of a win
    vibes_double_pulse();
  }

  // Reset round scores only if a game was won
  if (game_won) {
    s_state.p1_round_score = 0;
    s_state.p2_round_score = 0;
  }
}

// --- Click Handlers (The App's Core Logic) ---

/**
 * @brief Handles a single click on the UP button.
 * Action: Increase Player 1 score by 1.
 */
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  s_state.p1_round_score++;
  check_for_game_win();
  update_display();
}

/**
 * @brief Handles a single click on the DOWN button.
 * Action: Increase Player 2 score by 1.
 */
static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  s_state.p2_round_score++;
  check_for_game_win();
  update_display();
}

/**
 * @brief Handles a single click on the SELECT button.
 * Action: Reset ALL scores (Round and Game).
 */
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  reset_all_scores();
  update_display();
}

/**
 * @brief Handles a long press on the UP button.
 * Action: Reset only the current round scores (P1 RS and P2 RS).
 */
static void up_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  reset_round_scores();
  update_display();
  vibes_short_pulse(); // Confirmation vibe
}

/**
 * @brief Handles a long press on the DOWN button.
 * Action: Reset only the total game scores (P1 GS and P2 GS).
 */
static void down_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  reset_game_scores();
  update_display();
  vibes_short_pulse(); // Confirmation vibe
}

/**
 * @brief Configures all button handlers for the Main Score Window.
 */
static void click_config_provider(void *context) {
  // UP Button: Single click (P1 Score +1), Long click (Reset Round Scores)
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_long_click_subscribe(BUTTON_ID_UP, 700, up_long_click_handler, NULL); // 700ms long press

  // DOWN Button: Single click (P2 Score +1), Long click (Reset Game Scores)
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_long_click_subscribe(BUTTON_ID_DOWN, 700, down_long_click_handler, NULL); // 700ms long press

  // SELECT Button: Single click (Reset ALL Scores)
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

// --- Main Window Lifecycle Handlers ---

/**
 * @brief Initializes all UI elements when the main window loads.
 */
static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // 1. Player 1 Label (Top Left)
  s_p1_label = text_layer_create(GRect(0, 0, bounds.size.w / 2, 20));
  text_layer_set_text(s_p1_label, "Player 1");
  text_layer_set_font(s_p1_label, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_p1_label, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_p1_label));

  // 2. Player 2 Label (Top Right)
  s_p2_label = text_layer_create(GRect(bounds.size.w / 2, 0, bounds.size.w / 2, 20));
  text_layer_set_text(s_p2_label, "Player 2");
  text_layer_set_font(s_p2_label, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_p2_label, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_p2_label));

  // 3. Player 1 Round Score (Large Font, Left)
  s_p1_rs_layer = text_layer_create(GRect(0, 20, bounds.size.w / 2, 50));
  text_layer_set_background_color(s_p1_rs_layer, GColorClear);
  text_layer_set_text_color(s_p1_rs_layer, GColorRed);
  text_layer_set_font(s_p1_rs_layer, fonts_get_system_font(FONT_KEY_MAIN));
  text_layer_set_text_alignment(s_p1_rs_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_p1_rs_layer));

  // 4. Player 2 Round Score (Large Font, Right)
  s_p2_rs_layer = text_layer_create(GRect(bounds.size.w / 2, 20, bounds.size.w / 2, 50));
  text_layer_set_background_color(s_p2_rs_layer, GColorClear);
  text_layer_set_text_color(s_p2_rs_layer, GColorBlue);
  text_layer_set_font(s_p2_rs_layer, fonts_get_system_font(FONT_KEY_MAIN));
  text_layer_set_text_alignment(s_p2_rs_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_p2_rs_layer));

  // 5. Games Score (Center bottom)
  s_gs_layer = text_layer_create(GRect(0, 75, bounds.size.w, 30));
  text_layer_set_background_color(s_gs_layer, GColorClear);
  text_layer_set_font(s_gs_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_gs_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_gs_layer));

  // 6. Target/Instructions (Bottom status line)
  s_target_label = text_layer_create(GRect(0, 140, bounds.size.w, 20));
  // text_layer_set_text is done in update_display() now
  text_layer_set_font(s_target_label, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_target_label, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_target_label));

  // Initial display update
  update_display();
}

/**
 * @brief Destroys all UI elements when the main window unloads.
 */
static void main_window_unload(Window *window) {
  // Destroy TextLayers
  text_layer_destroy(s_p1_rs_layer);
  text_layer_destroy(s_p2_rs_layer);
  text_layer_destroy(s_gs_layer);
  text_layer_destroy(s_p1_label);
  text_layer_destroy(s_p2_label);
  text_layer_destroy(s_target_label);
}

// --- Menu Window Implementation ---

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  return ARRAY_LENGTH(s_menu_scores);
}

static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  static char s_buffer[32];
  snprintf(s_buffer, sizeof(s_buffer), "Play to %d points", s_menu_scores[cell_index->row]);
  menu_cell_basic_draw(ctx, cell_layer, s_buffer, "Target Score", NULL);
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  // 1. Set the global target score based on menu selection
  s_target_score = s_menu_scores[cell_index->row];

  // 2. Push the main score window onto the stack to start the game
  window_stack_push(s_main_window, true);
  APP_LOG(APP_LOG_LEVEL_INFO, "Target Score set to %d. Starting game.", s_target_score);
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

  // Set a clear title for the menu
  window_set_title(s_menu_window, "Choose Target Score");
}

static void menu_window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
}


// --- App Initialization and De-initialization ---

/**
 * @brief Sets up the main window and state.
 */
static void init() {
  // Initialize game state to zeros
  memset(&s_state, 0, sizeof(s_state));

  // 1. Create MAIN Score Window
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_set_click_config_provider(s_main_window, click_config_provider);

  // 2. Create MENU Window
  s_menu_window = window_create();
  window_set_window_handlers(s_menu_window, (WindowHandlers) {
    .load = menu_window_load,
    .unload = menu_window_unload
  });
  window_set_background_color(s_menu_window, GColorWhite);

  // Show the Menu Window first
  window_stack_push(s_menu_window, true);
}

/**
 * @brief Cleans up memory.
 */
static void deinit() {
  // Destroy both Windows
  window_destroy(s_main_window);
  window_destroy(s_menu_window);
}

// The main entry point for the Pebble app
int main(void) {
  init();
  app_event_loop();
  deinit();
}
