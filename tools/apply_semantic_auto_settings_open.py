from pathlib import Path


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly one match, got {count}")
    return text.replace(old, new, 1)

# ---- bridge.cpp ----
p = Path("src/bridge.cpp")
s = p.read_text(encoding="utf-8-sig")
s = replace_once(
    s,
    '#include "auto_settings_parser.h"\n#include "pickup_ui_logic.h"',
    '#include "auto_settings_parser.h"\n#include "auto_menu_ui_logic.h"\n#include "pickup_ui_logic.h"',
    "bridge include",
)

anchor = '''bool SelectRuntimeSaveSettingsButton(std::vector<RuntimeButton>& buttons, int& selectedIndex,
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

insert = r'''

auto_menu_ui_logic::Candidate ToAutoMenuCandidate(const pickup_ui_logic::Candidate& c) {
    return {c.name, c.text, c.descendants, c.ancestors};
}

bool ProbeAutoFightPanelOpen(bool& open, wchar_t* detail, std::size_t cap) {
    open = false;
    std::vector<RuntimeToggle> toggles;
    if (!EnumerateActiveToggles(toggles, detail, cap)) return false;
    std::vector<auto_menu_ui_logic::Candidate> candidates;
    candidates.reserve(toggles.size());
    for (const auto& toggle : toggles) candidates.push_back(ToAutoMenuCandidate(toggle.candidate));
    const auto selection = auto_menu_ui_logic::SelectAutoFightPanelProof(candidates);
    if (selection.kind == auto_menu_ui_logic::SelectionKind::Ambiguous) {
        SetText(detail, cap, L"AUTO SETTINGS PROOF BLOCKED: nhiều TogglePickUpTab trong AutoFightUI");
        for (const auto& toggle : toggles) {
            if (auto_menu_ui_logic::IsAutoFightPanelProof(ToAutoMenuCandidate(toggle.candidate)))
                AppendToggleDiagnostic(detail, cap, toggle);
        }
        return true;
    }
    open = selection.kind == auto_menu_ui_logic::SelectionKind::Unique;
    return true;
}

enum class SemanticControlKind { None, Button, Toggle };

struct SemanticControlSelection {
    auto_menu_ui_logic::SelectionKind kind = auto_menu_ui_logic::SelectionKind::None;
    SemanticControlKind controlKind = SemanticControlKind::None;
    int index = -1;
};

template <typename Selector>
bool SelectSemanticControl(std::vector<RuntimeButton>& buttons,
                           std::vector<RuntimeToggle>& toggles,
                           Selector selector,
                           SemanticControlSelection& out,
                           wchar_t* detail, std::size_t cap) {
    out = {};
    if (!EnumerateActiveButtons(buttons, detail, cap)) return false;
    if (!EnumerateActiveToggles(toggles, detail, cap)) return false;

    std::vector<auto_menu_ui_logic::Candidate> buttonCandidates;
    buttonCandidates.reserve(buttons.size());
    for (const auto& button : buttons) buttonCandidates.push_back(ToAutoMenuCandidate(button.candidate));
    std::vector<auto_menu_ui_logic::Candidate> toggleCandidates;
    toggleCandidates.reserve(toggles.size());
    for (const auto& toggle : toggles) toggleCandidates.push_back(ToAutoMenuCandidate(toggle.candidate));

    const auto b = selector(buttonCandidates);
    const auto t = selector(toggleCandidates);
    if (b.kind == auto_menu_ui_logic::SelectionKind::Ambiguous ||
        t.kind == auto_menu_ui_logic::SelectionKind::Ambiguous ||
        (b.kind == auto_menu_ui_logic::SelectionKind::Unique &&
         t.kind == auto_menu_ui_logic::SelectionKind::Unique)) {
        out.kind = auto_menu_ui_logic::SelectionKind::Ambiguous;
        return true;
    }
    if (b.kind == auto_menu_ui_logic::SelectionKind::Unique) {
        out.kind = b.kind;
        out.controlKind = SemanticControlKind::Button;
        out.index = b.index;
        return true;
    }
    if (t.kind == auto_menu_ui_logic::SelectionKind::Unique) {
        out.kind = t.kind;
        out.controlKind = SemanticControlKind::Toggle;
        out.index = t.index;
        return true;
    }
    return true;
}

