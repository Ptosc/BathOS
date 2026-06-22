#include "hardware.h"

static const uint8_t FOCUS_CANDLE_SPEED_DEFAULT = CANDLE_SPEED;
static const uint8_t FOCUS_CANDLE_SPEED_MIN = 48;
static const uint8_t FOCUS_CANDLE_SPEED_MAX = 200;

static const uint8_t FOCUS_CANDLE_INTENSITY_DEFAULT = CANDLE_INTENSITY;
static const uint8_t FOCUS_CANDLE_INTENSITY_MIN = 160;
static const uint8_t FOCUS_CANDLE_INTENSITY_MAX = 255;

static const uint8_t FOCUS_DEEP_SAT_MIN = 80;
static const uint8_t FOCUS_DEEP_SAT_MAX = 220;

static const uint8_t FOCUS_DEEP_HUE_MIN = 140;
static const uint8_t FOCUS_DEEP_HUE_MAX = 175;

static const float FOCUS_TUNING_SMOOTH = 0.055f;

static float candle_speed = FOCUS_CANDLE_SPEED_DEFAULT;
static float candle_speed_target = FOCUS_CANDLE_SPEED_DEFAULT;
static float candle_intensity = FOCUS_CANDLE_INTENSITY_DEFAULT;
static float candle_intensity_target = FOCUS_CANDLE_INTENSITY_DEFAULT;
static float deep_hue = FOCUS_DEEP_HUE;
static float deep_hue_target = FOCUS_DEEP_HUE;
static float deep_sat = FOCUS_DEEP_SAT;
static float deep_sat_target = FOCUS_DEEP_SAT;

static float clamp_f(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static float lerp_toward(float current, float target, float rate) {
  const float d = target - current;
  if (fabsf(d) < 0.04f) return target;
  return current + d * rate;
}

uint8_t focus_candle_speed_val() { return (uint8_t)(candle_speed + 0.5f); }
uint8_t focus_candle_intensity_val() { return (uint8_t)(candle_intensity + 0.5f); }
uint8_t focus_deep_hue_val() { return (uint8_t)(deep_hue + 0.5f); }
uint8_t focus_deep_sat_val() { return (uint8_t)(deep_sat + 0.5f); }

void focus_tuning_tick() {
  candle_speed = lerp_toward(candle_speed, candle_speed_target, FOCUS_TUNING_SMOOTH);
  candle_intensity = lerp_toward(candle_intensity, candle_intensity_target, FOCUS_TUNING_SMOOTH);
  deep_hue = lerp_toward(deep_hue, deep_hue_target, FOCUS_TUNING_SMOOTH);
  deep_sat = lerp_toward(deep_sat, deep_sat_target, FOCUS_TUNING_SMOOTH);
}

void focus_tuning_update(int16_t enc1_delta, int16_t enc2_delta, FocusPhase phase) {
  if (enc1_delta == 0 && enc2_delta == 0) return;

  if (phase == FOCUS_ARRIVAL) {
    if (enc1_delta != 0) {
      candle_speed_target = clamp_f(candle_speed_target + enc1_delta * 4.0f,
                                    FOCUS_CANDLE_SPEED_MIN, FOCUS_CANDLE_SPEED_MAX);
    }
    if (enc2_delta != 0) {
      candle_intensity_target = clamp_f(candle_intensity_target + enc2_delta * 4.0f,
                                        FOCUS_CANDLE_INTENSITY_MIN, FOCUS_CANDLE_INTENSITY_MAX);
    }
    return;
  }

  if (phase == FOCUS_DEEP) {
    if (enc1_delta != 0) {
      deep_sat_target = clamp_f(deep_sat_target + enc1_delta * 3.0f,
                                FOCUS_DEEP_SAT_MIN, FOCUS_DEEP_SAT_MAX);
    }
    if (enc2_delta != 0) {
      deep_hue_target = clamp_f(deep_hue_target + enc2_delta * (4.0f / 3.0f),
                                FOCUS_DEEP_HUE_MIN, FOCUS_DEEP_HUE_MAX);
    }
  }
}
