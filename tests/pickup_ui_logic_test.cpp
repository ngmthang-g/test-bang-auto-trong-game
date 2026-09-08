#include "pickup_ui_logic.h"
#include <iostream>
#include <vector>

using pickup_ui_logic::Candidate;
using pickup_ui_logic::SelectionKind;

int main() {
    int failures = 0;
#define CHECK(expr) do { if (!(expr)) { std::cerr << "CHECK failed at line " << __LINE__ << ": " #expr "\n"; ++failures; } } while (0)

    {
        std::vector<Candidate> candidates{
            {L"TogPickItem", L"", L"Nhặt vật phẩm", L"AutoFight/PickUp", 0},
            {L"TogAutoEat", L"", L"Tự sử dụng vật phẩm", L"AutoFight/PickUp", 1},
        };
        const auto result = pickup_ui_logic::SelectPickupToggle(candidates);
        CHECK(result.kind == SelectionKind::Unique);
        CHECK(result.index == 0);
        CHECK(result.score > 0);
    }

    {
        std::vector<Candidate> candidates{
            {L"PickItem", L"", L"", L"AutoFight/PickUp", 0},
        };
        const auto result = pickup_ui_logic::SelectPickupToggle(candidates);
        CHECK(result.kind == SelectionKind::Unique);
        CHECK(result.index == 0);
    }

    {
        std::vector<Candidate> candidates{
            {L"TogPickA", L"Nhặt vật phẩm", L"", L"AutoFight/PickUp", 0},
            {L"TogPickB", L"", L"Nhặt vật phẩm", L"AutoFight/PickUp", 1},
        };
        const auto result = pickup_ui_logic::SelectPickupToggle(candidates);
        CHECK(result.kind == SelectionKind::Ambiguous);
        CHECK(result.index == -1);
    }

    {
        std::vector<Candidate> candidates{
            {L"TogAutoEat", L"Tự sử dụng vật phẩm", L"", L"AutoFight/PickUp", 0},
            {L"TogFilter", L"Lọc đồ theo quy tắc", L"", L"AutoFight/PickUp", 0},
        };
        const auto result = pickup_ui_logic::SelectPickupToggle(candidates);
        CHECK(result.kind == SelectionKind::None);
        CHECK(result.index == -1);
    }

    {
        Candidate c{L"toggle_pick_item", L"", L"", L"", 0};
        CHECK(pickup_ui_logic::ScorePickupCandidate(c) > 0);
        c = {L"randomToggle", L"", L"Nhặt vật phẩm", L"", 0};
        CHECK(pickup_ui_logic::ScorePickupCandidate(c) > 0);
        c = {L"randomToggle", L"", L"", L"AutoFight/PickUp", 0};
        CHECK(pickup_ui_logic::ScorePickupCandidate(c) == 0);
    }

    // Live client reproduction 2026-09-08: navigation tab and the actual
    // pickup checkbox were both classified as pickup candidates in v0.2.
    {
        std::vector<Candidate> candidates{
            {L"TogPickUpEquipment", L"Nhặt vật phẩm", L"Text_-11993808/Nhặt vật phẩm/Image_-11993796",
             L"Image_-11993756/TabPickUp/Image_-11990960/AutoFightUI", 0},
            {L"TogglePickUpTab", L"Nhặt đồ", L"Text_-12003002/Nhặt đồ/Image_-12002990",
             L"RectTransform_-12002928/Image_-11990960/AutoFightUI", 1},
        };
        const auto item = pickup_ui_logic::SelectPickupToggle(candidates);
        CHECK(item.kind == SelectionKind::Unique);
        CHECK(item.index == 0);

        const auto tab = pickup_ui_logic::SelectPickupTab(candidates);
        CHECK(tab.kind == SelectionKind::Unique);
        CHECK(tab.index == 1);
    }

    // Fail closed: a generic "Nhặt đồ" toggle outside AutoFightUI is not the
    // AUTO navigation tab.
    {
        std::vector<Candidate> candidates{
            {L"TogglePickUpTab", L"Nhặt đồ", L"", L"SomeOtherPanel", 0},
        };
        const auto tab = pickup_ui_logic::SelectPickupTab(candidates);
        CHECK(tab.kind == SelectionKind::None);
    }

    // The content checkbox must belong to TabPickUp unless its exact internal
    // name is the live-proved TogPickUpEquipment control.
    {
        std::vector<Candidate> candidates{
            {L"randomToggle", L"Nhặt vật phẩm", L"", L"OtherPanel", 0},
        };
        const auto item = pickup_ui_logic::SelectPickupToggle(candidates);
        CHECK(item.kind == SelectionKind::None);
    }

    if (failures != 0) {
        std::cerr << "pickup_ui_logic_tests: " << failures << " failure(s)\n";
        return 1;
    }
    std::cout << "pickup_ui_logic_tests: PASS\n";
    return 0;
}