bool InvokeSemanticButton(RuntimeButton& button, const wchar_t* label,
                          wchar_t* detail, std::size_t cap) {
    if (!button.interactable || !button.hasClickHandler) {
        SetText(detail, cap, label);
        AppendText(detail, cap, L" BLOCKED: UIButton không interactable hoặc thiếu HandleClickEvent()");
        AppendButtonDiagnostic(detail, cap, button);
        return false;
    }
    const MethodInfo* click = ExactMethod(button.klass, "HandleClickEvent", 0, false);
    if (!click || !InvokeVoid(click, button.object, nullptr)) {
        SetText(detail, cap, label);
        AppendText(detail, cap, L" BLOCKED: HandleClickEvent() thất bại; không click lần hai");
        AppendButtonDiagnostic(detail, cap, button);
        return false;
    }
    SetText(detail, cap, label);
    AppendText(detail, cap, L" DISPATCH: UIButton.HandleClickEvent()");
    AppendButtonDiagnostic(detail, cap, button);
    return true;
}

bool InvokeSemanticToggle(RuntimeToggle& toggle, const wchar_t* label,
                          wchar_t* detail, std::size_t cap) {
    if (!toggle.interactable) {
        SetText(detail, cap, label);
        AppendText(detail, cap, L" BLOCKED: UIToggle không interactable");
        AppendToggleDiagnostic(detail, cap, toggle);
        return false;
    }
    if (toggle.candidate.selected == 1) {
        SetText(detail, cap, label);
        AppendText(detail, cap, L" NO-OP: UIToggle đã selected=1");
        AppendToggleDiagnostic(detail, cap, toggle);
        return true;
    }
    const MethodInfo* method = nullptr;
    const wchar_t* route = nullptr;
    if (toggle.hasSetSelected) {
        method = ExactMethod(toggle.klass, "set_Selected", 1, false, "System.Boolean");
        route = L"set_Selected(true)";
    } else if (toggle.hasSelectHandler) {
        method = ExactMethod(toggle.klass, "HandleSelectEvent", 1, false, "System.Boolean");
        route = L"HandleSelectEvent(true)";
    }
    if (!method) {
        SetText(detail, cap, label);
        AppendText(detail, cap, L" BLOCKED: UIToggle thiếu semantic route");
        AppendToggleDiagnostic(detail, cap, toggle);
        return false;
    }
    std::uint8_t yes = 1;
    void* args[] = {&yes};
    if (!InvokeVoid(method, toggle.object, args)) {
        SetText(detail, cap, label);
        AppendText(detail, cap, L" BLOCKED: invoke UIToggle thất bại; không thử route thứ hai");
        AppendToggleDiagnostic(detail, cap, toggle);
        return false;
    }
    bool readBack = false;
    if (!ReadToggleBool(toggle.object, toggle.klass, "get_Selected", readBack) || !readBack) {
        SetText(detail, cap, label);
        AppendText(detail, cap, L" BLOCKED: runtime read-back chưa selected=1");
        AppendToggleDiagnostic(detail, cap, toggle);
        return false;
    }
    SetText(detail, cap, label);
    AppendText(detail, cap, L" DISPATCH: UIToggle ");
    AppendText(detail, cap, route);
    AppendText(detail, cap, L" + read-back=1");
    AppendToggleDiagnostic(detail, cap, toggle);
    return true;
}

