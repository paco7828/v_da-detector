#pragma once

class GN1650 {
private:
  uint8_t DAT_PIN;
  uint8_t CLK_PIN;
  bool initialized = false;

  // System command
  static const uint8_t CMD_SYSTEM = 0x48;

  // Display control bits
  static const uint8_t DISP_ON = 0x01;
  static const uint8_t SEG_8 = 0x00;
  static const uint8_t WORK_MODE = 0x00;

  // Brightness levels
  static const uint8_t BRIGHT_1 = 0x10;
  static const uint8_t BRIGHT_2 = 0x20;
  static const uint8_t BRIGHT_3 = 0x30;
  static const uint8_t BRIGHT_4 = 0x40;
  static const uint8_t BRIGHT_5 = 0x50;
  static const uint8_t BRIGHT_6 = 0x60;
  static const uint8_t BRIGHT_7 = 0x70;
  static const uint8_t BRIGHT_8 = 0x00;

  // 7-segment digit patterns (common cathode only)
  const uint8_t segmentMap[11] = {
    0x3F,  // 0
    0x06,  // 1
    0x5B,  // 2
    0x4F,  // 3
    0x66,  // 4
    0x6D,  // 5
    0x7D,  // 6
    0x07,  // 7
    0x7F,  // 8
    0x6F,  // 9
    0x00   // 10 = blank
  };

  // Segment bit positions
  static const uint8_t SEG_A = 0x01;   // bit 0
  static const uint8_t SEG_B = 0x02;   // bit 1
  static const uint8_t SEG_C = 0x04;   // bit 2
  static const uint8_t SEG_D = 0x08;   // bit 3
  static const uint8_t SEG_E = 0x10;   // bit 4
  static const uint8_t SEG_F = 0x20;   // bit 5
  static const uint8_t SEG_G = 0x40;   // bit 6
  static const uint8_t SEG_DP = 0x80;  // bit 7

