#include <Arduino.h>
#include <SPI.h>
#include <mcp2515.h>
#include "PioneerSWC.h"

#define WAIT_FOR_SERIAL false
#define DEBUG_PRINT_TO_SERIAL true

struct can_frame canMsg;

// CAN bus addresses
const __u32 SWC_ADDR = /*0x90438040; */ 0x10438040;
const __u32 CD400_ADDR = /*0x90AD6080;*/ 0x10AD6080;
const __u8 SOURCE_CHANGED_ID = 0x00;
const __u8 SOURCE_AUX = 0x08;
const __u8 CD400_MEDIA_KEY = 0x03;

// MCP2515 configuration
const int MCP2515_SPI_CS_PIN = 10;
const uint8_t INT_PIN = A0;

// SWC and Media Source states
bool swcPressed = false;
__u8 mediaSource = 0x00;
bool muteHeld = false;
unsigned long mutePressTime = 0;
#define MUTE_HOLD_TIME 2000 // 2 seconds

// Initialize MCP2515 CAN controller
MCP2515 mcp2515(MCP2515_SPI_CS_PIN);

// MCP42100 Potentiometer control
const uint8_t MCP41100_CS_PIN   = 6;
const uint8_t MCP41100_MOSI_PIN = 4;
const uint8_t MCP41100_SCK_PIN  = 5;
PioneerSWC swc(MCP41100_CS_PIN, MCP41100_SCK_PIN, MCP41100_MOSI_PIN);

void sendSWCPressToPioneer(__u8 key);
void sendUnpressToPioneer();

void debugPrint(const char* message, bool newline = false) {
  if (DEBUG_PRINT_TO_SERIAL) {
    if (newline) {
      Serial.println(message);
    }
    else {
      Serial.print(message);
    }
  }
}

void setup() {
  Serial.begin(115200);
  while (WAIT_FOR_SERIAL && !Serial) {
    delay(1000);
    // Serial.println("Waiting for Serial...");
  }

  mcp2515.reset();
  mcp2515.setBitrate(CAN_33KBPS);
  mcp2515.setNormalMode();

  swc.begin();
  swc.release();

  debugPrint("------- CAN Read ----------", true);
  debugPrint("ID  DLC   DATA", true);
}

