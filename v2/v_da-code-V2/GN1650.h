class GN1650 {
private:
  byte DAT_PIN;
  byte CLK_PIN;
  bool initialized = false;

  const uint8_t segmentMap[10] = {
    0b00111111,  // 0
    0b00000110,  // 1
    0b01011011,  // 2
    0b01001111,  // 3
    0b01100110,  // 4
    0b01101101,  // 5
    0b01111101,  // 6
    0b00000111,  // 7
    0b01111111,  // 8
    0b01101111   // 9
  };

  // Start condition for communication
  void startCondition() {
    digitalWrite(this->DAT_PIN, HIGH);
    digitalWrite(this->CLK_PIN, HIGH);
    delayMicroseconds(2);
    digitalWrite(this->DAT_PIN, LOW);
    delayMicroseconds(2);
  }

  // Stop condition for communication
  void stopCondition() {
    digitalWrite(this->DAT_PIN, LOW);
    digitalWrite(this->CLK_PIN, HIGH);
    delayMicroseconds(2);
    digitalWrite(this->DAT_PIN, HIGH);
    delayMicroseconds(2);
  }

  // Send a byte to GN1650 with proper timing
  void sendByte(uint8_t data) {
    for (int i = 0; i < 8; i++) {
      digitalWrite(this->CLK_PIN, LOW);
      delayMicroseconds(2);
      digitalWrite(this->DAT_PIN, (data & 0x01));
      delayMicroseconds(2);
      digitalWrite(this->CLK_PIN, HIGH);
      delayMicroseconds(2);
      data >>= 1;
    }
  }

  // Set a single digit, 0x00 = blank
  void setDigit(uint8_t digitIndex, uint8_t value, bool decimalPoint = false) {
    if (!this->initialized) return;  // Safety check

    uint8_t dataByte;
    if (value <= 9) {
      dataByte = this->segmentMap[value];
      if (decimalPoint) dataByte |= 0b10000000;  // DP = MSB
    } else {
      dataByte = 0x00;  // Blank
    }

    this->startCondition();
    this->sendByte(digitIndex);  // digit address
    this->sendByte(dataByte);    // segment data
    this->stopCondition();
  }

public:
  void begin(byte datPin, byte clkPin) {
    this->DAT_PIN = datPin;
    this->CLK_PIN = clkPin;
    pinMode(this->DAT_PIN, OUTPUT);
    pinMode(this->CLK_PIN, OUTPUT);
    digitalWrite(this->DAT_PIN, HIGH);
    digitalWrite(this->CLK_PIN, HIGH);
    this->initialized = true;
  }

  // Display integer with fully blanked leading zeros
  void displayNumber(int num) {
    if (!this->initialized) return;  // Safety check

    if (num < 0) num = 0;
    if (num > 999) num = 999;

    uint8_t hundreds = num / 100;
    uint8_t tens = (num / 10) % 10;
    uint8_t ones = num % 10;

    // Leading zeros logic
    if (hundreds != 0) {
      this->setDigit(0, hundreds);
      this->setDigit(1, tens);
    } else {
      this->setDigit(0, 10);  // blank
      if (tens != 0) {
        this->setDigit(1, tens);
      } else {
        this->setDigit(1, 10);  // blank
      }
    }
    this->setDigit(2, ones);  // ones always shown
  }

  // Turn display on
  void displayOn() {
    if (!this->initialized) return;
    this->startCondition();
    this->sendByte(0x01);
    this->stopCondition();
  }

  // Turn display off
  void displayOff() {
    if (!this->initialized) return;
    this->startCondition();
    this->sendByte(0x00);
    this->stopCondition();
  }

  // Set brightness (0 - 15)
  void setBrightness(byte level) {
    if (!this->initialized) return;
    if (level > 15) {
      level = 15;
    }
    this->startCondition();
    this->sendByte(0x80 | level);
    this->stopCondition();
  }
};