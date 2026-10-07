#include <WiFi.h>

#include <ThingsBoard.h>

#include <Arduino_MQTT_Client.h>

#include <TinyGPS++.h>

#include <Wire.h>

#include <Adafruit_GFX.h>

#include <Adafruit_SSD1306.h>


// =====================================================
// 1. WIFI SETTINGS
// =====================================================

#define WIFI_SSID     "Xiaomi13T"
#define WIFI_PASSWORD "11122233"


// =====================================================
// 2. THINGSBOARD TOKEN
//    CHANGE ONLY THIS TOKEN WHEN NEEDED
// =====================================================

#define THINGSBOARD_TOKEN "a63h1u3m6fbjds5061ko"


// =====================================================
// 3. THINGSBOARD SERVER
// =====================================================

#define THINGSBOARD_SERVER "eu.thingsboard.cloud"


// =====================================================
// 4. GP02 GPS SETTINGS
// =====================================================

// GP02 TX -> ESP32 GPIO16
// GP02 RX -> ESP32 GPIO17

#define GPS_RX_PIN 16
#define GPS_TX_PIN 17

#define GPS_BAUD 9600


// =====================================================
// 5. OLED SETTINGS
// =====================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET -1

#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);


// =====================================================
// 6. GPS OBJECT
// =====================================================

TinyGPSPlus gps;

HardwareSerial GPS_Serial(1);


// =====================================================
// 7. WIFI + MQTT
// =====================================================

WiFiClient wifiClient;

Arduino_MQTT_Client mqttClient(wifiClient);

ThingsBoard tb(mqttClient);


// =====================================================
// 8. TIMING
// =====================================================

unsigned long lastSendTime = 0;

const unsigned long SEND_INTERVAL = 5000;


// =====================================================
// 9. OLED MESSAGE
// =====================================================

void showOLEDMessage(String line1, String line2)
{
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);

  display.setCursor(0, 15);
  display.println(line1);

  display.setCursor(0, 35);
  display.println(line2);

  display.display();
}


// =====================================================
// 10. WIFI CONNECTION
// =====================================================

void connectWiFi()
{
  Serial.print("Connecting to WiFi...");

  showOLEDMessage(
    "Connecting WiFi",
    "Please wait..."
  );

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);

    Serial.print(".");
  }

  Serial.println();

  Serial.println("WiFi connected");

  Serial.print("ESP32 IP: ");

  Serial.println(WiFi.localIP());

  showOLEDMessage(
    "WiFi Connected",
    WiFi.localIP().toString()
  );

  delay(1500);
}


// =====================================================
// 11. THINGSBOARD CONNECTION
// =====================================================

void connectThingsBoard()
{
  while (!tb.connected())
  {
    Serial.print("Connecting to ThingsBoard...");

    showOLEDMessage(
      "ThingsBoard",
      "Connecting..."
    );

    if (tb.connect(
          THINGSBOARD_SERVER,
          THINGSBOARD_TOKEN
        ))
    {
      Serial.println("connected");

      showOLEDMessage(
        "ThingsBoard",
        "Connected!"
      );

      delay(1000);
    }
    else
    {
      Serial.println(
        "failed. Retry in 5s..."
      );

      showOLEDMessage(
        "ThingsBoard",
        "Connection failed"
      );

      delay(5000);
    }
  }
}


// =====================================================
// 12. DISPLAY GPS DATA ON OLED
// =====================================================

void displayGPSData(
  double latitude,
  double longitude,
  double speed,
  int satellites
)
{
  display.clearDisplay();

  display.setTextColor(
    SSD1306_WHITE
  );

  display.setTextSize(1);


  // Latitude

  display.setCursor(0, 0);

  display.print("Lat:");

  display.println(
    latitude,
    6
  );


  // Longitude

  display.setCursor(0, 14);

  display.print("Lon:");

  display.println(
    longitude,
    6
  );


  // Speed

  display.setCursor(0, 28);

  display.print("Speed:");

  display.print(
    speed,
    1
  );

  display.println(" km/h");


  // Satellites

  display.setCursor(0, 42);

  display.print("Sat:");

  display.println(
    satellites
  );


  // ThingsBoard status

  display.setCursor(0, 56);

  if (tb.connected())
  {
    display.print("TB: Connected");
  }
  else
  {
    display.print("TB: Offline");
  }

  display.display();
}


// =====================================================
// 13. SEND GPS DATA
// =====================================================

