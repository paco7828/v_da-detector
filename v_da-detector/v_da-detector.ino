#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include "coordinates.h"

#define GPS_RX 12  // D6 (GPIO12)
#define GPS_TX 13  // D7 (GPIO13)
#define GPS_BAUD 9600
#define PIEZO 14  // D5 (GPIO 14)
#define LED_R 5   // D1 (GPIO5)
#define LED_G 4   // D2 (GPIO4)

SoftwareSerial gpsSerial(GPS_TX, GPS_RX);  // TX (to GPS RX), RX (to GPS TX)
TinyGPSPlus gps;

double currentLat, currentLon;
int currentSpeed;
bool playedSignalFoundSound = false;
bool playedNoSignalSound = false;
unsigned long lastBeepTime = 0;
bool within400Meters = false;

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(GPS_BAUD);
  pinMode(PIEZO, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  digitalWrite(LED_R, HIGH);
  digitalWrite(LED_G, HIGH);
  bootUpSound();
  delay(2000);
}

void loop() {
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());

    if (gps.location.isValid()) {
      if (!playedSignalFoundSound) {
        signalFoundSound();
        playedSignalFoundSound = true;
        playedNoSignalSound = false;
      }

      currentLat = gps.location.lat();
      currentLon = gps.location.lng();
      currentSpeed = gps.speed.kmph();
      checkProximityToTraffipax();

      digitalWrite(LED_R, HIGH);
      digitalWrite(LED_G, LOW);
    } else {
      if (!playedNoSignalSound) {
        Serial.println("No GPS fix yet...");
        signalSearchSound();
        playedNoSignalSound = true;
        playedSignalFoundSound = false;
      }
      digitalWrite(LED_G, HIGH);
      digitalWrite(LED_R, LOW);
    }
  }
}

void checkProximityToTraffipax() {
  bool traffipaxFound = false;

  for (int i = 0; i < sizeof(coordinates) / sizeof(coordinates[0]); i++) {
    double distance = getDistance(currentLat, currentLon, coordinates[i].lat, coordinates[i].lon);
    Serial.println(distance);

    if (distance <= 3000) {
      traffipaxFound = true;

      if (!within400Meters) {
        Serial.println("Warning! Approaching a traffipax!");
        beep3Times();
        within400Meters = true;
      }
      break;
    }
  }

  if (!traffipaxFound) {
    within400Meters = false;
  }
}

void beep3Times() {
  tone(PIEZO, 4000, 200);
  delay(250);
  tone(PIEZO, 4000, 200);
  delay(250);
  tone(PIEZO, 4000, 200);
  delay(250);
  noTone(PIEZO);
}

void bootUpSound() {
  tone(PIEZO, 1000, 200);
  delay(250);
  tone(PIEZO, 1500, 200);
  delay(250);
  tone(PIEZO, 2000, 200);
  delay(250);
  noTone(PIEZO);
}

void signalSearchSound() {
  tone(PIEZO, 1000, 1500);
}

void signalFoundSound() {
  tone(PIEZO, 2000, 200);
  delay(250);
  tone(PIEZO, 2500, 200);
  delay(250);
  noTone(PIEZO);
}

double toRadians(double degree) {
  return degree * M_PI / 180.0;
}

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