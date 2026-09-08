#include "auto_settings_parser.h"
#include <iostream>
#include <string>

using autosettings::GroupKind;

static int g_failures = 0;
#define CHECK(expr) do { if (!(expr)) { std::cerr << "CHECK failed at line " << __LINE__ << ": " #expr "\n"; ++g_failures; } } while (0)

static void TestFull41() {
    const std::wstring raw =
        L"4.1#0||0|1|500|-1_-1_-1_-1_-1_-1_-1|0|1|0"
        L"#1|500|0|0_1_3_0_1|0|0||0|drop_rules"
        L"#1|0|0|1|2|1|-1_-1|0|0|1|0|0|0|0"
        L"#1|50|1|60|1|1|-1|-1|0|70|1|0|1|0"
        L"#1|123|0|0|0|0|1|0|2|1|80|0|0|1|0|0"
        L"#1|1|0|0|406_407|1"
        L"#1|1|3|0|0|0|0|5|30|1|1|8|30|2|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0";
    const auto d = autosettings::Parse(raw);
    CHECK(d.version == L"4.1");
    CHECK(d.versionMatches);
    CHECK(!d.groups.empty());
    const auto* pick = autosettings::FindGroup(d, GroupKind::PickItem);
    CHECK(pick != nullptr);
    if (!pick) return;
    CHECK(pick->fields.size() >= 9);
    if (pick->fields.size() < 9) return;
    CHECK(pick->fields[0].name == L"IsOn");
    CHECK(pick->fields[0].raw == L"1");
    CHECK(pick->fields[7].name == L"IsAutoDropItem");
    CHECK(pick->fields[8].name == L"DropItemSettings");
    CHECK(pick->fields[8].raw == L"drop_rules");
}

static void TestBoolDisplay() {
    autosettings::Field f{L"Flag", L"1", autosettings::FieldType::Boolean};
    CHECK(autosettings::DisplayValue(f) == L"ON");
    f.raw = L"0";
    CHECK(autosettings::DisplayValue(f) == L"OFF");
    f.raw = L"x";
    CHECK(autosettings::DisplayValue(f) == L"x (BOOL?)");
}

static void TestTruncatedAndExtra() {
    const auto d = autosettings::Parse(L"4.1#1|2#1|500|0|x|0|0||0|rules|EXTRA");
    CHECK(d.schemaMismatch);
    const auto* pick = autosettings::FindGroup(d, GroupKind::PickItem);
    CHECK(pick != nullptr);
    if (!pick) return;
    CHECK(pick->extras.size() == 1);
    if (!pick->extras.empty()) CHECK(pick->extras[0] == L"EXTRA");
}

static void TestVersionMismatch() {
    const auto d = autosettings::Parse(L"9.9#######");
    CHECK(!d.versionMatches);
    CHECK(d.schemaMismatch);
    CHECK(d.raw == L"9.9#######");
}

static void TestFubenScheduleBounds() {
    const auto d = autosettings::Parse(L"4.1#######0|0|0|0|0|0|0|0|0|0|1|8|30|A|1|9");
    const auto* f = autosettings::FindGroup(d, GroupKind::FuBen);
    CHECK(f != nullptr);
    if (!f) return;
    CHECK(f->fields.size() >= 10);
    CHECK(d.schemaMismatch);
}

int main() {
    TestFull41();
    TestBoolDisplay();
    TestTruncatedAndExtra();
    TestVersionMismatch();
    TestFubenScheduleBounds();
    if (g_failures != 0) {
        std::cerr << "auto_settings_parser_tests: " << g_failures << " failure(s)\n";
        return 1;
    }
    std::cout << "auto_settings_parser_tests: PASS\n";
    return 0;
}
