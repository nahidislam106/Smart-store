# Store Room Monitor

A WiFi-based ESP32 system for real-time monitoring and control of store room environment parameters including temperature, humidity, air quality, gas levels, motion detection, fans, and lighting. This project features a modern, responsive web dashboard (see screenshot below) and local OLED display, making it ideal for smart automation in small rooms.

![Store Room Monitor Web Dashboard](assets/webInterface.jpeg)

_(Image 1: Web dashboard as seen on a connected device)_

---
![Store Room Monitor Circuit Design](assets/final.png)
## Features

- **Sensor Monitoring**:  
  - Temperature & Humidity (DHT11)
  - Air Quality (MQ135)
  - Gas Level (MQ2)
  - Motion Detection (PIR)
- **Device Control**:  
  - Manual Fan (relay toggle via web button)
  - Auto Fan (relay switched by motion)
  - PWM Fan (speed auto-adjusted by temperature)
  - LED Lighting (web button, dual outputs)
- **Web Dashboard**:  
  - Modern HTML/CSS interface with Bootstrap Icons
  - Live data updates (AJAX, 2s interval)
  - Responsive layout for mobile & desktop
  - One-click control for fan & light
- **OLED Display**:  
  - Real-time sensor and device status shown locally
- **WiFi Access Point**:  
  - ESP32 acts as AP; connect directly for dashboard access
  - Default SSID: `StoreRoomMonitor`, Password: `storeroom123`

---

## Hardware

| Component        | Pin Connections             |
|------------------|----------------------------|
| ESP32 Dev Board  |                            |
| DHT11            | Data: GPIO 2               |
| PIR Sensor       | Output: GPIO 27            |
| MQ135            | Analog: GPIO 35            |
| MQ2              | Analog: GPIO 34            |
| Fan1 (Relay)     | Relay: GPIO 17 (manual)    |
| Fan2 (Relay)     | Relay: GPIO 5 (auto/motion)|
| LED1, LED2       | GPIO 4, GPIO 16            |
| Motor (Fan3)     | PWM: GPIO 18, IN1: GPIO 19, IN2: GPIO 21 |
| OLED Display     | I2C (default: 0x3C)        |

---

## How It Works

1. **Startup**:  
   ESP32 boots into AP mode (`StoreRoomMonitor`). OLED displays system start.

2. **Web Dashboard**:  
   Connect to WiFi and visit `192.168.4.1` in browser.  
   - Dashboard shows live readings and device status (see [Image 1](#)).
   - Control manual fan and light with buttons.

3. **Sensor & Device Logic**:
   - **Motion**: Auto fan turns ON if motion detected.
   - **Temperature**: Fan3 PWM speed increases with temperature (32°C+).
   - **Manual Fan/Light**: Controlled via dashboard buttons.
   - **Air/Gas**: Readings mapped for % display.

4. **OLED Display**:  
   Shows all sensor readings and device status locally.

---

## Quick Start

1. **Wire hardware as per pinout above.**
2. **Flash ESP32** with `code.ino` using Arduino IDE (with required libraries).
3. **Connect device (mobile/PC) to WiFi AP**:  
   - SSID: `StoreRoomMonitor`
   - Password: `storeroom123`
4. **Open browser** at [http://192.168.4.1](http://192.168.4.1)
5. **Monitor and control** store room conditions through the dashboard.

---

## Required Libraries

Add these to your Arduino IDE:
- `WiFi.h`
- `WebServer.h`
- `DHT.h`
- `Wire.h`
- `Adafruit_GFX.h`
- `Adafruit_SSD1306.h`

---

## Screenshots

### Web Dashboard
![Store Room Monitor Web Dashboard](assets/webInterface.jpeg)
_(Displays temperature, humidity, air quality, gas level, motion, fan and LED status, and control buttons)_

---

## License

MIT License

---

## Credits

Developed by [nahidislam106](https://github.com/nahidislam106).  
Web dashboard design inspired by modern IoT UX best practices.

---

## Troubleshooting

- If OLED does not display, check I2C address and wiring.
- If sensors read 0 or fail, ensure proper power and connections.
- For custom AP credentials, change `ap_ssid` and `ap_pass` in `code.ino`.

---

## Customization

- Adjust sensor thresholds, relay logic, or dashboard appearance in `code.ino`.
- Add cloud integration or logging as needed.

---

## Contributing

Pull requests and suggestions are welcome!
