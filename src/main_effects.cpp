#include "hardware.h"

void render_off() {
  fill_solid(leds, NUMPIXELS, CRGB::Black);
}

void render_focus(const LightMod& m, FocusPhase phase) {
  (void)m;
  static FocusPhase rendered_phase = FOCUS_NONE;

  if (phase == FOCUS_ARRIVAL) {
    rendered_phase = FOCUS_ARRIVAL;
    render_focus_candle(leds, FOCUS_CANDLE_BRIGHT, FOCUS_CANDLE_DIM);
    return;
  }

  if (rendered_phase == FOCUS_ARRIVAL) {
    focus_candle_reset();
  }
  rendered_phase = phase;

  fill_solid(leds, NUMPIXELS, CHSV(focus_deep_hue_val(), focus_deep_sat_val(), FOCUS_DEEP_VAL));
}

void render_visual_state_to(CRGB* buf, const VisualState& vs) {
  if (!vs.active || vs.mode == MODE_OFF) {
    fill_solid(buf, NUMPIXELS, CRGB::Black);
    return;
  }

  switch (vs.mode) {
    case MODE_FOCUS:
      render_focus(mod, vs.focus_phase);
      break;
    case MODE_SHOWCASE:
      render_showcase();
      break;
    case MODE_CANVAS:
      render_canvas(millis());
      break;
    default:
      render_off();
      break;
  }

  for (int i = 0; i < NUMPIXELS; i++) buf[i] = leds[i];
}
