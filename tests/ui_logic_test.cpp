#include "ui_logic.h"
#include <iostream>

int main() {
    int failures = 0;
#define CHECK(expr) do { if (!(expr)) { std::cerr << "CHECK failed at line " << __LINE__ << ": " #expr "\n"; ++failures; } } while (0)
    CHECK(ui_logic::PickupStateText(-1) == L"CHƯA XÁC ĐỊNH");
    CHECK(ui_logic::PickupStateText(0) == L"OFF");
    CHECK(ui_logic::PickupStateText(1) == L"ON");
    CHECK(ui_logic::PickupStateText(7) == L"CHƯA XÁC ĐỊNH");
    CHECK(!ui_logic::CanEnablePickup(false, true));
    CHECK(!ui_logic::CanEnablePickup(true, false));
    CHECK(ui_logic::CanEnablePickup(true, true));
    if (failures != 0) {
        std::cerr << "ui_logic_tests: " << failures << " failure(s)\n";
        return 1;
    }
    std::cout << "ui_logic_tests: PASS\n";
    return 0;
}
