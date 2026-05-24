// NIET_Weather_Station_A_8
// All Libraries, AP, OTA, Debug Fix
// SHEETS

#include <Wire.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include "PMS.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include "DHT.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <HTTPClient.h>


#define LCD_LIB 0          // use only when needed.
#if LCD_LIB
  #include <LiquidCrystal_I2C.h>
#endif

#define ENABLE_OTA 1          // use only when needed.
#if ENABLE_OTA
  #include <ArduinoOTA.h>
#endif


#define DEBUG 1
#if DEBUG
  #define DPRINT(...) Serial.print(__VA_ARGS__)
  #define DPRINTLN(...) Serial.println(__VA_ARGS__)
  #define DPRINTF(...) Serial.printf(__VA_ARGS__)
#else
  #define DPRINT(...)
  #define DPRINTLN(...)
  #define DPRINTF(...)
#endif


#define LED_PIN 2 // Blue onboard LED

Preferences prefs;
WebServer server(80);

const char* apSSID = "Weather_Wifi_Setup";
const char* apPassword = "WeatherWindow";
String ssid = "";     // Enter Wifi Credentials to BYPASS Soft AP Mode and Connect to this wifi
String pass = "";

// Captive Portal Page
const char* htmlForm = R"rawliteral(
<!DOCTYPE html>
<html>
<head><title>ESP32 WiFi Setup</title></head>
<body>
  <h2>Configure WiFi</h2>
  <form action="/save" method="POST">
    SSID:<br><input type="text" name="ssid"><br>
    Password:<br><input type="password" name="pass"><br><br>
    <input type="submit" value="Save">
  </form>
</body>
</html>
)rawliteral";


// Google script Web_App_URL.
String Web_App_URL = "*********************************************************************************************************8";


// PMS5003 Sensor
HardwareSerial pmsSerial(1);
PMS pms(pmsSerial);
PMS::DATA data;  

// MQ Sensors
#define mq7Pin 32
#define mq135Pin 35

// UV Sensor GY-ML8511
#define uvPin 34
#define uvRef 25

// GPS Module
#define RXD2 16
#define TXD2 17
#define GPS_BAUD 9600
TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

// BMP280 Sensor
Adafruit_BMP280 bmp;

// DHT11 Sensor
#define DHTPIN 13
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Rain Sensor
#define RAIN_PIN 27

// LCD Display
#if LCD_LIB
  LiquidCrystal_I2C lcd(0x27, 16, 2);
#endif


//--------------------------------------------------------------------//
// Sensor value variables
//--------------------------------------------------------------------//
struct PMAqiData {
  float aqi25;
  float aqi10;
};

// String datetime = "";

float pm25 = 0.0;
float pm10 = 0.0;
float aqi25val = 0.0;
float aqi10val = 0.0;

float mq_co = 0.0;
float mq_aqi = 0.0;

float uvIndex_mWcm2 = 0.0;
String uvRisk = "low";

float lat = 0.0;
float lng = 0.0;

bool bmpDetected = false;
float BMPtemp_C = 0.0;
float BMPpressure_hPa = 0.0;

float DHTtemp_C = 0.0;
float DHThumid_percent = 0.0;

bool isRaining = false;
String rain = "No";

//--------------------------------------------------------------------//
// Function Prototypes
//--------------------------------------------------------------------//
void handleSave();
void startAccessPoint();
bool connectToWiFi();
#if ENABLE_OTA
  void setupOTA();
#endif
String getDateTimeString();
PMAqiData getPMAqi();
float getCOPercent(int mq7_pin);
float getAQIPercent(int mq135_pin);
float getUVIndex(int uv_pin);
void getGPS();
String sendToSheets();

//--------------------------------------------------------------------//
// Data Logging Variables
//--------------------------------------------------------------------//
unsigned long lastActionTime = 0;
const unsigned long printInterval = 500;    // 0.5 sec between function calls
const unsigned long sendDelay = 20*1000;      // 20 min delay after all functions called // For SHEETS SEND

int funcIndex = 0;
bool waitingToSend = false;

