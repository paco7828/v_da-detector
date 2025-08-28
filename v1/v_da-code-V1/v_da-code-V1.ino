#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include "coordinates.h"
#include <avr/pgmspace.h>

// Connections
const byte ledG = 4;     // D2 (GPIO4)
const byte ledR = 5;     // D1 (GPIO5)
const byte gpsRx = 12;   // D6 (GPIO12)
const byte gpsTx = 13;   // D7 (GPIO13)
const byte buzzer = 14;  // D5 (GPIO 14)

// Instance creation
SoftwareSerial gpsSerial(gpsTx, gpsRx);
TinyGPSPlus gps;

// GPS data
#define GPS_BAUD 9600
double currentLat, currentLon;
int currentSpeed;

// Buzzer
bool playedSignalFoundSound = false;
bool playedNoSignalSound = false;

// Presence in proximity range
bool withinProxRange = false;
#define PROX_RANGE 300  // meters
bool justLeftProxRange = false;

void setup() {
  // Set output pins
  pinMode(buzzer, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledR, OUTPUT);

  // Turn red led on at boot
  digitalWrite(ledG, HIGH);
  digitalWrite(ledR, LOW);

  gpsSerial.begin(GPS_BAUD);

  // Play boot sound with reduced intensity
  bootUpSound();
}

void loop() {
  // GPS is available
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // Process GPS data every 500ms
  static unsigned long lastProcessTime = 0;
  if (millis() - lastProcessTime > 500) {
    lastProcessTime = millis();

    // Found satellite(s) -> valid data
    if (gps.location.isValid()) {
      // Play sound once when signal is found - only if not already played
      if (!playedSignalFoundSound) {
        // Green led on when not in proximity range
        if (!withinProxRange) {
          digitalWrite(ledG, LOW);
          digitalWrite(ledR, HIGH);
        }

        signalSound(false);

        playedSignalFoundSound = true;
        playedNoSignalSound = false;
      }

      // Get current GPS data
      currentLat = gps.location.lat();
      currentLon = gps.location.lng();
      currentSpeed = gps.speed.kmph();

      // Check distance to nearest traffipax
      checkProximityToTraffipax();
    }
    // If no satellites are found
    else {
      // Play sound once when signal isn't found - only if not already played
      if (!playedNoSignalSound) {
        // First update LED status
        digitalWrite(ledG, HIGH);
        digitalWrite(ledR, LOW);

        // Then play sound with slight delay
        delay(100);
        signalSound(true);

        playedNoSignalSound = true;
        playedSignalFoundSound = false;
      } else {
        // Turn red led on if not already on
        digitalWrite(ledG, HIGH);
        digitalWrite(ledR, LOW);
      }
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

        // Change LED state - RED when in proximity
        digitalWrite(ledG, HIGH);
        digitalWrite(ledR, LOW);
        delay(100);

        alertWithStaggeredBeepAndBlink(5);

        // Keep RED on after alert
        digitalWrite(ledR, LOW);
        digitalWrite(ledG, HIGH);
      } else {
        // Ensure RED stays on while in proximity
        digitalWrite(ledR, LOW);
        digitalWrite(ledG, HIGH);
      }
      break;
    }
  }

  if (!traffipaxFound && withinProxRange) {
    // We just left the proximity zone - play exit sound
    withinProxRange = false;
    justLeftProxRange = true;

    // Switch back to GREEN
    digitalWrite(ledR, HIGH);
    digitalWrite(ledG, LOW);

    // Play the exit sound - 2 second beep at 4000Hz
    tone(buzzer, 4000, 2000);

  } else if (!traffipaxFound && !withinProxRange) {
    // Normal operation outside proximity - ensure GREEN is on
    digitalWrite(ledR, HIGH);
    digitalWrite(ledG, LOW);

    // Reset the flag once we're out
    justLeftProxRange = false;
  }
}

// Function for staggered beeping and LED blinking
void alertWithStaggeredBeepAndBlink(byte n) {
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < n; j++) {
      // First turn on LED
      digitalWrite(ledR, HIGH);
      delay(50);  // Brief delay

      // Then start tone
      tone(buzzer, 4000, 150);
      digitalWrite(ledR, LOW);
      delay(200);

      // Turn off Buzzer
      noTone(buzzer);
    }
    delay(400);
  }
}

// Boot beeping sound with reduced intensity
void bootUpSound() {
  int baseFreq = 800;
  for (int i = 0; i < 3; i++) {
    tone(buzzer, baseFreq, 100);
    delay(150);
    baseFreq += 400;
  }
  noTone(buzzer);
}

// Function to play sound based on state - simplified
void signalSound(bool isSearching) {
  int freq = isSearching ? 2000 : 4000;

  // Play two simple beeps
  tone(buzzer, freq, 100);
  delay(150);
  tone(buzzer, freq, 100);
  delay(150);
  noTone(buzzer);
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