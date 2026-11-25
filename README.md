# Barrier Gate System – ESP32-S3 Sensor Node

An ESP32-S3 reads the sensors of a car barrier gate and talks to a **Raspberry Pi over I2C**.

- **IR obstacle sensor (KY-032)**: detects when a car passes and tells the Pi.
- **DHT11 (KY-015)**: sends temperature and humidity to the Pi every hour.
- **Photoresistor (KY-018)**: tells the Pi whether it is day or night.
- **Active buzzer (KY-012)**: the Pi turns it on and off. It is **off** by default.

The Raspberry Pi is the I2C **master**. The ESP32 is an I2C **slave** at address **`0x08`**.

---

## Hardware

| # | Part | Model |
|---|---|---|
| 1 | Microcontroller | ESP32-S3 N16R8 (DevKitC-1 layout) |
| 2 | Main controller | Raspberry Pi (I2C bus 1) |
| 3 | Active buzzer | KY-012 |
| 4 | Temperature and humidity sensor | KY-015 (DHT11) |
| 5 | Photoresistor | KY-018 |
| 6 | IR obstacle avoidance sensor | KY-032 |
| 7 | Breadboard and jumper wires | Used for the shared 3V3 and GND rails |

---

## Pin Summary

| Device | Module pin | ESP32-S3 pin | Type |
|---|---|---|---|
| Raspberry Pi | SDA (Pi GPIO2, physical pin 3) | **GPIO8** | I2C data |
| Raspberry Pi | SCL (Pi GPIO3, physical pin 5) | **GPIO9** | I2C clock |
| Raspberry Pi | GND (physical pin 6) | **GND** | Ground |
| KY-012 Buzzer | S | **GPIO4** | Digital output |
| KY-015 DHT11 | S | **GPIO5** | Digital (1-wire) |
| KY-018 Photoresistor | S | **GPIO1** (ADC1_CH0) | Analog input |
| KY-032 IR sensor | OUT | **GPIO6** | Digital input (LOW = obstacle) |
| All sensors | + / VCC | **3V3** (through the + rail) | Power |
| All sensors and the Pi | − / GND | **GND** (through the − rail) | Ground |

---

## Full ESP32-S3 Wiring Diagram

This is the whole ESP32-S3 DevKitC-1 board seen from the top, with the USB ports at the bottom.
An arrow (`◄──` or `──►`) marks a pin that is used in this project. Pins without an arrow are not used.

```
                                  ┌─────────────────────┐
          + rail (3V3) ◄──   3V3 ●│                     │● GND    ──► - rail (GND)
                             3V3 ●│                     │● TX (43)
                             RST ●│                     │● RX (44)
       KY-012 Buzzer S ◄── GPIO4 ●│                     │● GPIO1  ──► KY-018 Photoresistor S
        KY-015 DHT11 S ◄── GPIO5 ●│                     │● GPIO2
         KY-032 IR OUT ◄── GPIO6 ●│                     │● GPIO42
                           GPIO7 ●│                     │● GPIO41
                          GPIO15 ●│       ESP32-S3      │● GPIO40
                          GPIO16 ●│        N16R8        │● GPIO39
                          GPIO17 ●│      DevKitC-1      │● GPIO38
                          GPIO18 ●│                     │● GPIO37   (PSRAM - do not use)
        Pi SDA (pin 3) ◄── GPIO8 ●│                     │● GPIO36   (PSRAM - do not use)
                           GPIO3 ●│                     │● GPIO35   (PSRAM - do not use)
                          GPIO46 ●│                     │● GPIO0    (BOOT button)
        Pi SCL (pin 5) ◄── GPIO9 ●│                     │● GPIO45
                          GPIO10 ●│                     │● GPIO48   (RGB LED)
                          GPIO11 ●│                     │● GPIO47
                          GPIO12 ●│                     │● GPIO21
                          GPIO13 ●│                     │● GPIO20   (USB D+)
                          GPIO14 ●│                     │● GPIO19   (USB D-)
                              5V ●│                     │● GND
                             GND ●│                     │● GND
                                  └───┤ USB ├───┤UART ├─┘
                                      └─────┘   └─────┘
```

> The pin order above follows the official ESP32-S3-DevKitC-1. Some clone boards put the pins in a different order, so **always check the labels printed on your board**.

---

## Power Rails (Shared 3V3 and GND)

The ESP32 has only a few 3V3 and GND pins, so all modules share them through the breadboard rails.
Every GND is the same electrical point, so sharing is safe.

```
ESP32 3V3 ───► [ + rail ] ──┬── KY-015 middle pin (VCC)
                            ├── KY-018 middle pin (VCC)
                            └── KY-032 +

ESP32 GND ───► [ − rail ] ──┬── KY-012 −
                            ├── KY-015 −
                            ├── KY-018 −
                            ├── KY-032 GND
                            └── Raspberry Pi GND (physical pin 6)
```

Together the sensors draw less than 60 mA, which the ESP32's 3.3 V regulator supplies easily.

---

## Wiring for Each Module

### 1. Raspberry Pi (I2C)