// An Array of Function Pointers Returning String
using DebugFunc = String(*)();
DebugFunc debugFunctions[] = {
  []() { return String("======== Printing Values ========"); },

  []() { return String("Timestamp: " + getDateTimeString());},

  []() {
    PMAqiData air = getPMAqi();
    return String("PM2.5: " + String(pm25) + " mW/cm2 | PM10: " + String(pm10) + " mW/cm2");
  },
  []() {
    PMAqiData air = getPMAqi();
    aqi25val = air.aqi25;
    aqi10val = air.aqi10;

    return String("PM2.5 AQI: " + String(aqi25val) + " | PM10 AQI: " + String(aqi10val));
  },

  []() {
    mq_co = getCOPercent(mq7Pin);
    mq_aqi = getAQIPercent(mq135Pin);

    return String("MQ_CO: " + String(mq_co) + " % | MQ_AQI: " + String(mq_aqi) + " %");
  },

  []() {
    uvIndex_mWcm2 = getUVIndex(uvPin);
    return String("UV Index mW/cm2: " + String(uvIndex_mWcm2) + " | Risk: " + uvRisk);
  },

  []() {
    if (gps.location.isValid()) {
      return String("Location: " + String(lat, 6) + ", " + String(lng, 6));
      // char buf[64];
      // snprintf(buf, sizeof(buf), "Location: %.6f, %.6f", lat, lng);
      // return String(buf);  // Convert C-string to Arduino String
    }
    return String("GPS: Not Found");
  },

  []() {
    if (bmpDetected) {
      BMPtemp_C = bmp.readTemperature();
      BMPpressure_hPa = bmp.readPressure() / 100.0F;
    } else {
      BMPtemp_C = 0.0;
      BMPpressure_hPa = 0.0;
    }
    return String("BMP Temperature: " + String(BMPtemp_C) + " °C | BMP Pressure: " + String(BMPpressure_hPa) + " hPa");
  },

  []() {
    int rainVal = digitalRead(RAIN_PIN);
    isRaining = (rainVal == HIGH); // HIGH means rain detected
    rain = (isRaining ? "Yes" : "No");
    return String("Is Raining: " + rain);
  },

  []() {
    DHTtemp_C = dht.readTemperature();
    DHThumid_percent = dht.readHumidity();

    if (isnan(DHTtemp_C) || isnan(DHThumid_percent)) {
      DHTtemp_C = 0.0;
      DHThumid_percent = 0.0;
    }
    return String("DHT Temperature: " + String(DHTtemp_C) + " °C | DHT Humidity: " + String(DHThumid_percent) + " %");
  },

  []() { return String("================================="); }
};

const int numFunctions = sizeof(debugFunctions) / sizeof(debugFunctions[0]);
//====================================================================//


//--------------------------------------------------------------------//
// Handle Wifi and AP
//--------------------------------------------------------------------//
void handleWiFi() {
  static bool apMode = false;

  if (ssid == "") {
    prefs.begin("wifi_config", false);
    ssid = prefs.getString("ssid", "");
    pass = prefs.getString("pass", "");
    prefs.end();
  }

  // If no credentials and not already in AP mode, start AP
  if (ssid == "" && !apMode) {
    WiFi.softAP(apSSID, apPassword);
    apMode = true;
    Serial.println("AP Mode started. Connect to SSID: ESP32_Setup, Password: 12345678");
    Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

    server.on("/", HTTP_GET, []() {
      server.send(200, "text/html", htmlForm);
    });

    server.on("/save", HTTP_POST, []() {
      String newSSID = server.arg("ssid");
      String newPass = server.arg("pass");

      if (newSSID != "") {
        prefs.begin("wifi_config", false);
        prefs.putString("ssid", newSSID);
        prefs.putString("pass", newPass);
        prefs.end();

        server.send(200, "text/plain", "Credentials saved. Rebooting...");
        delay(1000);
        ESP.restart();
      } else {
        server.send(400, "text/plain", "SSID cannot be empty.");
      }
    });

    server.begin();
    return;
  }

  // If already in AP mode, serve web interface
  if (apMode) {
    server.handleClient();
    return;
  }

  // If credentials exist and WiFi not connected, try to connect
  if (WiFi.status() != WL_CONNECTED && ssid != "") {
    WiFi.begin(ssid.c_str(), pass.c_str());
    Serial.print("Connecting to WiFi: "); Serial.println(ssid);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
      delay(500); Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());
    } else {
      Serial.println("\nWiFi connection failed.");
    }
  }
}
//====================================================================//


//--------------------------------------------------------------------//
// Setup OTA
//--------------------------------------------------------------------//
#if ENABLE_OTA
  void setupOTA() {
    ArduinoOTA
      .onStart([]() {
        DPRINTLN("OTA Starting...");
      })
      .onEnd([]() {
        DPRINTLN("OTA Complete!");
      })
      .onProgress([](unsigned int progress, unsigned int total) {
        DPRINTF("OTA Progress: %u%%\r", (progress * 100) / total);
      })
      .onError([](ota_error_t error) {
        DPRINTF("OTA Error[%u]\n", error);
      });

    ArduinoOTA.begin();
    DPRINTLN("OTA Ready. IP: " + WiFi.localIP().toString());
  }
#endif
//====================================================================//


