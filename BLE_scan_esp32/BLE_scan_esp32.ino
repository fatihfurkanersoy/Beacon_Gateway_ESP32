/*
   Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
   Ported to Arduino ESP32 by Evandro Copercini
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

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

      char mac_add_all[136];
      char mac_id_[13] = "FFFFFFFFFFFF";

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
      Serial.printf("manufacture: %s \n", pHex);
      free(pHex);
    }
  }
};

void setup()
{
  Serial.begin(115200);
  Serial.println("Scanning...");

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); // create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); // active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99); // less or equal setInterval value
}

void loop()
{
  // put your main code here, to run repeatedly:
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory
  delay(2000);
}