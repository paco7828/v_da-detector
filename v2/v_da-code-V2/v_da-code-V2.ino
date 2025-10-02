#include "Better-GPS.h"
#include "Better-RGB.h"
#include "GN1650.h"
#include "coordinates.h"

// Mode switch button
constexpr uint8_t MODE_SW = 0;

// RGB Led
constexpr uint8_t LED_R = 2;
constexpr uint8_t LED_G = 3;
constexpr uint8_t LED_B = 4;
constexpr bool LED_COMMON_CATHODE = false;

// GPS
constexpr uint8_t GPS_RX = 6;
constexpr uint8_t GPS_TX = 10;
double currentLat, currentLon;
int currentSpeed;

// Buzzer
constexpr uint8_t BUZZER = 7;
bool hadGpsFix = false;

// Led driver
constexpr uint8_t DATA_PIN = 8;
constexpr uint8_t CLK_PIN = 9;

// Proximity range
bool withinProxRange = false;
int proximityRange = 300;  // meters
bool justLeftProxRange = false;

// Variables for buzzer flashing sync
bool buzzerFlashState = false;
unsigned long buzzerFlashTimer = 0;
constexpr unsigned long BUZZER_FLASH_INTERVAL = 200;

// Loading animation
constexpr unsigned long LOADING_INTERVAL = 100;

// Earth radius in meters
constexpr double R = 6371000.0;

// Speed limit mode variables
enum SpeedMode {
  NONE = 0,
  LIMIT_50 = 1,
  LIMIT_70 = 2,
  LIMIT_90 = 3,
  LIMIT_110 = 4,
  LIMIT_130 = 5
};
SpeedMode currentSpeedMode = NONE;
const int speedLimits[6] = { 0, 50, 70, 90, 110, 130 };

// Button handling variables
bool lastButtonState = HIGH;
unsigned long buttonPressStartTime = 0;
bool buttonHoldProcessed = false;
constexpr unsigned long BUTTON_HOLD_TIME = 1500;  // 1.5 seconds for hold-to-reset

unsigned long modeDisplayEndTime = 0;
bool showingModeDisplay = false;

// Speed warning variables - made non-blocking
unsigned long lastSpeedWarningTime = 0;
bool isSpeedWarningActive = false;
unsigned long speedWarningBeepEndTime = 0;
bool speedWarningBeepActive = false;
unsigned long speedWarningLedEndTime = 0;
bool speedWarningLedActive = false;

// Non-blocking sound variables
unsigned long soundEndTime = 0;
bool soundActive = false;

// Instances
BetterGPS gps;
BetterRGB rgb;
GN1650 ledDriver;

void setup() {
  // Initialize button
  pinMode(MODE_SW, INPUT_PULLUP);

  // Start GPS
  gps.begin(GPS_RX, GPS_TX);

  // Initialize RGB with pins and common cathode configuration
  rgb.begin(LED_R, LED_G, LED_B, LED_COMMON_CATHODE);
  rgb.allOff();

  // Start GN1650
  ledDriver.begin(DATA_PIN, CLK_PIN, 8);

  // Perform startup segment test
  ledDriver.testSegments(80);

  delay(300);

  // Play boot sound
  bootUpSound();
}

void loop() {
  // Handle mode button press
  handleModeButton();

  // Handle non-blocking sounds
  if (soundActive && millis() >= soundEndTime) {
    noTone(BUZZER);
    soundActive = false;
  }

  // Update functions for custom classes
  gps.update();
  rgb.update();

  // Handle mode display timeout globally
  if (showingModeDisplay && millis() >= modeDisplayEndTime) {
    showingModeDisplay = false;
  }

  // GPS has fix
  if (gps.hasFix()) {
    // Play signal found sound if this is first fix or recovery from lost fix
    if (!hadGpsFix) {
      if (!withinProxRange) {
        rgb.setDigitalColor(false, true, false);  // Turn green on only
      }
      signalSound(false);  // Found signal sound
      hadGpsFix = true;
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
    } else {
      stopSpeedWarnings();
    }

    // Handle buzzer flashing when in proximity
    handleBuzzerFlashing();

    // Handle white LED flashing when in proximity
    handleWhiteFlashing();
  } else {
    // No GPS fix - show loading animation
    if (!showingModeDisplay) {
      // Play loading animation continuously
      ledDriver.loading(LOADING_INTERVAL);

      // Play signal lost sound if we previously had a fix
      if (hadGpsFix) {
        signalSound(true);  // "no signal" sound
        hadGpsFix = false;
      }

      // Steady red LED when no GPS fix (unless in mode display)
      if (!showingModeDisplay) {
        rgb.setDigitalColor(true, false, false);
      }

      // Reset state when no GPS fix but still in proximity range
      if (withinProxRange) {
        withinProxRange = false;
        noTone(BUZZER);
        rgb.allOff();
      }

      // Stop speed warnings when no GPS
      stopSpeedWarnings();
    }
  }
}

// Handle non-blocking white LED flashing
void handleWhiteFlashing() {
  if (withinProxRange) {
    // White LED state follows the buzzer flash state
    if (buzzerFlashState) {
      rgb.setDigitalColor(true, true, true);  // White on
    } else {
      rgb.allOff();  // Off
    }
  }
}

