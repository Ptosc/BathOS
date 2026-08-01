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

static const CRGB SPA_COLOR_MORNING(255, 215, 170);
static const CRGB SPA_COLOR_DAY(255, 175, 90);
static const CRGB SPA_COLOR_EVENING(255, 85, 12);
static const CRGB SPA_COLOR_NIGHT(255, 50, 5);
static const uint8_t NIGHT_MIN_BRIGHTNESS = 10;

static bool schedule_wifi_configured() {
  return WIFI_SSID[0] != '\0';
}

static void schedule_configure_timezone() {
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
  // Germany: CET (+1h) + CEST (+1h DST). configTime offsets are reliable on ESP32;
  // TZ alone with configTime(0,0) often leaves getLocalTime() at UTC.
  configTime(3600, 3600, "pool.ntp.org", "time.nist.gov");
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

static int night_minutes_since_22(int minutes) {
  if (minutes >= 22 * 60) return minutes - 22 * 60;
  return minutes + (24 * 60 - 22 * 60);
}

void schedule_init() {
  if (!schedule_wifi_configured()) {
#ifdef SCHEDULE_DEBUG
    Serial.println("[SCH] no WiFi credentials — schedule inactive");
#endif
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  const unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 12000) {
    delay(200);
  }
  if (WiFi.status() != WL_CONNECTED) return;

  schedule_configure_timezone();

  for (int i = 0; i < 20; i++) {
    struct tm t;
    if (getLocalTime(&t, 500)) {
      schedule_time_ready = true;
#ifdef SCHEDULE_DEBUG
      Serial.printf("[SCH] NTP ok, local %02d:%02d:%02d (CET/CEST)\n",
                    t.tm_hour, t.tm_min, t.tm_sec);
#endif
      return;
    }
    delay(250);
  }
#ifdef SCHEDULE_DEBUG
  Serial.println("[SCH] NTP failed — schedule uses Day fallback");
#endif
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

float schedule_local_minutes_f() {
  if (!schedule_time_ready) return -1.0f;

  struct tm t;
  if (!getLocalTime(&t, 100)) return -1.0f;
  return (float)(t.tm_hour * 60 + t.tm_min) + (float)t.tm_sec / 60.0f;
}

bool schedule_is_night(int minutes) {
  return minutes >= 22 * 60 || minutes < 5 * 60;
}

void spa_schedule_targets(float minutes, uint8_t* out_brightness, CRGB* out_color) {
  const int m05 = 5 * 60;
  const int m08 = 8 * 60;
  const int m16 = 16 * 60;
  const int m21 = 21 * 60;
  const int m22 = 22 * 60;
  const int night_span = 6 * 60; // 22:00 -> 04:00 color drift toward morning

  float brightness;
  CRGB color;

  if (minutes >= m05 && minutes < m08) {
    const float t = (float)(minutes - m05) / (float)(m08 - m05);
    brightness = schedule_lerp((float)NIGHT_MIN_BRIGHTNESS, 255.0f, t);
    color = schedule_lerp_rgb(SPA_COLOR_MORNING, SPA_COLOR_DAY, t);
  } else if (minutes >= m08 && minutes < m16) {
    brightness = 255.0f;
    color = SPA_COLOR_DAY;
  } else if (minutes >= m16 && minutes < m21) {
    const float t = (float)(minutes - m16) / (float)(m21 - m16);
    brightness = schedule_lerp(255.0f, 140.0f, t);
    color = schedule_lerp_rgb(SPA_COLOR_DAY, SPA_COLOR_EVENING, t);
  } else if (minutes >= m21 && minutes < m22) {
    const float t = (float)(minutes - m21) / (float)(m22 - m21);
    brightness = schedule_lerp(140.0f, (float)NIGHT_MIN_BRIGHTNESS, t);
    color = schedule_lerp_rgb(SPA_COLOR_EVENING, SPA_COLOR_NIGHT, t);
  } else {
    brightness = (float)NIGHT_MIN_BRIGHTNESS;

    const float t = (float)night_minutes_since_22((int)minutes) / (float)night_span;
    const float tn = (t > 1.0f) ? 1.0f : t;
    if (tn < 1.0f) {
      color = schedule_lerp_rgb(SPA_COLOR_NIGHT, SPA_COLOR_MORNING, tn);
    } else {
      color = SPA_COLOR_MORNING;
    }
  }

  *out_brightness = (uint8_t)constrain((int)(brightness + 0.5f), 0, 255);
  *out_color = color;
}

void spa_apply_schedule_defaults_now() {
  const float minutes = schedule_local_minutes_f();
  if (minutes < 0.0f) {
#ifdef SCHEDULE_DEBUG
    Serial.println("[SCH] no valid time — fallback Day RGB(255,235,215) bri=200");
#endif
    spa_apply_schedule_defaults(200, SPA_COLOR_DAY);
    return;
  }

  uint8_t brightness = 200;
  CRGB color = SPA_COLOR_DAY;
  spa_schedule_targets(minutes, &brightness, &color);
  spa_apply_schedule_defaults(brightness, color);
#ifdef SCHEDULE_DEBUG
  Serial.printf("[SCH] apply %02d:%02d → RGB(%u,%u,%u) bri=%u hue=%u sat=%u\n",
                (int)minutes / 60, (int)minutes % 60,
                color.r, color.g, color.b, brightness,
                spa_hue_val(), spa_sat_val());
#endif
}
