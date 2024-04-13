#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <WiFi.h>

#define SERVICE_UUID_1 "8cd0f8e8-ccbe-4b89-b41e-e2951da0e8ae"
#define CHARACTERISTIC_UUID_1_ALL "2b4f4ecd-7a3a-4f89-af91-6f4d4ce2e6db"

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic_1_1 = NULL;

BLE2902 *descriptor_1_1 = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;
bool streamFlag = false;
bool isRead = false;
uint32_t x = 0;
uint8_t batteryLevel = 57;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
    isRead = true;
    // BatteryLevelDescriptor->setNotifications(true);
  };

  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
  };
};

class MyCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *mypCharacteristic)
  {
    std::string rxValue = mypCharacteristic->getValue();
    std::string data = "";
    char command;
    if (!rxValue.empty() && rxValue.length() > 1)
    {
      data = rxValue.substr(1);
      command = rxValue[0];
    }
    else
    {
      command = rxValue[0];
      data = "";
    }

    if (!rxValue.empty())
    {
      switch (command)
      {
      case 'b':
        descriptor_1_1->setNotifications(true); // not sure if I needed it
        streamFlag = true;
        break;

      case 's':

        break;

      case 'c':

        break;

      case 'e':

        break;

      case 'd':

        break;

      case 'i':

        break;

      default:
        if (rxValue.length() > 1)
        {
          handleDataNotification(rxValue);
        }

        break;
      }
    }
  };

  void handleDataNotification(const std::string &data)
  {
    for (int i = 0; i < data.length(); i++)
    {
      Serial.print(data[i]);
    }
  }

  void onRead(BLECharacteristic mypCharacteristic)
  {
  }
};
void setup()
{
  Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init("ESP32-B");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService_1 = pServer->createService(SERVICE_UUID_1);

  // Create a BLE Characteristic
  pCharacteristic_1_1 = pService_1->createCharacteristic(
      CHARACTERISTIC_UUID_1_ALL,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_INDICATE);

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor

  descriptor_1_1 = new BLE2902();
  pCharacteristic_1_1->addDescriptor(descriptor_1_1);
  pCharacteristic_1_1->setCallbacks(new MyCallbacks());

  // Start the service
  pService_1->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();

  pAdvertising->setScanResponse(false);
  // pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  // pAdvertising->setMinPreferred(0x12);
  pAdvertising->setMinPreferred(0x0); // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
}

void loop()
{
  // notify changed value
  if (deviceConnected && streamFlag)
  {
    pCharacteristic_1_1->setValue((uint8_t *)&value, 4);
    pCharacteristic_1_1->notify();
    Serial.println(value);
    value++;

    delay(1000); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
  }

  if (deviceConnected)
  {
    // pCharacteristic_1_2 ->setValue(x);
  }

  // disconnecting
  if (!deviceConnected && oldDeviceConnected)
  {
    delay(500);                  // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected)
  {
    // do stuff here on connecting

    oldDeviceConnected = deviceConnected;
  }
}
