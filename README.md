# SpotCheck

SpotCheck is an ESP32-based parking spot occupancy monitor I built as a personal project. It uses an HC-SR04 ultrasonic sensor and a PIR motion sensor together to detect whether a car is parked in a spot. When a car is detected, the system closes a servo barrier, updates the status LEDs and a 16x2 LCD, and triggers a buzzer if an intrusion is detected (motion without an authorized vehicle).

I also designed a custom PCB in KiCad and a 3D printed wall-mounted enclosure in Fusion 360 to house everything.

---

## What It Does

- Continuously measures distance with the HC-SR04 to detect a parked car
- Uses the PIR sensor to catch motion when no car is present (intrusion detection)
- Closes the servo barrier when a car parks, opens it when the spot is free
- Green LED for available, red LED for occupied or alert
- LCD shows real-time spot status
- Buzzer sounds on intrusion (driven through a 2N2222 transistor)
- Serial output over USB for debugging

---

## Component List

| Component | Qty |
|---|---|
| ESP32 Dev Board | 1 |
| HC-SR04 Ultrasonic Sensor | 1 |
| PIR Motion Sensor (HC-SR501) | 1 |
| SG90 Servo Motor | 1 |
| I2C LCD 16x2 (PCF8574 backpack) | 1 |
| Active Buzzer | 1 |
| 2N2222 NPN Transistor | 1 |
| LM7805 Voltage Regulator (5V) | 1 |
| AMS1117-3.3 Voltage Regulator (3.3V) | 1 |
| Green LED | 1 |
| Red LED | 1 |
| 4.7k Resistors (I2C pull-ups) | 2 |
| Resistors for voltage divider (1k + 2k) | 2 |
| M3 screws and inserts | 4 |

---

## PCB Design

I designed a custom 2-layer PCB in KiCad (100x80mm). Here are the main design decisions:

**Dual rail power supply:** The board takes a higher voltage input (9-12V) and uses an LM7805 to regulate it down to 5V for the HC-SR04 and servo, and an AMS1117-3.3 to supply 3.3V to the ESP32 and other logic. Having both rails on-board means I only need one power input.

**Voltage divider on the ECHO line:** The HC-SR04 ECHO pin outputs 5V logic, but the ESP32 GPIO pins are 3.3V tolerant. Running 5V into them directly risks damaging the chip over time. I added a 1k/2k resistor voltage divider on that specific line to bring it down to about 3.3V before it reaches the ESP32. Simple and effective.

**2N2222 transistor for the buzzer:** The ESP32 GPIO pins can not source enough current to drive an active buzzer reliably. The 2N2222 NPN transistor handles the load. The GPIO just switches the base, the transistor pulls the buzzer to ground through the collector, and the buzzer gets its current from the 5V rail.

**4.7k I2C pull-up resistors:** The I2C spec requires pull-ups on SDA and SCL. The ESP32 has weak internal pull-ups but external 4.7k resistors give cleaner signal edges, especially with longer traces on the PCB.

I routed the board using FreeRouting via a Specctra DSN export from KiCad, then went back in manually to fix a few DRC errors, and exported the full Gerber set for JLCPCB.

---

## Enclosure

Designed in Fusion 360 for FDM 3D printing. It is a wall-mounted box with a 45-degree chamfered front face so the sensors angle diagonally downward toward the parking spot. The front has cutouts for the LCD and LEDs. The back is a removable lid held on with M3 screws into heat-set inserts, which makes it easy to get inside for maintenance or reflashing without breaking anything.

---

## Getting It Running

### Required Libraries

Install these through the Arduino Library Manager (Sketch > Include Library > Manage Libraries):

- **NewPing** by Tim Eckel
- **LiquidCrystal_I2C** by Frank de Brabander
- **Servo** (included with the ESP32 board package)

### ESP32 Board Setup

If you have not set up the ESP32 in Arduino IDE yet:

1. Open File > Preferences
2. Paste this URL into "Additional Board Manager URLs":
   `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
3. Go to Tools > Board > Boards Manager and install "esp32 by Espressif Systems"

### Uploading the Firmware

1. Clone this repo
2. Open `firmware/spotcheck.ino` in Arduino IDE
3. Select your ESP32 board from Tools > Board
4. Select the correct COM port under Tools > Port
5. Hit Upload

### I2C Address

The LCD default address is `0x27` in the code. If your LCD stays blank after upload, your PCF8574 backpack might be at `0x3F` instead. Run a quick I2C scanner sketch to check and update `LCD_ADDR` in the code.

---

## Gerbers

The Gerber files for the PCB are in [`hardware/gerbers/`](hardware/gerbers/). They are exported and ready to upload directly to JLCPCB.

---

## License

MIT License. See [LICENSE](LICENSE).