  void startCondition() {
    digitalWrite(CLK_PIN, HIGH);
    digitalWrite(DAT_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(DAT_PIN, LOW);
    delayMicroseconds(10);
  }

  void stopCondition() {
    digitalWrite(CLK_PIN, LOW);
    digitalWrite(DAT_PIN, LOW);
    delayMicroseconds(10);
    digitalWrite(CLK_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(DAT_PIN, HIGH);
    delayMicroseconds(10);
  }

  void writeBit(bool bit) {
    digitalWrite(CLK_PIN, LOW);
    delayMicroseconds(5);
    digitalWrite(DAT_PIN, bit ? HIGH : LOW);
    delayMicroseconds(5);
    digitalWrite(CLK_PIN, HIGH);
    delayMicroseconds(5);
  }

  void writeByte(uint8_t data) {
    // Send 8 bits MSB first
    for (int i = 7; i >= 0; i--) {
      writeBit((data >> i) & 0x01);
    }

    // ACK bit (9th clock)
    digitalWrite(CLK_PIN, LOW);
    pinMode(DAT_PIN, INPUT);
    delayMicroseconds(5);
    digitalWrite(CLK_PIN, HIGH);
    delayMicroseconds(5);
    pinMode(DAT_PIN, OUTPUT);
    digitalWrite(CLK_PIN, LOW);
    delayMicroseconds(5);
  }

  void sendCommand(uint8_t cmd1, uint8_t cmd2) {
    startCondition();
    writeByte(cmd1);
    writeByte(cmd2);
    stopCondition();
    delayMicroseconds(100);
  }

  void writeDisplayData(uint8_t addr, uint8_t data) {
    startCondition();
    writeByte(addr);
    writeByte(data);
    stopCondition();
    delayMicroseconds(100);
  }

  void setDigit(uint8_t digitIndex, uint8_t value, bool decimalPoint = false) {
    uint8_t addr;
    switch (digitIndex) {
      case 0: addr = 0x68; break;  // DIG1
      case 1: addr = 0x6A; break;  // DIG2
      case 2: addr = 0x6C; break;  // DIG3
      default: return;
    }

    uint8_t dataByte;
    if (value <= 10) {
      dataByte = segmentMap[value];
      if (decimalPoint) dataByte |= 0x80;  // DP bit
    } else {
      dataByte = 0x00;  // Blank
    }

    writeDisplayData(addr, dataByte);
  }

public:
  // Initialize the GN1650 driver (Common Cathode displays only)
  void begin(uint8_t datPin, uint8_t clkPin, uint8_t brightness = 8) {
    DAT_PIN = datPin;
    CLK_PIN = clkPin;

    pinMode(DAT_PIN, OUTPUT);
    pinMode(CLK_PIN, OUTPUT);
    digitalWrite(DAT_PIN, HIGH);
    digitalWrite(CLK_PIN, HIGH);

    delay(200);

    // CRITICAL: Must write RAM first, THEN enable display
    // Step 1: Clear all RAM locations
    writeDisplayData(0x68, 0x00);
    writeDisplayData(0x6A, 0x00);
    writeDisplayData(0x6C, 0x00);
    delay(10);

    // Step 2: Enable display with brightness
    setBrightness(brightness);

    initialized = true;
  }

  void displayNumber(int num) {
    if (!initialized) return;

    if (num < 0) num = 0;
    if (num > 999) num = 999;

    uint8_t hundreds = num / 100;
    uint8_t tens = (num / 10) % 10;
    uint8_t ones = num % 10;

    // Blank leading zeros
    if (hundreds == 0) {
      setDigit(0, 10);  // blank
      if (tens == 0) {
        setDigit(1, 10);  // blank
      } else {
        setDigit(1, tens);
      }
    } else {
      setDigit(0, hundreds);
      setDigit(1, tens);
    }
    setDigit(2, ones);
  }

  void clear() {
    if (!initialized) return;
    writeDisplayData(0x68, 0x00);
    writeDisplayData(0x6A, 0x00);
    writeDisplayData(0x6C, 0x00);
  }

  void testSegments(uint16_t delayMs = 100) {
    if (!initialized) return;

    // Test each segment: A, B, C, D, E, F, G, DP
    for (int bit = 0; bit < 8; bit++) {
      uint8_t pattern = (1 << bit);
      writeDisplayData(0x68, pattern);
      writeDisplayData(0x6A, pattern);
      writeDisplayData(0x6C, pattern);
      delay(delayMs);
    }
    clear();
  }

  void loading(uint16_t delayMs = 100) {
    if (!initialized) return;

    // Loading animation sequence: d1a, d1f, d1e, d1d, d2d, d3d, d3c, d3b, d3a, d2a
    const uint8_t sequence[][3] = {
      { SEG_A, 0x00, 0x00 },  // d1a
      { SEG_F, 0x00, 0x00 },  // d1f
      { SEG_E, 0x00, 0x00 },  // d1e
      { SEG_D, 0x00, 0x00 },  // d1d
      { 0x00, SEG_D, 0x00 },  // d2d
      { 0x00, 0x00, SEG_D },  // d3d
      { 0x00, 0x00, SEG_C },  // d3c
      { 0x00, 0x00, SEG_B },  // d3b
      { 0x00, 0x00, SEG_A },  // d3a
      { 0x00, SEG_A, 0x00 }   // d2a
    };

    for (int i = 0; i < 10; i++) {
      writeDisplayData(0x68, sequence[i][0]);
      writeDisplayData(0x6A, sequence[i][1]);
      writeDisplayData(0x6C, sequence[i][2]);
      delay(delayMs);
    }
  }

  void showDashes() {
    if (!initialized) return;
    // Segment G = bit 6 = 0x40
    writeDisplayData(0x68, 0x40);
    writeDisplayData(0x6A, 0x40);
    writeDisplayData(0x6C, 0x40);
  }

  void setBrightness(uint8_t level) {
    if (level < 1) level = 1;
    if (level > 8) level = 8;

    uint8_t brightCmd = (level == 8) ? BRIGHT_8 : (level << 4);
    sendCommand(CMD_SYSTEM, SEG_8 | WORK_MODE | brightCmd | DISP_ON);
    delay(10);
  }
};