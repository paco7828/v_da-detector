#include <TinyGPS++.h>
#include <HardwareSerial.h>

TinyGPSPlus gps;
HardwareSerial gpsSerial(1);

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600, SERIAL_8N1, 6, 10);

  // Configure for 10Hz
  byte ubxRate[] = { 0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0x64, 0x00, 0x01, 0x00, 0x01, 0x00, 0x7A, 0x12 };
  for (int i = 0; i < sizeof(ubxRate); i++) {
    gpsSerial.write(ubxRate[i]);
  }

  Serial.println("10Hz Latitude Output with TinyGPS++");
}

void loop() {
  while (gpsSerial.available() > 0) {
    if (gps.encode(gpsSerial.read())) {
      // New data decoded successfully
      if (gps.location.isValid()) {
        Serial.print("LAT: ");
        Serial.println(gps.location.lat(), 6);  // 6 decimal places
      }
    }
  }
}