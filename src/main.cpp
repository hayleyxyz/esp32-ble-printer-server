#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <esp_task_wdt.h>
#include <freertos/task.h>
#include <PrinterPacket.h>
#include <util.h>
#include "StreamingPacketParser.h"
#include "net.h"

class ApplicationBLECharacteristicCallbacks;

static BLEUUID advertisedServiceUUID(BLEUUID((uint16_t)0xaf30));
static BLEUUID genericAccessServiceUUID(BLEUUID((uint16_t)0x1800));
static BLEUUID deviceNameCharacteristicUUID(BLEUUID((uint16_t)0x2a00));
static BLEUUID printerServiceUUID(BLEUUID((uint16_t)0xae30));
static BLEUUID writeCharacteristicUUID(BLEUUID((uint16_t)0xae01));
static BLEUUID notifyCharacteristicUUID(BLEUUID((uint16_t)0xae02));
static BLEUUID write2CharacteristicUUID(BLEUUID((uint16_t)0xae03));
static BLEUUID notify2CharacteristicUUID(BLEUUID((uint16_t)0xae04));
static BLEUUID indicateCharacteristicUUID(BLEUUID((uint16_t)0xae05));
static BLEUUID readWriteCharacteristicUUID(BLEUUID((uint16_t)0xae10));
static BLEUUID auxServiceUUID(BLEUUID((uint16_t)0xae3a));
static BLEUUID auxWriteCharacteristicUUID(BLEUUID((uint16_t)0xae3b));
static BLEUUID auxNotifyCharacteristicUUID(BLEUUID((uint16_t)0xae3c));

static ApplicationBLECharacteristicCallbacks* characteristicCallbacks = nullptr;
static BLEServer* bleServer = nullptr;
static StreamingPacketParser* streamingPacketParser = nullptr;

class ApplicationBLECharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        auto data = pCharacteristic->getData();
        auto length = pCharacteristic->getLength();

        streamingPacketParser->parse(pCharacteristic->getData(), pCharacteristic->getLength());

        fflush(stdout);
    }
};

class ApplicationBLEServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        printf("onConnect\n");

        if (streamingPacketParser != nullptr) {
            delete streamingPacketParser;
        }

        streamingPacketParser = new StreamingPacketParser();
    }

    void onDisconnect(BLEServer *pServer) {
        printf("onDisconnect\n");
        delay(100);

        esp_restart();
    }

    void onMtuChanged(BLEServer* pServer, esp_ble_gatts_cb_param_t *param) {
        printf("onMtuChanged\n");
    }
};

void ble_server_init() {
    BLEDevice::init("MX05");
    bleServer = BLEDevice::createServer();
    
    bleServer->setCallbacks(new ApplicationBLEServerCallbacks());

    auto printerServce = bleServer->createService(printerServiceUUID);
    auto writeCharacteristic = printerServce->createCharacteristic(writeCharacteristicUUID, BLECharacteristic::PROPERTY_WRITE_NR);
    auto notifyCharacteristic = printerServce->createCharacteristic(notifyCharacteristicUUID, BLECharacteristic::PROPERTY_NOTIFY);
    auto write2Characteristic = printerServce->createCharacteristic(write2CharacteristicUUID, BLECharacteristic::PROPERTY_WRITE);
    auto notify2Characteristic = printerServce->createCharacteristic(notify2CharacteristicUUID, BLECharacteristic::PROPERTY_NOTIFY);
    auto indicateCharacteristic = printerServce->createCharacteristic(indicateCharacteristicUUID, BLECharacteristic::PROPERTY_INDICATE);
    auto readWriteCharacteristic = printerServce->createCharacteristic(readWriteCharacteristicUUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

    auto genericAccessService = bleServer->createService(genericAccessServiceUUID);
    auto deviceNameCharacteristic = genericAccessService->createCharacteristic(deviceNameCharacteristicUUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

    auto auxService = bleServer->createService(auxServiceUUID);
    auto auxWriteCharacteristic = auxService->createCharacteristic(auxWriteCharacteristicUUID, BLECharacteristic::PROPERTY_WRITE_NR);
    auto auxNotifyCharacteristic = auxService->createCharacteristic(auxNotifyCharacteristicUUID, BLECharacteristic::PROPERTY_NOTIFY);

    characteristicCallbacks = new ApplicationBLECharacteristicCallbacks();

    deviceNameCharacteristic->setValue("MX05");
    auxWriteCharacteristic->setCallbacks(characteristicCallbacks);
    auxNotifyCharacteristic->setCallbacks(characteristicCallbacks);
    write2Characteristic->setCallbacks(characteristicCallbacks);
    notify2Characteristic->setCallbacks(characteristicCallbacks);
    indicateCharacteristic->setCallbacks(characteristicCallbacks);
    readWriteCharacteristic->setCallbacks(characteristicCallbacks);
    deviceNameCharacteristic->setCallbacks(characteristicCallbacks);
    writeCharacteristic->setCallbacks(characteristicCallbacks);
    notifyCharacteristic->setCallbacks(characteristicCallbacks);

    printerServce->start();

    auto pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(advertisedServiceUUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    printf("BLE Address: %s\n", BLEDevice::getAddress().toString().c_str());
}

void setup() {
    Serial.begin(115200);

    // Disable the watchdog on all CPUs
    for (int x = 0; x < portNUM_PROCESSORS; x++) {
        auto handle = xTaskGetCurrentTaskHandleForCPU(x);
        if (esp_task_wdt_status(handle) == ESP_OK) {
            esp_task_wdt_delete(handle);
        }
    }

    ble_server_init();
}

void loop() {
    auto c = getchar();

    if (c == '\n') {
        putchar('\n');
    }
    
    delay(10);
}