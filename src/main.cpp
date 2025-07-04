#include <Arduino.h>
#include <SPI.h>
#include <mcp2515.h>

struct can_frame canMsg;

const __u32 SWC_ADDR = 0x10438040;
const __u32 CD400_ADDR = 0x10AD6080;

const int MCP2515_SPI_CS_PIN = 10;
const int INT_PIN = 2;

MCP2515 mcp2515(MCP2515_SPI_CS_PIN);
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
    Serial.print(canMsg.can_id, HEX); // print ID
    Serial.print(" "); 
    Serial.print(canMsg.can_dlc, HEX); // print DLC
    Serial.print(" ");
    
    for (int i = 0; i<canMsg.can_dlc; i++)  {  // print the data
      Serial.print(canMsg.data[i],HEX);
      Serial.print(" ");
    }

    Serial.println();      

    if (canMsg.can_id == SWC_ADDR) {
      Serial.println("Key send message received");
      if (canMsg.can_dlc == 1) {
        Serial.print("Key: ");
        Serial.println(canMsg.data[0], HEX);

        switch (canMsg.data[0]) {
          case 0x00:
            Serial.println("Unpress (All)");
            // @TODO
            break;
          case 0x01:
            Serial.println("Vol Up");
            // @TODO: Implement volume up functionality
            break;
          case 0x02:
            Serial.println("Vol Down");
            // @TODO: Implement volume down functionality
            break;
          case 0x03:
            Serial.println("Next");
            // @TODO: Implement next track functionality
            break;
          case 0x04:
            Serial.println("Prev");
            // @TODO: Implement previous track functionality
            break;
          case 0x05:
            Serial.println("SRC");
            // @TODO: Implement source change functionality
            break;
          case 0x06:
            Serial.println("Phone up / Voice");
            // @TODO: Implement phone up / voice functionality
            break;
          case 0x07:
            Serial.println("Mute / Phone down");
            // @TODO: Implement mute / phone down functionality
            break;
          default:
            Serial.println("Unknown key");
        }
      } else {
        Serial.println("Invalid key send message format");
      }
    }
  }
}
