#include "coordinates.h"

double lat, lon;
byte type;
int limit;

void setup() {
  Serial.begin(115200);
  delay(5000);

  // Number of coordinates
  const int numCoordinates = sizeof(coordinates) / sizeof(coordinates[0]);
  Serial.println(numCoordinates);

  for (int i = 0; i < numCoordinates; i++) {
    lat = coordinates[i].lat;
    lon = coordinates[i].lon;
    type = coordinates[i].type;
    limit = coordinates[i].limit;
    Serial.print("Lat: ");
    Serial.print(lat);
    Serial.print(", Lon: ");
    Serial.print(lon);
    Serial.print(", Type: ");
    Serial.print(type);
    Serial.print(", Limit: ");
    Serial.println(limit);
  }

  // GPS --> NodeMCU
  /*
  V --> 3.3V
  G --> GND
  T --> 3
  R --> 4
  */
}

void loop() {
}