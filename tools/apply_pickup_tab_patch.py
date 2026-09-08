from pathlib import Path

p = Path("src/bridge.cpp")
s = p.read_text(encoding="utf-8-sig")

old_struct = '''struct RuntimeToggle {
    Il2CppObject* object = nullptr;
    Il2CppClass* klass = nullptr;
    pickup_ui_logic::Candidate candidate{};
    bool interactable = false;
    bool hasSelectHandler = false;
};'''
new_struct = '''struct RuntimeToggle {
    Il2CppObject* object = nullptr;
    Il2CppClass* klass = nullptr;
    pickup_ui_logic::Candidate candidate{};
    bool interactable = false;
    bool hasSetSelected = false;
    bool hasSelectHandler = false;
};'''
if old_struct not in s:
    raise SystemExit("RuntimeToggle pattern not found")
s = s.replace(old_struct, new_struct, 1)

old_flags = '''        row.interactable = interactable;
        row.hasSelectHandler = ExactMethod(klass, "HandleSelectEvent", 1, false, "System.Boolean") != nullptr;'''
new_flags = '''        row.interactable = interactable;
        row.hasSetSelected = ExactMethod(klass, "set_Selected", 1, false, "System.Boolean") != nullptr;
        row.hasSelectHandler = ExactMethod(klass, "HandleSelectEvent", 1, false, "System.Boolean") != nullptr;'''
if old_flags not in s:
    raise SystemExit("RuntimeToggle flags pattern not found")
s = s.replace(old_flags, new_flags, 1)

old_select = '''bool SelectRuntimePickup(std::vector<RuntimeToggle>& toggles, int& selectedIndex,
                         pickup_ui_logic::SelectionKind& kind, wchar_t* detail, std::size_t cap) {
    selectedIndex = -1;
    kind = pickup_ui_logic::SelectionKind::None;
    if (!EnumerateActiveToggles(toggles, detail, cap)) return false;
    std::vector<pickup_ui_logic::Candidate> candidates;
    candidates.reserve(toggles.size());
    for (const auto& toggle : toggles) candidates.push_back(toggle.candidate);
    const auto selection = pickup_ui_logic::SelectPickupToggle(candidates);
    kind = selection.kind;
    selectedIndex = selection.index;
    return true;
}
'''
new_select = '''bool SelectRuntimePickup(std::vector<RuntimeToggle>& toggles, int& selectedIndex,
                         pickup_ui_logic::SelectionKind& kind, wchar_t* detail, std::size_t cap) {
    selectedIndex = -1;
    kind = pickup_ui_logic::SelectionKind::None;
    if (!EnumerateActiveToggles(toggles, detail, cap)) return false;
    std::vector<pickup_ui_logic::Candidate> candidates;
    candidates.reserve(toggles.size());
    for (const auto& toggle : toggles) candidates.push_back(toggle.candidate);
    const auto selection = pickup_ui_logic::SelectPickupToggle(candidates);
    kind = selection.kind;
    selectedIndex = selection.index;
    return true;
}

bool SelectRuntimePickupTab(std::vector<RuntimeToggle>& toggles, int& selectedIndex,
                            pickup_ui_logic::SelectionKind& kind, wchar_t* detail, std::size_t cap) {
    selectedIndex = -1;
    kind = pickup_ui_logic::SelectionKind::None;
    if (!EnumerateActiveToggles(toggles, detail, cap)) return false;
    std::vector<pickup_ui_logic::Candidate> candidates;
    candidates.reserve(toggles.size());
    for (const auto& toggle : toggles) candidates.push_back(toggle.candidate);
    const auto selection = pickup_ui_logic::SelectPickupTab(candidates);
    kind = selection.kind;
    selectedIndex = selection.index;
    return true;
}

enum class PickupTabEnsureStatus {
    Ready,
    Blocked,
    Error,
};

PickupTabEnsureStatus EnsurePickupTabSelected(wchar_t* detail, std::size_t cap) {
    std::vector<RuntimeToggle> toggles;
    int index = -1;
    pickup_ui_logic::SelectionKind kind{};
    if (!SelectRuntimePickupTab(toggles, index, kind, detail, cap))
        return PickupTabEnsureStatus::Error;

    if (kind != pickup_ui_logic::SelectionKind::Unique || index < 0 ||
        static_cast<std::size_t>(index) >= toggles.size()) {
        SetText(detail, cap, kind == pickup_ui_logic::SelectionKind::Ambiguous
            ? L"AUTO TAB BLOCKED: có nhiều TogglePickUpTab/Nhặt đồ; fail-closed"
            : L"AUTO TAB BLOCKED: chưa tìm thấy TogglePickUpTab/Nhặt đồ; hãy mở Thiết Lập AUTO");
        for (const auto& toggle : toggles) {
            if (pickup_ui_logic::ScorePickupTabCandidate(toggle.candidate) > 0)
                AppendToggleDiagnostic(detail, cap, toggle);
        }
        return PickupTabEnsureStatus::Blocked;
    }

    RuntimeToggle& tab = toggles[static_cast<std::size_t>(index)];
    const bool selected = tab.candidate.selected == 1;
    const auto route = pickup_ui_logic::ChoosePickupTabMutationRoute(
        selected, tab.interactable, tab.hasSetSelected, tab.hasSelectHandler);

    if (route == pickup_ui_logic::TabMutationRoute::Noop) {
        SetText(detail, cap, L"AUTO TAB: Nhặt đồ already selected; read-back=1");
        AppendToggleDiagnostic(detail, cap, tab);
        return PickupTabEnsureStatus::Ready;
    }
    if (route == pickup_ui_logic::TabMutationRoute::Blocked) {
        SetText(detail, cap, L"AUTO TAB BLOCKED: TogglePickUpTab không interactable hoặc thiếu route chọn nội bộ");
        AppendToggleDiagnostic(detail, cap, tab);
        return PickupTabEnsureStatus::Blocked;
    }

    std::uint8_t yes = 1;
    void* args[] = {&yes};
    const MethodInfo* method = nullptr;
    const wchar_t* routeName = nullptr;
    if (route == pickup_ui_logic::TabMutationRoute::SetSelected) {
        method = ExactMethod(tab.klass, "set_Selected", 1, false, "System.Boolean");
        routeName = L"set_Selected(true)";
    } else {
        method = ExactMethod(tab.klass, "HandleSelectEvent", 1, false, "System.Boolean");
        routeName = L"HandleSelectEvent(true)";
    }

    if (!method || !InvokeVoid(method, tab.object, args)) {
        SetText(detail, cap, L"AUTO TAB BLOCKED: invoke route chọn Nhặt đồ thất bại; không thử route thứ hai");
        AppendToggleDiagnostic(detail, cap, tab);
        return PickupTabEnsureStatus::Blocked;
    }

    bool readBack = false;
    if (!ReadToggleBool(tab.object, tab.klass, "get_Selected", readBack) || !readBack) {
        SetText(detail, cap, L"AUTO TAB BLOCKED: đã chọn Nhặt đồ nhưng get_Selected read-back chưa =1; không thử lần hai");
        AppendToggleDiagnostic(detail, cap, tab);
        return PickupTabEnsureStatus::Blocked;
    }

    SetText(detail, cap, L"AUTO TAB: ");
    AppendText(detail, cap, routeName);
    AppendText(detail, cap, L"; get_Selected read-back=1");
    AppendToggleDiagnostic(detail, cap, tab);
    return PickupTabEnsureStatus::Ready;
}
'''
if old_select not in s:
    raise SystemExit("SelectRuntimePickup pattern not found")
