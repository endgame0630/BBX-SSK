#include <pebble.h> // Include the main Pebble SDK header file, providing all necessary APIs

// --- Adaptive Configuration (Fonts) ---

// Large font for the main score numbers (Round and Win totals) and P1/P2 labels.
// Using GOTHIC_28_BOLD for a sharper, more readable, and modern digital aesthetic.
#define FONT_KEY_SCORE_MAIN FONT_KEY_GOTHIC_28_BOLD
// Medium font for headers (Player, Round, Win) and the Target score display.
#define FONT_KEY_LABEL FONT_KEY_GOTHIC_18_BOLD      

// --- Layout Constants (Dimensions tuned for the selected fonts) ---
// Height for the P1 and P2 score rows.
#define ROW_HEIGHT_SCORE 30
// Height for the header and target score rows.
#define ROW_HEIGHT_LABEL 20

// --- Menu Configuration and State ---
static int s_target_score = 4; // Default points needed to win a single game/round.
// Available fixed options for the target score in the main menu.
static const int s_menu_scores[] = {4, 7, 10}; 

// Define custom indices for the MenuLayer rows to improve readability.
#define FIXED_SCORE_OPTIONS ARRAY_LENGTH(s_menu_scores) // Index 0-2 (for 4, 7, 10)
#define CUSTOM_TARGET_ROW FIXED_SCORE_OPTIONS          // Index 3 (for Custom Target selection)
#define INSTRUCTIONS_ROW (CUSTOM_TARGET_ROW + 1)       // Index 4 (for Instructions screen)
#define TOTAL_MENU_ROWS (INSTRUCTIONS_ROW + 1)         // Total 5 rows in the menu

// Structure to hold all dynamic game state. This is the application's core data model.
typedef struct {
  int p1_round_score; // Player 1's score in the current round (resets on game win).
  int p2_round_score; // Player 2's score in the current round (resets on game win).
  int p1_game_score;  // Player 1's total games won (resets manually).
  int p2_game_score;  // Player 2's total games won (resets manually).
} AppState;

static AppState s_state; // Global instance of the game state.

// --- Window Elements (Layer Pointers) ---

// Main Windows
static Window *s_main_window; // The primary scoreboard screen.
static Window *s_menu_window; // The initial target selection menu.
static Window *s_instructions_window; // The instructions screen.
static Window *s_custom_target_window; // The score adjustment screen for custom target.

// Menu and Instructions Sub-layers
static MenuLayer *s_menu_layer;
static ScrollLayer *s_instructions_scroll_layer;
static TextLayer *s_instructions_text_layer;

// Custom Target Sub-layer
static TextLayer *s_custom_display_layer; // Displays the custom target score and instructions.

// Scoreboard Layers for Layered Display on the main screen
static TextLayer *s_plr_header_layer; // "Player" header.
static TextLayer *s_rnd_header_layer; // "Round" header.
static TextLayer *s_win_header_layer; // "Wins" header.
static TextLayer *s_p1_label_layer;   // "P1" label.
static TextLayer *s_p2_label_layer;   // "P2" label.
static TextLayer *s_p1_round_layer;   // P1 current round score.
static TextLayer *s_p2_round_layer;   // P2 current round score.
static TextLayer *s_p1_win_layer;     // P1 total game wins.
static TextLayer *s_p2_win_layer;     // P2 total game wins.
static TextLayer *s_target_layer;     // Displays the active target score at the bottom.

// --- Core Logic Functions ---

/**
 * @brief Resets all four score trackers (round and game wins) to zero.
 * Used when starting a new match or changing the target score.
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
 * Used by the Long UP button press.
 */
static void reset_round_scores() {
  s_state.p1_round_score = 0;
  s_state.p2_round_score = 0;
  APP_LOG(APP_LOG_LEVEL_INFO, "ACTION: Reset Round Scores.");
}

/**
 * @brief Resets only the total games won to zero.
 * Used by the Long DOWN button press.
 */
