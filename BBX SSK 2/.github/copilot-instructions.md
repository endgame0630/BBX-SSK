<!-- Generated guidance for AI coding agents. Keep concise and actionable. -->
# Copilot / AI Agent Instructions for this repo

This is a Pebble smartwatch application (C, Pebble SDK v3). The goal of these instructions is to help an AI coding agent be immediately productive and make safe, focused edits.

**Big Picture**
- **Platform:** Pebble SDK v3 watch app; multiple target platforms are declared in `package.json` (`aplite`, `basalt`, `chalk`, `diorite`, `emery`).
- **Language & structure:** Native C sources under `src/c/` (main file: `src/c/BBX SSK 2.c`). Optional JS/PebbleKit files live under `src/pkjs/` if present.
- **Build system:** Uses the Pebble Waf rules via `wscript` (calls `ctx.load('pebble_sdk')`) and standard `pebble` CLI tools.

**Primary workflows / commands**
- Build for all platforms: `pebble build`
- Clean build artifacts: `pebble clean`
- Install to emulator: `pebble install --emulator <platform>` (e.g. `basalt`)
- Screenshot an emulator: `pebble screenshot --no-open screenshot.png` — use this after UI changes.
- The `wscript` defines per-platform `pbl_build` and `pbl_bundle` behavior; prefer `pebble` CLI unless making advanced build tweaks.

**Project-specific patterns & conventions**
- UI is single-window driven with light-weight secondary windows (menu, instructions, custom target). Look at `main()`, `init()`, and window handlers in `src/c/BBX SSK 2.c`.
- Click handlers are registered via `window_set_click_config_provider` and helper functions named `*_click_handler` / `*_click_config_provider`.
- Layout is responsive: visual sizes and fonts are chosen at runtime from display dimensions (see `main_window_load` font-selection block).
- Decorative drawing uses a custom `decor_update_proc` Layer update proc.
- Logging and telemetry: use `APP_LOG(APP_LOG_LEVEL_INFO, ...)` for non-UI-visible actions.
- Vibration feedback uses `vibes_short_pulse()` and `vibes_double_pulse()` for user events.

**Important constants and places to edit**
- Target scores and menu options: `s_menu_scores[]` and `s_target_score` in `src/c/BBX SSK 2.c`.
- Instructions text shown to users: the `instructions_text` string in `instructions_window_load`.
- Fonts and layout helpers: `create_layer()` helper and `FONT_KEY_*` constants near the top of the C file.

**Safety & change guidance for AI agents**
- Avoid editing `package.json` `pebble.sdkVersion` or `uuid` unless explicitly requested by maintainers.
- Paths include spaces (repo root has spaces). When running commands or referencing files programmatically, escape or quote paths: e.g. `"BBX SSK 2/src/c/BBX SSK 2.c"`.
- Preserve existing `CLAUDE.md` content — it contains user-visible guidance and build commands. Merge only project-specific additions.
- When modifying UI, always run `pebble build` and capture a `pebble screenshot` to verify visual changes match requirements.

**Examples of quick edits**
- Fix a button action: find `up_click_handler` / `down_click_handler` and update `s_state` changes, then run `pebble build` + `pebble screenshot`.
- Change target options: edit `s_menu_scores[]`, update `FIXED_SCORE_OPTIONS` if length changes, and verify menu draw callbacks (`menu_draw_row_callback`).

**Testing & validation**
- Use the emulator targets in `package.json` for quick validation (`basalt` or `emery` for color devices).
- Validate logs via the Pebble console output when running or installing on an emulator.

**Files to inspect for context**
- `CLAUDE.md` — contains high-level project notes and commands (preserve/merge).
- `package.json` — Pebble project metadata and `targetPlatforms`.
- `wscript` — build orchestration (multi-platform bundling, worker support).
- `src/c/BBX SSK 2.c` — primary application logic, UI layout, input handlers.

If anything above is unclear or you want extra examples (e.g., a walkthrough for adding a new menu item, or a test harness), tell me which area to expand and I will update this file.