bool OpenAutoMenuSemantic(Response& response, wchar_t* detail, std::size_t cap) {
    response.ok = 0;
    response.resultCode = static_cast<std::int32_t>(ResultCode::MutationBlocked);
    bool panelOpen = false;
    if (!ProbeAutoFightPanelOpen(panelOpen, detail, cap)) return false;
    if (panelOpen) {
        response.ok = 1;
        response.resultCode = static_cast<std::int32_t>(ResultCode::Ok);
        SetText(detail, cap, L"AUTO STEP1 PASS: AutoFightUI đã mở; không bấm AUTO lại");
        return true;
    }

    std::vector<RuntimeButton> buttons;
    std::vector<RuntimeToggle> toggles;
    SemanticControlSelection selected{};
    if (!SelectSemanticControl(buttons, toggles, auto_menu_ui_logic::SelectHudAuto,
                               selected, detail, cap)) return false;
    if (selected.kind != auto_menu_ui_logic::SelectionKind::Unique || selected.index < 0) {
        SetText(detail, cap, selected.kind == auto_menu_ui_logic::SelectionKind::Ambiguous
            ? L"AUTO STEP1 BLOCKED: có nhiều semantic control mang nhãn AUTO"
            : L"AUTO STEP1 BLOCKED: chưa tìm thấy semantic control AUTO ngoài HUD");
        for (const auto& button : buttons) {
            if (auto_menu_ui_logic::IsHudAuto(ToAutoMenuCandidate(button.candidate)))
                AppendButtonDiagnostic(detail, cap, button);
        }
        for (const auto& toggle : toggles) {
            if (auto_menu_ui_logic::IsHudAuto(ToAutoMenuCandidate(toggle.candidate)))
                AppendToggleDiagnostic(detail, cap, toggle);
        }
        return true;
    }

    bool invoked = false;
    if (selected.controlKind == SemanticControlKind::Button &&
        static_cast<std::size_t>(selected.index) < buttons.size()) {
        invoked = InvokeSemanticButton(buttons[static_cast<std::size_t>(selected.index)], L"AUTO STEP1", detail, cap);
    } else if (selected.controlKind == SemanticControlKind::Toggle &&
               static_cast<std::size_t>(selected.index) < toggles.size()) {
        invoked = InvokeSemanticToggle(toggles[static_cast<std::size_t>(selected.index)], L"AUTO STEP1", detail, cap);
    }
    if (!invoked) return true;
    response.ok = 1;
    response.resultCode = static_cast<std::int32_t>(ResultCode::Ok);
    return true;
}

