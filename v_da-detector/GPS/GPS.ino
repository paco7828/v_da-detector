#include <SoftwareSerial.h>
#include <TinyGPS++.h>

#define GPS_RX 11
#define GPS_TX 10
#define GPS_BAUD 9600

SoftwareSerial gpsSerial(GPS_RX, GPS_TX);
TinyGPSPlus gps;

double currentLat, currentLon;
int currentSpeed;

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(GPS_BAUD);
}

void loop() {
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
    if (gps.location.isValid()) {
        // Valid GPS data
        currentLat = gps.location.lat();
        currentLon = gps.location.lng();
        currentSpeed = gps.speed.kmph();
    } else {
      // Waiting for GPS data
      Serial.println("No GPS fix yet...");
    }
  }
}