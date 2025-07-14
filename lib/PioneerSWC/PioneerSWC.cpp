#include "PioneerSWC.h"

PioneerSWC::PioneerSWC(uint8_t csPin, uint8_t sckPin, uint8_t mosiPin) {
  _csPin = csPin;
  _sckPin = sckPin;
  _mosiPin = mosiPin;
  _invertValue = true;
}

void PioneerSWC::begin() {
  Serial.println("PioneerSWC::begin()");
  pinMode(_csPin, OUTPUT);
  pinMode(_mosiPin, OUTPUT);
  pinMode(_sckPin, OUTPUT);

  digitalWrite(_mosiPin, LOW);
  digitalWrite(_csPin, HIGH);
  digitalWrite(_sckPin, LOW);
}

void PioneerSWC::setPot(uint8_t value) {
  Serial.print("PioneerSWC::setPot(value=");
  Serial.print(value);
  Serial.println(")");
  digitalWrite(_csPin, LOW);
  spiTransfer(0b00010001);
  if (_invertValue) {
    value = 255 - value; // Invert the value if needed
  }
  spiTransfer(value);
  digitalWrite(_csPin, HIGH);
}

void PioneerSWC::spiTransfer(uint8_t value) {
  Serial.print("PioneerSWC::spiTransfer(value=");
  Serial.print(value, HEX);
  Serial.println(")");
  for (int i = 7; i >= 0; i--) {
    digitalWrite(_mosiPin, (value >> i) & 0x01);
    digitalWrite(_sckPin, HIGH);
    delayMicroseconds(5);
    digitalWrite(_sckPin, LOW);
  }
}

uint8_t PioneerSWC::getValueForCommand(SWCCommand command) {
  switch (command) {
    case SWCCommand::Source:     return 0;
    case SWCCommand::Mute:       return 4;
    case SWCCommand::DisplayOff: return 11;
    case SWCCommand::Next:       return 16; // 14 - 19
    case SWCCommand::Prev:       return 24; // 20 - 28
    case SWCCommand::VolUp:      return 35; // 29 - 41
    case SWCCommand::VolDown:    return 52; // 42 - 65
    case SWCCommand::None:       return 0;
  }
  return 0;
}

void PioneerSWC::press(SWCCommand command) {
  Serial.print("PioneerSWC::press(command=");
  Serial.print(static_cast<int>(command));
  Serial.println(")");
  uint8_t value = getValueForCommand(command);
  setPot(value);
}

void PioneerSWC::release() {
  Serial.print("PioneerSWC::release()");
  setPot(255);
}
