/*
   Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
   Ported to Arduino ESP32 by Evandro Copercini
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include <PubSubClient.h> //MQTT
#include <ArduinoJson.h>  //JSON

// For OTA
#include <WiFi.h>
#include <WiFiClient.h>

// OTA ALL DEFINE
//--------------------------------------------------
const char *host = "Test_Beacon";
const char *ssid = "Test_Beacon";
const char *password = "Beacon_Test";

//-------------------------------------------------
//-------------------------------------------------
const char *broker = "194.31.59.188";          // MQTT hostname
int MQTT_PORT = 1883;                          // MQTT Port
const char *mqttUsername = "Gateway_username"; // MQTT username
const char *mqttPassword = "Gateway_password"; // MQTT password
const char *MQTTID = "Gateway_ID";
unsigned long lastMsg = 0;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

char mac_add_all[136];
char mac_id_[13] = "FFFFFFFFFFFF";
//------------------------------------------------
//------------------------------------------------
int scanTime = 5; // In seconds
BLEScan *pBLEScan;

String BLE_device_ID = "dontfound";

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    BLE_device_ID = advertisedDevice.getName().c_str();
    if (BLE_device_ID == "mcz")
    {

      sprintf(mac_add_all, "%s", advertisedDevice.getAddress().toString().c_str());
      mac_id_[0] = mac_add_all[0];
      mac_id_[1] = mac_add_all[1];
      mac_id_[2] = mac_add_all[3];
      mac_id_[3] = mac_add_all[4];
      mac_id_[4] = mac_add_all[6];
      mac_id_[5] = mac_add_all[7];
      mac_id_[6] = mac_add_all[9];
      mac_id_[7] = mac_add_all[10];
      mac_id_[8] = mac_add_all[12];
      mac_id_[9] = mac_add_all[13];
      mac_id_[10] = mac_add_all[15];
      mac_id_[11] = mac_add_all[16];

      Serial.printf("MAC: %s \n", mac_id_);
      Serial.printf("RSSI: %3d\n", advertisedDevice.getRSSI());

      // ManufacturerData'nın tamamını burada alıyoruz
      char *pHex = BLEUtils::buildHexData(NULL, (uint8_t *)advertisedDevice.getManufacturerData().data(), advertisedDevice.getManufacturerData().length());
      Serial.printf("manufacture: %s \n \n \n", pHex);

      //------------------------------------------------
      // MQTT ALL
      //------------------------------------------------

      if (!mqtt.connected())
      {
        if (mqtt.connect(MQTTID, mqttUsername, mqttPassword))
        {

          StaticJsonDocument<512> Message;
          Message["MAC"] = mac_id_;
          Message["RSSI"] = advertisedDevice.getRSSI();
          Message["Manufacture"] = pHex;
          char JSONmessageBuffer[200];
          serializeJsonPretty(Message, JSONmessageBuffer);
          Serial.println("JSONmessageBuffer");
          Serial.println(JSONmessageBuffer);
          if (mqtt.publish("v1/devices/me/telemetry", JSONmessageBuffer) == true)
          {
            Serial.println("Success sending message");
          }
          else
          {
            Serial.println("Error sending message");
          }
        }
        else
        {
          Serial.print("[FAILED] [ rc = ");
          Serial.print(mqtt.state());
        }
      }

      //------------------------------------------------
      //------------------------------------------------
      // MQTT JSON DATA

      free(pHex);
    }
  }
};

void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  for (int i = 0; i <= 5000; i++)
  { // döngü başladı
    if (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
      Serial.print(".");
    }
  }

  Serial.println();
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  mqtt.setServer(broker, MQTT_PORT);

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); // create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); // active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99); // less or equal setInterval value
}

void loop()
{

  // long now = millis();
  // if (now - lastMsg > 30000)
  //{
  // lastMsg = now;
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false); // put your main code here, to run repeatedly:
  pBLEScan->clearResults();                                       // delete results fromBLEScan buffer to release memory
  //}

  mqtt.loop();
}