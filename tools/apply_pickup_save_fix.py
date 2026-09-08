from pathlib import Path


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly one match, got {count}")
    return text.replace(old, new, 1)


p = Path("src/bridge.cpp")
s = p.read_text(encoding="utf-8-sig")

s = replace_once(
    s,
    '#include <vector>\n#include "pickup_ui_logic.h"',
    '#include <vector>\n#include "auto_settings_parser.h"\n#include "pickup_ui_logic.h"',
    "bridge parser include",
)

s = replace_once(
    s,
    '''bool IsToggleClass(Il2CppClass* klass) {
    return klass &&
           ExactMethod(klass, "get_Selected", 0, false) &&
           (ExactMethod(klass, "HandleSelectEvent", 1, false, "System.Boolean") ||
            ExactMethod(klass, "set_Selected", 1, false, "System.Boolean"));
}

struct UiRuntime {''',
    '''bool IsToggleClass(Il2CppClass* klass) {
    return klass &&
           ExactMethod(klass, "get_Selected", 0, false) &&
           (ExactMethod(klass, "HandleSelectEvent", 1, false, "System.Boolean") ||
            ExactMethod(klass, "set_Selected", 1, false, "System.Boolean"));
}

bool IsButtonClass(Il2CppClass* klass) {
    return klass && ExactMethod(klass, "HandleClickEvent", 0, false);
}

struct UiRuntime {''',
    "bridge button class predicate",
)

s = replace_once(
    s,
    '''    Il2CppClass* uiObject = nullptr;
    Il2CppClass* toggle = nullptr;
    FieldInfo* instances = nullptr;''',
    '''    Il2CppClass* uiObject = nullptr;
    Il2CppClass* toggle = nullptr;
    Il2CppClass* button = nullptr;
    FieldInfo* instances = nullptr;''',
    "bridge UiRuntime button field",
)

s = replace_once(
    s,
    '''        if (!g_ui.uiObject && Eq(name, "UIObject") && IsUiObjectClass(klass)) g_ui.uiObject = klass;
        if (!g_ui.toggle && Eq(name, "UIToggle") && IsToggleClass(klass)) g_ui.toggle = klass;
        if (g_ui.uiObject && g_ui.toggle) return;''',
    '''        if (!g_ui.uiObject && Eq(name, "UIObject") && IsUiObjectClass(klass)) g_ui.uiObject = klass;
        if (!g_ui.toggle && Eq(name, "UIToggle") && IsToggleClass(klass)) g_ui.toggle = klass;
        if (!g_ui.button && Eq(name, "UIButton") && IsButtonClass(klass)) g_ui.button = klass;
        if (g_ui.uiObject && g_ui.toggle && g_ui.button) return;''',
    "bridge metadata button discovery",
)

s = replace_once(
    s,
    '''    g_ui.uiObject = g_api.class_from_name(g_ui.image, "FGStudio.LuaSystem.Base", "UIObject");
    g_ui.toggle = g_api.class_from_name(g_ui.image, "FGStudio.LuaSystem.GUI", "UIToggle");
    if (g_ui.uiObject && !IsUiObjectClass(g_ui.uiObject)) g_ui.uiObject = nullptr;
    if (g_ui.toggle && !IsToggleClass(g_ui.toggle)) g_ui.toggle = nullptr;
    FindUiClassesByMetadata();
    g_ui.instances = g_ui.uiObject ? FindField(g_ui.uiObject, "instances") : nullptr;
    if (!g_ui.uiObject || !g_ui.toggle || !g_ui.instances) {
        SetText(detail, cap, L"Không resolve đủ UIObject.instances/UIToggle đã validate");
        return false;
    }''',
    '''    g_ui.uiObject = g_api.class_from_name(g_ui.image, "FGStudio.LuaSystem.Base", "UIObject");
    g_ui.toggle = g_api.class_from_name(g_ui.image, "FGStudio.LuaSystem.GUI", "UIToggle");
    g_ui.button = g_api.class_from_name(g_ui.image, "FGStudio.LuaSystem.GUI", "UIButton");
    if (g_ui.uiObject && !IsUiObjectClass(g_ui.uiObject)) g_ui.uiObject = nullptr;
    if (g_ui.toggle && !IsToggleClass(g_ui.toggle)) g_ui.toggle = nullptr;
    if (g_ui.button && !IsButtonClass(g_ui.button)) g_ui.button = nullptr;
    FindUiClassesByMetadata();
    g_ui.instances = g_ui.uiObject ? FindField(g_ui.uiObject, "instances") : nullptr;
    if (!g_ui.uiObject || !g_ui.toggle || !g_ui.button || !g_ui.instances) {
        SetText(detail, cap, L"Không resolve đủ UIObject.instances/UIToggle/UIButton đã validate");
        return false;
    }''',
    "bridge EnsureUiDiscovery button",
)