//--------------------------------------------------------------------//
// Date Time Function
//--------------------------------------------------------------------//
String getDateTimeString() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "TimeError";
  }
  char buffer[25];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H:%M:%S", &timeinfo);
  return String(buffer);
}
//====================================================================//


//--------------------------------------------------------------------//
// Air Quality - PM2.5 and PM10
//--------------------------------------------------------------------//
PMAqiData getPMAqi() {
  PMAqiData result = {0.0, 0.0};  // Default values

  pms.requestRead();
  if (pms.readUntil(data)) {
    DPRINTF("Air Quality: PM2.5: %d ug/m3, PM10: %d ug/m3\n", data.PM_AE_UG_2_5, data.PM_AE_UG_10_0);
  } else {
    DPRINTLN("Air Quality Sensor Failed");
    return result; // Return zeros if read failed
  }

  pm25 = data.PM_AE_UG_2_5;
  pm10 = data.PM_AE_UG_10_0;

  // PM2.5 to AQI
  if (pm25 <= 12.0)       result.aqi25 = map(pm25, 0, 12, 0, 50);
  else if (pm25 <= 35.4)  result.aqi25 = map(pm25, 12.1, 35.4, 51, 100);
  else if (pm25 <= 55.4)  result.aqi25 = map(pm25, 35.5, 55.4, 101, 150);
  else if (pm25 <= 150.4) result.aqi25 = map(pm25, 55.5, 150.4, 151, 200);
  else if (pm25 <= 250.4) result.aqi25 = map(pm25, 150.5, 250.4, 201, 300);
  else if (pm25 <= 350.4) result.aqi25 = map(pm25, 250.5, 350.4, 301, 400);
  else                    result.aqi25 = map(pm25, 350.5, 500.4, 401, 500);

  // PM10 to AQI
  if (pm10 <= 54.0)        result.aqi10 = map(pm10, 0, 54, 0, 50);
  else if (pm10 <= 154.0)  result.aqi10 = map(pm10, 55, 154, 51, 100);
  else if (pm10 <= 254.0)  result.aqi10 = map(pm10, 155, 254, 101, 150);
  else if (pm10 <= 354.0)  result.aqi10 = map(pm10, 255, 354, 151, 200);
  else if (pm10 <= 424.0)  result.aqi10 = map(pm10, 355, 424, 201, 300);
  else if (pm10 <= 504.0)  result.aqi10 = map(pm10, 425, 504, 301, 400);
  else                     result.aqi10 = map(pm10, 505, 604, 401, 500);

  return result;
}
//====================================================================//


//--------------------------------------------------------------------//
// AQI - MQ Sensors
//--------------------------------------------------------------------//
float getCOPercent(int mq7_pin) {
  int adcValue = analogRead(mq7_pin);

  const int adcCleanAir = 2000;     // ADC value in clean air (you must measure this!)
  const int adcPollutedAir = 3000;  // ADC value in max pollution (estimated or measured)

  // float co_percent = (float)(adcValue - adcCleanAir) / (adcPollutedAir - adcCleanAir) * 100.0;
  float co_percent = ((3.3 / 4095.0) * adcValue) * 0.01;
  return constrain(co_percent, 0.0, 100.0);
}

float getAQIPercent(int mq135_pin) {
  int adcValue = analogRead(mq135_pin);

  const int adcCleanAir = 1800;     // ADC value in clean air (you must measure this!)
  const int adcPollutedAir = 3000;  // ADC value in worst pollution (estimate/measure)

  // float aqi_percent = (float)(adcValue - adcCleanAir) / (adcPollutedAir - adcCleanAir) * 100.0;
  float aqi_percent = ((3.3 / 4095.0) * adcValue) * 0.03;
  return constrain(aqi_percent, 0.0, 500.0);
}
//====================================================================//


//--------------------------------------------------------------------//
// UV
//--------------------------------------------------------------------//
float getUVIndex(int uv_pin) {
  // float uvVoltage = ((3.3 / 4095.0) * analogRead(uv_pin)) - ((3.3 / 4095.0) * analogRead(uvRef));
  float uvVoltage = ((3.3 / 4095.0) * analogRead(uv_pin)) - 1.0;
  float uvIndex = uvVoltage < 0 ? 0 : uvVoltage / 0.1; // Convert to mW/cm2
  uvRisk = uvIndex < 0.3 ? "Low" : uvIndex < 0.6 ? "Moderate" : uvIndex < 0.9 ? "High" : "Extreme";

  return uvIndex;
}
//====================================================================//


