#include "auto_menu_ui_logic.h"
#include <iostream>
#include <vector>

using auto_menu_ui_logic::Candidate;
using auto_menu_ui_logic::SelectionKind;

int main() {
    int failures = 0;
#define CHECK(expr) do { if (!(expr)) { std::cerr << "CHECK failed at line " << __LINE__ << ": " #expr "\n"; ++failures; } } while (0)

    // HUD AUTO: visible label can be on a descendant Text node, matching the
    // actual UIObject.instances diagnostics used elsewhere in this tool.
    {
        std::vector<Candidate> candidates{
            {L"BtnAuto", L"", L"Text_-101/AUTO/Image_-102", L"MainUI/RightHud"},
            {L"BtnOther", L"PK", L"", L"MainUI/RightHud"},
        };
        const auto result = auto_menu_ui_logic::SelectHudAuto(candidates);
        CHECK(result.kind == SelectionKind::Unique);
        CHECK(result.index == 0);
    }

    // Fail closed if two active controls both look exactly like HUD AUTO.
    {
        std::vector<Candidate> candidates{
            {L"BtnAutoA", L"AUTO", L"", L"MainUI"},
            {L"BtnAutoB", L"AUTO", L"", L"AnotherHud"},
        };
        const auto result = auto_menu_ui_logic::SelectHudAuto(candidates);
        CHECK(result.kind == SelectionKind::Ambiguous);
        CHECK(result.index == -1);
    }

    // AUTO text inside the settings panel must never be treated as the HUD button.
    {
        std::vector<Candidate> candidates{
            {L"SomeAuto", L"AUTO", L"", L"AutoFightUI/Panel"},
        };
        const auto result = auto_menu_ui_logic::SelectHudAuto(candidates);
        CHECK(result.kind == SelectionKind::None);
    }

    // Small AUTO menu: Thiết lập can also live on a descendant Text node.
    {
        std::vector<Candidate> candidates{
            {L"BtnQuest", L"Nhiệm vụ", L"", L"AutoMenu"},
            {L"BtnSetting", L"", L"Text_-201/Thiết lập/Image_-202", L"AutoMenu"},
            {L"BtnStop", L"Dừng", L"", L"AutoMenu"},
        };
        const auto result = auto_menu_ui_logic::SelectSettingsChoice(candidates);
        CHECK(result.kind == SelectionKind::Unique);
        CHECK(result.index == 1);
    }

    // Once AutoFightUI is already open, its own labels are not menu-choice candidates.
    {
        std::vector<Candidate> candidates{
            {L"ButtonSaveSettings", L"Lưu thiết lập", L"", L"AutoFightUI"},
            {L"Title", L"Thiết Lập AUTO", L"", L"AutoFightUI"},
        };
        const auto result = auto_menu_ui_logic::SelectSettingsChoice(candidates);
        CHECK(result.kind == SelectionKind::None);
    }

    // Proof for the final state: active TogglePickUpTab under AutoFightUI is sufficient
    // evidence that the Auto settings panel is actually open.
    {
        std::vector<Candidate> candidates{
            {L"TogglePickUpTab", L"Nhặt đồ", L"", L"AutoFightUI"},
            {L"Other", L"AUTO", L"", L"MainUI"},
        };
        const auto result = auto_menu_ui_logic::SelectAutoFightPanelProof(candidates);
        CHECK(result.kind == SelectionKind::Unique);
        CHECK(result.index == 0);
    }

    if (failures != 0) {
        std::cerr << "auto_menu_ui_logic_tests: " << failures << " failure(s)\n";
        return 1;
    }
    std::cout << "auto_menu_ui_logic_tests: PASS\n";
    return 0;
}
