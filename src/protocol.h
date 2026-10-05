#pragma once
#include <stdint.h>

namespace battery {
constexpr unsigned ReportSize = 91; // Windows report ID + 90-byte Razer payload.
inline bool FeatureLength(unsigned bytes, uint8_t reportId) {
    // Some Windows HID drivers omit the zero report ID from bytesReturned.
    return bytes == ReportSize || (reportId == 0 && bytes == ReportSize - 1);
}
inline uint8_t Checksum(const uint8_t* packet) {
    uint8_t crc = 0;
    for (unsigned i = 3; i < 89; ++i) crc ^= packet[i];
    return crc;
}
inline void Request(uint8_t* packet, uint8_t command) {
    for (unsigned i = 0; i < ReportSize; ++i) packet[i] = 0;
    packet[2] = 0x1f;
    packet[6] = 2;
    packet[7] = 7;
    packet[8] = command;
    packet[89] = Checksum(packet);
}
inline bool Valid(const uint8_t* packet, uint8_t command) {
    return packet[0] == 0 && packet[1] == 2 && packet[2] == 0x1f &&
        packet[3] == 0 && packet[4] == 0 && packet[5] == 0 &&
        packet[6] == 2 && packet[7] == 7 && packet[8] == command &&
        packet[89] == Checksum(packet);
}
inline int Percent(uint8_t raw) { return (raw * 100 + 127) / 255; }
enum class Tone { Unavailable, Red, Yellow, Green };
inline Tone Color(int percent) {
    return percent < 0 ? Tone::Unavailable : percent < 25 ? Tone::Red :
        percent < 50 ? Tone::Yellow : Tone::Green;
}
inline bool Supported(uint16_t pid) {
    return pid == 0x00aa || pid == 0x00ab || pid == 0x00cc || pid == 0x00cd;
}
inline bool Wired(uint16_t pid) { return pid == 0x00aa || pid == 0x00cc; }
inline bool Full(int percent) { return percent == 100; }
}
