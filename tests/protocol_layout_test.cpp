#include "protocol.h"
#include <iostream>

int main() {
    int failures = 0;
#define CHECK(expr) do { if (!(expr)) { std::cerr << "CHECK failed at line " << __LINE__ << ": " #expr "\n"; ++failures; } } while (0)
    autosettings_probe::SharedBlock block{};
    CHECK(block.magic == autosettings_probe::kMagic);
    CHECK(block.protocolVersion == autosettings_probe::kProtocolVersion);
    CHECK(block.response.runtimePickupState == -1);
    CHECK(autosettings_probe::kAutoSettingsCapacity == 8192);
    CHECK(static_cast<std::uint32_t>(autosettings_probe::Command::ShutdownBridge) == 4u);
    if (failures != 0) {
        std::cerr << "protocol_layout_tests: " << failures << " failure(s)\n";
        return 1;
    }
    std::cout << "protocol_layout_tests: PASS\n";
    return 0;
}
