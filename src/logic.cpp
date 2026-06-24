#include "hardware.h"

static int last_mode = MODE_OFF;
static long last_spa_enc1 = 0;
static long last_spa_enc2 = 0;
static long last_showcase_enc1 = 0;
static long last_showcase_enc2 = 0;
static long last_canvas_enc1 = 0;
static long last_canvas_enc2 = 0;
static int schedule_last_hour = -1;

void update_spa_session() {
  const bool present = user_is_present();
  const bool in_spa = (mode == MODE_SPA);

  if (!in_spa || !present) {
    if (spa_phase != SPA_NONE) {
      spa_phase = SPA_NONE;
      spa_candle_reset();
    }
    return;
  }

  if (spa_phase == SPA_NONE) {
    spa_phase = SPA_BASE;
  }
}

void toggle_spa_candle() {
  if (mode != MODE_SPA || spa_phase == SPA_NONE) return;

  if (spa_phase == SPA_CANDLE) {
    spa_phase = SPA_BASE;
    spa_candle_reset();
    return;
  }

  spa_phase = SPA_CANDLE;
  spa_candle_reset();
}

static void handle_mode_change() {
  if (mode == last_mode) return;

  if (mode == MODE_SPA) {
    last_spa_enc1 = get_encoder1_pos();
    last_spa_enc2 = get_encoder2_pos();
    spa_apply_schedule_defaults_now();
    if (user_is_present()) {
      spa_phase = SPA_BASE;
    } else {
      spa_phase = SPA_NONE;
    }
    spa_candle_reset();
  }

  if (mode == MODE_SHOWCASE) {
    showcase_reset();
    last_showcase_enc1 = get_encoder1_pos();
    last_showcase_enc2 = get_encoder2_pos();
  }

  if (mode == MODE_CANVAS) {
    last_canvas_enc1 = get_encoder1_pos();
    last_canvas_enc2 = get_encoder2_pos();
  }

  last_mode = mode;
}

void update_spa_inputs() {
  if (mode != MODE_SPA) return;
  if (spa_phase == SPA_NONE) return;

  spa_tuning_tick();

  const long enc1 = get_encoder1_pos();
  const long enc2 = get_encoder2_pos();
  const int16_t d1 = (int16_t)(enc1 - last_spa_enc1);
  const int16_t d2 = (int16_t)(enc2 - last_spa_enc2);

  spa_tuning_update(d1, d2, button_is_held(BUTTON_T1));

  last_spa_enc1 = enc1;
  last_spa_enc2 = enc2;
}

void update_showcase_inputs() {
  if (mode != MODE_SHOWCASE) return;
  if (button_is_held(BUTTON_T1)) return;

  const long enc1 = get_encoder1_pos();
  const long enc2 = get_encoder2_pos();
  const int16_t d1 = (int16_t)(enc1 - last_showcase_enc1);
  const int16_t d2 = (int16_t)(enc2 - last_showcase_enc2);

  showcase_update(d1, d2, millis());

  last_showcase_enc1 = enc1;
  last_showcase_enc2 = enc2;
}

void update_canvas_inputs() {
  if (mode != MODE_CANVAS) return;
  if (button_is_held(BUTTON_T1)) return;

  canvas_tuning_tick();

  const long enc1 = get_encoder1_pos();
  const long enc2 = get_encoder2_pos();
  const int16_t d1 = (int16_t)(enc1 - last_canvas_enc1);
  const int16_t d2 = (int16_t)(enc2 - last_canvas_enc2);

  canvas_tuning_update(d1, d2);

  last_canvas_enc1 = enc1;
  last_canvas_enc2 = enc2;
}

void update_daynight_schedule() {
  if (!schedule_time_valid()) return;

  const bool present = user_is_present();
  static bool was_present = false;
  const bool session_start = present && !was_present;
  was_present = present;

  if (!present) return;

  const int hour = schedule_local_hour();
  if (hour < 0) return;

  const bool hour_changed = (schedule_last_hour != hour);
  schedule_last_hour = hour;

  if (!hour_changed && !session_start) return;

  if (mode == MODE_OFF) {
    mode = MODE_SPA;
    handle_mode_change();
    return;
  }

  if (mode == MODE_SPA) {
    spa_apply_schedule_defaults_now();
  }
}

static const unsigned long T2_DOUBLE_CLICK_MS = 400;
static unsigned long t2_last_click_ms = 0;
static bool t2_pending_single = false;

static void cycle_light_mode() {
  if (mode == MODE_OFF) {
    mode = MODE_SPA;
  } else if (mode == MODE_SPA) {
    mode = MODE_SHOWCASE;
  } else if (mode == MODE_SHOWCASE) {
    mode = MODE_CANVAS;
  } else {
    mode = MODE_SPA;
  }
  handle_mode_change();
}

static bool handle_t2_click() {
  const unsigned long now = millis();

  if (t2_pending_single && (now - t2_last_click_ms) <= T2_DOUBLE_CLICK_MS) {
    t2_pending_single = false;
    mode = MODE_OFF;
    handle_mode_change();
    trigger_status_shutdown();
    return true;
  }

  t2_pending_single = true;
  t2_last_click_ms = now;
  return false;
}

void update_mode_button_pending() {
  if (!t2_pending_single) return;
  if (millis() - t2_last_click_ms <= T2_DOUBLE_CLICK_MS) return;

  t2_pending_single = false;
  cycle_light_mode();
}

void handle_mode_buttons(ButtonEvent e) {
  switch (e) {
    case BUTTON_T1:
      trigger_zone(0, CRGB::Red);
      break;

    case BUTTON_T2:
      if (!handle_t2_click()) {
        trigger_zone(1, CRGB::Green);
      }
      break;

    case BUTTON_T3:
      toggle_spa_candle();
      trigger_zone(2, CRGB::Blue);
      break;

    default:
      break;
  }
}
