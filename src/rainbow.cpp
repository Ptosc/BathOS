#include "hardware.h"

static const float RAINBOW_MACRO_STEP = 0.045f;
static const float RAINBOW_MACRO_SMOOTH = 0.075f;
static const float RAINBOW_PARAM_SMOOTH = 0.055f;
// ~14 encoder detents to cross one full preset (smooth ring blend, not instant jump)
static const float RAINBOW_PRESET_ENCODER_STEP = 0.07f;
static const float RAINBOW_SPEED_SMOOTH = 0.12f;
static const float RAINBOW_MACRO_DEFAULT = 0.38f;
static const float RAINBOW_BASE_RATE = 0.010f;
static const unsigned long RAINBOW_MAX_DT_MS = 40;
static const uint8_t RAINBOW_VAL = 225;
static const uint8_t RAINBOW_GLEAM_BLEND = 8;

static const float RAINBOW_SPAN_SCALE_MIN = 0.72f;
static const float RAINBOW_SPAN_SCALE_MAX = 1.16f;
static const float RAINBOW_RENDERED_SPAN_MIN = 42.0f;
static const float RAINBOW_RENDERED_SPAN_MAX = 255.0f;
static const float RAINBOW_SAT_BASE_MIN_OFFSET = -12.0f;
static const float RAINBOW_SAT_BASE_MAX_OFFSET = 18.0f;
static const float RAINBOW_SAT_WAVE_MIN_SCALE = 0.70f;
static const float RAINBOW_SAT_WAVE_MAX_SCALE = 1.45f;
static const uint8_t RAINBOW_SAT_WAVE_MAX = 80;
static const uint8_t RAINBOW_GLEAM_A_MIN = 18;
static const uint8_t RAINBOW_GLEAM_A_MAX = 62;
static const uint8_t RAINBOW_GLEAM_B_MIN = 12;
static const uint8_t RAINBOW_GLEAM_B_MAX = 48;

// color_mode (T3): 0 drift, 1 reverse, 2 breathe, 3 orbit
static const float RAINBOW_ORBIT_GLEAM_MUL = 2.6f;
static const float RAINBOW_BREATHE_RATE = 0.0045f;

struct RainbowPreset {
  uint8_t hue_origin;
  uint8_t hue_span;
  uint8_t sat_base;
  uint8_t sat_wave;
  CRGB gleam;
  float speed_lo;
  float speed_hi;
  float gleam_rate;
  float gleam_spacing;
  uint8_t gleam_r_a;
  uint8_t gleam_r_b;
  uint8_t blur_lo;
  uint8_t blur_hi;
  uint8_t sat_step;
};

static const RainbowPreset RAINBOW_PRESETS[] = {
  // Prism — full spectrum, balanced drift
  {0, 255, 218, 22, CRGB(255, 248, 230), 0.25f, 2.20f, 1.00f, 0.55f, 10, 7, 54, 24, 5},
  // Aurora — narrow band, slow dreamy shimmer
  {104, 96, 205, 26, CRGB(214, 255, 238), 0.18f, 1.00f, 0.65f, 0.35f, 14, 11, 68, 32, 3},
  // Sunset — warm tight glow, meditative
  {232, 82, 224, 20, CRGB(255, 226, 178), 0.15f, 0.85f, 0.50f, 0.70f, 12, 8, 72, 36, 4},
  // Ocean — cool ripples, gentle motion
  {134, 78, 210, 24, CRGB(196, 232, 255), 0.20f, 1.15f, 0.75f, 0.45f, 11, 9, 60, 28, 4},
  // Candy — playful, crisp gleams
  {188, 108, 220, 18, CRGB(255, 215, 246), 0.35f, 2.50f, 1.35f, 0.40f, 8, 6, 48, 18, 6},
};

static const uint8_t RAINBOW_PRESET_COUNT =
    sizeof(RAINBOW_PRESETS) / sizeof(RAINBOW_PRESETS[0]);

static float macro = RAINBOW_MACRO_DEFAULT;
static float macro_target = RAINBOW_MACRO_DEFAULT;
static float preset_pos = 0.0f;
static uint8_t preset_idx = 0;

static float hue_origin = RAINBOW_PRESETS[0].hue_origin;
static float hue_origin_target = RAINBOW_PRESETS[0].hue_origin;
static float hue_span = RAINBOW_PRESETS[0].hue_span;
static float hue_span_target = RAINBOW_PRESETS[0].hue_span;
static float sat_base = RAINBOW_PRESETS[0].sat_base;
static float sat_base_target = RAINBOW_PRESETS[0].sat_base;
static float sat_wave = RAINBOW_PRESETS[0].sat_wave;
static float sat_wave_target = RAINBOW_PRESETS[0].sat_wave;
static CRGB gleam_color = RAINBOW_PRESETS[0].gleam;
static CRGB gleam_target = RAINBOW_PRESETS[0].gleam;

