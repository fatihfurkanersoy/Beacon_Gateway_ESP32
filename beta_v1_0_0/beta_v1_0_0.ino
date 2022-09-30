//-----------------------------------------------------------
// Includes
//-----------------------------------------------------------
#include <WiFi.h>
#include <WebServer.h>
#include <AutoConnect.h>
#include <ThingsBoard.h>
#include <NTPClient.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <sstream>
#include <ArduinoJson.h>
#include "SSD1306Wire.h" // legacy include: `#include "SSD1306.h"`
//-----------------------------------------------------------
// Definitions
//-----------------------------------------------------------
WebServer server;
AutoConnect portal(server);
AutoConnectConfig config;
AutoConnectAux intro;
//-----------------------------------------------------------
// Definitions
//-----------------------------------------------------------
// const char *filename = "/records.json";  // <- SD library uses 8.3 filenames
#define SD_MISO 2
#define SD_MOSI 15
#define SD_SCLK 14
#define SD_CS 13
File mczFile;
//------------------------------------------------------
// Define NTP Client to get time
//------------------------------------------------------
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
//------------------------------------------------------
// Variables to save date and time
//------------------------------------------------------
String formattedDate;
String dayStamp;
String timeStamp;
//------------------------------------------------------
// Variables for BLEScan
//------------------------------------------------------
int scanTime = 15; // In seconds
BLEScan *pBLEScan;
//------------------------------------------------------
// MQTT Varibales
//------------------------------------------------------
#define TOKEN "Mh7uNkALx73oE4KIWEvg"
#define THINGSBOARD_SERVER "192.168.9.102"
//#define THINGSBOARD_SERVER  "78.187.41.90"

//#define THINGSBOARD_SERVER   "mqtt.fluux.io"
#define THINGSBOARD_PORT 1883
String MAC = "FF:FF:FF:FF:FF:FF";
unsigned int updateInterval = 10000;
unsigned int BLEInterval = 7000;
unsigned int OledInterval = 10000;
unsigned long lastPub = 0;
unsigned long bleTaskTimer = 0;
unsigned long oledTaskTimer = 0;
char mac_chr[21];
WiFiClient espClient;
// Initialize ThingsBoard instance
ThingsBoard tb(espClient);
int ble_counter = 0;
SSD1306Wire display(0x3c, 21, 22); // 18=SDK  19=SCK  As per labeling on ESP32 DevKit
long rssi = 0;
//-----------------------------------------------------------
// Recroder Document Definations
//-----------------------------------------------------------
DynamicJsonDocument doc(16384);

int jsonIndex = 0;

void printDeviceInfo();
void printDeviceInfo();
//-----------------------------------------------------------
// Callback function for BLEScanner
//-----------------------------------------------------------
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    if ((!advertisedDevice.getName().compare("mcz")) || (!advertisedDevice.getName().compare("BEAC01")))
    {
      std::string strServiceData = advertisedDevice.getServiceData();
      uint8_t cServiceData[100];
      char charServiceData[100];
      strServiceData.copy((char *)cServiceData, strServiceData.length(), 0);

      unsigned long value1, value2;
      char charValue[5] = {
          0,
      };

      sprintf(charValue, "%02X%02X", cServiceData[9], cServiceData[10]);
      value1 = strtol(charValue, 0, 16);
      float temp = -46.85 + 175.72 / 65536 * (float)value1;

      sprintf(charValue, "%02X%02X", cServiceData[11], cServiceData[12]);
      value2 = strtol(charValue, 0, 16);
      float hum = -6.0 + 125.0 / 65536 * (float)value2;

      unsigned char value_bat = 0;
      sprintf(charValue, "%02d", cServiceData[0]);
      value_bat = atoi(charValue);

      char charMAC[18] = {
          0,
      };
      sprintf(charMAC, "%02X%02X%02X%02X%02X%02X-MT1", cServiceData[6], cServiceData[5], cServiceData[4], cServiceData[3], cServiceData[2], cServiceData[1]);

      JsonObject sensors = doc.createNestedObject(charMAC);

      sensors["gwid"] = mac_chr;
      sensors["rssi"] = advertisedDevice.getRSSI();
      sensors["bat"] = value_bat;
      sensors["temp"] = temp;
      sensors["hum"] = hum;

      jsonIndex++;
    }
  }
};
//-----------------------------------------------------------
// Static Functions
//-----------------------------------------------------------
static const char INTRO_PAGE[] PROGMEM = R"(
{ "title": "Microzerr", "uri": "/", "menu": true, "element": [
    { "name": "caption", "type": "ACText", "value": "<h2>Important Notes</h2>",  "style": "text-align:center;color:#2f4f4f;padding:10px;" },
    { "name": "content", "type": "ACText", "value": "This webserver published for cungigure your device. Pelase pay attention and read manuals before configure the device." } ]
}
)";
//-----------------------------------------------------------
// Setup Function
//-----------------------------------------------------------
void setup_easyconfig()
{
  portal.config(config);
  intro.load(INTRO_PAGE);
  portal.join({intro});
  portal.begin();
}
void setup_mainTasks()
{
}