insert_buttons = r'''

struct RuntimeButton {
    Il2CppObject* object = nullptr;
    Il2CppClass* klass = nullptr;
    pickup_ui_logic::Candidate candidate{};
    bool interactable = true;
    bool hasClickHandler = false;
};

bool EnumerateActiveButtons(std::vector<RuntimeButton>& buttons, wchar_t* detail, std::size_t cap) {
    buttons.clear();
    if (!EnsureUiDiscovery(detail, cap)) return false;
    Il2CppObject* dictionary = nullptr;
    g_api.field_static_get_value(g_ui.instances, &dictionary);
    Il2CppObject* entries = nullptr;
    std::int32_t count = 0;
    std::uintptr_t capacity = 0;
    if (!dictionary || !ReadLocal(dictionary, 0x18, entries) || !entries ||
        !ReadLocal(dictionary, 0x20, count) || count < 0 || count > 32768 ||
        !ReadLocal(entries, 0x18, capacity) || capacity > 32768) {
        SetText(detail, cap, L"UIObject.instances dictionary không hợp lệ khi quét nút Lưu thiết lập");
        return false;
    }

    for (std::uintptr_t i = 0; i < capacity; ++i) {
        Il2CppObject* object = nullptr;
        const std::size_t entry = 0x20 + static_cast<std::size_t>(i) * 0x18;
        if (!ReadLocal(entries, entry + 0x10, object) || !object) continue;
        Il2CppClass* klass = g_api.object_get_class(object);
        if (!klass || !g_api.class_is_assignable_from(g_ui.button, klass)) continue;

        bool active = false;
        if (!ReadToggleBool(object, klass, "get_ActiveInHierarchy", active) || !active) continue;
        bool interactable = true;
        bool interactableValue = true;
        if (ReadToggleBool(object, klass, "get_Interactable", interactableValue)) interactable = interactableValue;

        RuntimeButton row{};
        row.object = object;
        row.klass = klass;
        row.interactable = interactable;
        row.hasClickHandler = ExactMethod(klass, "HandleClickEvent", 0, false) != nullptr;
        (void)ReadStringMember(object, klass, "Name", row.candidate.name);
        (void)ReadStringMember(object, klass, "Text", row.candidate.text);
        CollectDescendantLabels(object, row.candidate.descendants);
        CollectAncestorLabels(object, row.candidate.ancestors);
        buttons.push_back(std::move(row));
    }
    return true;
}
'''

s = replace_once(
    s,
    '''        toggles.push_back(std::move(row));
    }
    return true;
}

void AppendToggleDiagnostic''',
    '''        toggles.push_back(std::move(row));
    }
    return true;
}''' + insert_buttons + '''

void AppendToggleDiagnostic''',
    "bridge enumerate buttons",
)