void sendGPSData()
{
  // Check GPS location

  if (!gps.location.isValid())
  {
    Serial.println(
      "GPS location is not valid yet."
    );

    showOLEDMessage(
      "Waiting for GPS...",
      "Searching satellites"
    );

    return;
  }


  // Latitude

  double latitude =
    gps.location.lat();


  // Longitude

  double longitude =
    gps.location.lng();


  // Altitude

  double altitude = 0;

  if (gps.altitude.isValid())
  {
    altitude =
      gps.altitude.meters();
  }


  // Speed

  double speed = 0;

  if (gps.speed.isValid())
  {
    speed =
      gps.speed.kmph();
  }


  // Satellites

  int satellites = 0;

  if (gps.satellites.isValid())
  {
    satellites =
      gps.satellites.value();
  }


  // HDOP

  double hdop = 0;

  if (gps.hdop.isValid())
  {
    hdop =
      gps.hdop.hdop();
  }


  // ===================================================
  // SERIAL MONITOR
  // ===================================================

  Serial.println();

  Serial.println(
    "========== GPS DATA =========="
  );

  Serial.print("Latitude:   ");

  Serial.println(
    latitude,
    6
  );

  Serial.print("Longitude:  ");

  Serial.println(
    longitude,
    6
  );

  Serial.print("Altitude:   ");

  Serial.print(
    altitude
  );

  Serial.println(" m");

  Serial.print("Speed:      ");

  Serial.print(
    speed
  );

  Serial.println(" km/h");

  Serial.print("Satellites: ");

  Serial.println(
    satellites
  );

  Serial.print("HDOP:       ");

  Serial.println(
    hdop
  );


  // ===================================================
  // SEND TO THINGSBOARD
  // ===================================================

  tb.sendTelemetryData(
    "latitude",
    latitude
  );

  tb.sendTelemetryData(
    "longitude",
    longitude
  );

  tb.sendTelemetryData(
    "altitude",
    altitude
  );

  tb.sendTelemetryData(
    "speed",
    speed
  );

  tb.sendTelemetryData(
    "satellites",
    satellites
  );

  tb.sendTelemetryData(
    "hdop",
    hdop
  );


  Serial.println(
    "GPS data sent to ThingsBoard"
  );

  Serial.println(
    "=============================="
  );


  // ===================================================
  // OLED
  // ===================================================

  displayGPSData(
    latitude,
    longitude,
    speed,
    satellites
  );
}


// =====================================================
// 14. SETUP
// =====================================================

void setup()
{
  Serial.begin(115200);


  // ===================================================
  // OLED START
  // ===================================================

  Wire.begin(
    21,
    22
  );

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS
      ))
  {
    Serial.println(
      "OLED initialization failed!"
    );

    while (true)
    {
      delay(1000);
    }
  }


  display.clearDisplay();

  display.setTextColor(
    SSD1306_WHITE
  );

  display.setTextSize(1);

  display.setCursor(20, 20);

  display.println(
    "ESP32 GPS"
  );

  display.setCursor(20, 35);

  display.println(
    "TRACKER"
  );

  display.display();

  delay(2000);


  // ===================================================
  // GPS START
  // ===================================================

  GPS_Serial.begin(
    GPS_BAUD,
    SERIAL_8N1,
    GPS_RX_PIN,
    GPS_TX_PIN
  );


  Serial.println();

  Serial.println(
    "================================"
  );

  Serial.println(
    "ESP32 + GP02 GPS + ThingsBoard"
  );

  Serial.println(
    "================================"
  );


  // ===================================================
  // WIFI
  // ===================================================

  connectWiFi();


  // ===================================================
  // THINGSBOARD
  // ===================================================

  connectThingsBoard();


  Serial.println();

  Serial.println(
    "Waiting for GPS satellite fix..."
  );


  showOLEDMessage(
    "GPS Tracker",
    "Waiting for GPS..."
  );
}


// =====================================================
// 15. LOOP
// =====================================================

void loop()
{
  // ===================================================
  // READ GPS CONTINUOUSLY
  // ===================================================

  while (GPS_Serial.available())
  {
    gps.encode(
      GPS_Serial.read()
    );
  }


  // ===================================================
  // WIFI CHECK
  // ===================================================

  if (WiFi.status() != WL_CONNECTED)
  {
    connectWiFi();
  }


  // ===================================================
  // THINGSBOARD CHECK
  // ===================================================

  if (!tb.connected())
  {
    connectThingsBoard();
  }


  tb.loop();


  // ===================================================
  // SEND EVERY 5 SECONDS
  // ===================================================

  if (
    millis() - lastSendTime >=
    SEND_INTERVAL
  )
  {
    lastSendTime = millis();

    sendGPSData();
  }
}