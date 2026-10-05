#include "protocol.h"
#include <stdio.h>
int main() {
    int failures = 0;
    auto check = [&](bool condition, const char* label) {
        if (!condition) { printf("FAIL: %s\n", label); ++failures; }
    };
    uint8_t packet[battery::ReportSize];
    check(battery::FeatureLength(90, 0) && battery::FeatureLength(91, 0) &&
        !battery::FeatureLength(89, 0) && !battery::FeatureLength(90, 1), "Windows report ID length normalization");
    battery::Request(packet, 0x80);
    check(packet[0] == 0 && packet[2] == 0x1f && packet[89] == 0x85, "Windows request framing");
    check(!battery::Valid(packet, 0x80), "request cannot be accepted as a response");
    packet[1] = 2; packet[10] = 255; packet[89] = battery::Checksum(packet);
    check(battery::Valid(packet, 0x80), "valid response");
    check(!battery::Valid(packet, 0x84), "reject charging response collision");
    packet[10] ^= 1;
    check(!battery::Valid(packet, 0x80), "reject corrupt payload");
    packet[10] ^= 1; packet[2] = 0x3f;
    check(!battery::Valid(packet, 0x80), "reject wrong transaction");
    packet[2] = 0x1f; packet[1] = 4;
    check(!battery::Valid(packet, 0x80), "reject mouse asleep / no response");
    check(battery::Percent(0) == 0 && battery::Percent(255) == 100 && battery::Percent(128) == 50, "battery conversion");
    for (int raw = 0; raw < 256; ++raw) {
        check(battery::Percent((uint8_t)raw) >= 0 && battery::Percent((uint8_t)raw) <= 100, "percentage range");
        if (raw > 0) check(battery::Percent((uint8_t)raw) >= battery::Percent((uint8_t)(raw - 1)), "monotonic percentage");
    }
    check(battery::Color(-1) == battery::Tone::Unavailable, "unavailable is never low battery");
    check(battery::Color(24) == battery::Tone::Red && battery::Color(25) == battery::Tone::Yellow, "25 percent boundary");
    check(battery::Color(49) == battery::Tone::Yellow && battery::Color(50) == battery::Tone::Green, "50 percent boundary");
    check(!battery::Full(99) && battery::Full(100), "checkmark only at 100 percent");
    check(battery::Supported(0x00aa) && battery::Supported(0x00ab) && battery::Supported(0x00cc) &&
        battery::Supported(0x00cd) && !battery::Supported(0x0099), "supported original and 35K transports");
    printf("Protocol tests: %s\n", failures ? "FAILED" : "passed");
    return failures ? 1 : 0;
}
