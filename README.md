# vx220-microcontroller



The ESP32 WROOM is running a custom firmware that acts as a analog to digital converter for the sensors. It then sends the data to the Raspberry Pi 4 over UART. 

```ESP32 GPIO17 (TX) ──────────────> Pi GPIO15 (RX) [Pin 10]
ESP32 GPIO16 (RX) <───────────── Pi GPIO14 (TX) [Pin 8]
ESP32 GND        ─────────────── Pi GND         [Pin 6]
``` 

Analog information will be harvested from the existing dashboard connector, while additional 3rd-party analog feeds (for now turbo pressure & oil temperature), will be connected independently. 

| Signal Name | Signal Type | Source Unit | Voltage Level | Connector Pin (est.) | Output to Raspberry Pi GPIO | ADC Resolution / Interface Suggestion |
| --- | --- | --- | --- | --- | --- | --- |
| RPM | Frequency-based (pulse train) | ECU tach output or coil trigger | ~12V square wave | ECU pin C8 (to cluster) | Frequency counter via microcontroller (interrupt-based) | Not applicable – count pulses per second |
| Coolant Temp | PWM duty-cycle | ECU PWM output | ~12V PWM | ECU pin C14 (to cluster) | Duty cycle reader via microcontroller PWM input or filtered ADC | 10-bit+ ADC or timer-capable MCU |
| Speed | Frequency-based (pulse train) | ABS module | ~12V square wave | ABS pin 25 (speed pulse) | Frequency counter or pulse capture (optocoupler to GPIO) | Not applicable – frequency-based |
| Fuel Level | Resistive analog | Tank level sender | 0–5V (via divider) | Cluster pin 11 (X30 plug) | Analog input via voltage divider to ADC | 10–12 bit ADC recommended |
| Oil Pressure (stock)m | On/Off (binary switch) | Oil pressure switch | 12V switched ground | Cluster pin 6 (X30 plug) | Digital GPIO with optocoupler or level shifter | None – digital only |
| Oil Pressure (aftermarket) | Analog 0–5V sensor | External sensor (0–150 psi) | 0–5V | Custom input (T-fitting) | Buffered analog input to ADC | 10–12 bit ADC (e.g. MCP3008) |
| Turbo Boost (MAP) | Analog 0–5V sensor | MAP sensor on intake manifold | 0–5V | ECU pin C18 or C5 | Analog input (buffered) | 10–12 bit ADC |
| MIL (Check Engine) | On/Off (binary) | ECU warning output | 12V when active | ECU pin C16 or cluster pin 17 | GPIO with optocoupler | None – digital only |
| ABS Warning | On/Off (binary) | ABS module | 12V switched | Cluster pin 12 (X30) | GPIO with level shifting or optocoupler | None |
| Airbag Warning | On/Off (binary) | Airbag ECU | 12V switched | Cluster pin 5 (X30) | GPIO with optocoupler | None |
| Turn Signals (L/R) | On/Off (12V pulsed) | Flasher relay outputs | 12V flashing | Cluster pins 9/10 (X30) | Edge detection or GPIO with pulse detection | Optional smoothing or interrupt logic |
| High Beam | On/Off (12V line) | Headlight relay | 12V constant | Cluster pin 8 (X30) | GPIO with resistor divider or opto-isolation | None |
| Parking Brake | On/Off (switch) | Handbrake lever switch | 12V switched ground | Cluster pin 4 (X30) | GPIO digital input | None |

Documentation for the ESP32 -> Raspberry Pi 4 payload can be found in the [ESP32 TLV Payload Specification](docs/esp32-payload.md) file. This document details the TLV (Type-Length-Value) frame structure, sensor data types, and provides examples of the communication protocol between the ESP32 and Raspberry Pi.


This document describes the structure of the payload sent by the ESP32 to the Raspberry Pi through UART, using a TLV (Type-Length-Value) format for extensibility and robustness.

---

## 1. Frame Structure

Each UART frame sent from the ESP32 to the Raspberry Pi has the following structure:

