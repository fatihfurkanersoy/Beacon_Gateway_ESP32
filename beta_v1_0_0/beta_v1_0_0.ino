/*
 *       Fatih Furkan
 *    Beacon Gateway ESP32
 *         V1_0_1
 */

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include <PubSubClient.h> //MQTT
#include <ArduinoJson.h>  //JSON

#include <WiFi.h>
#include <WiFiClient.h>

// OTA ALL DEFINE
//--------------------------------------------------
const char *host = "Test_Beacon";
const char *ssid = "Test_Beacon";
const char *password = "Beacon_Test";

//-------------------------------------------------
const char *broker = "api.ieasygroup.com"; // MQTT hostname
int MQTT_PORT = 61613;                     // MQTT Port
const char *mqttUsername = "kkmtest";      // MQTT username
const char *mqttPassword = "testpassword"; // MQTT password
const char *MQTTID = "Gateway_ID";
unsigned long lastMsg = 0;

String Beacon_MAC_Topic = "MAC_TOPIC_ERROR";
char datasendtopic[50]; // Verilein gonderildiği topic
char *pHex = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

char mac_add_all[136];
char mac_id_[13] = "FFFFFFFFFFFF";
//------------------------------------------------
int scanTime = 5; // In seconds
BLEScan *pBLEScan;
char JSONmessageBuffer[200];
String BLE_device_ID = "notfoundxx";

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

      // ManufacturerData'nın tamamını burada alıyoruz
      pHex = BLEUtils::buildHexData(NULL, (uint8_t *)advertisedDevice.getManufacturerData().data(), advertisedDevice.getManufacturerData().length());

      //------------------------------------------------
      // MQTT ALL

      //------------------------------------------------
      // MQTT JSON DATA
      StaticJsonDocument<512> Message;
      // JsonObject veri = Message.createNestedObject(mac_id_); // beacon adında daha büyük bir nesne oluşturup içine verileri koyuyoruz. Bu satırı yorum satırı yaparsak parse edilmiş halde gönderir verileri
      Message["MAC"] = mac_id_;
      Message["RSSI"] = advertisedDevice.getRSSI();
      Message["Manufacture"] = pHex;
      serializeJsonPretty(Message, JSONmessageBuffer);
      //------------------------------------------------
      // MQTT Publish
      Beacon_MAC_Topic = "kbeacon/publish/SogukZincir_01/"; // PUBLISH TOPIC
      Beacon_MAC_Topic += mac_id_;
      Beacon_MAC_Topic.toCharArray(datasendtopic, 50);
      Serial.print("DATA SEND TOPIC (PUBLISH) : ");
      Serial.println(datasendtopic);

      mqtt.publish(datasendtopic, JSONmessageBuffer);
      //------------------------------------------------
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

  Serial.print("Connecting to");
  Serial.println(ssid);
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());

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

  long now = millis();
  if (now - lastMsg > 60000)
  {
    if (mqtt.connect(MQTTID, mqttUsername, mqttPassword))
    {
      Serial.println("Connected MQTT");
    }
    else
    {
      Serial.print("[FAILED] [ rc = ");
      Serial.print(mqtt.state());
      ESP.restart();
    }

    lastMsg = now;
    BLEScanResults foundDevices = pBLEScan->start(scanTime, false); // put your main code here, to run repeatedly:
    pBLEScan->clearResults();                                       // delete results fromBLEScan buffer to release memory
  }

  mqtt.loop();
}