//--------------------------------------------------------------------//
// GPS
//--------------------------------------------------------------------//
void getGPS() {
  unsigned long start = millis();
  while (millis() - start < 1000) {
    while (gpsSerial.available()) gps.encode(gpsSerial.read());
  }
}
//====================================================================//


//--------------------------------------------------------------------//
// Google SHEETS Update Function
//--------------------------------------------------------------------//
String sendToSheets() {
  if (WiFi.status() != WL_CONNECTED) {
    return "WiFi not connected, skipping Sheets upload";
  }

  String Send_Data_URL = Web_App_URL + "?sts=write";

  Send_Data_URL += "&datetime=" + getDateTimeString();
  Send_Data_URL += "&BMPpressure_hPa=" + String(BMPpressure_hPa, 2);
  Send_Data_URL += "&BMPtemp_C=" + String(BMPtemp_C, 2);
  Send_Data_URL += "&DHThumid_percent=" + String(DHThumid_percent, 2);
  Send_Data_URL += "&DHTtemp_C=" + String(DHTtemp_C, 2);
  Send_Data_URL += "&aqi10val=" + String(aqi10val, 2);
  Send_Data_URL += "&aqi25val=" + String(aqi25val, 2);
  Send_Data_URL += "&lat=" + String(lat, 5);
  Send_Data_URL += "&lng=" + String(lng, 5);
  Send_Data_URL += "&mq_aqi=" + String(mq_aqi, 2);
  Send_Data_URL += "&mq_co=" + String(mq_co, 2);
  Send_Data_URL += "&pm10=" + String(pm10, 2);
  Send_Data_URL += "&pm25=" + String(pm25, 2);
  Send_Data_URL += "&rain=" + rain;
  Send_Data_URL += "&uvIndex_mWcm2=" + String(uvIndex_mWcm2, 2);
  Send_Data_URL += "&uvRisk=" + uvRisk;

  //------------The process of sending or writing data to Google Sheets------------
  // Initialize HTTPClient as "http".
  HTTPClient http;

  // HTTP PUT Request.
  http.begin(Send_Data_URL.c_str());
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  // Gets the HTTP status code.
  int httpCode = http.GET(); 
  DPRINT("HTTP Status Code : ");
  DPRINTLN(httpCode);

  // Getting response from google sheets.
  String result;
  if (httpCode > 0) {
    result = http.getString();
  } else {
    result = "http code Error received";
  }
  
  http.end();

  return result;
}
//====================================================================//



void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // Turn OFF LED by default


  handleWiFi(); // First call in setup
  while (WiFi.status() != WL_CONNECTED) {
    handleWiFi();
  }

  #if ENABLE_OTA
    setupOTA();
  #endif

  configTime(19800, 0, "pool.ntp.org", "time.nist.gov"); // IST = UTC+5:30 (19800 seconds)

  dht.begin();

  pmsSerial.begin(9600, SERIAL_8N1, 4, 2);
  pms.passiveMode();
  pms.wakeUp();
  delay(10000);

  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);
  delay(5000);
  getGPS();    // calling only once in startup only
  lat = gps.location.lat();
  lng = gps.location.lng();

  if (!bmp.begin(0x76)) {
    DPRINTLN("BMP280 NOT detected!");
    bmpDetected = false;
  } else {
    bmpDetected = true;
  }

  pinMode(RAIN_PIN, INPUT);

  #if LCD_LIB
    lcd.init();
    lcd.backlight();
  #endif
}

void loop() {
  //--------RECONNECT TO WIFI IF LOST--------
  handleWiFi(); // Keep it running for reconnection or AP handling

  //--------HANDLE OTA--------
  #if ENABLE_OTA
    ArduinoOTA.handle();
  #endif

  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_PIN, HIGH); // Turn ON LED

    //--------GET SENSOR DATA & SHEETS SEND USING millis--------
    unsigned long currentTime = millis();

    //--------GET SENSOR DATA--------
    if (!waitingToSend && funcIndex < numFunctions && currentTime - lastActionTime >= printInterval) {
      lastActionTime = currentTime;
      DPRINTLN(debugFunctions[funcIndex]());
      funcIndex++;
      if (funcIndex == numFunctions) {
        waitingToSend = true;
        lastActionTime = currentTime;  // Start 20s timer
      }
    }

    //--------SEND DATA TO SHEETS--------
    if (waitingToSend && currentTime - lastActionTime >= sendDelay) {
      DPRINT("Sheets Response: ");
      DPRINTLN(sendToSheets());
      funcIndex = 0;
      waitingToSend = false;
      lastActionTime = currentTime;
    }
  }
  
  //--------LED INDICATOR BASED ON WiFi--------
  else {
    digitalWrite(LED_PIN, LOW);  // Turn OFF LED
  }
} 





