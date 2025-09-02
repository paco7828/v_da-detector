#pragma once

class BetterRGB
{
private:
  // Constants
  byte RED;
  byte GREEN;
  byte BLUE;
  bool COMMON_CATHODE;

  // Time variables
  unsigned long digitalRedOnUntil = 0;
  unsigned long digitalGreenOnUntil = 0;
  unsigned long digitalBlueOnUntil = 0;
  unsigned long digitalColorOnUntil = 0;
  unsigned long analogRedOnUntil = 0;
  unsigned long analogGreenOnUntil = 0;
  unsigned long analogBlueOnUntil = 0;
  unsigned long analogColorOnUntil = 0;

public:
  void begin(byte red, byte green, byte blue, bool commonCathode)
  {
    this->RED = red;
    this->GREEN = green;
    this->BLUE = blue;
    this->COMMON_CATHODE = commonCathode;

    pinMode(this->RED, OUTPUT);
    pinMode(this->GREEN, OUTPUT);
    pinMode(this->BLUE, OUTPUT);

    // Initialize all pins to OFF state
    allOff();
  }

  // ---------------------------------------------- Simple digital functions ----------------------------------------------

  void setDigitalRed(bool isOn)
  {
    digitalWrite(this->RED, COMMON_CATHODE ? isOn : !isOn);
  }

  void setDigitalGreen(bool isOn)
  {
    digitalWrite(this->GREEN, COMMON_CATHODE ? isOn : !isOn);
  }

  void setDigitalBlue(bool isOn)
  {
    digitalWrite(this->BLUE, COMMON_CATHODE ? isOn : !isOn);
  }

  void allOn()
  {
    setDigitalColor(true, true, true);
  }

  void allOff()
  {
    setDigitalColor(false, false, false);
  }

  void setDigitalColor(bool red, bool green, bool blue)
  {
    setDigitalRed(red);
    setDigitalGreen(green);
    setDigitalBlue(blue);
  }

  // ---------------------------------------------- Simple analog functions ----------------------------------------------

  void setAnalogColor(int red, int green, int blue)
  {
    // Add bounds checking
    red = constrain(red, 0, 255);
    green = constrain(green, 0, 255);
    blue = constrain(blue, 0, 255);

    analogWrite(this->RED, COMMON_CATHODE ? red : 255 - red);
    analogWrite(this->GREEN, COMMON_CATHODE ? green : 255 - green);
    analogWrite(this->BLUE, COMMON_CATHODE ? blue : 255 - blue);
  }

  void setAnalogRed(int redValue)
  {
    redValue = constrain(redValue, 0, 255);
    analogWrite(this->RED, COMMON_CATHODE ? redValue : 255 - redValue);
  }

  void setAnalogGreen(int greenValue)
  {
    greenValue = constrain(greenValue, 0, 255);
    analogWrite(this->GREEN, COMMON_CATHODE ? greenValue : 255 - greenValue);
  }

  void setAnalogBlue(int blueValue)
  {
    blueValue = constrain(blueValue, 0, 255);
    analogWrite(this->BLUE, COMMON_CATHODE ? blueValue : 255 - blueValue);
  }

  // ---------------------------------------------- Time based functions ----------------------------------------------

  void update()
  {
    unsigned long currentTime = millis();

    // ---------------- Digital resets ----------------
    if (digitalRedOnUntil > 0 && currentTime >= digitalRedOnUntil)
    {
      setDigitalRed(false);
      digitalRedOnUntil = 0;
    }

    if (digitalGreenOnUntil > 0 && currentTime >= digitalGreenOnUntil)
    {
      setDigitalGreen(false);
      digitalGreenOnUntil = 0;
    }

    if (digitalBlueOnUntil > 0 && currentTime >= digitalBlueOnUntil)
    {
      setDigitalBlue(false);
      digitalBlueOnUntil = 0;
    }

    if (digitalColorOnUntil > 0 && currentTime >= digitalColorOnUntil)
    {
      setDigitalColor(false, false, false);
      digitalColorOnUntil = 0;
    }

    // ---------------- Analog resets ----------------
    if (analogColorOnUntil > 0 && currentTime >= analogColorOnUntil)
    {
      setAnalogColor(0, 0, 0);
      analogColorOnUntil = 0;
    }

    if (analogRedOnUntil > 0 && currentTime >= analogRedOnUntil)
    {
      setAnalogRed(0);
      analogRedOnUntil = 0;
    }

    if (analogGreenOnUntil > 0 && currentTime >= analogGreenOnUntil)
    {
      setAnalogGreen(0);
      analogGreenOnUntil = 0;
    }

    if (analogBlueOnUntil > 0 && currentTime >= analogBlueOnUntil)
    {
      setAnalogBlue(0);
      analogBlueOnUntil = 0;
    }
  }

  // ---------------------------------------------- Time based digital functions ----------------------------------------------

