#include "hardware.h"

static int last_mode = MODE_OFF;
static long last_spa_enc1 = 0;
static long last_spa_enc2 = 0;
static long last_showcase_enc1 = 0;
static long last_showcase_enc2 = 0;
static long last_canvas_enc1 = 0;
static long last_canvas_enc2 = 0;
static bool presence_fading = false;
static uint8_t presence_fade_level = 255;
static float presence_fade_value = 255.0f;
static unsigned long presence_fade_last_ms = 0;
static const CRGB NIGHT_CANVAS_COLOR(255, 20, 0);
static const uint8_t NIGHT_CANVAS_BRIGHTNESS_RAW = 80; // Gamma-mapped to about 20/255.
static uint8_t brightness_before_night = 200;
static bool night_brightness_active = false;

static void set_presence_fade_level(uint8_t level) {
  presence_fade_level = level;
  presence_fade_value = (float)level;
}

static bool presence_fade_is_complete() {
  return presence_fade_level == 0;
}

static void reset_presence_fade() {
  presence_fading = false;
  set_presence_fade_level(255);
  presence_fade_last_ms = millis();
}

static void step_presence_fade_level(unsigned long dt, bool ramp_up) {
  if (dt == 0) return;

  const unsigned long span = ramp_up ? PRESENCE_FADE_IN_MS : PRESENCE_FADE_OUT_MS;
  const float step = 255.0f * (float)dt / (float)span;

  if (ramp_up) {
    presence_fade_value += step;
    if (presence_fade_value > 255.0f) presence_fade_value = 255.0f;
  } else {
    presence_fade_value -= step;
    if (presence_fade_value < 0.0f) presence_fade_value = 0.0f;
  }

  presence_fade_level = (uint8_t)constrain(
      (int)(presence_fade_value + 0.5f), 0, 255);
}

static void update_presence_fade_level(bool present) {
  const unsigned long now = millis();
  if (presence_fade_last_ms == 0) presence_fade_last_ms = now;

  unsigned long dt = now - presence_fade_last_ms;
  presence_fade_last_ms = now;
  if (dt > 50) dt = 50;

  if (present) {
    if (presence_fade_value < 255.0f) {
      step_presence_fade_level(dt, true);
    }
    return;
  }

  if (mode == MODE_SPA && spa_phase == SPA_NONE) return;
  if (mode != MODE_SPA && mode != MODE_SHOWCASE && mode != MODE_CANVAS) return;
  if (mode != MODE_SPA && presence_fade_is_complete()) return;

  if (!presence_fade_is_complete()) {
    step_presence_fade_level(dt, false);
  }
}

static void update_spa_session_for_presence(bool present) {
  if (present) {
    presence_fading = (presence_fade_level < 255);
    if (spa_phase == SPA_NONE) {
      spa_phase = SPA_BASE;
      set_presence_fade_level(255);
      presence_fading = false;
      spa_apply_schedule_defaults_now();
    }
    update_presence_fade_level(true);
    return;
  }

  if (spa_phase == SPA_NONE) return;

  presence_fading = true;
  update_presence_fade_level(false);

  if (presence_fade_is_complete()) {
    presence_fading = false;
    spa_phase = SPA_NONE;
    spa_candle_reset();
  }
}

void update_presence_session() {
  const bool present = user_is_present();

  if (mode == MODE_OFF) {
    reset_presence_fade();
    if (spa_phase != SPA_NONE) {
      spa_phase = SPA_NONE;
      spa_candle_reset();
    }
    return;
  }

  if (mode == MODE_SPA) {
    update_spa_session_for_presence(present);
    return;
  }

  if (mode == MODE_SHOWCASE || mode == MODE_CANVAS) {
    if (present) {
      presence_fading = (presence_fade_level < 255);
      update_presence_fade_level(true);
      return;
    }

    if (presence_fade_is_complete()) return;

    presence_fading = true;
    update_presence_fade_level(false);
    return;
  }
}

void update_spa_session() {
  update_presence_session();
}

bool visual_is_lit() {
  if (mode == MODE_OFF) return false;
  if (user_is_present()) return true;
  if (mode == MODE_SPA && spa_phase != SPA_NONE) return true;
  if ((mode == MODE_SHOWCASE || mode == MODE_CANVAS) && !presence_fade_is_complete()) return true;
  return false;
}

