#include <Arduino.h>
#include "BLEDevice.h"
#include "ble_lib.h"
#include "display_lib.h"

static BLEUUID serviceUUID("0000fff0-0000-1000-8000-123456789abc");
static BLEUUID  w_charUUID("0000fff2-0000-1000-8000-123456789abc");
static BLEUUID  n_charUUID("0000fff1-0000-1000-8000-123456789abc");

static boolean doConnect = false;
static boolean connected = false;
static BLERemoteCharacteristic* pRemoteCharacteristic_w;
static BLERemoteCharacteristic* pRemoteCharacteristic_n;
static BLEAdvertisedDevice* myDevice;

static bool get_prompt = false;
extern byte rcv_data[];

static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  // Serial.print("[rcv]:");
  // Serial.print(length);
  // Serial.print(",");
  // Serial.println(isNotify);

  for (size_t i = 0; i < length; i++) {
    if (pData[i] == '\r') pData[i] = '\n';
    Serial.print((char)pData[i]);
    if (pData[i] == '>') {
      get_prompt = true;
      return;
    }
    // DLCが省略されてしまうのでrcv_data[+1]に格納する
    if ((pData[i] >= '0') && (pData[i] <= '9')) {
      if ((i % 2) == 0) {
        rcv_data[i / 2 + 1] = (pData[i] - '0') * 0x10;
      } else {
        rcv_data[i / 2 + 1] += (pData[i] - '0');
      }
    }
    if ((pData[i] >= 'A') && (pData[i] <= 'F')) {
      if ((i % 2) == 0) {
        rcv_data[i / 2 + 1] = (pData[i] - 'A' + 10) * 0x10;
      } else {
        rcv_data[i / 2 + 1] += (pData[i] - 'A' + 10);
      }
    }
  }
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.println("onConnect");
    disp_log.println("onConnect");
    disp_log.pushSprite(0, 0);
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
    disp_log.println("onDisconnect");
    disp_log.pushSprite(0, 0);
  }
};

bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());
    
  BLEClient*  pClient  = BLEDevice::createClient();
  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remove BLE Server.
  pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
  Serial.println(" - Connected to server");
  pClient->setMTU(517); //set client to request maximum MTU from server (default is 23 otherwise)
  
  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");

  // Obtain a reference to the characteristic in the service of the remote BLE server.
  pRemoteCharacteristic_w = pRemoteService->getCharacteristic(w_charUUID);
  if (pRemoteCharacteristic_w == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(w_charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our characteristic");

  if(pRemoteCharacteristic_w->canWrite()) {
    Serial.println("canWrite");
  }

  pRemoteCharacteristic_n = pRemoteService->getCharacteristic(n_charUUID);
  if(pRemoteCharacteristic_n->canNotify()) {
    pRemoteCharacteristic_n->registerForNotify(notifyCallback);
    Serial.println("canNotify");
  }

  connected = true;
  return true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());
    disp_log.print("BLE Advertised Device found: ");
    disp_log.println(advertisedDevice.toString().c_str());
    disp_log.pushSprite(0, 0);

    // if (advertisedDevice.haveName())
    // {
    //   Serial.print("Device name: ");
    //   Serial.println(advertisedDevice.getName().c_str());
    // }

    // if (advertisedDevice.haveServiceUUID())
    // {
    //   BLEUUID devUUID = advertisedDevice.getServiceUUID();
    //   Serial.print("Found ServiceUUID: ");
    //   Serial.println(devUUID.toString().c_str());
    // }

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;

    }
  }
};

void ble_init() {
  BLEDevice::init("");

  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  // pBLEScan->start(5, false);
  pBLEScan->start(30, false); // 30s待つ
}

bool ble_connectToServer() {
  if (doConnect == false) return false;

  doConnect = false;

  if (connectToServer()) {
    Serial.println("We are now connected to the BLE Server.");
    return true;
  } else {
    Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    return false;
  }
}

bool ble_connectedchk() {
  return connected;
}

void ble_write(String command) {
  String newValue = command + "\r";
  // Serial.print("[snd]:");
  Serial.println(command);
  pRemoteCharacteristic_w->writeValue(newValue.c_str(), newValue.length());
}

bool ble_promptchk() {
  bool ret = get_prompt;
  get_prompt = false;
  return ret;
}
