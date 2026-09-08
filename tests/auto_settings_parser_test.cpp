#include "auto_settings_parser.h"
#include <cassert>
#include <iostream>
#include <string>

using autosettings::GroupKind;

static void TestFull41() {
    const std::wstring raw =
        L"4.1#0||0|1|500|-1_-1_-1_-1_-1_-1_-1|0|1|0"
        L"#1|500|0|0_1_3_0_1|0|0||0|drop_rules"
        L"#1|0|0|1|2|1|-1_-1|0|0|1|0|0|0|0"
        L"#1|50|1|60|1|1|-1|-1|0|70|1|0|1|0"
        L"#1|123|0|0|0|0|1|0|2|1|80|0|0|1|0|0"
        L"#1|1|0|0|406_407|1"
        L"#1|1|3|0|0|0|0|5|30|1|1|8|30|2|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0";
    auto d = autosettings::Parse(raw);
    assert(d.version == L"4.1");
    assert(d.versionMatches);
    assert(!d.groups.empty());
    auto* pick = autosettings::FindGroup(d, GroupKind::PickItem);
    assert(pick);
    assert(pick->fields.size() >= 9);
    assert(pick->fields[0].name == L"IsOn");
    assert(pick->fields[0].raw == L"1");
    assert(pick->fields[7].name == L"IsAutoDropItem");
    assert(pick->fields[8].name == L"DropItemSettings");
    assert(pick->fields[8].raw == L"drop_rules");
}

static void TestBoolDisplay() {
    autosettings::Field f{L"Flag", L"1", autosettings::FieldType::Boolean};
    assert(autosettings::DisplayValue(f) == L"ON");
    f.raw = L"0";
    assert(autosettings::DisplayValue(f) == L"OFF");
    f.raw = L"x";
    assert(autosettings::DisplayValue(f) == L"x (BOOL?)");
}

static void TestTruncatedAndExtra() {
    auto d = autosettings::Parse(L"4.1#1|2#1|500|0|x|0|0||0|rules|EXTRA");
    assert(d.schemaMismatch);
    auto* pick = autosettings::FindGroup(d, GroupKind::PickItem);
    assert(pick);
    assert(pick->extras.size() == 1);
    assert(pick->extras[0] == L"EXTRA");
}

static void TestVersionMismatch() {
    auto d = autosettings::Parse(L"9.9#######");
    assert(!d.versionMatches);
    assert(d.schemaMismatch);
    assert(d.raw == L"9.9#######");
}

static void TestFubenScheduleBounds() {
    auto d = autosettings::Parse(L"4.1#######0|0|0|0|0|0|0|0|0|0|1|8|30|A|1|9");
    auto* f = autosettings::FindGroup(d, GroupKind::FuBen);
    assert(f);
    assert(f->fields.size() >= 10);
    assert(d.schemaMismatch);
}

int main() {
    TestFull41();
    TestBoolDisplay();
    TestTruncatedAndExtra();
    TestVersionMismatch();
    TestFubenScheduleBounds();
    std::cout << "auto_settings_parser_tests: PASS\n";
    return 0;
}