uint8_t presence_fade_mul() {
  if (mode == MODE_OFF) return 255;
  if (mode == MODE_SPA && spa_phase == SPA_NONE && !user_is_present()) return 255;
  if ((mode == MODE_SHOWCASE || mode == MODE_CANVAS) &&
      presence_fade_is_complete() && !user_is_present()) {
    return 255;
  }
  return presence_fade_level;
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

  if (last_mode == MODE_SPA && mode != MODE_SPA) {
    spa_phase = SPA_NONE;
    spa_candle_reset();
  }

  if (mode == MODE_SPA) {
    last_spa_enc1 = get_encoder1_pos();
    last_spa_enc2 = get_encoder2_pos();
    spa_apply_schedule_defaults_now();
    if (user_is_present() || (last_mode != MODE_OFF && !presence_fade_is_complete())) {
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
    if (last_mode == MODE_OFF && !user_is_present()) {
      set_presence_fade_level(0);
    }
    presence_fade_last_ms = millis();
    presence_fading = !user_is_present() && !presence_fade_is_complete();
  }

  if (mode == MODE_CANVAS) {
    last_canvas_enc1 = get_encoder1_pos();
    last_canvas_enc2 = get_encoder2_pos();
    if (last_mode == MODE_OFF && !user_is_present()) {
      set_presence_fade_level(0);
    }
    presence_fade_last_ms = millis();
    presence_fading = !user_is_present() && !presence_fade_is_complete();
  }

  if (mode == MODE_OFF) {
    reset_presence_fade();
    spa_phase = SPA_NONE;
    spa_candle_reset();
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
  static bool default_pending = true;

  if (!present) {
    if (!visual_is_lit()) default_pending = true;
    return;
  }
  if (!default_pending) return;

  const int minutes = schedule_local_minutes();
  if (minutes < 0) return;
  default_pending = false;

  if (schedule_is_night(minutes)) {
    if (!night_brightness_active) {
      brightness_before_night = g_brightness_raw;
      night_brightness_active = true;
    }
    set_global_brightness_immediate(NIGHT_CANVAS_BRIGHTNESS_RAW);
    canvas_apply_defaults_rgb(NIGHT_CANVAS_COLOR);
    if (mode != MODE_CANVAS) {
      mode = MODE_CANVAS;
      handle_mode_change();
    }
    return;
  }

  if (night_brightness_active) {
    set_global_brightness_immediate(brightness_before_night);
    night_brightness_active = false;
  }
  if (mode != MODE_SPA) {
    mode = MODE_SPA;
    handle_mode_change();
  }
}

static const unsigned long T2_DOUBLE_CLICK_MS = 400;
static const unsigned long T2_HOLD_MS = 700;
static unsigned long t2_last_click_ms = 0;
static bool t2_pending_single = false;
static unsigned long t2_hold_start_ms = 0;
static bool t2_hold_fired = false;

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

static void apply_always_on_wake() {
  if (mode == MODE_OFF) {
    mode = MODE_SPA;
    handle_mode_change();
    return;
  }

  if (mode == MODE_SPA && spa_phase == SPA_NONE) {
    spa_phase = SPA_BASE;
    set_presence_fade_level(255);
    presence_fading = false;
    spa_apply_schedule_defaults_now();
    spa_candle_reset();
  }
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
  if (t2_hold_fired) return;
  if (millis() - t2_last_click_ms <= T2_DOUBLE_CLICK_MS) return;

  t2_pending_single = false;
  cycle_light_mode();
}

void update_t2_hold() {
  if (button_is_held(BUTTON_T2)) {
    if (t2_hold_start_ms == 0) {
      t2_hold_start_ms = millis();
      return;
    }
    if (t2_hold_fired) return;
    if (millis() - t2_hold_start_ms < T2_HOLD_MS) return;

    t2_hold_fired = true;
    t2_pending_single = false;
    always_on = !always_on;
    if (always_on) {
      apply_always_on_wake();
      trigger_zone(1, CRGB(0, 220, 0));
    } else {
      trigger_zone(1, CRGB(0, 90, 0));
    }
    return;
  }

  t2_hold_start_ms = 0;
  t2_hold_fired = false;
}

void handle_mode_buttons(ButtonEvent e) {
  switch (e) {
    case BUTTON_T1:
      trigger_zone(0, CRGB::Red);
      break;

    case BUTTON_T2:
      if (t2_hold_fired) break;
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