static float speed_lo = RAINBOW_PRESETS[0].speed_lo;
static float speed_lo_target = RAINBOW_PRESETS[0].speed_lo;
static float speed_hi = RAINBOW_PRESETS[0].speed_hi;
static float speed_hi_target = RAINBOW_PRESETS[0].speed_hi;
static float gleam_rate = RAINBOW_PRESETS[0].gleam_rate;
static float gleam_rate_target = RAINBOW_PRESETS[0].gleam_rate;
static float gleam_spacing = RAINBOW_PRESETS[0].gleam_spacing;
static float gleam_spacing_target = RAINBOW_PRESETS[0].gleam_spacing;
static float gleam_r_a = RAINBOW_PRESETS[0].gleam_r_a;
static float gleam_r_a_target = RAINBOW_PRESETS[0].gleam_r_a;
static float gleam_r_b = RAINBOW_PRESETS[0].gleam_r_b;
static float gleam_r_b_target = RAINBOW_PRESETS[0].gleam_r_b;
static float blur_lo = RAINBOW_PRESETS[0].blur_lo;
static float blur_lo_target = RAINBOW_PRESETS[0].blur_lo;
static float blur_hi = RAINBOW_PRESETS[0].blur_hi;
static float blur_hi_target = RAINBOW_PRESETS[0].blur_hi;
static float sat_step = RAINBOW_PRESETS[0].sat_step;
static float sat_step_target = RAINBOW_PRESETS[0].sat_step;

static float speed = RAINBOW_PRESETS[0].speed_lo;
static float motion_phase = 0.0f;
static float gleam_phase = 0.0f;
static float breathe_phase = 0.0f;
static unsigned long last_ms = 0;

