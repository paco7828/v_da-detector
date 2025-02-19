#include "coordinates.h"
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>

TinyGPSPlus gps;

SoftwareSerial gpsSerial(13, 12);  // TX -> D7 (GPIO13), RX -> D6 (GPIO12)

double lat, lon;
byte type;
int limit;
int buzzerPin = 5;  // Use D1 (GPIO5), avoid TX (GPIO1)

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600);
  pinMode(buzzerPin, OUTPUT);

  // Wait for Serial Monitor to open (for debugging)
  while (!Serial) {
    delay(100);
  }

  Serial.println("\nGPS System Starting...");

  // Wait for GPS Module to Respond
  Serial.print("Checking GPS communication...");
  unsigned long start = millis();
  bool gpsAvailable = false;

  while (millis() - start < 5000) {  // Wait up to 5 seconds
    if (gpsSerial.available()) {
      gpsAvailable = true;
      break;
    }
    delay(100);
    Serial.print(".");
  }

  if (!gpsAvailable) {
    Serial.println("\nGPS module not responding! Check wiring.");
  } else {
    Serial.println("\nGPS communication established.");
  }

  // Beep to indicate startup
  tone(buzzerPin, 2000, 1000);
  delay(1000);

  // Print stored coordinates
  const int numCoordinates = sizeof(coordinates) / sizeof(coordinates[0]);
  Serial.println("Stored coordinates:");
  for (int i = 0; i < numCoordinates; i++) {
    Serial.printf("Lat: %.6f, Lon: %.6f, Type: %d, Limit: %d\n",
                  coordinates[i].lat, coordinates[i].lon, coordinates[i].type, coordinates[i].limit);
  }

  // Wait for GPS fix
  Serial.println("Waiting for GPS fix...");
  while (!gps.location.isValid()) {
    while (gpsSerial.available() > 0) {
      gps.encode(gpsSerial.read());
    }
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nGPS fix acquired!");
  tone(buzzerPin, 4000, 2000);
}

void loop() {
  while (gpsSerial.available() > 0) {
    if (gps.encode(gpsSerial.read())) {
      if (gps.location.isValid()) {
        Serial.printf("Current Position - Lat: %.6f, Lon: %.6f\n",
                      gps.location.lat(), gps.location.lng());
      }
    }
  }

  // Check if GPS is still responding
  if (millis() > 5000 && gps.charsProcessed() < 10) {
    Serial.println("No GPS data received! Check wiring.");
    delay(5000);
  }
}