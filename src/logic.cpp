#include "hardware.h"

static int last_mode = MODE_OFF;
static int saved_mode = MODE_SPA;
static long last_spa_enc1 = 0;
static long last_spa_enc2 = 0;
static long last_canvas_enc1 = 0;
static long last_canvas_enc2 = 0;
static long last_rainbow_enc1 = 0;
static long last_rainbow_enc2 = 0;
static bool presence_fading = false;
static uint8_t presence_fade_level = 255;
static unsigned long presence_fade_last_ms = 0;
static unsigned long presence_reenter_since_ms = 0;

static const unsigned long T2_HOLD_MS = 700;
static bool t2_press_active = false;
static unsigned long t2_press_ms = 0;
static bool t2_hold_fired = false;
static bool t2_encoder_used = false;

static void note_t2_modifier_used() {
  if (!button_is_held(BUTTON_T2)) return;
  t2_encoder_used = true;
}

static void reset_presence_fade() {
  presence_fading = false;
  presence_fade_level = 255;
  presence_fade_last_ms = millis();
  presence_reenter_since_ms = 0;
}

// While fade-out is running, brief sensor blips must not ramp brightness back up.
static bool presence_cancels_fade(bool present) {
  if (!present) {
    presence_reenter_since_ms = 0;
    return false;
  }
  if (!presence_fading || presence_fade_level >= 255) {
    presence_reenter_since_ms = 0;
    return true;
  }

  const unsigned long now = millis();
  if (presence_reenter_since_ms == 0) {
    presence_reenter_since_ms = now;
    return false;
  }
  if ((now - presence_reenter_since_ms) < PRESENCE_REENTER_MS) {
    return false;
  }

  presence_reenter_since_ms = 0;
  presence_fading = false;
  return true;
}

static void step_presence_fade_level(unsigned long dt, bool ramp_up) {
  if (dt == 0) return;

  const unsigned long span = ramp_up ? PRESENCE_FADE_IN_MS : PRESENCE_FADE_OUT_MS;
  unsigned long step = (255UL * dt) / span;
  if (step == 0) step = 1;

  if (ramp_up) {
    const uint16_t next = (uint16_t)presence_fade_level + step;
    presence_fade_level = (next >= 255) ? 255 : (uint8_t)next;
    return;
  }

  presence_fade_level = (presence_fade_level > step) ? presence_fade_level - step : 0;
}

static void update_presence_fade_level(bool present) {
  const unsigned long now = millis();
  if (presence_fade_last_ms == 0) presence_fade_last_ms = now;

  unsigned long dt = now - presence_fade_last_ms;
  presence_fade_last_ms = now;
  if (dt > 50) dt = 50;

  if (present) {
    if (presence_fade_level < 255) {
      step_presence_fade_level(dt, true);
    }
    return;
  }

  if (mode == MODE_SPA && spa_phase == SPA_NONE) return;
  if (mode != MODE_SPA && mode != MODE_CANVAS && mode != MODE_RAINBOW) return;
  if (mode != MODE_SPA && presence_fade_level == 0) return;

  if (presence_fade_level > 0) {
    step_presence_fade_level(dt, false);
  }
}