static void reset_game_scores() {
  s_state.p1_game_score = 0;
  s_state.p2_game_score = 0;
  APP_LOG(APP_LOG_LEVEL_INFO, "ACTION: Reset Game Scores.");
}

/**
 * @brief Updates the text displayed on all score layers on the main window.
 * This is called after any score change or reset action.
 */
static void update_display() {
  // Buffers must be static or allocated on the heap if persistent storage is needed.
  static char s_p1_round_buf[8];
  static char s_p2_round_buf[8];
  static char s_p1_win_buf[8];
  static char s_p2_win_buf[8];
  static char s_target_buf[32];

  // 1. Update Round Scores
  snprintf(s_p1_round_buf, sizeof(s_p1_round_buf), "%d", s_state.p1_round_score);
  text_layer_set_text(s_p1_round_layer, s_p1_round_buf);

  snprintf(s_p2_round_buf, sizeof(s_p2_round_buf), "%d", s_state.p2_round_score);
  text_layer_set_text(s_p2_round_layer, s_p2_round_buf);

  // 2. Update Win Scores
  snprintf(s_p1_win_buf, sizeof(s_p1_win_buf), "%d", s_state.p1_game_score);
  text_layer_set_text(s_p1_win_layer, s_p1_win_buf);

  snprintf(s_p2_win_buf, sizeof(s_p2_win_buf), "%d", s_state.p2_game_score);
  text_layer_set_text(s_p2_win_layer, s_p2_win_buf);
  
  // 3. Update Target Layer display
  snprintf(s_target_buf, sizeof(s_target_buf), "Target: %d", s_target_score);
  text_layer_set_text(s_target_layer, s_target_buf);
}

/**
 * @brief Checks if either player has reached the target score (s_target_score).
 * If a win is detected, increments the total game score and resets the round score.
 */
static void check_for_game_win() {
  bool game_won = false;
  
  // Check if Player 1 has reached the target
  if (s_state.p1_round_score >= s_target_score) {
    s_state.p1_game_score++; // Increment Player 1 game win count
    game_won = true;
    APP_LOG(APP_LOG_LEVEL_INFO, "Player 1 Wins Game! Total: %d", s_state.p1_game_score);
    vibes_double_pulse(); // Vibrate to signal a win
  
  // Check if Player 2 has reached the target
  } else if (s_state.p2_round_score >= s_target_score) {
    s_state.p2_game_score++; // Increment Player 2 game win count
    game_won = true;
    APP_LOG(APP_LOG_LEVEL_INFO, "Player 2 Wins Game! Total: %d", s_state.p2_game_score);
    vibes_double_pulse(); // Vibrate to signal a win
  }
  
  // If a game was won, reset the round scores for the next game
  if (game_won) {
    s_state.p1_round_score = 0;
    s_state.p2_round_score = 0;
  }
}

// --- Main Window Click Handlers & Config ---

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Score +1 for Player 1
  s_state.p1_round_score++; 
  check_for_game_win();     
  update_display();         
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Score +1 for Player 2
  s_state.p2_round_score++; 
  check_for_game_win();     
  update_display();         
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    // Reset ALL scores (round and game wins)
    reset_all_scores(); 
    update_display();   
}

static void up_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Reset only the current round scores
  reset_round_scores(); 
  update_display();     
  vibes_short_pulse();  
}

static void down_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Reset only the total game wins
  reset_game_scores();  
  update_display();     
  vibes_short_pulse();  
}

/**
 * @brief Sets up all click subscriptions for the main scoreboard window.
 */
