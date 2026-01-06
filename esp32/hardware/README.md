# Hardware Design

**ESP32 Modal Resonator Node - Hardware Specifications**

---

## Per-Node Components

### Core Module

**ESP32-C3-DevKitM-1** (~$5)
- MCU: ESP32-C3 (RISC-V, 160 MHz)
- RAM: 400 KB SRAM
- Flash: 4 MB
- WiFi: 2.4 GHz (ESP-NOW capable)
- GPIO: 22 pins
- I2S: Yes (for audio DAC)
- UART: 2 (MIDI + debug)

**Alternative**: ESP32-S3 (dual-core, more RAM) for demanding scenarios

---

### Audio DAC (Recommended)

**PCM5102A I2S DAC Module** (~$3)
- Resolution: 32-bit
- Sample rate: Up to 384 kHz (we use 48 kHz)
- SNR: ~112 dB
- Interface: I2S (3 wires: BCK, WS, DIN)
- Output: Line-level stereo (3.5mm)

**Connections**:
```
ESP32           PCM5102A
GPIO 25 (BCK)   → BCK (bit clock)
GPIO 26 (WS)    → LRCK (word select)
GPIO 27 (DIN)   → DIN (data in)
3.3V            → VIN
GND             → GND
```

**Alternative**: MCP4725 (I2C DAC, cheaper but lower bandwidth)

---

### Power Supply

**Per Node**:
- Input: 5V USB or barrel jack
- Regulator: AMS1117-3.3V LDO (if not on dev board)
- Current: ~200mA peak, ~150mA typical
- **8-node system**: 5V @ 2A total

**Options**:
1. USB hub (easiest for prototyping)
2. Single 5V/3A power supply with distribution board
3. Individual USB wall adapters

---

### MIDI Input (Controller Node Only)

**Components**:
- MIDI DIN-5 connector (female)
- 6N138 optocoupler (for isolation)
- 220Ω, 470Ω resistors
- 1N4148 diode

**Circuit**:
```
MIDI In Pin 5 ──┬── 220Ω ──┬── 6N138 (pin 2)
                │          │
                └── 1N4148 ─┘ (cathode to pin 2)

MIDI In Pin 4 ─────────────── GND

6N138 Pin 6 ─── 470Ω ─── 3.3V
6N138 Pin 6 ─── ESP32 GPIO (RX, e.g., GPIO 16)
6N138 Pin 3 ─── GND
```

**Notes**:
- Only needed on **one node** (controller/hub)
- Standard MIDI: 31250 baud, 8N1

---

### Optional: Status LEDs

**Single RGB LED** (WS2812B or similar)
- Shows node state (idle, running, error)
- Connects to one GPIO (e.g., GPIO 8)

**Color Code**:
- Blue: Idle/discovering
- Green: Running
- Red: Error
- Yellow: Configuring

---

## Complete Node Schematic (Simplified)

```
        ┌────────────────────────────────┐
        │   ESP32-C3 Dev Board           │
        │                                │
        │  GPIO 25 (BCK)  ────────────────┼──> PCM5102A
        │  GPIO 26 (WS)   ────────────────┼──>
        │  GPIO 27 (DIN)  ────────────────┼──>
        │                                │
        │  GPIO 16 (RX)   ────────────────┼──> MIDI Input (opt)
        │                                │
        │  GPIO 8         ────────────────┼──> Status LED (opt)
        │                                │
        │  USB (power+debug) <───── 5V   │
        └────────────────────────────────┘
                 │
                 v
          ┌──────────┐
          │ PCM5102A │
          └──────────┘
               │
               v
          3.5mm Jack ──> Audio Out
```

---

## Bill of Materials (Single Node)

| Component                | Qty | Unit Price | Total | Notes              |
|--------------------------|-----|------------|-------|--------------------|
| ESP32-C3-DevKitM-1       | 1   | $5.00      | $5.00 | Core MCU           |
| PCM5102A DAC breakout    | 1   | $3.00      | $3.00 | I2S audio          |
| 3.5mm audio jack         | 1   | $0.50      | $0.50 | Output connector   |
| USB cable (A to micro)   | 1   | $2.00      | $2.00 | Power + debug      |
| Perfboard/stripboard     | 1   | $1.00      | $1.00 | Assembly           |
| Jumper wires (M-F)       | 6   | $0.10      | $0.60 | Connections        |
| **Total per node**       |     |            | **$12.10** | Bare minimum   |

### Optional Components (Controller Node)
| Component                | Qty | Unit Price | Total | Notes              |
|--------------------------|-----|------------|-------|--------------------|
| MIDI DIN-5 connector     | 1   | $1.00      | $1.00 | MIDI input         |
| 6N138 optocoupler        | 1   | $0.50      | $0.50 | MIDI isolation     |
| Resistors (220Ω, 470Ω)  | 2   | $0.05      | $0.10 | MIDI circuit       |
| 1N4148 diode             | 1   | $0.10      | $0.10 | MIDI protection    |
| **Controller extras**    |     |            | **$1.70** |                |

