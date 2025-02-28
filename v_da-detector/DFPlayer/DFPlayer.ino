#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

// Define pins
#define POTMETER A0
#define DF_RX 10
#define DF_TX 11

/*
001.mp3 -> belterületi traffipax várható
002.mp3 -> külterületi traffipax várható
003.mp3 -> gyors-forgalmi traffipax várható
004.mp3 -> lassíts
005.mp3 -> sebességhatár 50 km/h
006.mp3 -> beep
007.mp3 -> GPS jel keresése
008.mp3 -> Műhold beazonosítva, jó utat!
*/

// Create instances
SoftwareSerial mySerial(DF_RX, DF_TX);  // RX, TX
DFRobotDFPlayerMini myDFPlayer;

// Helper variables
int lastVolume = -1;
unsigned long lastBeepTime = 0;
const unsigned long beepCooldown = 500;
const int volumeThreshold = 2;

// Rolling average filter settings
const int numSamples = 5;
int potSamples[numSamples] = { 0 };
int sampleIndex = 0;

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);
  if (!myDFPlayer.begin(mySerial)) {
    Serial.println("DFPlayer Mini not detected!");
    while (true)
      ;
  }
  myDFPlayer.volume(22);
}

void loop() {
  listenForVolumeChange();
}

void listenForVolumeChange() {
  // Read and filter the potentiometer value
  int potValue = smoothAnalogRead();
  int volume = map(potValue, 0, 1023, 0, 30);

  // Prevent small jumps near max volume
  if (potValue > 1000) {
    volume = 30;
  } else if (potValue < 20) {
    volume = 0;
  }

  // Only update volume if the change is greater than the threshold
  if (abs(volume - lastVolume) >= volumeThreshold) {
    myDFPlayer.volume(volume);
    Serial.print("Volume set to: ");
    Serial.println(volume);
    lastVolume = volume;

    // Play beep sound only if enough time has passed since last beep
    if (millis() - lastBeepTime > beepCooldown) {
      myDFPlayer.play(6);
      lastBeepTime = millis();
    }
  }
}

int smoothAnalogRead() {
  potSamples[sampleIndex] = analogRead(POTMETER);
  sampleIndex = (sampleIndex + 1) % numSamples;

  int sum = 0;

  for (int i = 0; i < numSamples; i++) {
    sum += potSamples[i];
  }
  return sum / numSamples;
}
