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

static bool schedule_wifi_configured() {
  return WIFI_SSID[0] != '\0';
}

static void schedule_configure_timezone() {
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
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

bool schedule_in_focus_hours(int hour) {
  return hour >= 6 && hour < 18;
}