### Optional: Enclosure
| Component                | Qty | Unit Price | Total | Notes              |
|--------------------------|-----|------------|-------|--------------------|
| 3D printed case          | 1   | $3.00      | $3.00 | Custom design      |
| M3 screws                | 4   | $0.10      | $0.40 | Assembly           |
| **Enclosure total**      |     |            | **$3.40** |                |

---

## 16-Node System Cost

**Total Cost Breakdown**:
- **15 regular nodes**: 15 × $12.10 = $181.50
- **1 controller node**: $12.10 + $1.70 = $13.80
- **Enclosures (optional)**: 16 × $3.40 = $54.40
- **Power hub**: $15.00 (USB hub or PSU)
- **MIDI cables**: $10.00

**Grand Total**:
- **Bare electronics**: $210.30
- **With enclosures**: $279.70

---

## PCB Design (Future)

### Custom PCB Features
- ESP32-C3 module (SMD)
- PCM5102A DAC (SMD)
- Onboard LDO regulator
- MIDI input circuit (optional population)
- Status RGB LED
- 3.5mm jack
- USB-C connector
- Compact size: ~50mm × 50mm

**Estimated PCB cost**: $5-8 per board (qty 16, from JLCPCB/PCBWay)

---

## Assembly Guide

### Step 1: Prepare Dev Board
1. Flash firmware via USB
2. Test boot and serial output
3. Configure node ID via menuconfig

### Step 2: Connect DAC
1. Solder header pins to PCM5102A breakout
2. Wire I2S signals (BCK, WS, DIN)
3. Connect power (3.3V, GND)
4. Test audio output with tone

### Step 3: Add MIDI (Controller Only)
1. Assemble MIDI input circuit on perfboard
2. Connect to ESP32 GPIO 16 (RX)
3. Test with MIDI keyboard

### Step 4: Enclosure (Optional)
1. Mount ESP32 and DAC in case
2. Drill holes for USB, audio jack
3. Add ventilation holes

### Step 5: Multi-Node Testing
1. Power on all nodes
2. Verify ESP-NOW discovery
3. Send test configuration
4. Validate audio output

---

## Wiring Diagram (Single Node)

```
    ┌─────────────────────────┐
    │   ESP32-C3 Dev Board    │
    │                         │
    │     [USB Port]          │ <── Power + Debug
    │                         │
    │  3V3 ────────────────┐  │
    │  GND ────────────┐   │  │
    │  G25 ──────┐     │   │  │
    │  G26 ────┐ │     │   │  │
    │  G27 ──┐ │ │     │   │  │
    └────────┼─┼─┼─────┼───┼──┘
             │ │ │     │   │
         DIN │ │ │     │   │
          WS ─┘ │ │     │   │
         BCK ───┘ │     │   │
                  │     │   │
    ┌─────────────┼─────┼───┼──┐
    │  PCM5102A   │     │   │  │
    │             │     │   │  │
    │  DIN ───────┘     │   │  │
    │  BCK ─────────────┘   │  │
    │  LRCK ────────────────┘  │
    │  VCC ────────────────────┘
    │  GND ────────────────────┐
    │                          │
    │  [LOUT] ───────┐         │
    │  [ROUT] ─────┐ │         │
    └──────────────┼─┼─────────┼─┘
                   │ │         │
              ┌────┴─┴────┐    │
              │ 3.5mm Jack│    │
              │  L  R  G  │    │
              └───────────┴────┘
                         GND
```

---

## Testing Checklist

### Single Node Test
- [ ] ESP32 boots and connects to serial
- [ ] WiFi MAC address readable
- [ ] I2S output detected (scope/logic analyzer)
- [ ] PCM5102A outputs audio (test tone)
- [ ] Node responds to network messages
- [ ] MIDI input works (if controller)

### Multi-Node Test
- [ ] All nodes discovered via ESP-NOW
- [ ] Configuration transfer successful
- [ ] Poke messages received
- [ ] Audio output from all nodes
- [ ] Latency < 5ms (measure with scope)
- [ ] No packet loss under load

---

## Troubleshooting

### No Audio Output
- Check I2S wiring (BCK, WS, DIN)
- Verify PCM5102A power (3.3V)
- Test with headphones (low impedance)
- Check sample rate (should be 48kHz)

### ESP-NOW Not Working
- Verify WiFi is initialized
- Check peer MAC addresses
- Ensure nodes on same WiFi channel
- Reduce distance (<10m for testing)

### MIDI Not Responding
- Check optocoupler orientation
- Verify UART baud rate (31250)
- Test with MIDI monitor tool
- Check MIDI cable wiring (pins 4, 5)

---

## Future Improvements

- Custom PCB with SMD components
- Battery power (LiPo + charging circuit)
- Bluetooth MIDI (BLE)
- SD card logging
- OLED display (status/diagnostics)
- Touch sensors (interactive control)

---

**End of Hardware Guide**

See `schematics/` for detailed circuit diagrams (coming soon).