bool ChooseAutoSettingsSemantic(Response& response, wchar_t* detail, std::size_t cap) {
    response.ok = 0;
    response.resultCode = static_cast<std::int32_t>(ResultCode::MutationBlocked);
    bool panelOpen = false;
    if (!ProbeAutoFightPanelOpen(panelOpen, detail, cap)) return false;
    if (panelOpen) {
        response.ok = 1;
        response.resultCode = static_cast<std::int32_t>(ResultCode::Ok);
        SetText(detail, cap, L"AUTO STEP2 PASS: AutoFightUI đã mở; proof TogglePickUpTab UNIQUE");
        return true;
    }

    std::vector<RuntimeButton> buttons;
    std::vector<RuntimeToggle> toggles;
    SemanticControlSelection selected{};
    if (!SelectSemanticControl(buttons, toggles, auto_menu_ui_logic::SelectSettingsChoice,
                               selected, detail, cap)) return false;
    if (selected.kind != auto_menu_ui_logic::SelectionKind::Unique || selected.index < 0) {
        SetText(detail, cap, selected.kind == auto_menu_ui_logic::SelectionKind::Ambiguous
            ? L"AUTO STEP2 BLOCKED: có nhiều semantic control 'Thiết lập'"
            : L"AUTO STEP2 BLOCKED: menu AUTO đã mở nhưng chưa tìm thấy 'Thiết lập'");
        for (const auto& button : buttons) {
            if (auto_menu_ui_logic::IsSettingsChoice(ToAutoMenuCandidate(button.candidate)))
                AppendButtonDiagnostic(detail, cap, button);
        }
        for (const auto& toggle : toggles) {
            if (auto_menu_ui_logic::IsSettingsChoice(ToAutoMenuCandidate(toggle.candidate)))
                AppendToggleDiagnostic(detail, cap, toggle);
        }
        return true;
    }

    wchar_t invokeDetail[512]{};
    bool invoked = false;
    if (selected.controlKind == SemanticControlKind::Button &&
        static_cast<std::size_t>(selected.index) < buttons.size()) {
        invoked = InvokeSemanticButton(buttons[static_cast<std::size_t>(selected.index)], L"AUTO STEP2", invokeDetail, _countof(invokeDetail));
    } else if (selected.controlKind == SemanticControlKind::Toggle &&
               static_cast<std::size_t>(selected.index) < toggles.size()) {
        invoked = InvokeSemanticToggle(toggles[static_cast<std::size_t>(selected.index)], L"AUTO STEP2", invokeDetail, _countof(invokeDetail));
    }
    if (!invoked) {
        SetText(detail, cap, invokeDetail);
        return true;
    }

    bool proof = false;
    wchar_t proofDetail[512]{};
    if (!ProbeAutoFightPanelOpen(proof, proofDetail, _countof(proofDetail))) return false;
    if (!proof) {
        SetText(detail, cap, invokeDetail);
        AppendText(detail, cap, L" | BLOCKED: đã dispatch Thiết lập nhưng AutoFightUI proof chưa xuất hiện; không click lần hai");
        return true;
    }

    response.ok = 1;
    response.resultCode = static_cast<std::int32_t>(ResultCode::Ok);
    SetText(detail, cap, invokeDetail);
    AppendText(detail, cap, L" | OPEN PASS: AutoFightUI active + TogglePickUpTab UNIQUE");
    return true;
}
'''

s = replace_once(s, anchor, anchor + insert, "bridge semantic helpers")

old_dispatch = '''    } else if (command == Command::EnsurePickupOn) {
        const bool transportOk = EnsurePickupOn(g_shared->response,
                                                g_shared->response.detail, kDetailCapacity);
        if (!transportOk) {
            g_shared->response.ok = 0;
            g_shared->response.resultCode = static_cast<std::int32_t>(ResultCode::MutationBlocked);
            g_shared->response.runtimePickupState = -1;
            g_shared->response.mutationAvailable = 0;
        }
    } else {'''
new_dispatch = '''    } else if (command == Command::EnsurePickupOn) {
        const bool transportOk = EnsurePickupOn(g_shared->response,
                                                g_shared->response.detail, kDetailCapacity);
        if (!transportOk) {
            g_shared->response.ok = 0;
            g_shared->response.resultCode = static_cast<std::int32_t>(ResultCode::MutationBlocked);
            g_shared->response.runtimePickupState = -1;
            g_shared->response.mutationAvailable = 0;
        }
    } else if (command == Command::OpenAutoMenuSemantic) {
        const bool transportOk = OpenAutoMenuSemantic(g_shared->response,
                                                      g_shared->response.detail, kDetailCapacity);
        if (!transportOk) {
            g_shared->response.ok = 0;
            g_shared->response.resultCode = static_cast<std::int32_t>(ResultCode::MutationBlocked);
        }
    } else if (command == Command::ChooseAutoSettingsSemantic) {
        const bool transportOk = ChooseAutoSettingsSemantic(g_shared->response,
                                                            g_shared->response.detail, kDetailCapacity);
        if (!transportOk) {
            g_shared->response.ok = 0;
            g_shared->response.resultCode = static_cast<std::int32_t>(ResultCode::MutationBlocked);
        }
    } else {'''
s = replace_once(s, old_dispatch, new_dispatch, "bridge command dispatch")
p.write_text(s, encoding="utf-8")

# ---- main.cpp ----
p = Path("src/main.cpp")
s = p.read_text(encoding="utf-8-sig")
s = replace_once(s, 'constexpr int IDC_LOG = 1008;', 'constexpr int IDC_LOG = 1008;\nconstexpr int IDC_OPEN_AUTO_SETTINGS = 1009;', 'main id')
s = replace_once(s, 'HWND g_log = nullptr;', 'HWND g_log = nullptr;\nHWND g_openAutoSettingsButton = nullptr;', 'main global')

anchor_func = '''void Layout(HWND hwnd) {'''
new_func = r'''void DoOpenAutoSettingsSemantic() {
    if (!EnsureSession()) return;
    Response step1{};
    std::wstring error;
    if (!g_session.Send(Command::OpenAutoMenuSemantic, step1, error)) {
        AppendLog(L"AUTO→THIẾT LẬP STEP1 FAIL: " + error);
        return;
    }
    AppendLog(std::wstring(step1.ok ? L"AUTO→THIẾT LẬP STEP1 PASS: " : L"AUTO→THIẾT LẬP STEP1 BLOCKED: ") + step1.detail);
    if (!step1.ok) return;

    // Wait in the controller only. Never sleep inside the injected bridge/game thread.
    Sleep(500);

    Response step2{};
    if (!g_session.Send(Command::ChooseAutoSettingsSemantic, step2, error)) {
        AppendLog(L"AUTO→THIẾT LẬP STEP2 FAIL: " + error);
        return;
    }
    AppendLog(std::wstring(step2.ok ? L"AUTO→THIẾT LẬP OPEN PASS: " : L"AUTO→THIẾT LẬP STEP2 BLOCKED: ") + step2.detail);
}

void Layout(HWND hwnd) {'''
s = replace_once(s, anchor_func, new_func, "main function")

s = replace_once(
    s,
    '    MoveWindow(g_runtime, 430, 52, w - 442, 24, TRUE);\n    const int logH = 130;\n    MoveWindow(g_output, 12, 84, w - 24, h - 84 - logH - 20, TRUE);',
    '    MoveWindow(g_runtime, 430, 52, w - 442, 24, TRUE);\n    MoveWindow(g_openAutoSettingsButton, 12, 80, 270, 28, TRUE);\n    const int logH = 130;\n    MoveWindow(g_output, 12, 116, w - 24, h - 116 - logH - 20, TRUE);',
    "main layout",
)

s = replace_once(
    s,
    '''            g_enableButton = CreateWindowW(L"BUTTON", L"BẬT NHẶT VẬT PHẨM", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                           0,0,0,0, hwnd, reinterpret_cast<HMENU>(IDC_ENABLE), nullptr, nullptr);
            g_runtime = CreateWindowW(L"STATIC", L"Runtime Nhặt vật phẩm: CHƯA XÁC ĐỊNH", WS_CHILD | WS_VISIBLE,''',
    '''            g_enableButton = CreateWindowW(L"BUTTON", L"BẬT NHẶT VẬT PHẨM", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                           0,0,0,0, hwnd, reinterpret_cast<HMENU>(IDC_ENABLE), nullptr, nullptr);
            g_openAutoSettingsButton = CreateWindowW(L"BUTTON", L"TEST AUTO → THIẾT LẬP", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                           0,0,0,0, hwnd, reinterpret_cast<HMENU>(IDC_OPEN_AUTO_SETTINGS), nullptr, nullptr);
            g_runtime = CreateWindowW(L"STATIC", L"Runtime Nhặt vật phẩm: CHƯA XÁC ĐỊNH", WS_CHILD | WS_VISIBLE,''',
    "main create button",
)

s = replace_once(
    s,
    '            else if (id == IDC_ENABLE && HIWORD(wParam) == BN_CLICKED) DoEnsurePickup();\n            else if (id == IDC_GAME && HIWORD(wParam) == CBN_SELCHANGE) {',
    '            else if (id == IDC_ENABLE && HIWORD(wParam) == BN_CLICKED) DoEnsurePickup();\n            else if (id == IDC_OPEN_AUTO_SETTINGS && HIWORD(wParam) == BN_CLICKED) DoOpenAutoSettingsSemantic();\n            else if (id == IDC_GAME && HIWORD(wParam) == CBN_SELCHANGE) {',
    "main command handler",
)

s = replace_once(s, L := 'L"Thần Long - Auto Settings Probe v0.2"', 'L"Thần Long - Auto Settings Probe v0.3 Semantic Open"', "main title")
p.write_text(s, encoding="utf-8")
