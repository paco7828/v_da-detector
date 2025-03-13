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
#define PROX_RANGE 300 // meters

void setup() {
  // Start serial communication
  gpsSerial.begin(GPS_BAUD);

  // Set output pins
  pinMode(buzzer, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledR, OUTPUT);

  // Turn leds off at boot
  digitalWrite(ledG, HIGH);
  digitalWrite(ledR, HIGH);

  // Play boot sound and wait 2 seconds
  bootUpSound();
  delay(2000);
}

void loop() {
  // GPS is available
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
    // Found satellite(s) -> valid data
    if (gps.location.isValid()) {
      // Play sound once when signal is found
      if (!playedSignalFoundSound) {
        signalSound(false);
        playedSignalFoundSound = true;
        playedNoSignalSound = false;
      }

      // Turn green led on
      digitalWrite(ledG, LOW);
      digitalWrite(ledR, HIGH);

      // Get current GPS data
      currentLat = gps.location.lat();
      currentLon = gps.location.lng();
      currentSpeed = gps.speed.kmph();

      // Check distance to nearest traffipax
      checkProximityToTraffipax();
    }
    // If no satellites are found
    else {
      // Play sound once when signal isn't found
      if (!playedNoSignalSound) {
        signalSound(true);
        playedNoSignalSound = true;
        playedSignalFoundSound = false;
      }

      // Turn red led on
      digitalWrite(ledG, HIGH);
      digitalWrite(ledR, LOW);
    }
  }
}

// Functon to check distance between traffipax and you
void checkProximityToTraffipax() {
  // Variable to track is traffipax is near
  bool traffipaxFound = false;

  // Loop through coordinates
  for (int i = 0; i < sizeof(coordinates) / sizeof(coordinates[0]); i++) {
    // Get distance for each coordinate from your location
    double distance = getDistance(currentLat, currentLon, coordinates[i].lat, coordinates[i].lon);

    // If distance is within proximity range
    if (distance <= PROX_RANGE) {
      traffipaxFound = true;

      // Trigger proximity range alert once
      if (!withinProxRange) {
        // Beep + blink 5-5 times
        beepNTimes(5, 4000, 200);
        blinkLed(ledR, 5);
        withinProxRange = true;
      }
      break;
    }
  }

  if (!traffipaxFound) {
    withinProxRange = false;
  }
}

// Function to beep n times
void beepNTimes(byte n, int frequency, int beepLength) {
  for (int i = 0; i < n; i++) {
    tone(buzzer, frequency, beepLength);
    delay(beepLength + 50);
  }
  noTone(buzzer);
}

// Boot beeping sound
void bootUpSound() {
  int baseFreq = 1000;
  for (int i = 0; i < 3; i++) {
    tone(buzzer, baseFreq, 200);
    delay(250);
    baseFreq += 500;
  }
  noTone(buzzer);
}

// Function to play sound based on state
void signalSound(bool isSearching) {
  int baseFreq;
  bool toReduce;
  // Signal not found
  if (isSearching) {
    baseFreq = 2000;
    toReduce = true;
  }
  // Signal found
  else {
    baseFreq = 2500;
    toReduce = false;
  }
  for (int i = 0; i < 2; i++) {
    tone(buzzer, baseFreq, 200);
    delay(250);
    // Reduce frequency by 500
    if (toReduce) {
      baseFreq -= 500;
    }
    // Increase frequency by 500
    else {
      baseFreq += 500;
    }
  }
  noTone(buzzer);
}

// Function to blink specific led n times
void blinkLed(byte ledPin, int blinkCount) {
  for (int i = 0; i < blinkCount; i++) {
    digitalWrite(ledPin, LOW);
    delay(100);
    digitalWrite(ledPin, HIGH);
    delay(100);
  }
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