#pragma once
#include <string>

namespace ui_logic {
inline std::wstring PickupStateText(int state) {
    if (state == 1) return L"ON";
    if (state == 0) return L"OFF";
    return L"CHƯA XÁC ĐỊNH";
}
inline bool CanEnablePickup(bool mutationAvailable, bool versionMatches) {
    return mutationAvailable && versionMatches;
}
} // namespace ui_logic
