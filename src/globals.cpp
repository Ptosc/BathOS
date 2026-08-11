#include "hardware.h"

// ===== REAL DEFINITIONS =====

CRGB leds[NUMPIXELS];
CRGB status_led[STATUS_LED_COUNT];
CRGB prev_frame[NUMPIXELS];
CRGB new_frame[NUMPIXELS];
CRGB last_displayed[NUMPIXELS];

int raw_distance = -1;
int filtered_distance = -1;

float presence = 0.0;
float sensor_presence = 0.0;

int mode = 0;
const int max_modes = 5; // off, spa, canvas, rainbow (+ unused showcase slot)
bool always_on = false;

SpaPhase spa_phase = SPA_NONE;

LightMod mod;
