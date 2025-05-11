import serial
import struct
import os

# Map TLV type IDs to names and value format
TLV_TYPES = {
    0x01: ("RPM", "H"),             # uint16_t
    0x02: ("Boost Pressure (mbar)", "H"),
    0x03: ("Oil Pressure", "H"),
    0x04: ("Fuel Level", "H"),
    0x05: ("Speed", "H"),
    0x06: ("Status Flags", "B"),    # uint8_t
    0x07: ("Steering Angle", "h"),  # int16_t
    0x08: ("Brake Pressure", "H"),
    0x09: ("Throttle Position", "B"),
    0x0A: ("Gear Position", "B"),
}

SERIAL_PORT = "/dev/ttyS0"  # Or /dev/ttyAMA0 if that's what you use
BAUDRATE = 115200

# Dictionary to store current values
current_values = {name: "---" for _, (name, _) in TLV_TYPES.items()}

def parse_tlv_packet(data):
    t = data[0]
    l = data[1]
    v = data[2:2+l]
    if t in TLV_TYPES:
        name, fmt = TLV_TYPES[t]
        # struct format: < = little-endian, H = uint16, h = int16, B = uint8
        value = struct.unpack("<" + fmt, v)[0]
        current_values[name] = value
    else:
        print(f"Unknown Type {t:02X}, Length {l}, Raw: {v.hex()}")

def display_values():
    os.system('cls' if os.name == 'nt' else 'clear')
    print(f"Listening on {SERIAL_PORT} at {BAUDRATE} baud...")
    print("\nCurrent Values:")
    print("-" * 30)
    for name in current_values.keys():
        print(f"{name:<20}: {current_values[name]:>8}")

def main():
    ser = serial.Serial(SERIAL_PORT, BAUDRATE, timeout=1)
    print(f"Listening on {SERIAL_PORT} at {BAUDRATE} baud...")
    buffer = bytearray()
    while True:
        data = ser.read(64)
        if data:
            buffer.extend(data)
            # Try to parse as many TLV packets as possible
            while len(buffer) >= 2:
                t = buffer[0]
                l = buffer[1]
                if len(buffer) < 2 + l:
                    break  # Wait for more data
                packet = buffer[:2 + l]
                parse_tlv_packet(packet)
                buffer = buffer[2 + l:]
            display_values()

if __name__ == "__main__":
    main()