insert_button_helpers = r'''

void AppendButtonDiagnostic(wchar_t* detail, std::size_t cap, const RuntimeButton& button) {
    AppendText(detail, cap, L" [N=");
    AppendText(detail, cap, button.candidate.name.c_str());
    AppendText(detail, cap, L" T=");
    AppendText(detail, cap, button.candidate.text.c_str());
    AppendText(detail, cap, L" D=");
    AppendText(detail, cap, button.candidate.descendants.c_str());
    AppendText(detail, cap, L" A=");
    AppendText(detail, cap, button.candidate.ancestors.c_str());
    AppendText(detail, cap, L" I=");
    AppendInt(detail, cap, button.interactable ? 1 : 0);
    AppendText(detail, cap, L"]");
}

bool SelectRuntimeSaveSettingsButton(std::vector<RuntimeButton>& buttons, int& selectedIndex,
                                     pickup_ui_logic::SelectionKind& kind,
                                     wchar_t* detail, std::size_t cap) {
    selectedIndex = -1;
    kind = pickup_ui_logic::SelectionKind::None;
    if (!EnumerateActiveButtons(buttons, detail, cap)) return false;
    std::vector<pickup_ui_logic::Candidate> candidates;
    candidates.reserve(buttons.size());
    for (const auto& button : buttons) candidates.push_back(button.candidate);
    const auto selection = pickup_ui_logic::SelectSaveSettingsButton(candidates);
    kind = selection.kind;
    selectedIndex = selection.index;
    return true;
}
'''

s = replace_once(
    s,
    '''    AppendText(detail, cap, L"]");
}

bool SelectRuntimePickup''',
    '''    AppendText(detail, cap, L"]");
}''' + insert_button_helpers + '''

bool SelectRuntimePickup''',
    "bridge save button selector helpers",
)

# Pickup runtime is mutable when either the preferred setter or the verified
# select handler exists. The old bridge incorrectly advertised only the handler.
old_mut = 'response.mutationAvailable = (toggle.interactable && toggle.hasSelectHandler) ? 1 : 0;'
count = s.count(old_mut)
if count != 2:
    raise SystemExit(f"bridge mutationAvailable: expected 2 matches, got {count}")
s = s.replace(old_mut, 'response.mutationAvailable = (toggle.interactable && (toggle.hasSetSelected || toggle.hasSelectHandler)) ? 1 : 0;')

start = s.find('bool EnsurePickupOn(Response& response, wchar_t* detail, std::size_t cap) {')
end = s.find('\nbool EnsureMapping() {', start)
if start < 0 or end < 0:
    raise SystemExit("bridge EnsurePickupOn splice anchors not found")

