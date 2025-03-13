#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include "coordinates.h"

// Connections
const byte LED_G = 4;    // D2 (GPIO4)
const byte LED_R = 5;    // D1 (GPIO5)
const byte GPS_RX = 12;  // D6 (GPIO12)
const byte GPS_TX = 13;  // D7 (GPIO13)
const byte BUZZER = 14;  // D5 (GPIO 14)

// Instance creation
SoftwareSerial gpsSerial(GPS_TX, GPS_RX);
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
#define PROX_RANGE 4000

void setup() {
  // Start serial communication
  Serial.begin(115200);
  gpsSerial.begin(GPS_BAUD);

  // Set output pins
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_R, OUTPUT);

  // Turn leds off at boot
  digitalWrite(LED_G, HIGH);
  digitalWrite(LED_R, HIGH);

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
      digitalWrite(LED_G, LOW);
      digitalWrite(LED_R, HIGH);

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
        Serial.println("No GPS fix yet...");
        signalSound(true);
        playedNoSignalSound = true;
        playedSignalFoundSound = false;
      }

      // Turn red led on
      digitalWrite(LED_G, HIGH);
      digitalWrite(LED_R, LOW);
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
        // Bee to indicate near traffipax
        Serial.println("Warning! Approaching a traffipax!");
        beepNTimes(coordinates[i].limit, 4000, 200);
        withinProxRange = true;
      }
      break;
    }
  }

  if (!traffipaxFound) {
    withinProxRange = false;
  }
}

// Function to beep n/10 times
void beepNTimes(byte n, int frequency, int beepLength) {
  n = n / 10;
  for (int i = 0; i < n; i++) {
    tone(BUZZER, frequency, beepLength);
    delay(beepLength + 50);
  }
  noTone(BUZZER);
}

// Boot beeping sound
void bootUpSound() {
  int baseFreq = 1000;
  for (int i = 0; i < 3; i++) {
    tone(BUZZER, baseFreq, 200);
    delay(250);
    baseFreq += 500;
  }
  noTone(BUZZER);
}

// Function to play sound based on state
void signalSound(bool isSearching) {
  int baseFreq;
  bool toReduce;
  // Signal not found
  if (isSearching) {
    baseFreq = 2500;
    toReduce = true;
  }
  // Signal found
  else {
    baseFreq = 2000;
    toReduce = false;
  }
  for (int i = 0; i < 2; i++) {
    tone(BUZZER, baseFreq, 200);
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