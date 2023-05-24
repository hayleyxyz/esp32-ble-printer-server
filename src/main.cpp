#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "PrinterPacket.h"
#include "StreamingPacketParser.h"
#include "util.h"

class ApplicationBLECharacteristicCallbacks;
class StreamingPacketParser;

static BLEUUID advertisedServiceUUID(BLEUUID((uint16_t)0xaf30));
static BLEUUID genericAccessServiceUUID(BLEUUID((uint16_t)0x1800));
static BLEUUID printerServiceUUID(BLEUUID((uint16_t)0xae30));
static BLEUUID writeCharacteristicUUID(BLEUUID((uint16_t)0xae01));
static BLEUUID notifyCharacteristicUUID(BLEUUID((uint16_t)0xae02));

ApplicationBLECharacteristicCallbacks* characteristicCallbacks = nullptr;
BLEServer* pServer = nullptr;
StreamingPacketParser* streamingPacketParser = nullptr;

class ApplicationBLECharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        auto data = pCharacteristic->getData();
        auto length = pCharacteristic->getLength();
        
        Serial.println("onWrite");
        hex_dump(data, length);

        streamingPacketParser->parse(pCharacteristic->getData(), pCharacteristic->getLength());
    }
};

class ApplicationBLEServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *pServer) {
        Serial.println("onConnect");
        Serial.flush();

        if (streamingPacketParser != nullptr) {
            delete streamingPacketParser;
        }

        streamingPacketParser = new StreamingPacketParser();
    }

    void onDisconnect(BLEServer *pServer) {
        Serial.println("onDisconnect");
        Serial.flush();
    }
};

void print_chip_info() {
    esp_chip_info_t chip_info;

    esp_chip_info(&chip_info);

    Serial.println(ESP.getChipModel());

    Serial.print("Features: ");

    std::vector<String> featureStrings;

    if (chip_info.features & CHIP_FEATURE_BT) featureStrings.push_back("Bluetooth Classic");
    if (chip_info.features & CHIP_FEATURE_BLE) featureStrings.push_back("Bluetooth LE");
    if (chip_info.features & CHIP_FEATURE_WIFI_BGN) featureStrings.push_back("2.4GHz WiFi");
    if (chip_info.features & CHIP_FEATURE_EMB_FLASH) featureStrings.push_back("Embedded flash memory");

    for (int i = 0; i < featureStrings.size(); i++) {
        Serial.print(featureStrings[i]);

        if (i < featureStrings.size() - 1) {
            Serial.print(", ");
        }
    }

    Serial.println();
    
    Serial.printf("Cores: %d, ", chip_info.cores);
    Serial.printf("silicon revision %d, ", chip_info.revision);
    Serial.printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
        (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
}

void setup() {
    Serial.begin(115200);

    print_chip_info();

    BLEDevice::init("MX05");
    pServer = BLEDevice::createServer();

    pServer->setCallbacks(new ApplicationBLEServerCallbacks());

    auto printerServce = pServer->createService(printerServiceUUID);
    auto writeCharacteristic = printerServce->createCharacteristic(writeCharacteristicUUID, BLECharacteristic::PROPERTY_WRITE_NR);
    auto notifyCharacteristic = printerServce->createCharacteristic(notifyCharacteristicUUID, BLECharacteristic::PROPERTY_NOTIFY);

    characteristicCallbacks = new ApplicationBLECharacteristicCallbacks();

    writeCharacteristic->setCallbacks(characteristicCallbacks);
    notifyCharacteristic->setCallbacks(characteristicCallbacks);

    printerServce->start();

    auto pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(advertisedServiceUUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    Serial.println("BLE ready");
}

void loop() {
    delay(2000);
}