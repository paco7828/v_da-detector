#define RGB_RED 9
#define RGB_GREEN 10
#define RGB_BLUE 5

void setup() {
  pinMode(RGB_RED, OUTPUT);
  pinMode(RGB_GREEN, OUTPUT);
  pinMode(RGB_BLUE, OUTPUT);
}

void loop() {
  pulseColor("red");
}

void pulseColor(String color) {
  byte redValue = 255, greenValue = 255, blueValue = 255;

  // Fade in
  for (int i = 255; i >= 0; i--) {
    if (color.equalsIgnoreCase("red")) {
      redValue = i;
    } else if (color.equalsIgnoreCase("green")) {
      greenValue = i;
    } else if (color.equalsIgnoreCase("blue")) {
      blueValue = i;
    }

    analogWrite(RGB_RED, redValue);
    analogWrite(RGB_GREEN, greenValue);
    analogWrite(RGB_BLUE, blueValue);
    delay(5);
  }

  // Fade out
  for (int i = 0; i <= 255; i++) {
    if (color.equalsIgnoreCase("red")) {
      redValue = i;
    } else if (color.equalsIgnoreCase("green")) {
      greenValue = i;
    } else if (color.equalsIgnoreCase("blue")) {
      blueValue = i;
    }

    analogWrite(RGB_RED, redValue);
    analogWrite(RGB_GREEN, greenValue);
    analogWrite(RGB_BLUE, blueValue);
    delay(5);
  }
}