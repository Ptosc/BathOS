#include "hardware.h"
#include <WiFi.h>
#include <time.h>

#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif
#ifndef WIFI_PASS
#define WIFI_PASS ""
#endif

static bool schedule_time_ready = false;

static const CRGB SPA_COLOR_MORNING(235, 245, 255);
static const CRGB SPA_COLOR_DAY(255, 235, 215);
static const CRGB SPA_COLOR_EVENING(255, 180, 110);
static const CRGB SPA_COLOR_NIGHT(255, 120, 35);

static bool schedule_wifi_configured() {
  return WIFI_SSID[0] != '\0';
}

static void schedule_configure_timezone() {
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
}

static float schedule_lerp(float a, float b, float t) {
  if (t <= 0.0f) return a;
  if (t >= 1.0f) return b;
  return a + (b - a) * t;
}

static CRGB schedule_lerp_rgb(CRGB a, CRGB b, float t) {
  return CRGB(
    (uint8_t)(a.r + (b.r - a.r) * t + 0.5f),
    (uint8_t)(a.g + (b.g - a.g) * t + 0.5f),
    (uint8_t)(a.b + (b.b - a.b) * t + 0.5f));
}

static int night_minutes_since_21(int minutes) {
  if (minutes >= 21 * 60) return minutes - 21 * 60;
  return minutes + (24 * 60 - 21 * 60);
}

void schedule_init() {
  if (!schedule_wifi_configured()) return;

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  const unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 12000) {
    delay(200);
  }
  if (WiFi.status() != WL_CONNECTED) return;

  schedule_configure_timezone();
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  for (int i = 0; i < 20; i++) {
    struct tm t;
    if (getLocalTime(&t, 500)) {
      schedule_time_ready = true;
      return;
    }
    delay(250);
  }
}

bool schedule_time_valid() {
  return schedule_time_ready;
}

int schedule_local_hour() {
  if (!schedule_time_ready) return -1;

  struct tm t;
  if (!getLocalTime(&t, 100)) return -1;
  return t.tm_hour;
}

int schedule_local_minutes() {
  if (!schedule_time_ready) return -1;

  struct tm t;
  if (!getLocalTime(&t, 100)) return -1;
  return t.tm_hour * 60 + t.tm_min;
}

void spa_schedule_targets(int minutes, uint8_t* out_brightness, CRGB* out_color) {
  const int m05 = 5 * 60;
  const int m08 = 8 * 60;
  const int m16 = 16 * 60;
  const int m21 = 21 * 60;
  const int night_span = 8 * 60;

  float brightness;
  CRGB color;

  if (minutes >= m05 && minutes < m08) {
    const float t = (float)(minutes - m05) / (float)(m08 - m05);
    brightness = schedule_lerp(180.0f, 255.0f, t);
    color = schedule_lerp_rgb(SPA_COLOR_MORNING, SPA_COLOR_DAY, t);
  } else if (minutes >= m08 && minutes < m16) {
    brightness = 255.0f;
    color = SPA_COLOR_DAY;
  } else if (minutes >= m16 && minutes < m21) {
    const float t = (float)(minutes - m16) / (float)(m21 - m16);
    brightness = schedule_lerp(255.0f, 140.0f, t);
    color = schedule_lerp_rgb(SPA_COLOR_DAY, SPA_COLOR_EVENING, t);
  } else {
    const float t = (float)night_minutes_since_21(minutes) / (float)night_span;
    const float tn = (t > 1.0f) ? 1.0f : t;
    brightness = schedule_lerp(140.0f, 40.0f, tn);

    if (tn < 0.625f) {
      const float tc = tn / 0.625f;
      color = schedule_lerp_rgb(SPA_COLOR_EVENING, SPA_COLOR_NIGHT, tc);
    } else {
      const float tc = (tn - 0.625f) / 0.375f;
      color = schedule_lerp_rgb(SPA_COLOR_NIGHT, SPA_COLOR_MORNING, tc);
    }
  }

  *out_brightness = (uint8_t)constrain((int)(brightness + 0.5f), 0, 255);
  *out_color = color;
}

void spa_apply_schedule_defaults_now() {
  const int minutes = schedule_local_minutes();
  if (minutes < 0) {
    spa_apply_schedule_defaults(200, SPA_COLOR_DAY);
    return;
  }

  uint8_t brightness = 200;
  CRGB color = SPA_COLOR_DAY;
  spa_schedule_targets(minutes, &brightness, &color);
  spa_apply_schedule_defaults(brightness, color);
}
