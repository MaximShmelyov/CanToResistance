#include <Arduino.h>
#include <SPI.h>
#include <mcp2515.h>

struct can_frame canMsg;

// CAN bus addresses
const __u32 SWC_ADDR = /*0x90438040; */ 0x10438040;
const __u32 CD400_ADDR = /*0x90AD6080;*/ 0x10AD6080;
const __u8 SOURCE_CHANGED_ID = 0x00;
const __u8 SOURCE_AUX = 0x08;
const __u8 CD400_MEDIA_KEY = 0x03;

// MCP2515 configuration
const int MCP2515_SPI_CS_PIN = 10;
const int INT_PIN = 2;

// SWC and Media Source states
bool swcPressed = false;
__u8 mediaSource = 0x00;

// Initialize MCP2515 CAN controller
MCP2515 mcp2515(MCP2515_SPI_CS_PIN);

// MCP42100 Potentiometer control
const int MCP42100_CS_PIN   = 6;
const int MCP42100_MOSI_PIN = 11;
const int MCP42100_SCK_PIN  = 9;
byte addressPot0 =     0b00010001;      //To define potentiometer use last two BITS 01= POT 0
byte addressPot1 =     0b00010010;      //To define potentiometer use last two BITS 10= POT 1
byte addressPot0and1 = 0b00010011;  //To define potentiometer use last two BITS 10= POT 0 and 1

void setResistance(byte address, uint32_t targetOhms);
void digitalPotWrite(byte value, byte address);
void sendSWCPressToPioneer(__u8 key);
void sendUnpressToPioneer();

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(1000);
    // Serial.println("Waiting for Serial...");
  }

  mcp2515.reset();
  mcp2515.setBitrate(CAN_33KBPS);
  mcp2515.setNormalMode();

  // Setup up MCP42100
  pinMode(MCP42100_CS_PIN, OUTPUT);
  pinMode(MCP42100_MOSI_PIN, OUTPUT);
  pinMode(MCP42100_SCK_PIN, OUTPUT);
  digitalWrite(MCP42100_CS_PIN, HIGH);
  digitalWrite(MCP42100_SCK_PIN, LOW); // SPI Mode 0: SCK начинается с LOW

  sendUnpressToPioneer(); // Start with unpressed state

  Serial.println("------- CAN Read ----------");
  Serial.println("ID  DLC   DATA");
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
      Serial.print("Source: ");
      Serial.println(mediaSource, HEX);

      // If the source changed from AUX to something else, release SWC
      if (prevMediaSource == SOURCE_AUX && mediaSource != SOURCE_AUX && swcPressed) {
        Serial.println("Source changed. Forcing SWC release.");
        swcPressed = false;
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
          Serial.println("SWC released (Unpress All)");
          swcPressed = false;
          sendUnpressToPioneer();
        }
      }
      // Press when AUX is active
      else if (mediaSource == SOURCE_AUX) {
        swcPressed = true;
        Serial.print("SWC Key pressed: ");
        Serial.println(key, HEX);
        sendSWCPressToPioneer(key);
      }
    }

    if ((id & CD400_ADDR) == CD400_ADDR && canMsg.data[0] == CD400_MEDIA_KEY && mediaSource == SOURCE_AUX) {
      __u8 key = canMsg.data[2];
      __u8 state = canMsg.data[7];

      // Press
      if (state == 0x00) {
        Serial.print("CD400 Panel Key: ");
        Serial.println(key, HEX);
        sendSWCPressToPioneer(key);
        swcPressed = true;
      } 
      // Release
      else /*if (state == 0x01 || state == 0x02)*/ {
        Serial.println("CD400 Panel Key released");
        sendUnpressToPioneer();
        swcPressed = false;
      }
    }
  }
}

void sendSWCPressToPioneer(__u8 key) {
  uint32_t resistance = 100000; // Default high impedance (~100kΩ)

  switch (key) {
    case 0x01: // Vol Up
      resistance = 2100; // ~2.1kΩ
      Serial.println("Emulate: Vol Up");
      break;

    case 0x02: // Vol Down
      resistance = 3100; // ~3.1kΩ
      Serial.println("Emulate: Vol Down");
      break;

    case 0x03: // Next
    case 0x13: // CD400 Next
      resistance = 740; // ~0.74kΩ
      Serial.println("Emulate: Next Track");
      break;

    case 0x04: // Prev
    case 0x19: // CD400 Prev
      resistance = 1300; // ~1.3kΩ
      Serial.println("Emulate: Prev Track");
      break;

    // case 0x05: // SRC
    //   resistance = 270; // ~270Ω
    //   Serial.println("Emulate: Source");
    //   break;

    case 0x06: // Phone Up / Voice
      resistance = 4600; // ~4.6kΩ
      Serial.println("Emulate: Voice");
      break;

    case 0x18: // CD400 Play/Pause
      resistance = 4600; // ~4.6kΩ @TODO: find Play/Pause value
      Serial.println("Emulate: CD400 Play/Pause");
      break;

    // case 0x07: // Mute
    //   resistance = 8600; // ~8.6kΩ
    //   Serial.println("Emulate: Mute / Phone Down");
    //   break;

    default:
      Serial.println("Unknown key, no action");
      return;
  }
  setResistance(addressPot0, resistance);
}

void sendUnpressToPioneer() {
  Serial.println("Pioneer: emulate unpress");
  uint32_t resistance = 100000; // ~100kΩ (high impedance)
  setResistance(addressPot0and1, resistance);
  setResistance(addressPot0, resistance);
  setResistance(addressPot1, resistance);
}

void setResistance(byte address, uint32_t targetOhms) {
  const uint32_t maxOhms = 100000; // For MCP42100 (100k)
  const int steps = 255;      // 8-bit resolution

  int position = constrain((long)targetOhms * steps / maxOhms, 0, 255);

  digitalPotWrite(position, address);

  Serial.print("Resistance set to ~");
  Serial.print((long)position * maxOhms / steps);
  Serial.println(" ohms");
}


void spiTransfer(byte value);
// MCP42100 SPI transfer function
void digitalPotWrite(byte value, byte address)
{
  Serial.print("Writing value: ");
  Serial.print(value);
  Serial.print(" to address: ");
  Serial.println(address);
  digitalWrite(MCP42100_CS_PIN, LOW); //Set Chip Active
  spiTransfer(address);
  spiTransfer(value);
  digitalWrite(MCP42100_CS_PIN, HIGH); //Set Chip Inactive
}

void spiTransfer(byte value) {
  for (int i = 7; i >= 0; i--) {
    digitalWrite(MCP42100_MOSI_PIN, (value >> i) & 0x01); // Set bit
    digitalWrite(MCP42100_SCK_PIN, HIGH);                // Set clock high
    delayMicroseconds(1);                       // Short delay
    digitalWrite(MCP42100_SCK_PIN, LOW);                 // Set clock low
  }
}