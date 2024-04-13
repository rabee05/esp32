#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <WiFi.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID_1 "8cd0f8e8-ccbe-4b89-b41e-e2951da0e8ae"
#define CHARACTERISTIC_UUID "0bc1cb7d-3ff4-486d-b60d-78c084fa73ae"

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic_wifi = NULL;
BLE2902 *descriptor_wifi = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
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
    if (!rxValue.empty() && rxValue.length() > 1 && rxValue.find(":"))
    {
      command = rxValue[0];
      data = rxValue.substr(2);
      // Serial.println(data.c_str());
    }
    else
    {
      command = rxValue[0];
      data = "";
      // Serial.println(command);
    }

    if (!rxValue.empty())
    {
      switch (command)
      {
      case 's':
      {
        String availableNetworks = scanNetworks();
        Serial.println(availableNetworks);
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
            pCharacteristic_wifi->setValue(chunkData);
            pCharacteristic_wifi->notify();
            delay(100);
          }

          beginIndex = endIndex;
        }
      }
      break;

      case 'c':
      {
        size_t delimiterPos = data.find(":");
        if (delimiterPos != std::string::npos)
        {
          Serial.println("Yes, ssid and pass were sent");
          std::string receivedSSID = data.substr(0, delimiterPos);
          std::string receivedPassword = data.substr(delimiterPos + 1);
          Serial.println(receivedSSID.c_str());
          Serial.println(receivedPassword.c_str());
          if (connectWiFi(receivedSSID.c_str(), receivedPassword.c_str()))
          {
            pCharacteristic_wifi->setValue("connected");
            pCharacteristic_wifi->notify();
            Serial.println("connected");
          }
          else
          {
            pCharacteristic_wifi->setValue("error");
            pCharacteristic_wifi->notify();
            Serial.println("connection error");
          }
        }

        break;
      }

      case 'e':
        if (toggleWiFi("enable"))
        {
          pCharacteristic_wifi->setValue("enabled");
          pCharacteristic_wifi->notify();
          Serial.println("toggled-enabled");
        }
        else
        {
          pCharacteristic_wifi->setValue("error");
          pCharacteristic_wifi->notify();
          Serial.println("toggled-enabled-error");
        }
        break;

      case 'd':
        if (toggleWiFi("disable"))
        {
          pCharacteristic_wifi->setValue("disabled");
          pCharacteristic_wifi->notify();
          Serial.println("Toggled-disabled");
        }
        else
        {
          pCharacteristic_wifi->setValue("error");
          pCharacteristic_wifi->notify();
          Serial.println("Toggled-disabled-error");
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
            pCharacteristic_wifi->setValue(chunkData);
            pCharacteristic_wifi->notify();
            Serial.println(chunkData);
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

  pCharacteristic_wifi = pService_1->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_INDICATE |
          BLECharacteristic::PROPERTY_WRITE_NR |
          BLECharacteristic::PROPERTY_BROADCAST);

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  descriptor_wifi = new BLE2902();
  pCharacteristic_wifi->addDescriptor(descriptor_wifi);
  pCharacteristic_wifi->setCallbacks(new MyCallbacks());

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
  if (deviceConnected)
  {
    delay(1000); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
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
