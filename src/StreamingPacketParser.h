#pragma once

#include <cstdint>
#include <cstddef>

#include "PrinterPacket.h"

class StreamingPacketParser
{
public:
    enum ParseState
    {
        Header,
        Data,
        Footer,
    };

    void parse(uint8_t* data, size_t length);

private:
    ParseState state = ParseState::Header;

    PacketHeader* header = nullptr;
    uint8_t* packetData = nullptr;
    size_t remainingPacketDataLength = 0;
    size_t remainingPacketFooterLength = 0;
    char headerBuffer[sizeof(PacketHeader)];
    uint32_t headerIndex = 0;
    u_int8_t checksum = 0;
};
