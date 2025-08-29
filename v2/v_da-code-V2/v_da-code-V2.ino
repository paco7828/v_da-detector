#include "Better-GPS.h"
#include "Better-RGB.h"
#include "GN1650.h"
#include "coordinates.h"

// Mode switch button
const byte MODE_SW = 0;

// RGB Led
const byte LED_R = 2;
const byte LED_G = 3;
const byte LED_B = 4;
const bool COMMON_CATHODE = false;

// GPS
const byte GPS_RX = 6;
const byte GPS_TX = -1;  // not used
const int GPS_BAUD = 9600;
double currentLat, currentLon;
int currentSpeed;

// Buzzer
const byte BUZZER = 7;
bool playedSignalFoundSound = false;
bool playedNoSignalSound = false;

// Variables for red LED blinking when searching
unsigned long lastRedBlinkTime = 0;
const unsigned long RED_BLINK_INTERVAL = 500;

// Led driver
const byte DATA_PIN = 8;
const byte CLK_PIN = 9;

// Proximity range
bool withinProxRange = false;
const int PROX_RANGE = 300;  // meters
bool justLeftProxRange = false;

// Variables for buzzer flashing sync
bool buzzerFlashState = false;
unsigned long buzzerFlashTimer = 0;
const unsigned long BUZZER_FLASH_INTERVAL = 200;

// Instances
BetterGPS gps;
BetterRGB rgb;
GN1650 ledDriver;

void setup() {
  // Start serial communication
  Serial.begin(115200);

  // Start GPS
  gps.begin(GPS_RX, GPS_TX, GPS_BAUD);

  // Initialize RGB with pins and common cathode configuration
  rgb.begin(LED_R, LED_G, LED_B, COMMON_CATHODE);
  rgb.allOff();

  // Start GN1650
  ledDriver.begin(DATA_PIN, CLK_PIN);

  // Set brightness and turn display on
  ledDriver.setBrightness(7);
  ledDriver.displayOn();

  // Play boot sound
  bootUpSound();
}

void loop() {
  // Update functions for custom classes
  gps.update();
  rgb.update();

  // GPS has fix
  if (gps.hasFix()) {
    // Play signal found
    if (!playedSignalFoundSound) {
      // When not in proximity range
      if (!withinProxRange) {
        // Green Led ON
        rgb.setDigitalGreen(true);
      }

      // Play signal not found sound
      signalSound(false);

      // Set sound variables accordingly
      playedSignalFoundSound = true;
      playedNoSignalSound = false;
    }

    // Get current GPS data
    currentLat = gps.getLatitude();
    currentLon = gps.getLongitude();
    currentSpeed = gps.getSpeedKmph();

    ledDriver.displayNumber(currentSpeed);

    // Check distance to nearest traffipax
    checkProximityToTraffipax();

    // Handle buzzer flashing when in proximity
    handleBuzzerFlashing();
  } else {
    // Play sound once when signal isn't found - only if not already played
    if (!playedNoSignalSound) {
      // Play signal found sound
      signalSound(true);

      // Set sound variables accordingly
      playedNoSignalSound = true;
      playedSignalFoundSound = false;
    } else {
      // Red Led blinking with  interval when searching for GPS signal
      unsigned long currentTime = millis();
      if (currentTime - lastRedBlinkTime >= RED_BLINK_INTERVAL) {
        static bool redLedState = false;

        if (redLedState) {
          rgb.setDigitalRed(false);
        } else {
          rgb.setDigitalRed(true);
        }

        redLedState = !redLedState;
        lastRedBlinkTime = currentTime;
      }
    }

    // When no GPS fix and in proximity range
    if (withinProxRange) {
      withinProxRange = false;
      noTone(BUZZER);
      rgb.allOff();
    }
  }

  // Small delay to prevent watchdog issues
  delay(10);
}

// Function to check distance between traffipax and you
void checkProximityToTraffipax() {
  bool traffipaxFound = false;

  for (int i = 0; i < sizeof(coordinates) / sizeof(coordinates[0]); i++) {
    // Access lat + lon from flash memory
    double lat = pgm_read_float(&coordinates[i].lat);
    double lon = pgm_read_float(&coordinates[i].lon);

    double distance = getDistance(currentLat, currentLon, lat, lon);

    if (distance <= PROX_RANGE) {
      traffipaxFound = true;

      if (!withinProxRange) {
        withinProxRange = true;  // Prevent repeated alerts

        // Start by turning leds off and begin flashing
        rgb.allOff();
        buzzerFlashTimer = millis();  // Initialize buzzer timer
      }

      // Continue red-blue flashing while in proximity
      rgb.flashRB();

      break;
    }
  }

  if (!traffipaxFound && withinProxRange) {
    // We just left the proximity zone - play exit sound
    withinProxRange = false;
    justLeftProxRange = true;

    // Stop flashing and switch back to GREEN
    rgb.allOff();
    rgb.setDigitalGreen(true);

    // Stop any buzzer activity
    noTone(BUZZER);

    // Play the exit sound - 2 second beep at 4000Hz
    tone(BUZZER, 4000, 2000);
  } else if (!traffipaxFound && !withinProxRange) {
    // Normal operation outside proximity - ensure GREEN is on
    rgb.setDigitalGreen(true);

    // Reset the flag once we're out
    justLeftProxRange = false;
  }
}

// Function to handle buzzer flashing in sync with RGB
void handleBuzzerFlashing() {
  if (withinProxRange) {
    unsigned long currentTime = millis();

    if (currentTime - buzzerFlashTimer >= BUZZER_FLASH_INTERVAL) {
      if (buzzerFlashState) {
        // Turn buzzer on (beep at 4000Hz for the flash duration)
        tone(BUZZER, 4000, BUZZER_FLASH_INTERVAL - 50);  // Slightly shorter than interval
      } else {
        // Turn buzzer off
        noTone(BUZZER);
      }

      buzzerFlashState = !buzzerFlashState;
      buzzerFlashTimer = currentTime;
    }
  }
}

// Boot beeping sound with reduced intensity
void bootUpSound() {
  int baseFreq = 800;
  for (int i = 0; i < 3; i++) {
    tone(BUZZER, baseFreq, 100);
    delay(150);
    baseFreq += 400;
  }
  noTone(BUZZER);
}

// Function to play sound based on state
void signalSound(bool isSearching) {
  int freq = isSearching ? 2000 : 4000;

  // Play two simple beeps
  tone(BUZZER, freq, 100);
  delay(150);
  tone(BUZZER, freq, 100);
  delay(150);
  noTone(BUZZER);
}

// Function to convert degrees to radians
double toRadians(double degree) {
  return degree * M_PI / 180.0;
}

// Function to calculate distance between 2 latitude and longitude points
double getDistance(double lat1, double lon1, double lat2, double lon2) {
  // Earth radius in meters
  const double R = 6371000.0;

  // Convert latitudes and longitudes to radians
  lat1 = toRadians(lat1);
  lat2 = toRadians(lat2);
  lon1 = toRadians(lon1);
  lon2 = toRadians(lon2);

  // Get distance in lat, lon
  double distance_lat = lat2 - lat1;
  double distance_lon = lon2 - lon1;

  // Square of half the chord length
  double a = sin(distance_lat / 2) * sin(distance_lat / 2) + cos(lat1) * cos(lat2) * sin(distance_lon / 2) * sin(distance_lon / 2);

  // Angular distance in radians
  double c = 2 * atan2(sqrt(a), sqrt(1 - a));

  // Distance in meters
  return R * c;
}