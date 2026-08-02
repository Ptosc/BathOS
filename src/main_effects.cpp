#include "hardware.h"

void render_off() {
  fill_solid(leds, NUMPIXELS, CRGB::Black);
}

void render_visual_state_to(CRGB* buf, const VisualState& vs) {
  if (!vs.active || vs.mode == MODE_OFF) {
    fill_solid(buf, NUMPIXELS, CRGB::Black);
    return;
  }

  switch (vs.mode) {
    case MODE_SPA:
      render_spa(vs.spa_phase);
      break;
    case MODE_CANVAS:
      render_canvas(millis());
      break;
    case MODE_RAINBOW:
      render_rainbow();
      break;
    default:
      render_off();
      break;
  }

  for (int i = 0; i < NUMPIXELS; i++) buf[i] = leds[i];
}