  void keepDigitalRedFor(unsigned long ms)
  {
    setDigitalRed(true);
    digitalRedOnUntil = millis() + ms;
  }

  void keepDigitalGreenFor(unsigned long ms)
  {
    setDigitalGreen(true);
    digitalGreenOnUntil = millis() + ms;
  }

  void keepDigitalBlueFor(unsigned long ms)
  {
    setDigitalBlue(true);
    digitalBlueOnUntil = millis() + ms;
  }

  void keepDigitalColorFor(bool red, bool green, bool blue, unsigned long ms)
  {
    setDigitalColor(red, green, blue);
    digitalColorOnUntil = millis() + ms;
  }

  // ---------------------------------------------- Time based analog functions ----------------------------------------------

  void keepAnalogRedFor(int v, unsigned long ms)
  {
    setAnalogRed(v);
    analogRedOnUntil = millis() + ms;
  }

  void keepAnalogGreenFor(int v, unsigned long ms)
  {
    setAnalogGreen(v);
    analogGreenOnUntil = millis() + ms;
  }

  void keepAnalogBlueFor(int v, unsigned long ms)
  {
    setAnalogBlue(v);
    analogBlueOnUntil = millis() + ms;
  }

  void keepAnalogColorFor(int red, int green, int blue, unsigned long ms)
  {
    setAnalogColor(red, green, blue);
    analogColorOnUntil = millis() + ms;
  }

  // ---------------------------------------------- Fading functions ----------------------------------------------

  void fadeFromRedToBlue(unsigned int steps = 50, unsigned int delayMs = 20)
  {
    for (int i = 0; i <= steps; i++)
    {
      int redValue = 255 - (255 * i / steps);
      int blueValue = 255 * i / steps;
      setAnalogColor(redValue, 0, blueValue);
      delay(delayMs);
    }
  }

  void fadeFromRedToGreen(unsigned int steps = 50, unsigned int delayMs = 20)
  {
    for (int i = 0; i <= steps; i++)
    {
      int redValue = 255 - (255 * i / steps);
      int greenValue = 255 * i / steps;
      setAnalogColor(redValue, greenValue, 0);
      delay(delayMs);
    }
  }

  void fadeFromGreenToRed(unsigned int steps = 50, unsigned int delayMs = 20)
  {
    for (int i = 0; i <= steps; i++)
    {
      int greenValue = 255 - (255 * i / steps);
      int redValue = 255 * i / steps;
      setAnalogColor(redValue, greenValue, 0);
      delay(delayMs);
    }
  }

  void fadeFromGreenToBlue(unsigned int steps = 50, unsigned int delayMs = 20)
  {
    for (int i = 0; i <= steps; i++)
    {
      int greenValue = 255 - (255 * i / steps);
      int blueValue = 255 * i / steps;
      setAnalogColor(0, greenValue, blueValue);
      delay(delayMs);
    }
  }

  void fadeFromBlueToRed(unsigned int steps = 50, unsigned int delayMs = 20)
  {
    for (int i = 0; i <= steps; i++)
    {
      int blueValue = 255 - (255 * i / steps);
      int redValue = 255 * i / steps;
      setAnalogColor(redValue, 0, blueValue);
      delay(delayMs);
    }
  }

  void fadeFromBlueToGreen(unsigned int steps = 50, unsigned int delayMs = 20)
  {
    for (int i = 0; i <= steps; i++)
    {
      int blueValue = 255 - (255 * i / steps);
      int greenValue = 255 * i / steps;
      setAnalogColor(0, greenValue, blueValue);
      delay(delayMs);
    }
  }

  // ---------------------------------------------- Flashing functions ----------------------------------------------

  void flashRGB(unsigned int delayMs = 200)
  {
    setDigitalColor(true, false, false); // Red
    delay(delayMs);
    setDigitalColor(false, true, false); // Green
    delay(delayMs);
    setDigitalColor(false, false, true); // Blue
    delay(delayMs);
    allOff();
  }

  void flashRG(unsigned int delayMs = 200)
  {
    setDigitalColor(true, false, false); // Red
    delay(delayMs);
    setDigitalColor(false, true, false); // Green
    delay(delayMs);
    allOff();
  }

  void flashRB(unsigned int delayMs = 200)
  {
    setDigitalColor(true, false, false); // Red
    delay(delayMs);
    setDigitalColor(false, false, true); // Blue
    delay(delayMs);
    allOff();
  }

  void flashGB(unsigned int delayMs = 200)
  {
    setDigitalColor(false, true, false); // Green
    delay(delayMs);
    setDigitalColor(false, false, true); // Blue
    delay(delayMs);
    allOff();
  }

  void flashWhite(unsigned int delayMs = 200){
    setDigitalColor(true, true, true);
    delay(delayMs);
    setDigitalColor(false, false, false);
    delay(delayMs);
  };
};