# CAN-to-Pioneer SWC Bridge (Arduino)

This project listens to a vehicle CAN bus (via **MCP2515**) and emulates **Pioneer** Steering Wheel Control (SWC) button presses by driving a **digital potentiometer** (**MCP41xx/MCP41100**) that is wired into the Pioneer head unit’s SWC input.

It is designed for setups where:
- SWC buttons are available on the car CAN bus, **but**
- the head unit expects **resistive ladder** values on its SWC wire.

The firmware translates specific CAN frames (steering wheel buttons and/or a CD400 panel) into Pioneer-compatible “button press” resistor values using a digital pot.

---

## How it works

### 1) CAN input (MCP2515)
- Reads CAN frames using the `mcp2515` Arduino library.
- Uses **33 kbps** bitrate (`CAN_33KBPS`) in the current code (common for some GM / GMLAN segments).

### 2) Media source gating (AUX only)
The bridge only forwards presses when the current media source is **AUX**:
- It monitors a “source changed” frame from a module referred to as `CD400`.
- If source changes *from AUX to something else*, it force-releases any “stuck” SWC press to prevent the Pioneer from thinking a button is held down.

### 3) Pioneer SWC emulation (MCP41100)
`PioneerSWC` bit-bangs SPI to an MCP41xx digital potentiometer:
- Each Pioneer command maps to a wiper value (0–255) that corresponds to a resistive value on the SWC line.
- `press(cmd)` sets a command value.
- `release()` sets value `255` (no button).

---

## Hardware

### Required parts
- **Arduino** (tested conceptually with boards where pin `10` is available for SPI CS; adjust for your board)
- **MCP2515** CAN controller module (SPI)
- **MCP41xx / MCP41100** digital potentiometer (SPI-like interface; this project bit-bangs MOSI/SCK/CS)
- Vehicle CAN transceiver (usually included on MCP2515 modules)
- Pioneer head unit with resistive SWC input (wired according to your model)

### Default pinout (from the code)
**MCP2515**
- `CS`  → Arduino **D10**
- `INT` → Arduino **A0** (defined, but not used in the shown loop)

**MCP41100 (digital pot)**
- `CS`   → Arduino **D6**
- `MOSI` → Arduino **D4**
- `SCK`  → Arduino **D5**
- `MISO` is not required (write-only)

> ⚠️ Make sure you wire the digital potentiometer correctly into the Pioneer SWC circuit. Most Pioneer units use a resistive ladder to ground. Verify your head unit’s SWC wiring diagram.

---

## CAN message logic (current firmware)

### Addresses / IDs
The code matches incoming frame IDs using a mask-style check:

```cpp
if ((id & SWC_ADDR) == SWC_ADDR) { ... }
if ((id & CD400_ADDR) == CD400_ADDR) { ... }
```

Configured constants:

- `SWC_ADDR  = 0x10438040`
- `CD400_ADDR = 0x10AD6080`

### Source change (CD400)
Detects a “source changed” event when:
- `(id & CD400_ADDR) == CD400_ADDR`
- `data[0] == 0x00` (`SOURCE_CHANGED_ID`)
- `data[1] == 0x12`
- `data[2]` is treated as the new media source

AUX is:
- `SOURCE_AUX = 0x08`

When source leaves AUX and a button is considered pressed, it forces a release.

### Steering wheel buttons (SWC)
- Requires `(id & SWC_ADDR) == SWC_ADDR` and `DLC == 1`
- `data[0]` is the key
- `key == 0x00` is treated as “release all”
- Other keys are forwarded **only** if `mediaSource == SOURCE_AUX`

### CD400 panel media keys
When:
- `(id & CD400_ADDR) == CD400_ADDR`
- `data[0] == 0x03` (`CD400_MEDIA_KEY`)
- `mediaSource == SOURCE_AUX`

Then:
- `data[2]` is treated as a key
- `data[7]` is treated as state  
  - `0x00` → press  
  - otherwise → release

---

## Supported key mapping

The function `sendSWCPressToPioneer(key)` currently maps these keys:

| Input key | Meaning (comment)         | Pioneer emulation |
|----------:|----------------------------|-------------------|
| `0x03`    | Next                        | `Next`            |
| `0x13`    | CD400 Next                  | `Next`            |
| `0x04`    | Prev                        | `Prev`            |
| `0x19`    | CD400 Prev                  | `Prev`            |
| `0x18`    | CD400 Play/Pause            | `Mute`            |

These are currently **ignored** (by design, per comments):
- `0x01` Vol Up
- `0x02` Vol Down
- `0x05` SRC
- `0x06` Phone Up / Voice
- `0x07` Mute / Phone Down

> If you want to enable them, you can uncomment the corresponding `swc.press(...)` lines and adjust the `PioneerSWC` command mapping / resistance values to match your head unit.

---

## PioneerSWC command values

`PioneerSWC::getValueForCommand()` maps SWC commands to wiper values:

- `Source` → `0`
- `Mute` → `4`
- `DisplayOff` → `11`
- `Next` → `16`
- `Prev` → `24`
- `VolUp` → `35`
- `VolDown` → `52`
- `None` / release → `255`

There is also an `_invertValue` flag enabled by default:
- The code uses `value = 255 - value` before sending it to the pot.

If your wiring produces reversed behavior, set `_invertValue = false` in `PioneerSWC` (or expose it as a setting).

---

## Building & flashing

This repo looks compatible with **PlatformIO** (recommended) or Arduino IDE.

### PlatformIO (recommended)
1. Install PlatformIO (VS Code extension).
2. Create / adjust `platformio.ini` for your board (example below).
3. Build and upload.

Example `platformio.ini` (adjust board/ports to your setup):

```ini
[env:uno]
platform = atmelavr
board = uno
framework = arduino
monitor_speed = 115200

lib_deps =
  autowp/autowp-mcp2515
```

### Arduino IDE
1. Install required libraries:
   - MCP2515 library (e.g. `autowp-mcp2515`)
2. Open `src/main.cpp` as a sketch (or move code into an `.ino`).
3. Select board + port, then upload.

---

## Configuration

Key places to tweak:

- **CAN bitrate**  
  ```cpp
  mcp2515.setBitrate(CAN_33KBPS);
  ```
- **CAN address constants** (`SWC_ADDR`, `CD400_ADDR`)
- **Media source filtering** (`SOURCE_AUX`)
- **Pin definitions**
  - MCP2515 CS: `MCP2515_SPI_CS_PIN`
  - MCP41100 pins: `MCP41100_CS_PIN`, `MCP41100_MOSI_PIN`, `MCP41100_SCK_PIN`

---

## Safety & notes

- Automotive CAN work can affect vehicle behavior. Use **proper isolation**, fusing, and verify you are on the correct CAN segment.
- The firmware currently does *read-only* CAN, but always validate your wiring and modules.
- If you experience “stuck button” behavior, ensure the CAN “release” frames are correctly received, and verify the source-change release logic.

---

## License

MIT

---

## Disclaimer

This is a hobby/DIY project. You are responsible for ensuring safe installation and compliance with local laws and vehicle warranty constraints.