// Handle mode button press with debouncing and hold-to-reset
void handleModeButton() {
  bool currentButtonState = digitalRead(MODE_SW);

  // Button pressed (falling edge detection)
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    buttonPressStartTime = millis();
    buttonHoldProcessed = false;
  }

  // Button held down - check for hold-to-reset
  if (currentButtonState == LOW && !buttonHoldProcessed) {
    if (millis() - buttonPressStartTime >= BUTTON_HOLD_TIME) {
      // Hold detected - reset to NONE mode
      currentSpeedMode = NONE;
      showModeIndication();
      buttonHoldProcessed = true;

      // Stop any active speed warnings
      stopSpeedWarnings();
    }
  }

  // Button released (rising edge detection)
  if (lastButtonState == LOW && currentButtonState == HIGH) {
    // Only process short press if hold wasn't already processed
    if (!buttonHoldProcessed) {
      // Short press - cycle through modes
      currentSpeedMode = (SpeedMode)((currentSpeedMode + 1) % 6);
      showModeIndication();

      // Stop any active speed warnings
      stopSpeedWarnings();
    }
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

  // Turn green LED on and play buzzer tone
  rgb.allOff();
  delay(200);
  rgb.setDigitalColor(false, true, false);
  tone(BUZZER, 3700, 200);

  // Set timers for when to turn off LED and stop sound
  soundActive = true;
  soundEndTime = millis() + 200;
}

// Helper function to restore normal LED state
void restoreNormalLedState() {
  if (gps.hasFix() && !withinProxRange && !showingModeDisplay && !isSpeedWarningActive) {
    rgb.setDigitalColor(false, true, false);  // Green on for GPS signal
  }
}

// Stop speed warnings and restore normal LED state
void stopSpeedWarnings() {
  if (isSpeedWarningActive) {
    isSpeedWarningActive = false;
    speedWarningBeepActive = false;
    speedWarningLedActive = false;
    noTone(BUZZER);

    // Restore normal LED state after stopping warnings
    restoreNormalLedState();
  }
}

void handleSpeedLimitWarning() {
  // If no speed limit mode is set, do nothing
  if (currentSpeedMode == NONE) {
    stopSpeedWarnings();
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
      warningInterval = 250;  // 0.25 second interval
    } else {
      // 15+ km/h over: fast beeping
      warningInterval = 150;  // 0.15 second interval
    }

    // Handle beeping timing
    unsigned long currentTime = millis();
    if (currentTime - lastSpeedWarningTime >= warningInterval) {
      if (!speedWarningBeepActive) {
        tone(BUZZER, 3700, 100);  // Short beep
        speedWarningBeepActive = true;
        speedWarningBeepEndTime = currentTime + 100;

        // Start white LED flashing
        rgb.allOff();                           // Clear all LEDs first
        rgb.setDigitalColor(true, true, true);  // White (all LEDs on)
        speedWarningLedActive = true;
        speedWarningLedEndTime = currentTime + (warningInterval / 2);

        lastSpeedWarningTime = currentTime;
      }
    }

    // Handle LED state changes
    if (speedWarningLedActive) {
      if (currentTime >= speedWarningLedEndTime) {
        if (currentTime < lastSpeedWarningTime + warningInterval) {
          // Turn off LED for second half (flash off)
          rgb.allOff();
          speedWarningLedEndTime = lastSpeedWarningTime + warningInterval;
        } else {
          // End of cycle - turn off LED and prepare for next cycle
          rgb.allOff();
          speedWarningLedActive = false;
        }
      }
    }

    // Reset beep flag when beep ends
    if (speedWarningBeepActive && currentTime >= speedWarningBeepEndTime) {
      speedWarningBeepActive = false;
    }

  } else {
    // Under speed limit - stop warnings
    stopSpeedWarnings();
  }
}

// Function to check distance between traffipax and you
void checkProximityToTraffipax() {
  // Adjust proximity range based on speed
  if (currentSpeed <= 50) {
    // below or at 50km/h
    proximityRange = 300;  // meters
  } else if (currentSpeed <= 70) {
    // between 51 and 70km/h
    proximityRange = 400;  // meters
  } else {
    // above 70km/h
    proximityRange = 500;  // meters
  }

  bool traffipaxFound = false;

  for (int i = 0; i < sizeof(coordinates) / sizeof(coordinates[0]); i++) {
    // Access lat + lon from flash memory
    double lat = coordinates[i].lat;
    double lon = coordinates[i].lon;

    double distance = getDistance(currentLat, currentLon, lat, lon);

    if (distance <= proximityRange) {
      traffipaxFound = true;

      if (!withinProxRange) {
        withinProxRange = true;  // Prevent repeated alerts

        // Stop any speed warnings when entering traffipax proximity
        stopSpeedWarnings();

        // Start by turning leds off and begin flashing
        rgb.allOff();
        buzzerFlashTimer = millis();                    // Initialize buzzer timer
        rgb.startWhiteFlashing(BUZZER_FLASH_INTERVAL);  // Start non-blocking white flash
      }

      break;
    }
  }

  if (!traffipaxFound && withinProxRange) {
    // We just left the proximity zone - play exit sound
    withinProxRange = false;
    justLeftProxRange = true;

    // Stop flashing and switch back to GREEN
    rgb.stopWhiteFlashing();  // Stop white flashing
    rgb.setDigitalColor(false, true, false);

    // Stop any buzzer activity
    noTone(BUZZER);

    // Play the exit sound - 2 second beep at 3700Hz
    tone(BUZZER, 3700, 2000);
  } else if (!traffipaxFound && !withinProxRange) {
    // Normal operation outside proximity - ensure GREEN is on (unless speed warning is active)
    if (!isSpeedWarningActive && gps.hasFix() && !showingModeDisplay) {
      rgb.setDigitalColor(false, true, false);
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
        // Turn buzzer on and white LED on
        tone(BUZZER, 3700, BUZZER_FLASH_INTERVAL);
        rgb.setDigitalColor(true, true, true);  // White on
      } else {
        // Turn buzzer off and white LED off
        noTone(BUZZER);
        rgb.allOff();  // Off
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