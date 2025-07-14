#ifndef PIONEER_SWC_H
#define PIONEER_SWC_H

#include <Arduino.h>

enum class SWCCommand {
  Source,
  Mute,
  DisplayOff,
  Next,
  Prev,
  VolUp,
  VolDown,
  None  // Для release
};

class PioneerSWC {
  public:
    PioneerSWC(uint8_t csPin, uint8_t sckPin, uint8_t mosiPin);
    void begin();
    void press(SWCCommand command);
    void release();

  private:
    uint8_t _csPin;
    uint8_t _sckPin;
    uint8_t _mosiPin;
    bool _invertValue;
    void setPot(uint8_t value);
    void spiTransfer(uint8_t value);
    uint8_t getValueForCommand(SWCCommand command);
};

#endif
