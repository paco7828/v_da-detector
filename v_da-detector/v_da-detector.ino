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

      Serial.println(currentLat);
      Serial.println(currentLon);
      Serial.println(currentSpeed);

      checkProximityToTraffipax();

      digitalWrite(LED_R, HIGH);
      digitalWrite(LED_G, LOW); // Green LED on
    } else {
      if (!playedNoSignalSound) {
        Serial.println("No GPS fix yet...");
        signalSearchSound();
        playedNoSignalSound = true;
        playedSignalFoundSound = false;
      }
      digitalWrite(LED_G, HIGH);
      digitalWrite(LED_R, LOW); // Red LED on
    }
  }
}

void checkProximityToTraffipax() {
  for (int i = 0; i < sizeof(coordinates) / sizeof(coordinates[0]); i++) {
    double distance = getDistance(currentLat, currentLon, coordinates[i].lat, coordinates[i].lon);
    if (distance <= 500) {
      Serial.println("Warning! Approaching a traffipax!");
      alertSound();
      break;
    }
  }
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

void alertSound() {
  for (int i = 0; i < 10; i++) {
    tone(PIEZO, 3000, 200);
    delay(300);
  }
  noTone(PIEZO);
}

double getDistance(double lat1, double lon1, double lat2, double lon2) {
  const double R = 6371000; // Earth radius in meters
  double dLat = radians(lat2 - lat1);
  double dLon = radians(lon2 - lon1);
  double a = sin(dLat / 2) * sin(dLat / 2) + cos(radians(lat1)) * cos(radians(lat2)) * sin(dLon / 2) * sin(dLon / 2);
  double c = 2 * atan2(sqrt(a), sqrt(1 - a));
  return R * c;
}