```
 Raspberry Pi 40-pin header (top-left corner)

        3V3  (1) ●  ● (2)  5V
  SDA  GPIO2 (3) ●  ● (4)  5V
  SCL  GPIO3 (5) ●  ● (6)  GND
             ...    ...

  Pi pin 3 (SDA) ─────────── ESP32 GPIO8
  Pi pin 5 (SCL) ─────────── ESP32 GPIO9
  Pi pin 6 (GND) ─────────── ESP32 GND (− rail)
```

- **Do not** connect the Pi's 3V3 or 5V to the ESP32. Each board has its own power supply.
- Both boards use 3.3 V logic, so no level shifter is needed.
- The Pi already has pull-up resistors on SDA and SCL, so you don't need to add any.

### 2. KY-012 Active Buzzer

```
   ┌─────────┐
   │  (( ))  │
   │ BUZZER  │
   └─┬──┬──┬─┘
     S  •  −
     │  │  │
     │  │  └──── GND (− rail)
     │  └─────── not connected
     └────────── ESP32 GPIO4
```

### 3. KY-015 DHT11 Temperature and Humidity Sensor

```
   ┌─────────┐
   │ ▒▒▒▒▒▒▒ │
   │  DHT11  │
   └─┬──┬──┬─┘
     S  +  −
     │  │  │
     │  │  └──── GND (− rail)
     │  └─────── 3V3 (+ rail)
     └────────── ESP32 GPIO5
```

### 4. KY-018 Photoresistor

```
   ┌─────────┐
   │   (~)   │
   │   LDR   │
   └─┬──┬──┬─┘
     S  +  −
     │  │  │
     │  │  └──── GND (− rail)
     │  └─────── 3V3 (+ rail)
     └────────── ESP32 GPIO1 (ADC1)
```

### 5. KY-032 IR Obstacle Avoidance Sensor

```
   ┌──────────────┐
   │  (IR)  (IR)  │
   │   TX    RX   │
   └─┬───┬───┬───┬┘
    GND  +  OUT  EN
     │   │   │   │
     │   │   │   └── not connected (keep the jumper on the board)
     │   │   └────── ESP32 GPIO6
     │   └────────── 3V3 (+ rail)
     └────────────── GND (− rail)
```

> ⚠️ **Power every sensor from 3V3, not 5V.** ESP32-S3 pins are not 5 V tolerant.

---

## I2C Protocol

### ESP32 → Pi (the Pi reads 32 bytes, text padded with `\0`)

| Message | When it is sent |
|---|---|
| `CAR_PASSED` | A car blocked the IR sensor and then left it |
| `TEMP:25.0,HUM:60.0` | At startup, then every hour |
| `DHT_ERROR` | The DHT11 reading failed |
| `LIGHT:DAY` / `LIGHT:NIGHT` | At startup, and whenever it changes between day and night |
| `NONE` | Nothing new |

The ESP32 can hold up to 20 messages in a queue. The Pi should read often, for example every 200 ms.

### Pi → ESP32 (the Pi writes 1 byte)

| Byte | Action |
|---|---|
| `1` | Buzzer ON |
| `0` | Buzzer OFF (the default) |

---

## Raspberry Pi Example (Python)

```bash
sudo raspi-config        # Interface Options → I2C → Enable
pip install smbus2
i2cdetect -y 1           # should show 08
```

```python
from smbus2 import SMBus, i2c_msg
import time

ADDR = 0x08
bus = SMBus(1)

def read_esp():
    msg = i2c_msg.read(ADDR, 32)
    bus.i2c_rdwr(msg)
    return bytes(msg).split(b'\0')[0].decode()

def buzzer(on):
    bus.write_byte(ADDR, 1 if on else 0)

while True:
    text = read_esp()
    if text != "NONE":
        print("ESP32:", text)
        if text == "CAR_PASSED":
            buzzer(True); time.sleep(0.5); buzzer(False)
    time.sleep(0.2)
```

> Use `i2c_msg.read`, **not** `read_i2c_block_data`. That function sends a `0` byte first, and the ESP32 would take it as "buzzer OFF".

---

## Build and Upload

The project uses [PlatformIO](https://platformio.org/).

```bash
pio run                  # build
pio run -t upload        # upload
pio device monitor       # serial monitor (115200 baud)
```

Libraries (PlatformIO installs them automatically):
- `adafruit/DHT sensor library`
- `adafruit/Adafruit Unified Sensor`

---

## Calibration

| Setting in `src/main.cpp` | What to do |
|---|---|
| `DARK_IS_HIGH` | If the light reading is reversed (it says `LIGHT:DAY` when you cover the sensor), change it to `false` |
| `LIGHT_THRESHOLD` | The value (0–4095) that separates day from night. Change it to fit your lighting |
| `DHT_INTERVAL` | How often the DHT11 data is sent. The default is 1 hour |
| KY-032 potentiometers | Turn them to set the IR detection distance |

---

## Pins to Avoid on the ESP32-S3 N16R8

| GPIO | Reason |
|---|---|
| 26–32 | Used by the flash memory (not on the header) |
| 35, 36, 37 | Used by the octal PSRAM |
| 0, 3, 45, 46 | Strapping pins that affect boot |
| 19, 20 | Native USB |
| 43, 44 | UART0, used by the serial monitor |
| 48 | Onboard RGB LED |
