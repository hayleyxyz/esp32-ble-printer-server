#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "PrinterPacket.h"

class ApplicationBLECharacteristicCallbacks;

static BLEUUID advertisedServiceUUID(BLEUUID((uint16_t)0xaf30));
static BLEUUID genericAccessServiceUUID(BLEUUID((uint16_t)0x1800));
static BLEUUID printerServiceUUID(BLEUUID((uint16_t)0xae30));
static BLEUUID writeCharacteristicUUID(BLEUUID((uint16_t)0xae01));
static BLEUUID notifyCharacteristicUUID(BLEUUID((uint16_t)0xae02));

ApplicationBLECharacteristicCallbacks *callbacks = nullptr;
BLEServer *pServer = nullptr;

enum PrinterCommand : uint8_t
{
    PrintData = 0xA2,
    Status = 0xA3,
    SetHeat = 0xA4,
    PrintStartStop = 0xA6,
    SetEnergy = 0xAF,
    Draft = 0xBE,
    PaperFeedSpeed = 0xBD,
    PrintDataCompressed = 0xBF,
};

String printerCommandToString(uint8_t command)
{
    switch (command)
    {
    case PrinterCommand::PrintData:
        return "PrintData";
    case PrinterCommand::Status:
        return "Status";
    case PrinterCommand::SetHeat:
        return "SetHeat";
    case PrinterCommand::PrintStartStop:
        return "PrintStartStop";
    case PrinterCommand::SetEnergy:
        return "SetEnergy";
    case PrinterCommand::Draft:
        return "Draft";
    case PrinterCommand::PaperFeedSpeed:
        return "PaperFeedSpeed";
    case PrinterCommand::PrintDataCompressed:
        return "PrintDataCompressed";
    default:
        return "Unknown";
    }
}

void hex_dump(uint8_t *data, size_t length)
{
    for (int i = 0; i < length; i++) {
        if (data[i] < 0x10) {
            Serial.print("0"); 
        }

        Serial.print(String(data[i], HEX) + " ");
    }

    Serial.println();
}

class ApplicationBLECharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        auto data = pCharacteristic->getData();
        auto length = pCharacteristic->getLength();
        auto packetStart = data;

        PacketHeader header;
        uint8_t *packetData = nullptr;

        if (!PrinterPacket::dissectPacket(packetStart, length, &header, &packetData))
        {
            Serial.print("Invalid packet: ");

            for (int i = 0; i < length; i++) {
                if (data[i] < 0x10) {
                    Serial.print("0"); 
                }

                Serial.print(String(data[i], HEX) + " ");
            }

            Serial.println();

            return;
        }

        if (header.command == PrinterCommand::Status) {
            uint8_t statusResponse[] = {0x51, 0x78, 0xA3, 0x01, 0x3, 0x00, 0x00, 0x01, 0xCA, 0x6D, 0xFF };

            auto notifyCharacteristic = pServer->getServiceByUUID(printerServiceUUID)
                ->getCharacteristic(notifyCharacteristicUUID);

            notifyCharacteristic->setValue(statusResponse, sizeof(statusResponse));
            notifyCharacteristic->notify();
        }

        Serial.print("Command: " + printerCommandToString(header.command) + "(" + String(header.command, HEX) + ") ");

        if (header.command == 0xf2) {
            Serial.println();

            hex_dump(data, length);
        }
        else {
            hex_dump(packetData, header.length.length16);
        }
    }

    void onNotify(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        Serial.println("onNotify");
        for (int i = 0; i < value.length(); i++) {
        Serial.print(value[i], HEX);
        }
        Serial.println();
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

    callbacks = new ApplicationBLECharacteristicCallbacks();

    BLEDevice::init("MX05");
    pServer = BLEDevice::createServer();

    auto printerServce = pServer->createService(printerServiceUUID);
    auto writeCharacteristic = printerServce->createCharacteristic(writeCharacteristicUUID, BLECharacteristic::PROPERTY_WRITE_NR);
    auto notifyCharacteristic = printerServce->createCharacteristic(notifyCharacteristicUUID, BLECharacteristic::PROPERTY_NOTIFY);

    writeCharacteristic->setCallbacks(callbacks);
    notifyCharacteristic->setCallbacks(callbacks);

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
    // put your main code here, to run repeatedly:
    delay(2000);
}