static void click_config_provider(void *context) {
  // UP button: Single click to score P1, Long click to reset round scores
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_long_click_subscribe(BUTTON_ID_UP, 700, up_long_click_handler, NULL);
  
  // DOWN button: Single click to score P2, Long click to reset game wins
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_long_click_subscribe(BUTTON_ID_DOWN, 700, down_long_click_handler, NULL);
  
  // SELECT button: Single click to reset all scores
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

// --- Utility Functions ---

/**
 * @brief Returns the width of the action bar, if present.
 * This is used to make layout responsive and account for the buttons on rectangular devices.
 */
static int action_bar_width_always() {
  #ifdef PBL_RECT
    return 30; // Standard width for the action bar area on rectangular devices
  #else
    // Action bar is integrated or invisible on round/steel devices (often 0 or system handles it)
    return 0;  
  #endif
}

/**
 * @brief Helper function to create TextLayers with consistent properties (color, font, alignment).
 * @param window_layer The parent layer to add the TextLayer to.
 * @param rect The bounding box for the layer.
 * @param text_color The foreground text color.
 * @param font_key The key for the system font to use.
 * @param alignment The horizontal text alignment (e.g., GTextAlignmentCenter).
 * @return A pointer to the newly created TextLayer.
 */
static TextLayer* create_layer(Layer *window_layer, GRect rect, GColor text_color, const char *font_key, GTextAlignment alignment) {
    TextLayer *layer = text_layer_create(rect);
    text_layer_set_background_color(layer, GColorClear);  
    text_layer_set_text_color(layer, text_color);        
    text_layer_set_font(layer, fonts_get_system_font(font_key)); 
    text_layer_set_text_alignment(layer, alignment);      
    layer_add_child(window_layer, text_layer_get_layer(layer)); 
    return layer;
}

// --- Main Window Handlers (Layout Logic) ---

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  window_set_background_color(window, GColorWhite);
  
  // --- Vertical Layout Calculation (Centering the main score block) ---
  const int table_height = ROW_HEIGHT_LABEL + (2 * ROW_HEIGHT_SCORE);
  const int target_layer_height = ROW_HEIGHT_LABEL;
  // Total height of the score block (Headers, P1, P2) plus the Target layer and a margin.
  const int total_block_height = table_height + 4 + target_layer_height; 

  // Calculate starting Y position to center the block vertically on the screen.
  const int y_start = (bounds.size.h - total_block_height) / 2;

  // Calculate individual row Y positions based on new heights
  const int header_y = y_start;
  const int p1_y = header_y + ROW_HEIGHT_LABEL;
  const int p2_y = p1_y + ROW_HEIGHT_SCORE;
  const int target_y = p2_y + ROW_HEIGHT_SCORE + 4; // Extra 4px margin for separation
  
  // --- Horizontal Layout (Responsive Column Widths) ---
  const int ab_width = action_bar_width_always();
  // Usable width excludes the action bar area for the score columns.
  const int usable_width = bounds.size.w - ab_width; 
  
  // Calculate equal width for the three columns (Player | Round | Wins).
  const int col_width = usable_width / 3;

  // Column X positions (0, 1, 2)
  const int x_col_0 = 0;
  const int x_col_1 = col_width;
  const int x_col_2 = col_width * 2;
  
  // --- Color Definitions (P1 Red, P2 Blue for Color Devices) ---
#ifdef PBL_COLOR
  const GColor p1_color = GColorRed;
  const GColor p2_color = GColorBlue;
  const GColor header_color = GColorBlack; 
#else
  // Monochrome/B&W fallback
  const GColor p1_color = GColorBlack;
  const GColor p2_color = GColorBlack;
  const GColor header_color = GColorBlack;
#endif

  // 1. Header Row: Player, Round, Wins
  s_plr_header_layer = create_layer(window_layer, GRect(x_col_0, header_y, col_width, ROW_HEIGHT_LABEL), header_color, FONT_KEY_LABEL, GTextAlignmentCenter);
  text_layer_set_text(s_plr_header_layer, "PLR");

  s_rnd_header_layer = create_layer(window_layer, GRect(x_col_1, header_y, col_width, ROW_HEIGHT_LABEL), header_color, FONT_KEY_LABEL, GTextAlignmentCenter);
  text_layer_set_text(s_rnd_header_layer, "RND");

  s_win_header_layer = create_layer(window_layer, GRect(x_col_2, header_y, col_width, ROW_HEIGHT_LABEL), header_color, FONT_KEY_LABEL, GTextAlignmentCenter);
  text_layer_set_text(s_win_header_layer, "WIN");

  // 2. Player 1 Score Row
  s_p1_label_layer = create_layer(window_layer, GRect(x_col_0, p1_y, col_width, ROW_HEIGHT_SCORE), p1_color, FONT_KEY_SCORE_MAIN, GTextAlignmentCenter);
  text_layer_set_text(s_p1_label_layer, "P1");

  s_p1_round_layer = create_layer(window_layer, GRect(x_col_1, p1_y, col_width, ROW_HEIGHT_SCORE), p1_color, FONT_KEY_SCORE_MAIN, GTextAlignmentCenter);
  
  s_p1_win_layer = create_layer(window_layer, GRect(x_col_2, p1_y, col_width, ROW_HEIGHT_SCORE), p1_color, FONT_KEY_SCORE_MAIN, GTextAlignmentCenter);

  // 3. Player 2 Score Row
  s_p2_label_layer = create_layer(window_layer, GRect(x_col_0, p2_y, col_width, ROW_HEIGHT_SCORE), p2_color, FONT_KEY_SCORE_MAIN, GTextAlignmentCenter);
  text_layer_set_text(s_p2_label_layer, "P2");

  s_p2_round_layer = create_layer(window_layer, GRect(x_col_1, p2_y, col_width, ROW_HEIGHT_SCORE), p2_color, FONT_KEY_SCORE_MAIN, GTextAlignmentCenter);

  s_p2_win_layer = create_layer(window_layer, GRect(x_col_2, p2_y, col_width, ROW_HEIGHT_SCORE), p2_color, FONT_KEY_SCORE_MAIN, GTextAlignmentCenter);

  // 4. Target Layer (spans full width below the main table)
  s_target_layer = create_layer(window_layer, GRect(0, target_y, bounds.size.w, target_layer_height), header_color, FONT_KEY_LABEL, GTextAlignmentCenter);

  update_display(); // Set initial scores/text
}

