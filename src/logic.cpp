#include "hardware.h"

static unsigned long focus_deep_at = 0;
static bool focus_phase_manual = false;
static int last_mode = MODE_OFF;
static long last_showcase_enc1 = 0;
static long last_showcase_enc2 = 0;
static long last_focus_enc1 = 0;
static long last_focus_enc2 = 0;
static long last_canvas_enc1 = 0;
static long last_canvas_enc2 = 0;

static const uint8_t CANVAS_EVENING_HUE = 3;
static const uint8_t CANVAS_EVENING_SAT = 240;
static int schedule_last_hour = -1;

void update_focus_session() {
  const bool present = user_is_present();
  const bool in_focus = (mode == MODE_FOCUS);

  if (!in_focus || !present) {
    focus_deep_at = 0;
    focus_phase = FOCUS_NONE;
    focus_phase_manual = false;
    focus_candle_reset();
    return;
  }

  if (focus_deep_at == 0) {
    focus_deep_at = millis() + FOCUS_WARMUP_MS;
  }

  if (focus_phase_manual) return;

  focus_phase = (millis() >= focus_deep_at) ? FOCUS_DEEP : FOCUS_ARRIVAL;
}

void skip_focus_warmup() {
  if (mode != MODE_FOCUS || focus_phase != FOCUS_ARRIVAL) return;
  focus_deep_at = millis();
  focus_phase = FOCUS_DEEP;
  focus_phase_manual = true;
}

void toggle_focus_color() {
  if (mode != MODE_FOCUS || focus_phase == FOCUS_NONE) return;
  focus_phase_manual = true;
  focus_phase = (focus_phase == FOCUS_DEEP) ? FOCUS_ARRIVAL : FOCUS_DEEP;
}

static void handle_mode_change() {
  if (mode == last_mode) return;

  if (mode == MODE_SHOWCASE) {
    showcase_reset();
    last_showcase_enc1 = get_encoder1_pos();
    last_showcase_enc2 = get_encoder2_pos();
  }

  if (mode == MODE_FOCUS) {
    last_focus_enc1 = get_encoder1_pos();
    last_focus_enc2 = get_encoder2_pos();
    const bool already_at_desk = user_is_present();
    focus_phase_manual = false;
    if (already_at_desk) {
      focus_deep_at = millis();
      focus_phase = FOCUS_DEEP;
      focus_candle_reset();
    } else {
      focus_deep_at = 0;
      focus_phase = FOCUS_NONE;
    }
  }

  if (mode == MODE_CANVAS) {
    last_canvas_enc1 = get_encoder1_pos();
    last_canvas_enc2 = get_encoder2_pos();
  }

  last_mode = mode;
}

void update_showcase_inputs() {
  if (mode != MODE_SHOWCASE) return;

  const long enc1 = get_encoder1_pos();
  const long enc2 = get_encoder2_pos();
  const int16_t d1 = (int16_t)(enc1 - last_showcase_enc1);
  const int16_t d2 = (int16_t)(enc2 - last_showcase_enc2);

  showcase_update(d1, d2, millis());

  last_showcase_enc1 = enc1;
  last_showcase_enc2 = enc2;
}

void update_focus_inputs() {
  if (mode != MODE_FOCUS) return;

  focus_tuning_tick();

  if (focus_phase == FOCUS_NONE) return;

  const long enc1 = get_encoder1_pos();
  const long enc2 = get_encoder2_pos();
  const int16_t d1 = (int16_t)(enc1 - last_focus_enc1);
  const int16_t d2 = (int16_t)(enc2 - last_focus_enc2);

  focus_tuning_update(d1, d2, focus_phase);

  last_focus_enc1 = enc1;
  last_focus_enc2 = enc2;
}

void update_canvas_inputs() {
  if (mode != MODE_CANVAS) return;

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

  const bool focus_slot = schedule_in_focus_hours(hour);
  const bool hour_changed = (schedule_last_hour != hour);
  schedule_last_hour = hour;

  if (!hour_changed && !session_start) return;

  if (focus_slot) {
    if (mode != MODE_FOCUS) {
      mode = MODE_FOCUS;
      handle_mode_change();
    }
    return;
  }

  canvas_apply_defaults(CANVAS_EVENING_HUE, CANVAS_EVENING_SAT);
  if (mode != MODE_CANVAS) {
    mode = MODE_CANVAS;
    handle_mode_change();
  }
}

static const unsigned long T2_DOUBLE_CLICK_MS = 400;
static unsigned long t2_last_click_ms = 0;
static bool t2_pending_single = false;

static void cycle_light_mode() {
  if (mode == MODE_OFF) {
    mode = MODE_FOCUS;
  } else if (mode == MODE_FOCUS) {
    mode = MODE_SHOWCASE;
  } else if (mode == MODE_SHOWCASE) {
    mode = MODE_CANVAS;
  } else {
    mode = MODE_FOCUS;
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
      skip_focus_warmup();
      trigger_zone(0, CRGB::Red);
      break;

    case BUTTON_T2:
      if (!handle_t2_click()) {
        trigger_zone(1, CRGB::Green);
      }
      break;

    case BUTTON_T3:
      if (mode == MODE_FOCUS) {
        toggle_focus_color();
      }
      trigger_zone(2, CRGB::Blue);
      break;

    default:
      break;
  }
}
