#include "hardware.h"

static const int SHOWCASE_PALETTE_COUNT = 6;
static const float SHOWCASE_SPEED_MIN = 0.3f;
static const float SHOWCASE_SPEED_MAX = 3.0f;
static const float SHOWCASE_SPEED_DEFAULT = 1.2f;
static const float SHOWCASE_BASE_RATE = 0.045f;

static const uint8_t SHOWCASE_FADE_SLOW = 18;
static const uint8_t SHOWCASE_FADE_FAST = 40;
static const uint8_t SHOWCASE_BLUR_AMOUNT = 64;
static const unsigned long SHOWCASE_MAX_DT_MS = 32;

static const int SHOWCASE_TRAIL_LEN = 12;
static const int SHOWCASE_COLLISION_DIST = 8;
static const unsigned long SHOWCASE_COLLISION_COOLDOWN_MS = 400;

struct ShowcasePalette {
  uint8_t hue_a;
  uint8_t hue_b;
};

static const ShowcasePalette SHOWCASE_PALETTES[] = {
  {192, 160},
  {0, 16},
  {128, 150},
  {192, 42},
  {96, 64},
  {240, 0},
};

static const int8_t HEAD_OFFSETS[] = {-2, -1, 0, 1, 2};
static const uint8_t HEAD_VAL[] = {90, 160, 255, 200, 75};
static const uint8_t HEAD_SAT[] = {255, 255, 48, 255, 240};
static const int HEAD_LED_COUNT = 5;

static float speed = SHOWCASE_SPEED_DEFAULT;
static float speed_target = SHOWCASE_SPEED_DEFAULT;
static uint8_t palette_idx = 0;
static float pos_left = 0.0f;
static float pos_right = 0.0f;
static unsigned long last_ms = 0;
static uint8_t collision_flash = 0;
static unsigned long last_collision_ms = 0;

static float showcase_clamp_speed(float v) {
  if (v < SHOWCASE_SPEED_MIN) return SHOWCASE_SPEED_MIN;
  if (v > SHOWCASE_SPEED_MAX) return SHOWCASE_SPEED_MAX;
  return v;
}

static uint8_t showcase_fade_amount(float s) {
  const float t = (s - SHOWCASE_SPEED_MIN) / (SHOWCASE_SPEED_MAX - SHOWCASE_SPEED_MIN);
  return (uint8_t)(SHOWCASE_FADE_SLOW + t * (float)(SHOWCASE_FADE_FAST - SHOWCASE_FADE_SLOW));
}

static int showcase_wrap_index(int index) {
  while (index < 0) index += NUMPIXELS;
  while (index >= NUMPIXELS) index -= NUMPIXELS;
  return index;
}

static int showcase_comet_distance(float a, float b) {
  float d = fabsf(a - b);
  if (d > (float)NUMPIXELS * 0.5f) d = (float)NUMPIXELS - d;
  return (int)(d + 0.5f);
}

static float showcase_wrap_pos(float pos) {
  pos = fmodf(pos, (float)NUMPIXELS);
  if (pos < 0.0f) pos += (float)NUMPIXELS;
  return pos;
}

static void showcase_add_pixel(int index, CRGB color) {
  if (index < 0 || index >= NUMPIXELS) return;
  leds[index] += color;
}

static void showcase_add_pixel_subpixel(float pos, CRGB color) {
  pos = showcase_wrap_pos(pos);
  const int base = (int)pos;
  const float frac = pos - (float)base;

  if (frac < 0.002f) {
    showcase_add_pixel(base, color);
    return;
  }

  const uint8_t w1 = (uint8_t)(frac * 255.0f + 0.5f);
  const uint8_t w0 = 255 - w1;

  CRGB c0 = color;
  c0.nscale8_video(w0);
  showcase_add_pixel(base, c0);

  CRGB c1 = color;
  c1.nscale8_video(w1);
  showcase_add_pixel(showcase_wrap_index(base + 1), c1);
}

static void showcase_draw_trail(float center, uint8_t hue, int behind_sign) {
  const uint8_t trail_scale = (uint8_t)constrain(
      (int)(SHOWCASE_TRAIL_LEN - (speed - SHOWCASE_SPEED_MIN) * 3.0f), 6, SHOWCASE_TRAIL_LEN);

  for (int i = 1; i <= trail_scale; i++) {
    const float trail_pos = center - behind_sign * (float)i;
    const uint8_t val = (uint8_t)(200 - i * (180 / SHOWCASE_TRAIL_LEN));
    const uint8_t sat = (uint8_t)constrain(255 - i * 8, 180, 255);
    showcase_add_pixel_subpixel(trail_pos, CHSV(hue, sat, val));
  }
}

