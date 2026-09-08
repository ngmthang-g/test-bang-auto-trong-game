#include "ui_logic.h"
#include <cassert>
#include <iostream>

int main() {
    assert(ui_logic::PickupStateText(-1) == L"CHƯA XÁC ĐỊNH");
    assert(ui_logic::PickupStateText(0) == L"OFF");
    assert(ui_logic::PickupStateText(1) == L"ON");
    assert(ui_logic::PickupStateText(7) == L"CHƯA XÁC ĐỊNH");
    assert(!ui_logic::CanEnablePickup(false, true));
    assert(!ui_logic::CanEnablePickup(true, false));
    assert(ui_logic::CanEnablePickup(true, true));
    std::cout << "ui_logic_tests: PASS\n";
    return 0;
}
