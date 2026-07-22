#include "hardware.h"

static void spa_candle_colors(CRGB& bright, CRGB& dim) {
  const uint8_t hue = spa_hue_val();
  const uint8_t sat = spa_sat_val();
  bright = CHSV(hue, sat, 255);
  dim = CHSV(hue, (uint8_t)((sat * 3) / 8), 30);
}

void render_spa(SpaPhase phase) {
  if (phase == SPA_CANDLE) {
    CRGB bright;
    CRGB dim;
    spa_candle_colors(bright, dim);
    render_spa_candle(leds, bright, dim);

    const uint8_t val = spa_brightness_val();
    if (val < 255) {
      for (int i = 0; i < NUMPIXELS; i++) {
        leds[i].nscale8_video(val);
      }
    }
    return;
  }

  if (phase != SPA_BASE) {
    fill_solid(leds, NUMPIXELS, CRGB::Black);
    return;
  }

  CRGB c = spa_base_color();
  c.nscale8_video(spa_brightness_val());
  fill_solid(leds, NUMPIXELS, c);
}