static void showcase_draw_head(float center, uint8_t hue) {
  for (int i = 0; i < HEAD_LED_COUNT; i++) {
    const CRGB c = CHSV(hue, HEAD_SAT[i], HEAD_VAL[i]);
    showcase_add_pixel_subpixel(center + (float)HEAD_OFFSETS[i], c);
  }
}

static void showcase_apply_blur() {
  blur1d(leds, NUMPIXELS, SHOWCASE_BLUR_AMOUNT);
}

static void showcase_update_collision(float left, float right, unsigned long now_ms) {
  const int dist = showcase_comet_distance(left, right);
  if (dist < SHOWCASE_COLLISION_DIST &&
      now_ms - last_collision_ms >= SHOWCASE_COLLISION_COOLDOWN_MS) {
    collision_flash = 255;
    last_collision_ms = now_ms;
  }
  collision_flash = qsub8(collision_flash, 6);
}

static int showcase_collision_center(int a, int b) {
  const int forward = (b - a + NUMPIXELS) % NUMPIXELS;
  if (forward <= NUMPIXELS / 2) {
    return showcase_wrap_index(a + forward / 2);
  }
  const int backward = (a - b + NUMPIXELS) % NUMPIXELS;
  return showcase_wrap_index(b + backward / 2);
}

static void showcase_draw_collision_flash(int head_left, int head_right, uint8_t hue) {
  if (collision_flash == 0) return;

  const int mid = showcase_collision_center(head_left, head_right);
  const CRGB white = CHSV(hue, 40, collision_flash);
  for (int i = -4; i <= 4; i++) {
    CRGB c = white;
    c.nscale8_video((uint8_t)(255 - abs(i) * 35));
    showcase_add_pixel(showcase_wrap_index(mid + i), c);
  }
}

void showcase_reset() {
  speed = SHOWCASE_SPEED_DEFAULT;
  speed_target = SHOWCASE_SPEED_DEFAULT;
  palette_idx = 0;
  pos_left = 0.0f;
  pos_right = (float)(NUMPIXELS - 1);
  last_ms = 0;
  collision_flash = 0;
  last_collision_ms = 0;
}

void showcase_update(int16_t enc1_delta, int16_t enc2_delta, unsigned long now_ms) {
  if (last_ms == 0) last_ms = now_ms;

  if (enc1_delta != 0) {
    speed_target = showcase_clamp_speed(speed_target + enc1_delta * 0.08f);
  }

  if (enc2_delta != 0) {
    int next = (int)palette_idx + ((enc2_delta > 0) ? 1 : -1);
    while (next < 0) next += SHOWCASE_PALETTE_COUNT;
    palette_idx = (uint8_t)(next % SHOWCASE_PALETTE_COUNT);
  }

  speed += (speed_target - speed) * 0.2f;

  unsigned long dt = now_ms - last_ms;
  if (dt > SHOWCASE_MAX_DT_MS) dt = SHOWCASE_MAX_DT_MS;
  last_ms = now_ms;
  const float step = speed * (float)dt * SHOWCASE_BASE_RATE;

  pos_left += step;
  pos_right -= step;

  while (pos_left >= (float)NUMPIXELS) pos_left -= (float)NUMPIXELS;
  while (pos_left < 0.0f) pos_left += (float)NUMPIXELS;
  while (pos_right >= (float)NUMPIXELS) pos_right -= (float)NUMPIXELS;
  while (pos_right < 0.0f) pos_right += (float)NUMPIXELS;

  showcase_update_collision(pos_left, pos_right, now_ms);
}

void render_showcase() {
  const ShowcasePalette& palette = SHOWCASE_PALETTES[palette_idx];
  const int head_left = (int)(pos_left + 0.5f);
  const int head_right = (int)(pos_right + 0.5f);

  fadeToBlackBy(leds, NUMPIXELS, showcase_fade_amount(speed));

  showcase_draw_trail(pos_left, palette.hue_a, 1);
  showcase_draw_trail(pos_right, palette.hue_b, -1);

  showcase_apply_blur();

  showcase_draw_collision_flash(head_left, head_right, palette.hue_a);

  showcase_draw_head(pos_left, palette.hue_a);
  showcase_draw_head(pos_right, palette.hue_b);
}