static void update_spa_session_for_presence(bool present) {
  if (presence_cancels_fade(present)) {
    if (spa_phase == SPA_NONE) {
      spa_phase = SPA_BASE;
      presence_fade_level = 255;
      presence_fading = false;
      spa_apply_schedule_defaults_now();
    }
    update_presence_fade_level(true);
    return;
  }

  if (spa_phase == SPA_NONE) return;

  presence_fading = true;
  update_presence_fade_level(false);

  if (presence_fade_level == 0) {
    presence_fading = false;
    presence_reenter_since_ms = 0;
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

  if (mode == MODE_CANVAS || mode == MODE_RAINBOW) {
    if (presence_cancels_fade(present)) {
      update_presence_fade_level(true);
      return;
    }

    if (presence_fade_level == 0) return;

    presence_fading = true;
    update_presence_fade_level(false);
    return;
  }
}

void update_spa_session() {
  update_presence_session();
}

bool visual_is_lit() {
  if (user_is_present()) return true;
  if (mode == MODE_SPA && spa_phase != SPA_NONE) return true;
  if ((mode == MODE_CANVAS || mode == MODE_RAINBOW) && presence_fade_level > 0) return true;
  return false;
}

uint8_t presence_fade_mul() {
  if (mode == MODE_OFF) return 255;
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
    if (user_is_present()) {
      spa_phase = SPA_BASE;
      presence_fade_level = 255;
    } else {
      spa_phase = SPA_NONE;
    }
    spa_candle_reset();
  }

  if (mode == MODE_CANVAS) {
    last_canvas_enc1 = get_encoder1_pos();
    last_canvas_enc2 = get_encoder2_pos();
    presence_fade_level = user_is_present() ? 255 : 0;
    presence_fade_last_ms = millis();
    presence_fading = false;
  }

  if (mode == MODE_RAINBOW) {
    rainbow_reset();
    last_rainbow_enc1 = get_encoder1_pos();
    last_rainbow_enc2 = get_encoder2_pos();
    presence_fade_level = user_is_present() ? 255 : 0;
    presence_fade_last_ms = millis();
    presence_fading = false;
  }

  if (mode == MODE_OFF) {
    reset_presence_fade();
    spa_phase = SPA_NONE;
    spa_candle_reset();
  }

  if (mode == MODE_SPA || mode == MODE_CANVAS || mode == MODE_RAINBOW) {
    saved_mode = mode;
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
  const bool t2_held = button_is_held(BUTTON_T2);

  if (t2_held && d2 != 0) {
    note_t2_modifier_used();
  }

  spa_tuning_update(d1, d2, t2_held);

  if (d1 != 0) {
    trigger_encoder_feedback(0, (d1 > 0) ? 1 : -1, CHSV(28, 50, 255));
  }
  if (d2 != 0) {
    const CRGB c2 = t2_held ? CRGB(CHSV(220, 220, 255)) : CRGB(CHSV(spa_hue_val(), 200, 255));
    trigger_encoder_feedback(1, (d2 > 0) ? 1 : -1, c2);
  }

  last_spa_enc1 = enc1;
  last_spa_enc2 = enc2;
}

void update_canvas_inputs() {
  if (mode != MODE_CANVAS) return;

  canvas_tuning_tick();

  const long enc1 = get_encoder1_pos();
  const long enc2 = get_encoder2_pos();
  (void)enc1;
  const int16_t d2 = (int16_t)(enc2 - last_canvas_enc2);

  if (button_is_held(BUTTON_T2) && d2 != 0) {
    note_t2_modifier_used();
    canvas_tuning_update(d2, 0); // sat
    trigger_encoder_feedback(1, (d2 > 0) ? 1 : -1, CHSV(220, 220, 255));
  } else {
    canvas_tuning_update(0, d2); // hue
    if (d2 != 0) {
      trigger_encoder_feedback(1, (d2 > 0) ? 1 : -1, CHSV(canvas_hue_val(), 200, 255));
    }
  }

  last_canvas_enc1 = enc1;
  last_canvas_enc2 = enc2;
}

void update_rainbow_inputs() {
  if (mode != MODE_RAINBOW) return;

  const long enc1 = get_encoder1_pos();
  const long enc2 = get_encoder2_pos();
  (void)enc1;
  const int16_t d2 = (int16_t)(enc2 - last_rainbow_enc2);

  rainbow_update(0, d2, millis());

  if (d2 != 0) {
    trigger_encoder_feedback(1, (d2 > 0) ? 1 : -1, CHSV(160, 200, 255));
  }

  last_rainbow_enc1 = enc1;
  last_rainbow_enc2 = enc2;
}

void update_daynight_schedule() {
  if (!schedule_time_valid()) return;

  const bool present = user_is_present();
  static bool was_present = false;
  const bool session_start = present && !was_present;
  was_present = present;

  if (!session_start) return;

  if (mode == MODE_OFF) {
    mode = MODE_SPA;
    handle_mode_change();
  }
}

static void mode_next() {
  if (mode == MODE_OFF) {
    mode = (saved_mode == MODE_SPA || saved_mode == MODE_CANVAS || saved_mode == MODE_RAINBOW)
               ? saved_mode
               : MODE_SPA;
  } else if (mode == MODE_SPA) {
    mode = MODE_CANVAS;
  } else if (mode == MODE_CANVAS) {
    mode = MODE_RAINBOW;
  } else {
    mode = MODE_SPA;
  }
  handle_mode_change();
}

static void mode_prev() {
  if (mode == MODE_OFF) return;

  if (mode == MODE_SPA) {
    mode = MODE_RAINBOW;
  } else if (mode == MODE_CANVAS) {
    mode = MODE_SPA;
  } else if (mode == MODE_RAINBOW) {
    mode = MODE_CANVAS;
  } else {
    mode = MODE_SPA;
  }
  handle_mode_change();
}

static void power_toggle() {
  if (mode == MODE_OFF) {
    mode = (saved_mode == MODE_SPA || saved_mode == MODE_CANVAS || saved_mode == MODE_RAINBOW)
               ? saved_mode
               : MODE_SPA;
    handle_mode_change();
    trigger_zone(1, CRGB::Green);
    return;
  }

  if (mode == MODE_SPA || mode == MODE_CANVAS || mode == MODE_RAINBOW) {
    saved_mode = mode;
  }
  always_on = false;
  mode = MODE_OFF;
  handle_mode_change();
  trigger_status_shutdown();
}

static void apply_always_on_wake() {
  if (mode == MODE_OFF) {
    mode = (saved_mode == MODE_SPA || saved_mode == MODE_CANVAS || saved_mode == MODE_RAINBOW)
               ? saved_mode
               : MODE_SPA;
    handle_mode_change();
    return;
  }

  if (mode == MODE_SPA && spa_phase == SPA_NONE) {
    spa_phase = SPA_BASE;
    presence_fade_level = 255;
    presence_fading = false;
    spa_apply_schedule_defaults_now();
    spa_candle_reset();
  }
}

void update_mode_button_pending() {
  // T2 uses press/release + hold; no deferred single-click queue.
}

void update_t2_hold() {
  if (button_is_held(BUTTON_T2)) {
    if (!t2_press_active) return;
    if (t2_hold_fired || t2_encoder_used) return;
    if (millis() - t2_press_ms < T2_HOLD_MS) return;

    t2_hold_fired = true;
    always_on = !always_on;
    if (always_on) {
      apply_always_on_wake();
      trigger_zone(1, CRGB(0, 220, 0));
    } else {
      trigger_zone(1, CRGB(0, 90, 0));
    }
    return;
  }

  if (t2_press_active) {
    if (!t2_hold_fired && !t2_encoder_used) {
      power_toggle();
    }
    t2_press_active = false;
    t2_hold_fired = false;
    t2_encoder_used = false;
  }
}

void handle_mode_buttons(ButtonEvent e) {
  switch (e) {
    case BUTTON_T1:
      mode_prev();
      trigger_zone(0, CRGB::Red);
      break;

    case BUTTON_T2:
      t2_press_active = true;
      t2_press_ms = millis();
      t2_hold_fired = false;
      t2_encoder_used = false;
      break;

    case BUTTON_T3:
      mode_next();
      trigger_zone(2, CRGB::Blue);
      break;

    default:
      break;
  }
}
