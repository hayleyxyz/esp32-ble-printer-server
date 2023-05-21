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

class ApplicationBLECharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        auto data = pCharacteristic->getData();
        auto length = pCharacteristic->getLength();

        PacketHeader header;
        uint8_t *packetData = nullptr;

        if (!PrinterPacket::dissectPacket(data, length, &header, &packetData))
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

        for (int i = 0; i < header.length.length16; i++) {
            if (packetData[i] < 0x10) {
                Serial.print("0"); 
            }

            Serial.print(String(packetData[i], HEX) + " ");
        }

        Serial.println();
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

void setup() {
    Serial.begin(115200);

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
}

void loop() {
    // put your main code here, to run repeatedly:
    delay(2000);
}