s = s.replace(old_select, new_select, 1)

probe_start = s.find("bool ProbePickupRuntime(Response& response, wchar_t* detail, std::size_t cap) {")
probe_end = s.find("\nbool EnsureMapping()", probe_start)
if probe_start < 0 or probe_end < 0:
    raise SystemExit("Probe/Ensure block boundaries not found")

new_probe_ensure = r'''bool ProbePickupRuntime(Response& response, wchar_t* detail, std::size_t cap) {
    PopulatePersistedSnapshot(response);
    response.ok = 1;
    response.runtimePickupState = -1;
    response.mutationAvailable = 0;
    response.resultCode = static_cast<std::int32_t>(ResultCode::RuntimeUnknown);

    wchar_t tabDetail[512]{};
    const PickupTabEnsureStatus tabStatus = EnsurePickupTabSelected(tabDetail, _countof(tabDetail));
    if (tabStatus == PickupTabEnsureStatus::Error) {
        SetText(detail, cap, tabDetail);
        return false;
    }
    if (tabStatus == PickupTabEnsureStatus::Blocked) {
        SetText(detail, cap, tabDetail);
        return true;
    }

    // Re-enumerate only after the tab proof. TabPickUp content controls can be
    // inactive while another AUTO tab is selected.
    std::vector<RuntimeToggle> toggles;
    int index = -1;
    pickup_ui_logic::SelectionKind kind{};
    if (!SelectRuntimePickup(toggles, index, kind, detail, cap)) return false;

    if (kind == pickup_ui_logic::SelectionKind::None) {
        SetText(detail, cap, tabDetail);
        AppendText(detail, cap, L" | Nhặt đồ Selected=1 nhưng TogPickUpEquipment chưa active; không thử callback tab lần hai. Active toggles=");
        AppendInt(detail, cap, static_cast<int>(toggles.size()));
        const std::size_t show = std::min<std::size_t>(toggles.size(), 4);
        for (std::size_t i = 0; i < show; ++i) AppendToggleDiagnostic(detail, cap, toggles[i]);
        return true;
    }
    if (kind == pickup_ui_logic::SelectionKind::Ambiguous || index < 0 ||
        static_cast<std::size_t>(index) >= toggles.size()) {
        SetText(detail, cap, tabDetail);
        AppendText(detail, cap, L" | Có nhiều UIToggle Nhặt vật phẩm trong TabPickUp; fail-closed.");
        for (const auto& toggle : toggles) {
            if (pickup_ui_logic::ScorePickupCandidate(toggle.candidate) > 0)
                AppendToggleDiagnostic(detail, cap, toggle);
        }
        return true;
    }

    RuntimeToggle& toggle = toggles[static_cast<std::size_t>(index)];
    response.runtimePickupState = toggle.candidate.selected;
    response.mutationAvailable = (toggle.interactable && toggle.hasSelectHandler) ? 1 : 0;
    response.resultCode = static_cast<std::int32_t>(ResultCode::Ok);
    SetText(detail, cap, tabDetail);
    AppendText(detail, cap, L" | UIToggle Nhặt vật phẩm UNIQUE; runtime=");
    AppendInt(detail, cap, response.runtimePickupState);
    AppendText(detail, cap, L" interactable=");
    AppendInt(detail, cap, toggle.interactable ? 1 : 0);
    AppendText(detail, cap, L" HandleSelectEvent=");
    AppendInt(detail, cap, toggle.hasSelectHandler ? 1 : 0);
    AppendToggleDiagnostic(detail, cap, toggle);
    return true;
}

bool EnsurePickupOn(Response& response, wchar_t* detail, std::size_t cap) {
    PopulatePersistedSnapshot(response);
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

    // Re-snapshot after selecting the navigation row so the item toggle must be
    // proven from the currently active TabPickUp content, never from stale UI.
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
            : L" | BLOCKED: Nhặt đồ Selected=1 nhưng chưa thấy TogPickUpEquipment; không thử callback tab lần hai");
        return true;
    }

    RuntimeToggle& toggle = toggles[static_cast<std::size_t>(index)];
    bool selected = toggle.candidate.selected == 1;
    response.runtimePickupState = selected ? 1 : 0;
    response.mutationAvailable = (toggle.interactable && toggle.hasSelectHandler) ? 1 : 0;

    if (selected) {
        response.ok = 1;
        response.resultCode = static_cast<std::int32_t>(ResultCode::Ok);
        SetText(detail, cap, tabDetail);
        AppendText(detail, cap, L" | Nhặt vật phẩm đã ON; no-op, read-back=ON");
        PopulatePersistedSnapshot(response);
        return true;
    }

    if (!toggle.interactable || !toggle.hasSelectHandler) {
        response.ok = 0;
        response.resultCode = static_cast<std::int32_t>(ResultCode::MutationBlocked);
        response.mutationAvailable = 0;
        SetText(detail, cap, tabDetail);
        AppendText(detail, cap, L" | BLOCKED: TogPickUpEquipment đọc được nhưng thiếu HandleSelectEvent(bool) interactable đã chứng minh");
        AppendToggleDiagnostic(detail, cap, toggle);
        return true;
    }

    const MethodInfo* selectEvent = ExactMethod(toggle.klass, "HandleSelectEvent", 1, false, "System.Boolean");
    std::uint8_t yes = 1;
    void* args[] = {&yes};
    if (!selectEvent || !InvokeVoid(selectEvent, toggle.object, args)) {
        response.ok = 0;
        response.resultCode = static_cast<std::int32_t>(ResultCode::MutationBlocked);
        response.mutationAvailable = 0;
        SetText(detail, cap, tabDetail);
        AppendText(detail, cap, L" | BLOCKED: HandleSelectEvent(true) của TogPickUpEquipment ném lỗi/không invoke được");
        return true;
    }

    bool readBack = false;
    if (!ReadToggleBool(toggle.object, toggle.klass, "get_Selected", readBack) || !readBack) {
        response.ok = 0;
        response.resultCode = static_cast<std::int32_t>(ResultCode::MutationBlocked);
        response.runtimePickupState = readBack ? 1 : 0;
        response.mutationAvailable = 0;
        SetText(detail, cap, tabDetail);
        AppendText(detail, cap, L" | BLOCKED: đã dispatch HandleSelectEvent(true) nhưng runtime read-back chưa ON; không thử ghi lần hai");
        PopulatePersistedSnapshot(response);
        return true;
    }

    response.ok = 1;
    response.resultCode = static_cast<std::int32_t>(ResultCode::Ok);
    response.runtimePickupState = 1;
    response.mutationAvailable = 1;
    PopulatePersistedSnapshot(response);
    SetText(detail, cap, tabDetail);
    AppendText(detail, cap, L" | ENSURE PASS: TogPickUpEquipment HandleSelectEvent(true) + get_Selected read-back=ON; AutoSettings đã re-read");
    return true;
}
'''

s = s[:probe_start] + new_probe_ensure + s[probe_end:]
p.write_text(s, encoding="utf-8")
print("patched", p, "lines=", len(s.splitlines()))
