# NIET Weather Station 🌦️

An IoT-based smart weather monitoring station installed inside the college campus for real-time environmental data collection and cloud logging.

The system continuously monitors multiple atmospheric and air-quality parameters using various sensors connected to an ESP32. Data is uploaded to Google Sheets over WiFi for remote monitoring and analysis.

---

## 🚀 Features

- Real-time weather monitoring
- Air quality monitoring (PM2.5 & PM10)
- UV intensity detection
- CO and AQI estimation using MQ sensors
- GPS location tracking
- Rain detection
- Temperature & humidity monitoring
- Atmospheric pressure monitoring
- Automatic Google Sheets data logging
- WiFi provisioning through captive portal
- OTA (Over-The-Air) firmware updates
- Automatic WiFi reconnection
- Debug logging support

---

## 🛠️ Hardware Used

### Microcontroller
- ESP32

### Sensors
- PMS5003 Dust Sensor
- MQ7 Gas Sensor
- MQ135 Air Quality Sensor
- GY-ML8511 UV Sensor
- BMP280 Pressure Sensor
- DHT11 Temperature & Humidity Sensor
- Rain Sensor
- GPS Module

### Other Components
- Status LED
- Optional I2C LCD Display

---

## 📊 Parameters Measured

| Parameter | Sensor Used |
|---|---|
| PM2.5 | PMS5003 |
| PM10 | PMS5003 |
| AQI | PMS5003 + MQ135 |
| CO Level | MQ7 |
| UV Intensity | GY-ML8511 |
| Temperature | DHT11 & BMP280 |
| Humidity | DHT11 |
| Atmospheric Pressure | BMP280 |
| Rain Detection | Rain Sensor |
| GPS Coordinates | GPS Module |

---

## 🌐 WiFi Configuration

The system supports automatic WiFi provisioning using ESP32 Soft AP mode.

### Default AP Credentials

SSID:
```text
Weather_Wifi_Setup
```

Password:
```text
WeatherWindow
```

After connecting to the AP:
1. Open browser
2. Enter WiFi credentials
3. ESP32 stores credentials in flash memory
4. Device reboots and connects automatically

---

## ☁️ Cloud Data Logging

Sensor data is uploaded to Google Sheets using Google Apps Script Web App integration.

Logged data includes:
- Timestamp
- Air quality values
- Temperature
- Humidity
- Pressure
- UV Index
- GPS coordinates
- Rain status

---

## 🔄 OTA Firmware Updates

The project supports OTA (Over-The-Air) updates using ArduinoOTA.

This allows wireless firmware updates without reconnecting the ESP32 physically.

---


## 📌 Libraries Used

```cpp
TinyGPS++
PMS
Adafruit BMP280
DHT Sensor Library
WiFi
WebServer
Preferences
HTTPClient
ArduinoOTA
```

---

## ⚙️ Working Principle

1. ESP32 initializes all connected sensors.
2. WiFi connection is established automatically.
3. If WiFi credentials are unavailable, ESP32 enters Access Point mode.
4. Sensor values are collected periodically.
5. Data is processed and converted into meaningful environmental parameters.
6. Data is uploaded to Google Sheets using HTTP requests.
7. OTA service remains active for remote firmware updates.

---

## 📈 Future Improvements

- Dedicated web dashboard
- Data visualization graphs
- ThingSpeak / Firebase integration
- Local SD card backup
- Solar-powered operation
- Mobile application support
- Weather prediction using ML models

---



## 👨‍💻 Developed By

Developed by the college technical club team for real-time environmental monitoring inside the campus.

---

## 📜 License

This project is intended for educational and research purposes.
