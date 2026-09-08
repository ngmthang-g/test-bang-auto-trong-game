#include "protocol.h"
#include <cassert>
#include <iostream>

int main() {
    autosettings_probe::SharedBlock block{};
    assert(block.magic == autosettings_probe::kMagic);
    assert(block.protocolVersion == autosettings_probe::kProtocolVersion);
    assert(block.response.runtimePickupState == -1);
    assert(autosettings_probe::kAutoSettingsCapacity == 8192);
    std::cout << "protocol_layout_tests: PASS\n";
    return 0;
}
