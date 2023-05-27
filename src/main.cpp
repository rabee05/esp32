#include <Arduino.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic_1_1 = NULL;
BLECharacteristic *pCharacteristic_1_2 = NULL;
BLECharacteristic *pCharacteristic_1_3 = NULL;
//BLECharacteristic* pCharacteristic_1_4 = NULL;
BLECharacteristic *pCharacteristic_2_1 = NULL;
BLECharacteristic *pCharacteristic_2_2 = NULL;
BLECharacteristic *pCharacteristic_3_1 = NULL;

BLE2902 *descriptor_1_1 = NULL;
BLE2902 *descriptor_1_2 = NULL;
BLE2902 *descriptor_1_3 = NULL;

BLE2902 *BatteryLevelDescriptor = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;
bool streamFlag = false;
bool isRead = false;
uint32_t x = 0;
uint8_t batteryLevel = 57;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID_1 "8cd0f8e8-ccbe-4b89-b41e-e2951da0e8ae"
#define CHARACTERISTIC_UUID_1_ALL "2b4f4ecd-7a3a-4f89-af91-6f4d4ce2e6db"
#define CHARACTERISTIC_UUID_1_R "beda398f-b495-4c1f-b06a-fa086e24cdb1"
#define CHARACTERISTIC_UUID_1_W "0bc1cb7d-3ff4-486d-b60d-78c084fa73ae"
#define CHARACTERISTIC_UUID_1_N "98106d21-0955-4a40-882b-e64017b914cf"

#define SERVICE_UUID_2 "93c24215-8714-4a1f-b286-a8119b0b67a9"
#define CHARACTERISTIC_UUID_2_W_N "95a88554-007b-433e-af5a-1c5c99ddfe04"
#define CHARACTERISTIC_UUID_2_N "f6a7a104-7d4a-4876-b976-c5095b4e6f8a"

#define SERVICE_UUID_3 "180F"
#define BATTERY_LEVEL_CHARACTERISTIC_UUID_R_N "2A19"

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
    isRead = true;
    //BatteryLevelDescriptor->setNotifications(true);
  };

  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
    Serial.println("onDisconnect");
  };
};

class MyCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *mypCharacteristic)
  {
    std::string rxValue = mypCharacteristic->getValue();

    if (rxValue.length() > 0)
    {

      if (rxValue[0] == 'b' || rxValue[0] == 'B')
      {
        descriptor_1_1->setNotifications(true);
        Serial.println("Notification has been triggered");
        streamFlag = true;
        Serial.print("stream flag: ");
        Serial.println(rxValue[0]);
      }
      else
      {
        x = rxValue.length();
        Serial.println("*********");
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++)
          Serial.print(rxValue[i]);

        Serial.println();
        Serial.println("*********");
      }
    }
  };

  void onRead(BLECharacteristic mypCharacteristic)
  {
    std::string name = "Rabee";
    Serial.print(name[0]);
  }
};
void setup()
{
  Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init("EH-Gateway");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService_1 = pServer->createService(SERVICE_UUID_1);
  BLEService *pService_2 = pServer->createService(SERVICE_UUID_2);
  BLEService *pService_3 = pServer->createService(SERVICE_UUID_3);

  // Create a BLE Characteristic
  pCharacteristic_1_1 = pService_1->createCharacteristic(
      CHARACTERISTIC_UUID_1_ALL,
          BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_INDICATE);

  pCharacteristic_1_2 = pService_1->createCharacteristic(
      CHARACTERISTIC_UUID_1_R,
      BLECharacteristic::PROPERTY_READ);

  pCharacteristic_1_3 = pService_1->createCharacteristic(
      CHARACTERISTIC_UUID_1_W,
      BLECharacteristic::PROPERTY_WRITE);

  pCharacteristic_3_1 = pService_3->createCharacteristic(
                        BATTERY_LEVEL_CHARACTERISTIC_UUID_R_N, 
                        BLECharacteristic::PROPERTY_READ
                        );

  pService_3->addCharacteristic(pCharacteristic_3_1); // not sure if we need this one

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor

  descriptor_1_1 = new BLE2902();
  pCharacteristic_1_1->addDescriptor(descriptor_1_1);
  pCharacteristic_1_1->setCallbacks(new MyCallbacks());

  descriptor_1_2 = new BLE2902();
  pCharacteristic_1_1->addDescriptor(descriptor_1_2);
  pCharacteristic_1_2->setCallbacks(new MyCallbacks());

  descriptor_1_3 = new BLE2902();
  pCharacteristic_1_3->addDescriptor(descriptor_1_3);
  pCharacteristic_1_3->setCallbacks(new MyCallbacks());

  BatteryLevelDescriptor = new BLE2902();
  BatteryLevelDescriptor->setValue("Percentage 0 - 100");
  pCharacteristic_3_1->addDescriptor(BatteryLevelDescriptor);


  // Start the service
  pService_1->start();
  pService_2->start();
  pService_3->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  //pAdvertising->addServiceUUID(SERVICE_UUID_1);
  pAdvertising->setScanResponse(false);
  //pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  //pAdvertising->setMinPreferred(0x12);
  pAdvertising->setMinPreferred(0x0); // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();

  Serial.println("Waiting a client connection to notify...");
  //Serial.println(BLEDevice::getMTU());
}

void loop()
{
  // notify changed value
  if (deviceConnected && streamFlag)
  {
    //descriptor_1_1->setNotifications(true);
    pCharacteristic_1_1->setValue((uint8_t *)&value, 4);
    pCharacteristic_1_1->notify();
    Serial.println(value);
    value++;

    delay(1000); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
  }

  if (deviceConnected)
  {
    //pCharacteristic_1_2 ->setValue(x);
  }

  pCharacteristic_3_1->setValue(&batteryLevel, 1);
  //pCharacteristic_3_1->notify();
  batteryLevel++;
  if (int(batteryLevel) == 100)
  {
    batteryLevel = 0;
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