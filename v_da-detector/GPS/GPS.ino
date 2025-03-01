#include <SoftwareSerial.h>
#include <TinyGPS++.h>

#define GPS_RX 12  // D6 (GPIO12)
#define GPS_TX 13  // D7 (GPIO13)
#define GPS_BAUD 9600
#define PIEZO 14  // D5 (GPIO 14)

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
  bootUpSound();
  delay(2000);
}

void loop() {
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
    if (gps.location.isValid()) {
      // Valid GPS data
      if (!playedSignalFoundSound) {
        signalFoundSound();
        playedSignalFoundSound = true;
      }
      currentLat = gps.location.lat();
      currentLon = gps.location.lng();
      currentSpeed = gps.speed.kmph();
      Serial.println(currentLat);
      Serial.println(currentLon);
      Serial.println(currentSpeed);
    } else {
      // Waiting for GPS data
      if (!playedNoSignalSound) {
        Serial.println("No GPS fix yet...");
        signalSearchSound();
        playedNoSignalSound = true;
      }
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