void setup_ble()
{
  BLEDevice::init("");
  //  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_SCAN,ESP_PWR_LVL_P7);
  pBLEScan = BLEDevice::getScan(); // create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); // active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99); // less or equal setInterval value
}
void setup_ota()
{
  config.ota = AC_OTA_BUILTIN;
}
void setup_hardware()
{
  Serial.println("Initializing OLED Display");
  display.init();

  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  printDeviceInfo();
  timeClient.begin();
  timeClient.setTimeOffset(10800);
  doc.clear();
}
//-----------------------------------------------------------
// Functions
//-----------------------------------------------------------
void set_serial_number()
{
  MAC = WiFi.macAddress();
  MAC.remove(2, 1);
  MAC.remove(4, 1);
  MAC.remove(6, 1);
  MAC.remove(8, 1);
  MAC.remove(10, 1);
  MAC = MAC + "-MG-1220";
  Serial.println(MAC);
  MAC.toCharArray(mac_chr, 21);
}
void send_data_mqtt()
{
  timeClient.update();
  formattedDate = timeClient.getFormattedTime();
  Serial.println(MAC);
  if (WiFi.status() == WL_CONNECTED)
  {
    if (!tb.connected())
    {

      Serial.print("Connecting to: ");
      Serial.print(THINGSBOARD_SERVER);
      Serial.print("Token : ");
      Serial.println(mac_chr);

      if (!tb.connect(THINGSBOARD_SERVER, mac_chr, THINGSBOARD_PORT))
      {
        Serial.println("Failed to connect");
        return;
      }

      Serial.println("Sending data...");

      // serializeJsonPretty(doc, Serial);

      // for(int i=0;i<jsonIndex;i++)
      {
        char buffer[4096] = {
            0,
        };
        serializeJson(doc, buffer);

        Serial.println(buffer);

        tb.sendTelemetryJson(buffer);
      }
      doc.clear();
      jsonIndex = 0;
      tb.disconnect();
      // tb.loop();
    }
  }
}
void BLETask()
{
  BLEDevice::init("microGate");
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory
  ble_counter = 0;
  BLEDevice::deinit(false);
}
//-----------------------------------------------------------
// Main Setup
//-----------------------------------------------------------
void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  setup_ota();
  setup_easyconfig();
  setup_hardware();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());

  Serial.print("Wifi Power: ");
  Serial.println(WiFi.getTxPower());

  WiFi.setTxPower(WIFI_POWER_19_5dBm);

  set_serial_number();
  setup_ble();
}
//-----------------------------------------------------------
// Loop Tasks
//-----------------------------------------------------------
void loop()
{
  // put your main code here, to run repeatedly:
  portal.handleClient();

  if (millis() - lastPub > updateInterval)
  { // 2.1 Sn
    send_data_mqtt();
    lastPub = millis();
  }
  if (millis() - bleTaskTimer > BLEInterval)
  { // 2 Sn
    BLETask();
    bleTaskTimer = millis();
  }
  if (millis() - oledTaskTimer > OledInterval)
  { // 120 Sn
    timeClient.update();
    rssi = WiFi.RSSI();
    printOLED();
    oledTaskTimer = millis();
  }
}

//----------------------------------------------
// OLED Display C and RH task
//----------------------------------------------
void printOLED()
{
  // clear the display
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);

  if (rssi > -55)
  {
    display.drawString(0, 0, timeClient.getFormattedTime() + "          IIIII");
  }
  else if (rssi<-55 & rssi> - 65)
  {
    display.drawString(0, 0, timeClient.getFormattedTime() + "          IIII");
  }
  else if (rssi<-65 & rssi> - 70)
  {
    display.drawString(0, 0, timeClient.getFormattedTime() + "          III");
  }
  else if (rssi<-70 & rssi> - 78)
  {
    display.drawString(0, 0, timeClient.getFormattedTime() + "          II");
  }
  else if (rssi<-78 & rssi> - 82)
  {
    display.drawString(0, 0, timeClient.getFormattedTime() + "          I");
  }
  else
  {
    display.drawString(0, 0, timeClient.getFormattedTime() + "          X");
  }
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 16, "IP: " + WiFi.localIP().toString());
  display.drawString(0, 26, "MAC: " + String(WiFi.macAddress()));

  display.display();
}
//----------------------------------------------
// OLED Display DEvice Information
//----------------------------------------------
void printDeviceInfo()
{
  // clear the display
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, "Microzer");
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 16, "IP: " + WiFi.localIP().toString());
  display.drawString(0, 26, "MAC: " + String(WiFi.macAddress()));
  // write the buffer to the display
  display.display();
}