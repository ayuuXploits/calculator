# Arduino LCD Calculator

A hardware-based arithmetic calculator built for Arduino using the **LCD Keypad Shield**.  
Input via onboard buttons, results shown live on the 16×2 LCD — no Serial Monitor required.

---

## Installation

### 1. Clone the repository

```bash
git clone https://github.com/ayuuXploits/arduino-lcd-calculator.git
cd arduino-lcd-calculator
```

### 2. Open in Arduino IDE

```bash
arduino calculator.ino
```

### 3. (Optional) Upload via Arduino CLI

```bash
# Install arduino-cli (Linux/macOS)
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh

# Compile
arduino-cli compile --fqbn arduino:avr:uno calculator.ino

# Upload (replace /dev/ttyUSB0 with your port)
arduino-cli upload -p /dev/ttyUSB0 --fqbn arduino:avr:uno calculator.ino
```

### 4. Find your port

```bash
# Linux / macOS
arduino-cli board list

# Windows (PowerShell)
[System.IO.Ports.SerialPort]::getportnames()
```

---

## Hardware Requirements

- Arduino Uno (or compatible board)
- LCD Keypad Shield (DFRobot-compatible, 16×2 LCD + 5 buttons)
- **Library:** `LiquidCrystal.h` (built into Arduino IDE)

## Shield Pin Mapping

| Interface    | Pins              |
|--------------|-------------------|
| LCD RS       | D8                |
| LCD EN       | D9                |
| LCD D4–D7    | D4–D7             |
| Button input | A0 (resistor ladder) |

## Button Controls

| Button | Action                            |
|--------|-----------------------------------|
| UP     | Increment digit / cycle operator  |
| DOWN   | Decrement digit / cycle operator  |
| RIGHT  | Confirm entry / advance           |
| SELECT | Calculate result (=)              |
| LEFT   | Clear / reset                     |

## LCD Display Layout

```
Row 1:  42 + 58
Row 2:  = 100
```

---

## Author

**ayuuXploits** — [github.com/ayuuXploits](https://github.com/ayuuXploits)

---

## License

MIT