```
| HDR | LEN | VER |   TLV Payload   | CRC16 | EOF |
|-----|-----|-----|-----------------|-------|-----|
|0xAA |  1B | 1B  |   variable      |  2B   |0x55 |
```
- **HDR**: Start byte (0xAA)
- **LEN**: Payload length in bytes (excluding HDR, LEN, CRC16, EOF)
- **VER**: Protocol version (start at 0x01)
- **TLV Payload**: One or more TLV-encoded sensor values
- **CRC16**: CRC-16-CCITT (XModem or CCITT-FALSE) over VER and TLV Payload
- **EOF**: End byte (0x55)

---

## 2. TLV Payload Format

The payload consists of a sequence of TLV (Type-Length-Value) entries:

```
| ID  | L   | Value         |
|-----|-----|---------------|
| 1B  | 1B  | L bytes       |
```
- **ID**: Sensor or data type identifier (see table below)
- **L**: Length of Value in bytes
- **Value**: Raw data, big-endian encoding

Multiple TLVs can be concatenated in a single frame.

---

## 3. Sensor/Data Type ID Map

| ID   | Name                | Type    | Encoding         | Range/Scale         |
|------|---------------------|---------|------------------|---------------------|
| 0x01 | Fuel level          | uint16  | Big-endian       | 0–4095 (ADC)        |
| 0x02 | Oil pressure        | uint16  | Big-endian       | 0–4095 (ADC)        |
| 0x03 | Boost pressure      | uint16  | Big-endian       | 0–4095 (ADC)        |
| 0x04 | RPM                 | uint16  | Big-endian       | 0–16000             |
| 0x05 | Vehicle speed       | uint16  | Big-endian       | 0–400               |
| 0x06 | Status flags        | uint8   | Bitfield         | See below           |
| 0x07 | Steering angle      | int16   | Big-endian       | -7200 to +7200 (0.1°/LSB) |
| 0x08 | Brake pressure      | uint16  | Big-endian       | 0–20000 (0.01 bar/LSB) |
| 0x09 | Throttle position   | uint8   |                  | 0–100 (%)           |
| 0x0A | Gear position       | uint8   |                  | 0–7                 |
| 0x0B | Tyre pressure FL    | uint16  | Big-endian       | 0–400 (0.01 bar/LSB)|
| 0x0C | Tyre pressure FR    | uint16  | Big-endian       | 0–400 (0.01 bar/LSB)|
| 0x0D | Tyre pressure RL    | uint16  | Big-endian       | 0–400 (0.01 bar/LSB)|
| 0x0E | Tyre pressure RR    | uint16  | Big-endian       | 0–400 (0.01 bar/LSB)|
| 0x0F | Tyre temp FL        | int16   | Big-endian       | -200 to +2000 (0.1°C/LSB)|
| 0x10 | Tyre temp FR        | int16   | Big-endian       | -200 to +2000 (0.1°C/LSB)|
| 0x11 | Tyre temp RL        | int16   | Big-endian       | -200 to +2000 (0.1°C/LSB)|
| 0x12 | Tyre temp RR        | int16   | Big-endian       | -200 to +2000 (0.1°C/LSB)|

Unused codes (0x80–0xFF) are reserved for future use.

---

## 4. Status Flags (0x06)

Status flags are sent as a single byte (bitfield):
- Bit 0: MIL (Check Engine)
- Bit 1: ABS Warning
- Bit 2: Airbag Warning
- Bit 3: Left Turn
- Bit 4: Right Turn
- Bit 5: High Beam
- Bit 6: Parking Brake
- Bit 7: Reserved

---

## 5. Example Frame

Suppose the ESP32 sends fuel level, oil pressure, RPM, speed, status flags, and steering angle:

```
AA                  // HDR
14                  // LEN (20 bytes payload)
01                  // VER
01 02 0B D2         // TLV: Fuel level = 3026
02 02 03 20         // TLV: Oil pressure = 800
04 02 30 39         // TLV: RPM = 12345
05 02 00 96         // TLV: Speed = 150
06 01 12            // TLV: Flags = 0b00010010
07 02 01 F4         // TLV: Steering angle = +500 (50.0°)
A1 B4               // CRC16
55                  // EOF
```

---

## 6. Notes
- All multi-byte values are big-endian.
- CRC16 is calculated over VER and the entire TLV payload.
- The frame is extensible: new TLV IDs can be added without breaking compatibility.
- The Raspberry Pi should ignore unknown TLV IDs.

