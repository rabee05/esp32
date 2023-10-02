#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <WiFi.h>

#define SERVICE_UUID_1 "8cd0f8e8-ccbe-4b89-b41e-e2951da0e8ae"
#define CHARACTERISTIC_UUID_1_ALL "2b4f4ecd-7a3a-4f89-af91-6f4d4ce2e6db"
#define CHARACTERISTIC_UUID_1_R "beda398f-b495-4c1f-b06a-fa086e24cdb1"
#define CHARACTERISTIC_UUID_1_N "98106d21-0955-4a40-882b-e64017b914cf"
#define CHARACTERISTIC_UUID_1_W_WIFI "0bc1cb7d-3ff4-486d-b60d-78c084fa73ae"
#define CHARACTERISTIC_UUID_1_WIFI_CONNECT "0ecbf7ca-60c2-11ee-8c99-0242ac120002"

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic_1_1 = NULL;
BLECharacteristic *pCharacteristic_1_2 = NULL;
BLECharacteristic *pCharacteristic_1_3 = NULL;
BLECharacteristic *pCharacteristic_1_4 = NULL;

BLE2902 *descriptor_1_1 = NULL;
BLE2902 *descriptor_1_2 = NULL;
BLE2902 *descriptor_1_3 = NULL;
BLE2902 *descriptor_1_4 = NULL;

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
    }
    else
    {
      command = rxValue[0];
    }

    if (!rxValue.empty())
    {
      switch (command)
      {
      case 'a':
        descriptor_1_1->setNotifications(true); // not sure if I needed it
        streamFlag = true;
        break;

      case 's':
      {
        String availableNetworks = scanNetworks();
        // Serial.println(availableNetworks);
        int beginIndex = 0;
        int endIndex = 0;

        // Send the data in chunks
        while (beginIndex < availableNetworks.length())
        {
          endIndex = beginIndex + 20;
          if (endIndex > availableNetworks.length())
          {
            endIndex = availableNetworks.length();
          }
          String chunk = availableNetworks.substring(beginIndex, endIndex);
          const char *chunkData = chunk.c_str();

          if (deviceConnected)
          {
            pCharacteristic_1_3->setValue(chunkData);
            pCharacteristic_1_3->notify();
            delay(100);
          }

          beginIndex = endIndex;
        }
      }
      break;

      case 'c':
      {
        Serial.println("rxValue.c_str()");
        size_t delimiterPos = data.find(":");
        if (delimiterPos != std::string::npos)
        {
          std::string receivedSSID = data.substr(0, delimiterPos);
          std::string receivedPassword = data.substr(delimiterPos + 1);
          // Serial.println(receivedSSID.c_str());
          // Serial.println(receivedPassword.c_str());
          if (connectWiFi(receivedSSID.c_str(), receivedPassword.c_str()))
          {
            pCharacteristic_1_4->setValue("connected");
            pCharacteristic_1_4->notify();
          }
          else
          {
            pCharacteristic_1_3->setValue("error");
            pCharacteristic_1_3->notify();
          }
        }

        break;
      }

      case 'e':
        if (toggleWiFi("enable"))
        {
          pCharacteristic_1_3->setValue("enabled");
          pCharacteristic_1_3->notify();
        }
        else
        {
          pCharacteristic_1_3->setValue("error");
          pCharacteristic_1_3->notify();
        }
        break;

      case 'd':
        if (toggleWiFi("disable"))
        {
          pCharacteristic_1_3->setValue("disabled");
          pCharacteristic_1_3->notify();
        }
        else
        {
          pCharacteristic_1_3->setValue("error");
          pCharacteristic_1_3->notify();
        }
        break;

      case 'i':
      {
        String wifiInfo = getWiFiInfo();
        Serial.println(wifiInfo);
        int beginIndex = 0;
        int endIndex = 0;

        // Send the data in chunks
        while (beginIndex < wifiInfo.length())
        {
          endIndex = beginIndex + 20; // Taking 20 characters at a time
          if (endIndex > wifiInfo.length())
          {
            endIndex = wifiInfo.length();
          }
          String chunk = wifiInfo.substring(beginIndex, endIndex);
          const char *chunkData = chunk.c_str();

          if (deviceConnected)
          {
            pCharacteristic_1_3->setValue(chunkData);
            pCharacteristic_1_3->notify();
            delay(100); // A delay to ensure data is sent and processed before the next chunk
          }

          beginIndex = endIndex;
        }
      }
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

  String scanNetworks()
  {
    int n = WiFi.scanNetworks();
    String jsonString = "[";
    for (int i = 0; i < n; i++)
    {
      jsonString += "{";
      jsonString += "\"mac\":\"" + WiFi.BSSIDstr(i) + "\",";
      jsonString += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
      jsonString += "\"quality\":" + String(WiFi.RSSI(i)) + ",";

      if (WiFi.encryptionType(i) == WIFI_AUTH_OPEN)
      {
        jsonString += "\"security\":\"None\"";
      }
      else if (WiFi.encryptionType(i) == WIFI_AUTH_WEP)
      {
        jsonString += "\"security\":\"WEP\"";
      }
      else if (WiFi.encryptionType(i) == WIFI_AUTH_WPA_PSK)
      {
        jsonString += "\"security\":\"WPA-PSK\"";
      }
      else if (WiFi.encryptionType(i) == WIFI_AUTH_WPA2_PSK)
      {
        jsonString += "\"security\":\"WPA2-PSK\"";
      }
      else if (WiFi.encryptionType(i) == WIFI_AUTH_WPA_WPA2_PSK)
      {
        jsonString += "\"security\":\"WPA/WPA2-PSK\"";
      }
      else
      {
        jsonString += "\"security\":\"Other\"";
      }

      jsonString += "}";

      if (i < n - 1)
      {
        jsonString += ",";
      }
    }

    jsonString += "]";
    return jsonString;
  }

  String getWiFiInfo()
  {
    String jsonData;

    jsonData += "{";
    if (WiFi.status() == WL_CONNECTED)
    {
      String currentSSID = WiFi.SSID();
      bool isSecure = false;

      int n = WiFi.scanNetworks();
      for (int i = 0; i < n; ++i)
      {
        if (WiFi.SSID(i) == currentSSID)
        {
          wifi_auth_mode_t encryption = WiFi.encryptionType(i);
          if (encryption != WIFI_AUTH_OPEN)
          {
            isSecure = true;
          }
          break;
        }
      }

      jsonData += "\"ssid\":\"" + currentSSID + "\",";
      jsonData += "\"ip\":\"" + WiFi.localIP().toString() + "\",";

      if (isSecure)
      {
        jsonData += "\"secure\":true";
      }
      else
      {
        jsonData += "\"secure\":false";
      }
    }
    else
    {
      jsonData += "\"ssid\":\"Not connected\",";
      jsonData += "\"ip\":\"0.0.0.0\",";
      jsonData += "\"secure\":false";
    }

    jsonData += "}";
    return jsonData;
  }

  bool connectWiFi(const char *ssid, const char *password)
  {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    int timeout = 10 * 4; // Wait 10 seconds
    while (WiFi.status() != WL_CONNECTED && timeout-- > 0)
    {
      delay(250);
    }

    if (WiFi.status() == WL_CONNECTED)
    {
      return true;
    }
    else
    {
      return false;
    }
  }

  bool toggleWiFi(const char *status)
  {
    WiFi.mode(WIFI_OFF);
    delay(100);
    if (WiFi.status() == WL_DISCONNECTED)
    {
      return true;
    }
    else
    {
      return false;
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
  BLEDevice::init("EH-Gateway");

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

  pCharacteristic_1_2 = pService_1->createCharacteristic(
      CHARACTERISTIC_UUID_1_R,
      BLECharacteristic::PROPERTY_READ);

  pCharacteristic_1_3 = pService_1->createCharacteristic(
      CHARACTERISTIC_UUID_1_W_WIFI,
      BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_READ);

  pCharacteristic_1_4 = pService_1->createCharacteristic(
      CHARACTERISTIC_UUID_1_WIFI_CONNECT,
      BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_READ);

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

  descriptor_1_4 = new BLE2902();
  pCharacteristic_1_4->addDescriptor(descriptor_1_4);
  pCharacteristic_1_4->setCallbacks(new MyCallbacks());

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