static void main_window_unload(Window *window) {
  // Destroy all individual layers when the main window is closed.
  text_layer_destroy(s_plr_header_layer);
  text_layer_destroy(s_rnd_header_layer);
  text_layer_destroy(s_win_header_layer);
  
  text_layer_destroy(s_p1_label_layer);
  text_layer_destroy(s_p1_round_layer);
  text_layer_destroy(s_p1_win_layer);
  text_layer_destroy(s_p2_label_layer);
  text_layer_destroy(s_p2_round_layer);
  text_layer_destroy(s_p2_win_layer);
  text_layer_destroy(s_target_layer);
}

// --- Menu Window Handlers (Standard Pebble Menu Logic) ---

// Callback to determine the total number of rows in the menu.
static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  return TOTAL_MENU_ROWS; 
}

// Callback for drawing each individual menu row.
static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
    const uint16_t index = cell_index->row; 
    
    if (index < FIXED_SCORE_OPTIONS) {
        // Fixed score target selection rows (Indices 0, 1, 2)
        static char s_title_buffer[32];
        static char s_subtitle_buffer[32]; 

        // Title displays the available target score.
        snprintf(s_title_buffer, sizeof(s_title_buffer), "Target: %d points", s_menu_scores[index]);

        // Subtitle indicates if this is the currently selected score.
        if (s_menu_scores[index] == s_target_score) {
            snprintf(s_subtitle_buffer, sizeof(s_subtitle_buffer), "Current Target");
        } else {
            snprintf(s_subtitle_buffer, sizeof(s_subtitle_buffer), "Set as Target");
        }
        
        menu_cell_basic_draw(ctx, cell_layer, s_title_buffer, s_subtitle_buffer, NULL);
    } else if (index == CUSTOM_TARGET_ROW) {
        // Custom target row (Index 3): Allows user to set a score 1-10.
        static char s_buffer[32];
        snprintf(s_buffer, sizeof(s_buffer), "Current: %d", s_target_score);
        menu_cell_basic_draw(ctx, cell_layer, "Custom Target", s_buffer, NULL);
    } else {
        // Instructions row (Index 4)
        menu_cell_basic_draw(ctx, cell_layer, "Instructions", "How to use the app", NULL);
    }
}