static float clamp_f(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static float wrap_f(float v, float max_v) {
  v = fmodf(v, max_v);
  if (v < 0.0f) v += max_v;
  return v;
}

static float lerp_toward(float current, float target, float rate) {
  const float d = target - current;
  if (fabsf(d) < 0.001f) return target;
  return current + d * rate;
}

static float mix_f(float a, float b, float t) {
  return a + (b - a) * t;
}

static uint8_t mix_u8(uint8_t a, uint8_t b, float t) {
  return (uint8_t)(mix_f((float)a, (float)b, t) + 0.5f);
}

static float ease_macro(float t) {
  t = clamp_f(t, 0.0f, 1.0f);
  return t * t * (3.0f - 2.0f * t);
}

static float hue_shortest_delta(float from, float to) {
  float d = to - from;
  while (d > 128.0f) d -= 256.0f;
  while (d < -128.0f) d += 256.0f;
  return d;
}

static float lerp_hue_toward(float current, float target, float rate) {
  const float d = hue_shortest_delta(current, target);
  if (fabsf(d) < 0.001f) return target;
  return current + d * rate;
}

static uint16_t ring_distance(uint16_t a, uint16_t b) {
  uint16_t d = (a > b) ? (a - b) : (b - a);
  const uint16_t wrapped = (uint16_t)(NUMPIXELS - d);
  return (d < wrapped) ? d : wrapped;
}

static uint16_t ring_index(float pos) {
  uint16_t rounded = (uint16_t)(pos + 0.5f);
  if (rounded >= NUMPIXELS) rounded = 0;
  return rounded;
}

static uint8_t soft_gleam(uint16_t led_pos, uint16_t center, uint8_t radius, uint8_t strength) {
  if (radius == 0 || strength == 0) return 0;
  const uint16_t d = ring_distance(led_pos, center);
  if (d >= radius) return 0;

  const uint8_t falloff = 255 - (uint8_t)(((uint16_t)d * 255) / radius);
  return scale8_video(scale8_video(falloff, falloff), strength);
}

static int rainbow_ring_index(int i) {
  while (i < 0) i += RAINBOW_PRESET_COUNT;
  return i % RAINBOW_PRESET_COUNT;
}

static float lerp_hue(float a, float b, float t) {
  return wrap_f(a + hue_shortest_delta(a, b) * t, 256.0f);
}

static CRGB lerp_gleam(CRGB a, CRGB b, float t) {
  return CRGB(mix_u8(a.r, b.r, t), mix_u8(a.g, b.g, t), mix_u8(a.b, b.b, t));
}

static void rainbow_update_targets_from_position() {
  if (preset_pos < 0.0f) {
    preset_pos += (float)RAINBOW_PRESET_COUNT;
  }
  preset_pos = wrap_f(preset_pos, (float)RAINBOW_PRESET_COUNT);

  const int idx_a = rainbow_ring_index((int)preset_pos);
  const int idx_b = rainbow_ring_index(idx_a + 1);
  const float t = preset_pos - floorf(preset_pos);
  const RainbowPreset& a = RAINBOW_PRESETS[idx_a];
  const RainbowPreset& b = RAINBOW_PRESETS[idx_b];

  hue_origin_target = lerp_hue((float)a.hue_origin, (float)b.hue_origin, t);
  hue_span_target = mix_f((float)a.hue_span, (float)b.hue_span, t);
  sat_base_target = mix_f((float)a.sat_base, (float)b.sat_base, t);
  sat_wave_target = mix_f((float)a.sat_wave, (float)b.sat_wave, t);
  gleam_target = lerp_gleam(a.gleam, b.gleam, t);
  speed_lo_target = mix_f(a.speed_lo, b.speed_lo, t);
  speed_hi_target = mix_f(a.speed_hi, b.speed_hi, t);
  gleam_rate_target = mix_f(a.gleam_rate, b.gleam_rate, t);
  gleam_spacing_target = mix_f(a.gleam_spacing, b.gleam_spacing, t);
  gleam_r_a_target = mix_f((float)a.gleam_r_a, (float)b.gleam_r_a, t);
  gleam_r_b_target = mix_f((float)a.gleam_r_b, (float)b.gleam_r_b, t);
  blur_lo_target = mix_f((float)a.blur_lo, (float)b.blur_lo, t);
  blur_hi_target = mix_f((float)a.blur_hi, (float)b.blur_hi, t);
  sat_step_target = mix_f((float)a.sat_step, (float)b.sat_step, t);
  preset_idx = (uint8_t)rainbow_ring_index((int)(preset_pos + 0.5f));
}

static void rainbow_smooth_params() {
  macro = lerp_toward(macro, macro_target, RAINBOW_MACRO_SMOOTH);
  hue_origin = wrap_f(lerp_hue_toward(hue_origin, hue_origin_target, RAINBOW_PARAM_SMOOTH), 256.0f);
  hue_span = lerp_toward(hue_span, hue_span_target, RAINBOW_PARAM_SMOOTH);
  sat_base = lerp_toward(sat_base, sat_base_target, RAINBOW_PARAM_SMOOTH);
  sat_wave = lerp_toward(sat_wave, sat_wave_target, RAINBOW_PARAM_SMOOTH);
  speed_lo = lerp_toward(speed_lo, speed_lo_target, RAINBOW_PARAM_SMOOTH);
  speed_hi = lerp_toward(speed_hi, speed_hi_target, RAINBOW_PARAM_SMOOTH);
  gleam_rate = lerp_toward(gleam_rate, gleam_rate_target, RAINBOW_PARAM_SMOOTH);
  gleam_spacing = lerp_toward(gleam_spacing, gleam_spacing_target, RAINBOW_PARAM_SMOOTH);
  gleam_r_a = lerp_toward(gleam_r_a, gleam_r_a_target, RAINBOW_PARAM_SMOOTH);
  gleam_r_b = lerp_toward(gleam_r_b, gleam_r_b_target, RAINBOW_PARAM_SMOOTH);
  blur_lo = lerp_toward(blur_lo, blur_lo_target, RAINBOW_PARAM_SMOOTH);
  blur_hi = lerp_toward(blur_hi, blur_hi_target, RAINBOW_PARAM_SMOOTH);
  sat_step = lerp_toward(sat_step, sat_step_target, RAINBOW_PARAM_SMOOTH);
  nblend(gleam_color, gleam_target, RAINBOW_GLEAM_BLEND);
}

static float rainbow_presence_lift() {
  return 0.86f + 0.14f * clamp_f(presence, 0.0f, 1.0f);
}

void rainbow_reset() {
  last_ms = 0;
}

void rainbow_apply_schedule(uint8_t preset_index, float energy) {
  preset_pos = (float)(preset_index % RAINBOW_PRESET_COUNT);
  rainbow_update_targets_from_position();
  macro_target = clamp_f(energy, 0.0f, 1.0f);
}

void rainbow_update(int16_t enc1_delta, int16_t enc2_delta, unsigned long now_ms) {
  if (last_ms == 0) last_ms = now_ms;

  if (enc1_delta != 0) {
    macro_target = clamp_f(macro_target + (float)enc1_delta * RAINBOW_MACRO_STEP, 0.0f, 1.0f);
  }

  if (enc2_delta != 0) {
    preset_pos += (float)enc2_delta * RAINBOW_PRESET_ENCODER_STEP;
    rainbow_update_targets_from_position();
  }

  rainbow_smooth_params();

  const float energy = ease_macro(macro);
  const float speed_target = mix_f(speed_lo, speed_hi, energy) * rainbow_presence_lift();
  speed = lerp_toward(speed, speed_target, RAINBOW_SPEED_SMOOTH);

  unsigned long dt = now_ms - last_ms;
  if (dt > RAINBOW_MAX_DT_MS) dt = RAINBOW_MAX_DT_MS;
  last_ms = now_ms;

  const float motion_dir = (mod.color_mode == 1) ? -1.0f : 1.0f;
  float motion_rate = speed * (float)dt * RAINBOW_BASE_RATE * motion_dir;
  float gleam_motion_rate = speed * gleam_rate * (float)dt * 0.006f * motion_dir;

  if (mod.color_mode == 2) {
    breathe_phase = wrap_f(breathe_phase + (float)dt * RAINBOW_BREATHE_RATE, 256.0f);
    const float breathe = (float)sin8((uint8_t)breathe_phase) / 255.0f;
    motion_rate *= 0.55f + 0.45f * breathe;
    gleam_motion_rate *= 0.35f + 0.65f * breathe;
  } else if (mod.color_mode == 3) {
    gleam_motion_rate *= RAINBOW_ORBIT_GLEAM_MUL;
  }

  motion_phase = wrap_f(motion_phase + motion_rate, 256.0f);
  gleam_phase = wrap_f(gleam_phase + gleam_motion_rate, (float)NUMPIXELS);
}

void render_rainbow() {
  const float denom = (NUMPIXELS > 1) ? (float)(NUMPIXELS - 1) : 1.0f;
  const float energy = ease_macro(macro);

  const float span_scale = mix_f(RAINBOW_SPAN_SCALE_MIN, RAINBOW_SPAN_SCALE_MAX, energy);
  const float rendered_span = clamp_f(
      hue_span * span_scale,
      RAINBOW_RENDERED_SPAN_MIN,
      RAINBOW_RENDERED_SPAN_MAX);
  const uint16_t hue_step_8 = (uint16_t)((rendered_span / denom) * 256.0f + 0.5f);

  uint16_t hue_acc = (uint16_t)((uint8_t)(wrap_f(hue_origin + motion_phase, 256.0f) + 0.5f)) << 8;
  uint8_t sat_phase = (uint8_t)(motion_phase * 0.45f);

  const uint8_t sat_base_val = (uint8_t)clamp_f(
      sat_base + mix_f(RAINBOW_SAT_BASE_MIN_OFFSET, RAINBOW_SAT_BASE_MAX_OFFSET, energy),
      0.0f,
      255.0f);
  const uint8_t sat_wave_val = (uint8_t)clamp_f(
      sat_wave * mix_f(RAINBOW_SAT_WAVE_MIN_SCALE, RAINBOW_SAT_WAVE_MAX_SCALE, energy),
      0.0f,
      RAINBOW_SAT_WAVE_MAX);

  float gleam_strength_mul = 0.85f + 0.15f * clamp_f(presence, 0.0f, 1.0f);

  if (mod.color_mode == 2) {
    const float breathe = (float)sin8((uint8_t)breathe_phase) / 255.0f;
    gleam_strength_mul *= 0.45f + 0.55f * breathe;
  }

  const uint8_t gleam_a_strength = (uint8_t)(mix_u8(RAINBOW_GLEAM_A_MIN, RAINBOW_GLEAM_A_MAX, energy) * gleam_strength_mul);
  const uint8_t gleam_b_strength = (uint8_t)(mix_u8(RAINBOW_GLEAM_B_MIN, RAINBOW_GLEAM_B_MAX, energy) * gleam_strength_mul);
  const uint8_t blur_amount = mix_u8((uint8_t)(blur_lo + 0.5f), (uint8_t)(blur_hi + 0.5f), energy);

  const uint16_t gleam_a = ring_index(gleam_phase);
  const uint16_t gleam_b = ring_index(wrap_f(
      gleam_phase + (float)NUMPIXELS * gleam_spacing,
      (float)NUMPIXELS));
  const uint8_t radius_a = (uint8_t)(gleam_r_a + 0.5f);
  const uint8_t radius_b = (uint8_t)(gleam_r_b + 0.5f);
  const uint8_t sat_step_val = (uint8_t)(sat_step + 0.5f);

  for (int i = 0; i < NUMPIXELS; i++) {
    const uint8_t hue = (uint8_t)(hue_acc >> 8);
    const uint8_t sat_wave_pos = sin8(sat_phase);
    const uint8_t sat = qsub8(qadd8(sat_base_val, sat_wave_val), scale8(sat_wave_pos, sat_wave_val));

    leds[i] = CHSV(hue, sat, RAINBOW_VAL);

    const uint8_t gleam = qadd8(
        soft_gleam((uint16_t)i, gleam_a, radius_a, gleam_a_strength),
        soft_gleam((uint16_t)i, gleam_b, radius_b, gleam_b_strength));
    leds[i] = blend(leds[i], gleam_color, gleam);

    hue_acc += hue_step_8;
    sat_phase += sat_step_val;
  }

  blur1d(leds, NUMPIXELS, blur_amount);
}