new_ensure = r'''int PersistedPickupState(const Response& response) {
    if (!response.autoSettings[0]) return -1;
    const autosettings::Document doc = autosettings::Parse(response.autoSettings);
    if (!doc.versionMatches) return -1;
    const autosettings::Group* pick = autosettings::FindGroup(doc, autosettings::GroupKind::PickItem);
    if (!pick || pick->fields.empty()) return -1;
    const std::wstring& raw = pick->fields[0].raw;
    if (raw == L"1") return 1;
    if (raw == L"0") return 0;
    return -1;
}

bool EnsurePickupOn(Response& response, wchar_t* detail, std::size_t cap) {
    PopulatePersistedSnapshot(response);
    const int persistedBefore = PersistedPickupState(response);
    response.runtimePickupState = -1;
    response.mutationAvailable = 0;

    wchar_t tabDetail[512]{};
    const PickupTabEnsureStatus tabStatus = EnsurePickupTabSelected(tabDetail, _countof(tabDetail));
    if (tabStatus == PickupTabEnsureStatus::Error) {
        SetText(detail, cap, tabDetail);
        return false;
    }
    if (tabStatus == PickupTabEnsureStatus::Blocked) {
        response.ok = 0;
        response.resultCode = static_cast<std::int32_t>(ResultCode::MutationBlocked);
        SetText(detail, cap, tabDetail);
        return true;
    }

    std::vector<RuntimeToggle> toggles;
    int index = -1;
    pickup_ui_logic::SelectionKind kind{};
    if (!SelectRuntimePickup(toggles, index, kind, detail, cap)) return false;
    if (kind != pickup_ui_logic::SelectionKind::Unique || index < 0 ||
        static_cast<std::size_t>(index) >= toggles.size()) {
        response.ok = 0;
        response.resultCode = static_cast<std::int32_t>(ResultCode::MutationBlocked);
        SetText(detail, cap, tabDetail);
        AppendText(detail, cap, kind == pickup_ui_logic::SelectionKind::Ambiguous
            ? L" | BLOCKED: nhiều UIToggle Nhặt vật phẩm trong TabPickUp; không ghi"
            : L" | BLOCKED: Nhặt đồ Selected=1 nhưng chưa thấy TogPickUpEquipment; không ghi");
        return true;
    }

    RuntimeToggle& toggle = toggles[static_cast<std::size_t>(index)];
    const bool selected = toggle.candidate.selected == 1;
    const auto route = pickup_ui_logic::ChoosePickupItemMutationRoute(
        selected, toggle.interactable, toggle.hasSetSelected, toggle.hasSelectHandler);
    response.runtimePickupState = selected ? 1 : 0;
    response.mutationAvailable = route == pickup_ui_logic::TabMutationRoute::Blocked ? 0 : 1;

    SetText(detail, cap, tabDetail);
    bool mutated = false;
    if (route == pickup_ui_logic::TabMutationRoute::Blocked) {
        response.ok = 0;
        response.resultCode = static_cast<std::int32_t>(ResultCode::MutationBlocked);
        AppendText(detail, cap, L" | BLOCKED: TogPickUpEquipment không interactable hoặc thiếu set_Selected/HandleSelectEvent");
        AppendToggleDiagnostic(detail, cap, toggle);
        return true;
    }

    if (route != pickup_ui_logic::TabMutationRoute::Noop) {
        std::uint8_t yes = 1;
        void* args[] = {&yes};
        const MethodInfo* method = nullptr;
        const wchar_t* routeName = nullptr;
        if (route == pickup_ui_logic::TabMutationRoute::SetSelected) {
            method = ExactMethod(toggle.klass, "set_Selected", 1, false, "System.Boolean");
            routeName = L"set_Selected(true)";
        } else {
            method = ExactMethod(toggle.klass, "HandleSelectEvent", 1, false, "System.Boolean");
            routeName = L"HandleSelectEvent(true)";
        }
        if (!method || !InvokeVoid(method, toggle.object, args)) {
            response.ok = 0;
            response.resultCode = static_cast<std::int32_t>(ResultCode::MutationBlocked);
            response.mutationAvailable = 0;
            AppendText(detail, cap, L" | BLOCKED: invoke TogPickUpEquipment thất bại; không thử route thứ hai");
            return true;
        }
        bool readBack = false;
        if (!ReadToggleBool(toggle.object, toggle.klass, "get_Selected", readBack) || !readBack) {
            response.ok = 0;
            response.resultCode = static_cast<std::int32_t>(ResultCode::MutationBlocked);
            response.runtimePickupState = readBack ? 1 : 0;
            response.mutationAvailable = 0;
            AppendText(detail, cap, L" | BLOCKED: đã dispatch ");
            AppendText(detail, cap, routeName);
            AppendText(detail, cap, L" nhưng runtime read-back chưa ON; không thử lần hai");
            PopulatePersistedSnapshot(response);
            return true;
        }
        mutated = true;
        response.runtimePickupState = 1;
        AppendText(detail, cap, L" | RUNTIME PASS: TogPickUpEquipment ");
        AppendText(detail, cap, routeName);
        AppendText(detail, cap, L" + get_Selected=ON");
    } else {
        response.runtimePickupState = 1;
        AppendText(detail, cap, L" | RUNTIME PASS: Nhặt vật phẩm đã ON; không toggle lại");
    }

    // If no UI mutation was needed and persisted state is already ON, both
    // independent proofs are satisfied; do not click Save redundantly.
    if (!mutated && persistedBefore == 1) {
        response.ok = 1;
        response.resultCode = static_cast<std::int32_t>(ResultCode::Ok);
        AppendText(detail, cap, L" | PERSISTED PASS: PICKITEM.IsOn đã ON; no-op");
        return true;
    }

    // Tick state and persistence are separate in this client. Invoke exactly one
    // unique Save Settings UIButton inside AutoFightUI, then prove the serialized
    // PICKITEM.IsOn changed to ON. Never click twice in one command.
    std::vector<RuntimeButton> buttons;
    int saveIndex = -1;
    pickup_ui_logic::SelectionKind saveKind{};
    if (!SelectRuntimeSaveSettingsButton(buttons, saveIndex, saveKind, detail, cap)) return false;
    if (saveKind != pickup_ui_logic::SelectionKind::Unique || saveIndex < 0 ||
        static_cast<std::size_t>(saveIndex) >= buttons.size()) {
        response.ok = 0;
        response.resultCode = static_cast<std::int32_t>(ResultCode::MutationBlocked);
        AppendText(detail, cap, saveKind == pickup_ui_logic::SelectionKind::Ambiguous
            ? L" | SAVE BLOCKED: có nhiều nút Lưu thiết lập trong AutoFightUI; không click"
            : L" | SAVE BLOCKED: chưa tìm thấy nút Lưu thiết lập trong AutoFightUI");
        for (const auto& button : buttons) {
            if (pickup_ui_logic::ScoreSaveSettingsButtonCandidate(button.candidate) > 0)
                AppendButtonDiagnostic(detail, cap, button);
        }
        return true;
    }

    RuntimeButton& save = buttons[static_cast<std::size_t>(saveIndex)];
    if (!save.interactable || !save.hasClickHandler) {
        response.ok = 0;
        response.resultCode = static_cast<std::int32_t>(ResultCode::MutationBlocked);
        AppendText(detail, cap, L" | SAVE BLOCKED: nút Lưu thiết lập không interactable hoặc thiếu HandleClickEvent()");
        AppendButtonDiagnostic(detail, cap, save);
        return true;
    }

    const MethodInfo* click = ExactMethod(save.klass, "HandleClickEvent", 0, false);
    if (!click || !InvokeVoid(click, save.object, nullptr)) {
        response.ok = 0;
        response.resultCode = static_cast<std::int32_t>(ResultCode::MutationBlocked);
        AppendText(detail, cap, L" | SAVE BLOCKED: HandleClickEvent() của Lưu thiết lập thất bại; không click lần hai");
        return true;
    }

    PopulatePersistedSnapshot(response);
    const int persistedAfter = PersistedPickupState(response);
    if (persistedAfter != 1) {
        response.ok = 0;
        response.resultCode = static_cast<std::int32_t>(ResultCode::MutationBlocked);
        AppendText(detail, cap, persistedAfter == 0
            ? L" | SAVE DISPATCHED nhưng PERSISTED PICKITEM.IsOn vẫn OFF; không click lần hai"
            : L" | SAVE DISPATCHED nhưng chưa chứng minh được PERSISTED PICKITEM.IsOn; không click lần hai");
        AppendButtonDiagnostic(detail, cap, save);
        return true;
    }

    response.ok = 1;
    response.resultCode = static_cast<std::int32_t>(ResultCode::Ok);
    response.runtimePickupState = 1;
    response.mutationAvailable = 1;
    AppendText(detail, cap, L" | SAVE PASS: HandleClickEvent() Lưu thiết lập + PERSISTED PICKITEM.IsOn=ON");
    AppendButtonDiagnostic(detail, cap, save);
    return true;
}
'''

s = s[:start] + new_ensure + s[end:]
p.write_text(s, encoding="utf-8")

# Link the already-tested parser into the bridge so final persistence proof is
# evaluated inside the same command that clicks Save Settings.
p = Path("CMakeLists.txt")
s = p.read_text(encoding="utf-8-sig")
s = replace_once(
    s,
    "add_library(ThanLongAutoSettingsBridge SHARED src/bridge.cpp)",
    "add_library(ThanLongAutoSettingsBridge SHARED src/bridge.cpp src/auto_settings_parser.cpp)",
    "CMake bridge parser source",
)
p.write_text(s, encoding="utf-8")

print("pickup setter + save-settings persistence patch applied")