// Callback fired when a menu item is selected (SELECT button press).
static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  const uint16_t index = cell_index->row;
  
  if (index < FIXED_SCORE_OPTIONS) {
    // A fixed score was selected.
    s_target_score = s_menu_scores[index];
    reset_all_scores(); // Reset scores when target changes.
    window_stack_push(s_main_window, true); // Go to scoreboard.
    
  } else if (index == CUSTOM_TARGET_ROW) {
    // Custom target chosen, push the adjustment window.
    window_stack_push(s_custom_target_window, true);
  } else {
    // Instructions chosen, push the instructions window.
    window_stack_push(s_instructions_window, true);
  }
}

static void menu_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_menu_layer = menu_layer_create(bounds);
  // Assign menu callback functions.
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_rows = menu_get_num_rows_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
  });

  // Automatically handle button presses for menu navigation.
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void menu_window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
}


// --- Instructions Window Handlers ---

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

  // Create text layer with a large initial height to contain all content.
  s_instructions_text_layer = text_layer_create(GRect(h_margin, 0, content_width, 2000));
  text_layer_set_text(s_instructions_text_layer, instructions_text);
  text_layer_set_font(s_instructions_text_layer, fonts_get_system_font(FONT_KEY_LABEL));
  text_layer_set_text_color(s_instructions_text_layer, GColorBlack);

  // Get the actual height needed for the text.
  GSize content_size = text_layer_get_content_size(s_instructions_text_layer);
  
  // Adjust the text layer frame to fit the content size precisely.
  layer_set_frame(text_layer_get_layer(s_instructions_text_layer), 
                  GRect(h_margin, 0, content_width, content_size.h + 8));

  // Create the scroll layer to hold the text content.
  s_instructions_scroll_layer = scroll_layer_create(bounds);
  
  // Set the scroll layer's content size based on the text size.
  scroll_layer_set_content_size(s_instructions_scroll_layer, GSize(bounds.size.w, content_size.h + 8));
  // Add the text layer as a child of the scroll layer.
  scroll_layer_add_child(s_instructions_scroll_layer, text_layer_get_layer(s_instructions_text_layer));

  // Enable button clicks for the scroll layer (to allow scrolling with UP/DOWN).
  scroll_layer_set_click_config_onto_window(s_instructions_scroll_layer, window);

  layer_add_child(window_layer, scroll_layer_get_layer(s_instructions_scroll_layer));
}

static void instructions_window_unload(Window *window) {
  // Destroy the scroll layer and its child text layer.
  scroll_layer_destroy(s_instructions_scroll_layer);
  text_layer_destroy(s_instructions_text_layer);
}

// --- Custom Target Window Logic ---

// Forward declaration of the update function.
static void update_custom_display();

// Click Handlers for Custom Target Window (Min 1, Max 10)
static void custom_up_handler(ClickRecognizerRef recognizer, void *context) {
    // Increment the target score, up to a maximum of 10.
    if (s_target_score < 10) {
      s_target_score++;
      update_custom_display();
    } else {
      vibes_short_pulse(); // Indicate boundary reached.
    }
}

static void custom_down_handler(ClickRecognizerRef recognizer, void *context) {
    // Decrement the target score, down to a minimum of 1.
    if (s_target_score > 1) {
      s_target_score--;
      update_custom_display();
    } else {
      vibes_short_pulse(); // Indicate boundary reached.
    }
}

static void custom_select_handler(ClickRecognizerRef recognizer, void *context) {
    // SELECT confirms the score, resets the match, and returns to the main screen.
    APP_LOG(APP_LOG_LEVEL_INFO, "ACTION: Custom Target Score set to %d", s_target_score);
    reset_all_scores(); 
    
    window_stack_pop(true); // Pop the custom window.
    window_stack_push(s_main_window, true); // Push the main scoreboard.
}

