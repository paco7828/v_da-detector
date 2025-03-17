#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include "coordinates.h"

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

void setup() {
  // Set output pins
  pinMode(buzzer, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledR, OUTPUT);

  // Turn leds off at boot
  digitalWrite(ledG, HIGH);
  digitalWrite(ledR, HIGH);

  // Add a small delay before starting GPS serial
  delay(1000);
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
        // First update LED status
        digitalWrite(ledG, LOW);
        digitalWrite(ledR, HIGH);

        // Then play sound with slight delay
        delay(100);
        signalSound(false);

        playedSignalFoundSound = true;
        playedNoSignalSound = false;
      } else {
        // Turn green led on if not already on
        digitalWrite(ledG, LOW);
        digitalWrite(ledR, HIGH);
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
    double distance = getDistance(currentLat, currentLon, coordinates[i].lat, coordinates[i].lon);

    if (distance <= PROX_RANGE) {
      traffipaxFound = true;

      if (!withinProxRange) {
        withinProxRange = true;  // Prevent repeated alerts

        // First, change LED state
        digitalWrite(ledG, HIGH);
        digitalWrite(ledR, LOW);
        delay(100);  // Brief delay

        // Then handle alert sequence with staggered timing
        alertWithStaggeredBeepAndBlink(5);

        // Restore LEDs: Red OFF, Green ON
        digitalWrite(ledR, HIGH);
        digitalWrite(ledG, LOW);
      }
      break;
    }
  }

  if (!traffipaxFound) {
    withinProxRange = false;  // Reset for next detection
  }
}

// Function for staggered beeping and LED blinking
void alertWithStaggeredBeepAndBlink(byte n) {
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < n; j++) {
      // First turn on LED
      digitalWrite(ledR, LOW);
      delay(50);  // Brief delay

      // Then start tone
      tone(buzzer, 4000, 150);
      delay(200);  // Duration of beep

      // Turn off LED
      digitalWrite(ledR, HIGH);
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
  int freq = isSearching ? 1000 : 2000;

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