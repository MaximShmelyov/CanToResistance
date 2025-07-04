#include <Arduino.h>
#include <SPI.h>
#include <mcp2515.h>

struct can_frame canMsg;

const __u32 SWC_ADDR = 0x10438040;
const __u32 CD400_ADDR = 0x10AD6080;

const int MCP2515_SPI_CS_PIN = 10;
const int INT_PIN = 2;

MCP2515 mcp2515(MCP2515_SPI_CS_PIN);

// 0x08 = AUX
int mediaSource = -1;

void setup() {
  Serial.begin(115200);

  mcp2515.reset();
  mcp2515.setBitrate(CAN_33KBPS);
  mcp2515.setNormalMode();

  Serial.println("------- CAN Read ----------");
  Serial.println("ID  DLC   DATA");
}

void loop() {
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    Serial.print(canMsg.can_id, HEX);
    Serial.print(" ");
    Serial.print(canMsg.can_dlc, HEX);
    Serial.print(" ");

    for (int i = 0; i < canMsg.can_dlc; i++) {
      Serial.print(canMsg.data[i], HEX);
      Serial.print(" ");
    }

    Serial.println();

    // Media source change detection
    if (canMsg.can_id == CD400_ADDR && canMsg.data[0] == 0x00 && canMsg.data[1] == 0x12) {
      mediaSource = canMsg.data[2];
      Serial.print("Active media source changed to: ");
      Serial.println(mediaSource, HEX);
    }

    // SWC
    if (canMsg.can_id == SWC_ADDR && canMsg.can_dlc == 1 && mediaSource == 0x08) {
      uint8_t key = canMsg.data[0];
      Serial.print("SWC Key: ");
      Serial.println(key, HEX);

      switch (key) {
        case 0x00:
          Serial.println("Unpress (All)");
          break;
        case 0x01:
          Serial.println("Vol Up");
          break;
        case 0x02:
          Serial.println("Vol Down");
          break;
        case 0x03:
          Serial.println("Next");
          break;
        case 0x04:
          Serial.println("Prev");
          break;
        case 0x05:
          Serial.println("SRC");
          break;
        case 0x06:
          Serial.println("Phone up / Voice");
          break;
        case 0x07:
          Serial.println("Mute / Phone down");
          break;
        default:
          Serial.println("Unknown SWC key");
      }
    }

    // CD400 panel key processing
    if (canMsg.can_id == CD400_ADDR && canMsg.data[0] == 0x03 && mediaSource == 0x08) {
      uint8_t key = canMsg.data[2];
      uint8_t pressState = canMsg.data[7];

      if (pressState == 0x00) { // Only on press
        Serial.print("CD400 Panel Key Pressed: ");
        Serial.println(key, HEX);

        switch (key) {
          case 0x13:
            Serial.println("Next (CD400)");
            break;
          case 0x19:
            Serial.println("Prev (CD400)");
            break;
          case 0x18:
            Serial.println("Play/Pause (CD400)");
            break;
          default:
            Serial.println("Unknown CD400 panel key");
        }
      }
    }
  }
}
