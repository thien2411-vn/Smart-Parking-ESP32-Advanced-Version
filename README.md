# Smart Parking ESP32 Advanced Version

## 🌟 Introduction

Welcome to **Smart Parking ESP32 Advanced Version**, a cutting-edge smart parking management system designed using the ESP32 microcontroller. This intelligent system streamlines vehicle entry, exit, and payment processes using RFID technology, real-time tracking, and cloud-based logging. It is highly reliable, easy to deploy, and scalable for modern smart parking applications.

---

## 🛠 Features

- **Entry Management**:
  - Dispenses RFID cards for new vehicles entering.
  - Records precise entry times.

- **Exit Management**:
  - Calculates parking fees based on real-time or fallback timestamps.
  - Displays charges and opens the exit barrier after confirmation.

- **Time Synchronization**:
  - Synchronizes time via NTP (uses fallback logic for offline scenarios).

- **Remote Logging**:
  - Sends event logs to Google Sheets for real-time monitoring and data storage.

- **Vehicle Detection**:
  - Uses IR sensors to ensure cars pass through entry/exit barriers.

- **User-Friendly Interface**:
  - LCD screen for clear communication of instructions and feedback.
  - Buzzer and LED for auditory and visual signals.

---

## 🔧 Hardware Components

### Mandatory:
- **ESP32**: Central controller with WiFi capabilities.
- **MFRC522 RFID Reader (x2)**: For reading RFID cards at entry and exit gates.
- **LiquidCrystal_I2C (16x2 LCD)**: Displays system messages to users.
- **Servo Motors (x3)**: Control entry, exit, and card dispensing mechanisms.
- **IR Sensors (x2)**: Detect vehicle passage at the gates.
- **Push Button**: For requesting new RFID cards.
- **Buzzer**: Provides audio feedback.
- **LED**: Indicates system status.

### Optional:
- Power supply for ESP32 and connected peripherals.
- EEPROM for persistent storage of card data.

---

## 📋 Software Requirements

### 1. **Programming Environment**
- [Arduino IDE](https://www.arduino.cc/en/software) (recommended)
- Required board settings: **ESP32 Dev Module**

### 2. **Libraries**
Ensure the following libraries are installed:
- `WiFi.h`
- `HTTPClient.h`
- `MFRC522`
- `Wire.h`
- `LiquidCrystal_I2C`
- `ESP32Servo`
- `EEPROM.h`
- `time.h`

---

## 🚀 Getting Started

### 1. **Hardware Setup**
- Connect the hardware components as per the pin configuration provided in the source code.
- Ensure proper power supply and connectivity for all components.

### 2. **Code Configuration**
- Update the following in `main.cpp`:
  - WiFi Credentials:
    ```cpp
    const char* ssid = "YourWiFiName";
    const char* password = "YourWiFiPassword";
    ```
  - Google Sheets Script URL:
    ```cpp
    String scriptUrl = "https://script.google.com/macros/s/YOUR_SCRIPT_URL_HERE/exec";
    ```
- Modify any other project-specific settings, including pin definitions, if needed.

### 3. **Upload Code**
- Open the code in Arduino IDE.
- Select the correct board (**ESP32 Dev Module**) and port from the Tools menu.
- Hit **Upload** to flash the code onto the ESP32.

### 4. **Run the System**
- Once powered, the system will:
  - Configure all peripherals.
  - Connect to WiFi (use a hotspot if no router is available).
  - Start managing the parking flow.

🎉 Your **Project** is now live!

---

## 📊 Demonstration Flow

1. **Vehicle Entry**:
   - Press the button to request an RFID card. 
   - The system dispenses a card.
   - Scan the card and pass through the barrier.

2. **Vehicle Exit**:
   - Scan the same card at the exit gate.
   - The system calculates the fee, displays it, and logs the event.
   - Pass through the exit barrier.

3. **Logs in Google Sheets**:
   - View real-time event logs in the linked Google Sheets document.

---

## ⚙️ Pin Configurations

| **Peripheral**       | **Pin**        |
|-----------------------|----------------|
| RFID Reader (Entry)   | SS: `3`, RST: `5` |
| RFID Reader (Exit)    | SS: `1`, RST: `22` |
| Servo (Entry Barrier) | `14`           |
| Servo (Exit Barrier)  | `13`           |
| Servo (Card Dispenser)| `12`           |
| IR Sensor (Entry)     | `16`           |
| IR Sensor (Exit)      | `17`           |
| Buzzer                | `32`           |
| LED                   | `27`           |
| Button                | `4`            |
| LCD (I2C)             | `25` (SDA), `26` (SCL) |

---

## 💡 Future Enhancements

- **Cloud Dashboard**:
  - Integrate an online dashboard for viewing parking statistics.
- **Multi-Tier Fee System**:
  - Allow dynamic pricing based on parking duration.
- **Enhanced Offline Capability**:
  - Store logs locally during internet downtime and synchronize later.

---

## 🤝 Contribution

- Feel free to fork this repository and contribute improvements via pull requests.
- Report issues or suggest features via the [Issues tab](https://github.com/thien2411-vn/Smart-Parking-ESP32-Advanced-Version/issues).

---

## 📞 Support

For any queries or support, please contact [thien2411-vn](https://github.com/thien2411-vn).

---

## 🌐 Acknowledgements

- Special thanks to the open-source community and contributors of various Arduino libraries used in this project.
- Video credit:
- [![Demo Video](https://img.youtube.com/vi/WQs1M3PLUOs/0.jpg)](https://youtu.be/WQs1M3PLUOs)
If you find this project helpful, please ⭐ star the repository to support my work!
