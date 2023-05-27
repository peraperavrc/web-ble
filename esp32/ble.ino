#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_chip_info.h"
#include "driver/adc.h"

BLEServer* pServer = nullptr;
BLECharacteristic* pCharacteristic = nullptr;
bool deviceConnected = false;
bool oldDeviceConnected = false;

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
};

void setup() {
  Serial.begin(115200);
  BLEDevice::init("ESP32_BLE");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService* pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);

  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();
  pServer->getAdvertising()->start();
  Serial.println("Waiting for client connection to notify...");

  // Initialize the ADC
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_0);
}

void loop() {
  if (deviceConnected) {
    uint8_t data[28];
    uint32_t cpuUsage = esp_get_free_heap_size();
    uint32_t freeHeap = xPortGetFreeHeapSize();
    uint32_t uptime = millis();  // Get the uptime in milliseconds
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    uint8_t model = chip_info.model;                  // Get the ESP32's model
    uint32_t flashChipSize = ESP.getFlashChipSize();  // Get the size of the flash chip
    esp_reset_reason_t reason = esp_reset_reason();   // Get the reason for the last reset
    int16_t hallValue = hallRead();                   // Get the value from the Hall effect sensor

    memcpy(data, &cpuUsage, sizeof(cpuUsage));
    memcpy(data + sizeof(cpuUsage), &freeHeap, sizeof(freeHeap));
    memcpy(data + sizeof(cpuUsage) + sizeof(freeHeap), &uptime, sizeof(uptime));
    memcpy(data + sizeof(cpuUsage) + sizeof(freeHeap) + sizeof(uptime), &model, sizeof(model));
    memcpy(data + sizeof(cpuUsage) + sizeof(freeHeap) + sizeof(uptime) + sizeof(model), &flashChipSize, sizeof(flashChipSize));
    memcpy(data + sizeof(cpuUsage) + sizeof(freeHeap) + sizeof(uptime) + sizeof(model) + sizeof(flashChipSize), &reason, sizeof(reason));
    memcpy(data + sizeof(cpuUsage) + sizeof(freeHeap) + sizeof(uptime) + sizeof(model) + sizeof(flashChipSize) + sizeof(reason), &hallValue, sizeof(hallValue));
    pCharacteristic->setValue(data, sizeof(data));
    pCharacteristic->notify();
    delay(10);
  }

  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    Serial.println("Started advertising");
    oldDeviceConnected = deviceConnected;
  }

  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
}