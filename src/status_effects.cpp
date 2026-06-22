#include "hardware.h"

static const unsigned long ZONE_FADE_MS = 2000;
static const unsigned long SHUTDOWN_TOTAL_MS = 650;
static const unsigned long SHUTDOWN_ACK_MS = 80;
static const unsigned long SHUTDOWN_STAGGER_MS = 120;
static const unsigned long SHUTDOWN_FADE_MS = 230;
static const CRGB SHUTDOWN_COLOR = CHSV(0, 200, 255);

struct ZoneEffect {
  CRGB color = CRGB::Black;
  unsigned long triggered_ms = 0;
  bool active = false;
};

static ZoneEffect zones[3];
static bool shutdown_active = false;
static unsigned long shutdown_start_ms = 0;

static const int SHUTDOWN_PAIRS[3][2] = {{0, 5}, {1, 4}, {2, 3}};

// Ease-out cubic: stays bright longer, then glows out slowly at the end.
static uint8_t fade_intensity(unsigned long elapsed) {
  if (elapsed >= ZONE_FADE_MS) return 0;
  float t = (float)elapsed / (float)ZONE_FADE_MS;
  float rem = 1.0f - t;
  float eased = rem * rem * rem;
  return (uint8_t)(eased * 255.0f + 0.5f);
}

static uint8_t shutdown_ease_out(unsigned long elapsed, unsigned long duration) {
  if (elapsed >= duration) return 0;
  const float t = (float)elapsed / (float)duration;
  const float rem = 1.0f - t;
  return (uint8_t)(rem * rem * rem * 255.0f + 0.5f);
}

static uint8_t shutdown_pair_brightness(unsigned long elapsed, int pair) {
  const unsigned long fade_start = SHUTDOWN_ACK_MS + (unsigned long)pair * SHUTDOWN_STAGGER_MS;
  uint8_t peak = 200;

  if (pair == 2 && elapsed < SHUTDOWN_ACK_MS) {
    return (uint8_t)(160 + (elapsed * 95) / SHUTDOWN_ACK_MS);
  }
  if (pair == 2 && elapsed < fade_start) {
    peak = 255;
  }

  if (elapsed < fade_start) return peak;
  return shutdown_ease_out(elapsed - fade_start, SHUTDOWN_FADE_MS);
}

static void render_status_shutdown(unsigned long elapsed) {
  for (int pair = 0; pair < 3; pair++) {
    const uint8_t brightness = shutdown_pair_brightness(elapsed, pair);
    if (brightness == 0) continue;

    for (int j = 0; j < 2; j++) {
      const int idx = SHUTDOWN_PAIRS[pair][j];
      uint8_t edge = (idx == 0 || idx == STATUS_LED_COUNT - 1) ? 180 : 255;
      CRGB c = SHUTDOWN_COLOR;
      c.nscale8_video(brightness);
      c.nscale8_video(edge);
      status_led[idx] = c;
    }
  }
}

void trigger_status_shutdown() {
  shutdown_start_ms = millis();
  shutdown_active = true;
  for (int z = 0; z < 3; z++) zones[z].active = false;
}

void trigger_zone(uint8_t zone, CRGB color) {
  if (zone >= 3) return;
  zones[zone].color = color;
  zones[zone].triggered_ms = millis();
  zones[zone].active = true;
}

void update_status() {
  const unsigned long now = millis();

  for (int i = 0; i < STATUS_LED_COUNT; i++) {
    status_led[i] = CRGB::Black;
  }

  if (shutdown_active) {
    const unsigned long elapsed = now - shutdown_start_ms;
    if (elapsed >= SHUTDOWN_TOTAL_MS) {
      shutdown_active = false;
      return;
    }
    render_status_shutdown(elapsed);
    return;
  }

  for (int z = 0; z < 3; z++) {
    if (!zones[z].active) continue;

    unsigned long elapsed = now - zones[z].triggered_ms;
    if (elapsed >= ZONE_FADE_MS) {
      zones[z].active = false;
      continue;
    }

    // Ease-out fade: full brightness -> off over ZONE_FADE_MS
    uint8_t intensity = fade_intensity(elapsed);

    int start = z * 2;
    int end = start + 2;

    for (int i = start; i < end; i++) {
      uint8_t edge_falloff = (i == start || i == end - 1) ? 180 : 255;
      CRGB c = zones[z].color;
      c.nscale8_video(intensity);
      c.nscale8_video(edge_falloff);
      status_led[i] = c;
    }
  }
}