void loop() {
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
  // Serial.print(canMsg.can_id, HEX);
    // Serial.print(" ");
    // Serial.print(canMsg.can_dlc, HEX);
    // Serial.print(" ");

    // for (int i = 0; i < canMsg.can_dlc; i++) {
    //   Serial.print(canMsg.data[i], HEX);
    //   Serial.print(" ");
    // }

    // Serial.println();

    // if (false){//canMsg.can_dlc == 8) {
    //   Serial.print("ID ");
    //   Serial.print(canMsg.can_id, HEX);
    //   Serial.print(" DLC ");
    //   Serial.print(canMsg.can_dlc, HEX);
    //   Serial.print(" DLC: ");

    //   for (int i = 0; i < canMsg.can_dlc; i++) {
    //     Serial.print(canMsg.data[i], HEX);
    //     Serial.print(" ");
    //   }

    //   Serial.println();
    // }

    __u32 id = canMsg.can_id;
    // Serial.print(id);
    // Serial.println();
    // Source change (CD400)
    if ((id & CD400_ADDR) == CD400_ADDR && canMsg.data[0] == SOURCE_CHANGED_ID && canMsg.data[1] == 0x12) {
      __u8 prevMediaSource = mediaSource;
      mediaSource = canMsg.data[2];
      if (DEBUG_PRINT_TO_SERIAL) {
        Serial.print("Source: ");
        Serial.println(mediaSource, HEX);
      }

      // If the source changed from AUX to something else, release SWC
      if (prevMediaSource == SOURCE_AUX && mediaSource != SOURCE_AUX && swcPressed) {
        debugPrint("Source changed. Forcing SWC release.", true);
        swcPressed = false;
        muteHeld = false;
        sendUnpressToPioneer(); // emulate unpress
      }
    }

    // SWC Buttons
    if ((id & SWC_ADDR) == SWC_ADDR && canMsg.can_dlc == 1) {
      __u8 key = canMsg.data[0];
      // Serial.println("Got SWC");
      // Serial.print("SWC Key: ");
      // Serial.println(key, HEX);
      if (key == 0x00) {
        // Release is always processed
        if (swcPressed) {
          debugPrint("SWC released (Unpress All)", true);
          swcPressed = false;

          if (muteHeld) {
            muteHeld = false;
            debugPrint("Mute hold cancelled", true);
          }

          sendUnpressToPioneer();
        }
      }
      // Press when AUX is active
      else if (mediaSource == SOURCE_AUX) {
        swcPressed = true;

        if (key == 0x07) { // Mute button hold logic
          if (!muteHeld) {
            mutePressTime = millis();
            muteHeld = true;
          } else if (millis() - mutePressTime >= MUTE_HOLD_TIME) {
            swc.press(SWCCommand::DisplayOff);
            debugPrint("Mute held for 2s: Emulate Display Off", true);
            muteHeld = false;
          }
        } else {
          muteHeld = false;
          if (DEBUG_PRINT_TO_SERIAL) {
            Serial.print("SWC Key pressed: ");
            Serial.println(key, HEX);
          }
          sendSWCPressToPioneer(key);
        }
      }
    }

    if ((id & CD400_ADDR) == CD400_ADDR && canMsg.data[0] == CD400_MEDIA_KEY && mediaSource == SOURCE_AUX) {
      __u8 key = canMsg.data[2];
      __u8 state = canMsg.data[7];

      // Press
      if (state == 0x00) {
        if (DEBUG_PRINT_TO_SERIAL) {
          Serial.print("CD400 Panel Key: ");
          Serial.println(key, HEX);
        }
        sendSWCPressToPioneer(key);
        swcPressed = true;
      } 
      // Release
      else /*if (state == 0x01 || state == 0x02)*/ {
        debugPrint("CD400 Panel Key released", true);
        sendUnpressToPioneer();
        swcPressed = false;
      }
    }
  }
}

void sendSWCPressToPioneer(__u8 key) {
  switch (key) {
    case 0x01: // Vol Up
      debugPrint("Ignore Vol Up key (0x01) from SWC, Pioneer does not need it", true);
      // swc.press(SWCCommand::VolUp);
      // Serial.println("Emulate: Vol Up");
      break;

    case 0x02: // Vol Down
      debugPrint("Ignore Vol Down key (0x02) from SWC, Pioneer does not need it", true);
      // swc.press(SWCCommand::VolDown);
      // Serial.println("Emulate: Vol Down");
      break;

    case 0x03: // Next
    case 0x13: // CD400 Next
      swc.press(SWCCommand::Next);
      debugPrint("Emulate: Next Track", true);
      break;

    case 0x04: // Prev
    case 0x19: // CD400 Prev
      swc.press(SWCCommand::Prev);
      debugPrint("Emulate: Prev Track", true);
      break;

    case 0x05: // SRC
      debugPrint("Ignore SRC key (0x05) from SWC, Pioneer does not need it", true);
      // swc.press(SWCCommand::Source);
      // Serial.println("Emulate: Source");
      break;

    case 0x06: // Phone Up / Voice
      debugPrint("Ignore Phone Up / Voice key (0x06) from SWC, Pioneer does not support it", true);
      // swc.press(SWCCommand::DisplayOff);
      // Serial.println("Emulate: Phone Up / Voice as Display Off");
      break;

    case 0x18: // CD400 Play/Pause
      swc.press(SWCCommand::Mute);
      debugPrint("Emulate: CD400 Play/Pause as Mute", true);
      break;

    case 0x07: // Mute
      debugPrint("Ignore Mute key (0x07) from SWC, Pioneer does not support it", true);
      // swc.press(SWCCommand::Mute);
      // Serial.println("Emulate: Mute / Phone Down");
      break;

    default:
      debugPrint("Unknown key, no action", true);
      return;
  }
}

void sendUnpressToPioneer() {
  debugPrint("Pioneer: emulate unpress", true);
  swc.release();
}