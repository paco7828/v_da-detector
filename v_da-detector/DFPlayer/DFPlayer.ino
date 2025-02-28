#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

SoftwareSerial mySerial(10, 11);  // RX, TX
DFRobotDFPlayerMini myDFPlayer;

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);
  if (!myDFPlayer.begin(mySerial)) {
    Serial.println("DFPlayer Mini not detected!");
    while (true)
      ;
  }
  Serial.println("DFPlayer Mini ready!");
  myDFPlayer.volume(5);  // Set volume (0 to 30)
  Serial.println("Playing File 001.mp3");
  myDFPlayer.play(1);
}

void loop() {
}