/**
 * @brief Sets up click subscriptions for the custom target window.
 */
static void custom_click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_UP, custom_up_handler);
    window_single_click_subscribe(BUTTON_ID_DOWN, custom_down_handler);
    window_single_click_subscribe(BUTTON_ID_SELECT, custom_select_handler);
}

/**
 * @brief Formats and updates the instructional text displayed on the custom target screen.
 */
static void update_custom_display() {
    static char s_buffer[128];
    // Uses multiple lines for clarity on the small screen.
    snprintf(s_buffer, sizeof(s_buffer), 
      "Set Win Target\n%d\n(1 - 10)\nUP/DOWN to change\nSELECT to confirm", 
      s_target_score);
    text_layer_set_text(s_custom_display_layer, s_buffer);
}

static void custom_target_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    window_set_background_color(window, GColorWhite);

    // 1. Initial creation with full bounds. We must adjust the frame later for centering.
    s_custom_display_layer = create_layer(
      window_layer, bounds, GColorBlack, FONT_KEY_LABEL, GTextAlignmentCenter);
    
    // Ensure the current score is within bounds (1-10) before displaying/editing.
    if (s_target_score < 1) {
      s_target_score = 1; 
    } else if (s_target_score > 10) {
      s_target_score = 10;
    }
    
    update_custom_display(); // 2. Text is set on the layer.

    // --- Vertical Centering Logic ---
    // 3. Get the required size for the multi-line content.
    GSize content_size = text_layer_get_content_size(s_custom_display_layer);
    
    // 4. Calculate the new Y position to center the content vertically.
    // Add a small buffer of 4 pixels to the content height for better appearance.
    const int content_height_buffered = content_size.h + 4;
    int y_centered = (bounds.size.h - content_height_buffered) / 2;
    
    // 5. Adjust the layer frame to be centered vertically and match the content height.
    layer_set_frame(text_layer_get_layer(s_custom_display_layer), 
                    GRect(0, y_centered, bounds.size.w, content_height_buffered));
    // --- End Vertical Centering Logic ---
}

static void custom_target_window_unload(Window *window) {
    text_layer_destroy(s_custom_display_layer);
}


// --- App Lifecycle ---

static void init(void) {
  // 1. Initialize State
  // Set all initial scores to zero.
  s_state = (AppState){.p1_round_score = 0, .p2_round_score = 0, .p1_game_score = 0, .p2_game_score = 0};

  // 2. Create all necessary Windows
  s_main_window = window_create();
  s_menu_window = window_create();
  s_instructions_window = window_create();
  s_custom_target_window = window_create(); 

  // 3. Set Window Handlers and Click Providers
  
  // Main Window: Setup score display/update logic
  window_set_click_config_provider(s_main_window, click_config_provider);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Menu Window: Setup menu navigation
  window_set_window_handlers(s_menu_window, (WindowHandlers) {
    .load = menu_window_load,
    .unload = menu_window_unload
  });

  // Instructions Window: Setup scrollable instructions
  window_set_window_handlers(s_instructions_window, (WindowHandlers) {
    .load = instructions_window_load,
    .unload = instructions_window_unload
  });
  
  // Custom Target Window: Setup score adjustment logic
  window_set_click_config_provider(s_custom_target_window, custom_click_config_provider);
  window_set_window_handlers(s_custom_target_window, (WindowHandlers) {
    .load = custom_target_window_load,
    .unload = custom_target_window_unload
  });


  // 4. Push Menu Window onto the stack FIRST to allow target selection before play.
  window_stack_push(s_menu_window, true);
}

static void deinit(void) {
  // Destroy all windows when the application exits.
  window_destroy(s_main_window);
  window_destroy(s_menu_window);
  window_destroy(s_instructions_window);
  window_destroy(s_custom_target_window); 
}

// The main entry point function for the Pebble application.
int main(void) {
  init(); // Initialize the app.
  app_event_loop(); // Start the main event loop.
  deinit(); // Clean up resources when the app is terminated.
  return 0;
}
