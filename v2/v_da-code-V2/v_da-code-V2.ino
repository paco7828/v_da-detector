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

// Speed limit mode variables
enum SpeedMode {
  NONE = 0,
  LIMIT_50 = 1,
  LIMIT_90 = 2,
  LIMIT_110 = 3,
  LIMIT_130 = 4
};

SpeedMode currentSpeedMode = NONE;
int speedLimits[5] = { 0, 50, 90, 110, 130 };
bool buttonPressed = false;
bool lastButtonState = HIGH;
unsigned long modeDisplayEndTime = 0;
bool showingModeDisplay = false;

// Speed warning variables
unsigned long lastSpeedWarningTime = 0;
bool isSpeedWarningActive = false;

// Instances
BetterGPS gps;
BetterRGB rgb;
GN1650 ledDriver;

void setup() {
  // Initialize button
  pinMode(MODE_SW, INPUT_PULLUP);

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
  // Handle mode button press
  handleModeButton();

  // Update functions for custom classes
  gps.update();
  rgb.update();

  // Handle mode display timeout globally
  if (showingModeDisplay && millis() >= modeDisplayEndTime) {
    showingModeDisplay = false;

    // If GPS still searching, restart red blinking immediately
    if (!gps.hasFix()) {
      lastRedBlinkTime = millis();  // reset blink timer
      rgb.setDigitalRed(true);      // turn red on right away
    }
  }

  // GPS has fix
  if (gps.hasFix()) {
    // Play signal found once
    if (!playedSignalFoundSound) {
      if (!withinProxRange) {
        rgb.setDigitalRed(false);   // Red off to prevent both colors present
        rgb.setDigitalGreen(true);  // Green LED ON
      }
      signalSound(false);  // Found signal sound
      playedSignalFoundSound = true;
      playedNoSignalSound = false;
    }

    // Get current GPS data
    currentLat = gps.getLatitude();
    currentLon = gps.getLongitude();
    currentSpeed = gps.getSpeedKmph();

    // Display speed unless showing mode
    if (!showingModeDisplay) {
      ledDriver.displayNumber(currentSpeed);
    }

    // Check distance to nearest traffipax
    checkProximityToTraffipax();

    // Handle speed limit warnings if not in proximity
    if (!withinProxRange) {
      handleSpeedLimitWarning();
    } else if (isSpeedWarningActive) {
      isSpeedWarningActive = false;
      noTone(BUZZER);
    }

    // Handle buzzer flashing when in proximity
    handleBuzzerFlashing();
  } else {
    if (!playedNoSignalSound) {
      signalSound(true);  // "no signal" sound once
      playedNoSignalSound = true;
      playedSignalFoundSound = false;
    } else {
      // Red LED blinking (unless mode display is active)
      if (!showingModeDisplay) {
        unsigned long currentTime = millis();
        if (currentTime - lastRedBlinkTime >= RED_BLINK_INTERVAL) {
          static bool redLedState = false;
          rgb.setDigitalRed(redLedState);
          redLedState = !redLedState;
          lastRedBlinkTime = currentTime;
        }
      }
    }

    // Reset state when no GPS fix but still in proximity range
    if (withinProxRange) {
      withinProxRange = false;
      noTone(BUZZER);
      rgb.allOff();
    }

    // Stop speed warnings when no GPS
    isSpeedWarningActive = false;
    noTone(BUZZER);
  }

  // Small delay to prevent watchdog issues
  delay(10);
}

// Handle mode button press with debouncing
void handleModeButton() {
  bool currentButtonState = digitalRead(MODE_SW);

  // Button pressed (falling edge detection)
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    // Cycle through modes
    currentSpeedMode = (SpeedMode)((currentSpeedMode + 1) % 5);

    // Show mode indication
    showModeIndication();

    // Stop any active speed warnings
    isSpeedWarningActive = false;
    noTone(BUZZER);
  }

  lastButtonState = currentButtonState;
}

// Show mode indication with LED display and sound
void showModeIndication() {
  // Display the speed limit for 3 seconds
  if (currentSpeedMode == NONE) {
    ledDriver.displayNumber(0);  // Show 0 for no limit
  } else {
    ledDriver.displayNumber(speedLimits[currentSpeedMode]);
  }

  showingModeDisplay = true;
  modeDisplayEndTime = millis() + 3000;  // Show for 3 seconds

  // Turn off all LEDs first to prevent color mixing
  rgb.allOff();

  // Green blink for all modes
  rgb.keepDigitalGreenFor(500);  // Green blink for 0.5 second

  // Reset the red blink timer to prevent immediate red LED activation
  lastRedBlinkTime = millis() + 600;  // Delay red blinking restart by 600ms

  // Play mode sound
  playModeSound();
}

// Play sound for mode selection
void playModeSound() {
  // Single beep at 2000Hz for all mode switches
  tone(BUZZER, 3700, 200);
  delay(250);
  noTone(BUZZER);
}

// Handle speed limit warnings
void handleSpeedLimitWarning() {
  // If no speed limit mode is set, do nothing
  if (currentSpeedMode == NONE) {
    isSpeedWarningActive = false;
    return;
  }

  int speedLimit = speedLimits[currentSpeedMode];
  int speedDifference = currentSpeed - speedLimit;

  if (speedDifference > 0) {
    // Over speed limit
    if (!isSpeedWarningActive) {
      isSpeedWarningActive = true;
      lastSpeedWarningTime = millis();
    }

    // Calculate warning intensity based on speed difference
    unsigned long warningInterval;

    if (speedDifference <= 5) {
      // 1-5 km/h over: slow beeping
      warningInterval = 500;  // 0.5 second interval
    } else if (speedDifference <= 15) {
      // 6-15 km/h over: medium beeping
      warningInterval = 200;  // 0.25 second interval
    } else {
      // 15+ km/h over: fast beeping
      warningInterval = 100;  // 0.1 second interval
    }

    // Handle beeping timing
    unsigned long currentTime = millis();
    if (currentTime - lastSpeedWarningTime >= warningInterval) {
      tone(BUZZER, 3700, 100);  // Short beep

      // Flash red LED briefly to indicate speed warning
      rgb.setDigitalColor(true, false, false);
      delay(warningInterval / 2);
      rgb.setDigitalColor(false, false, true);
      delay(warningInterval / 2);
      rgb.allOff();

      lastSpeedWarningTime = currentTime;
    }
  } else {
    // Under speed limit - stop warnings
    if (isSpeedWarningActive) {
      isSpeedWarningActive = false;
      noTone(BUZZER);
    }
  }
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

        // Stop any speed warnings when entering traffipax proximity
        isSpeedWarningActive = false;
        noTone(BUZZER);

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
    tone(BUZZER, 3700, 2000);
  } else if (!traffipaxFound && !withinProxRange) {
    // Normal operation outside proximity - ensure GREEN is on (unless speed warning is active)
    if (!isSpeedWarningActive) {
      rgb.setDigitalGreen(true);
    }

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
        tone(BUZZER, 3700, BUZZER_FLASH_INTERVAL - 50);  // Slightly shorter than interval
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
  int baseFreq = 3300;
  for (int i = 0; i < 3; i++) {
    tone(BUZZER, baseFreq, 100);
    delay(150);
    baseFreq += 200;
  }
  noTone(BUZZER);
}

// Function to play sound based on state
void signalSound(bool isSearching) {
  int freq = isSearching ? 2500 : 4000;

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