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
    '#include <algorithm>\n#include <cstddef>',
    '#include <algorithm>\n#include <cmath>\n#include <cstddef>',
    "bridge cmath include",
)

s = replace_once(
    s,
    '''void AppendInt(wchar_t* out, std::size_t cap, int value) {\n    wchar_t buffer[32]{};\n    swprintf_s(buffer, _countof(buffer), L"%d", value);\n    AppendText(out, cap, buffer);\n}\n''',
    '''void AppendInt(wchar_t* out, std::size_t cap, int value) {\n    wchar_t buffer[32]{};\n    swprintf_s(buffer, _countof(buffer), L"%d", value);\n    AppendText(out, cap, buffer);\n}\n\nvoid AppendFloat(wchar_t* out, std::size_t cap, float value) {\n    wchar_t buffer[32]{};\n    swprintf_s(buffer, _countof(buffer), L"%.3f", static_cast<double>(value));\n    AppendText(out, cap, buffer);\n}\n''',
    "bridge AppendFloat",
)

s = replace_once(
    s,
    '''const Il2CppImage* AssemblyCSharp() {\n    Il2CppDomain* domain = g_api.domain_get ? g_api.domain_get() : nullptr;\n    if (!domain) return nullptr;\n    const Il2CppAssembly* assembly = g_api.domain_assembly_open(domain, "Assembly-CSharp");\n    if (!assembly) assembly = g_api.domain_assembly_open(domain, "Assembly-CSharp.dll");\n    return assembly ? g_api.assembly_get_image(assembly) : nullptr;\n}\n''',
    '''const Il2CppImage* AssemblyCSharp() {\n    Il2CppDomain* domain = g_api.domain_get ? g_api.domain_get() : nullptr;\n    if (!domain) return nullptr;\n    const Il2CppAssembly* assembly = g_api.domain_assembly_open(domain, "Assembly-CSharp");\n    if (!assembly) assembly = g_api.domain_assembly_open(domain, "Assembly-CSharp.dll");\n    return assembly ? g_api.assembly_get_image(assembly) : nullptr;\n}\n\nconst Il2CppImage* UnityCore() {\n    Il2CppDomain* domain = g_api.domain_get ? g_api.domain_get() : nullptr;\n    if (!domain) return nullptr;\n    const Il2CppAssembly* assembly = g_api.domain_assembly_open(domain, "UnityEngine.CoreModule");\n    if (!assembly) assembly = g_api.domain_assembly_open(domain, "UnityEngine.CoreModule.dll");\n    return assembly ? g_api.assembly_get_image(assembly) : nullptr;\n}\n''',
    "bridge UnityCore",
)

s = replace_once(
    s,
    '''bool InvokeBool(const MethodInfo* method, void* instance, bool& out) {\n    out = false;\n    if (!method) return false;\n    void* exc = nullptr;\n    Il2CppObject* boxed = g_api.runtime_invoke(method, instance, nullptr, &exc);\n    if (exc || !boxed) return false;\n    const Il2CppType* type = g_api.method_get_return_type(method);\n    char* name = type ? g_api.type_get_name(type) : nullptr;\n    if (!name) return false;\n    const bool typeOk = Eq(name, "System.Boolean");\n    g_api.free_fn(name);\n    if (!typeOk) return false;\n    void* raw = g_api.object_unbox(boxed);\n    if (!raw) return false;\n    out = *reinterpret_cast<const std::uint8_t*>(raw) != 0;\n    return true;\n}\n''',
    '''bool InvokeBool(const MethodInfo* method, void* instance, bool& out) {\n    out = false;\n    if (!method) return false;\n    void* exc = nullptr;\n    Il2CppObject* boxed = g_api.runtime_invoke(method, instance, nullptr, &exc);\n    if (exc || !boxed) return false;\n    const Il2CppType* type = g_api.method_get_return_type(method);\n    char* name = type ? g_api.type_get_name(type) : nullptr;\n    if (!name) return false;\n    const bool typeOk = Eq(name, "System.Boolean");\n    g_api.free_fn(name);\n    if (!typeOk) return false;\n    void* raw = g_api.object_unbox(boxed);\n    if (!raw) return false;\n    out = *reinterpret_cast<const std::uint8_t*>(raw) != 0;\n    return true;\n}\n\nbool InvokeInt32(const MethodInfo* method, void* instance, std::int32_t& out) {\n    out = 0;\n    if (!method) return false;\n    void* exc = nullptr;\n    Il2CppObject* boxed = g_api.runtime_invoke(method, instance, nullptr, &exc);\n    if (exc || !boxed) return false;\n    const Il2CppType* type = g_api.method_get_return_type(method);\n    char* name = type ? g_api.type_get_name(type) : nullptr;\n    if (!name) return false;\n    const bool typeOk = Eq(name, "System.Int32");\n    g_api.free_fn(name);\n    if (!typeOk) return false;\n    void* raw = g_api.object_unbox(boxed);\n    if (!raw) return false;\n    out = *reinterpret_cast<const std::int32_t*>(raw);\n    return true;\n}\n\nstruct Float2 { float x = 0.0f; float y = 0.0f; };\nstruct Float3 { float x = 0.0f; float y = 0.0f; float z = 0.0f; };\n\nbool InvokeVector2(const MethodInfo* method, void* instance, void** args, Float2& out) {\n    if (!method) return false;\n    void* exc = nullptr;\n    Il2CppObject* boxed = g_api.runtime_invoke(method, instance, args, &exc);\n    if (exc || !boxed) return false;\n    const Il2CppType* type = g_api.method_get_return_type(method);\n    char* name = type ? g_api.type_get_name(type) : nullptr;\n    if (!name) return false;\n    const bool typeOk = Eq(name, "UnityEngine.Vector2");\n    g_api.free_fn(name);\n    if (!typeOk) return false;\n    void* raw = g_api.object_unbox(boxed);\n    if (!raw) return false;\n    out = *reinterpret_cast<const Float2*>(raw);\n    return true;\n}\n\nbool InvokeVector3(const MethodInfo* method, void* instance, Float3& out) {\n    if (!method) return false;\n    void* exc = nullptr;\n    Il2CppObject* boxed = g_api.runtime_invoke(method, instance, nullptr, &exc);\n    if (exc || !boxed) return false;\n    const Il2CppType* type = g_api.method_get_return_type(method);\n    char* name = type ? g_api.type_get_name(type) : nullptr;\n    if (!name) return false;\n    const bool typeOk = Eq(name, "UnityEngine.Vector3");\n    g_api.free_fn(name);\n    if (!typeOk) return false;\n    void* raw = g_api.object_unbox(boxed);\n    if (!raw) return false;\n    out = *reinterpret_cast<const Float3*>(raw);\n    return true;\n}\n''',
    "bridge numeric/vector invokes",
)

position_helpers = r'''

struct ScreenRuntime {
    bool attempted = false;
    Il2CppClass* screen = nullptr;
    Il2CppClass* rectTransformUtility = nullptr;
    const MethodInfo* getWidth = nullptr;
    const MethodInfo* getHeight = nullptr;
    const MethodInfo* worldToScreenPoint = nullptr;
};

ScreenRuntime g_screen;

bool EnsureScreenRuntime() {
    if (g_screen.attempted)
        return g_screen.screen && g_screen.getWidth && g_screen.getHeight;
    g_screen.attempted = true;
    const Il2CppImage* image = UnityCore();
    if (!image) return false;
    g_screen.screen = g_api.class_from_name(image, "UnityEngine", "Screen");
    g_screen.rectTransformUtility = g_api.class_from_name(image, "UnityEngine", "RectTransformUtility");
    if (!g_screen.screen) return false;
    g_screen.getWidth = ExactMethod(g_screen.screen, "get_width", 0, true);
    g_screen.getHeight = ExactMethod(g_screen.screen, "get_height", 0, true);
    if (g_screen.rectTransformUtility)
        g_screen.worldToScreenPoint = ExactMethod(g_screen.rectTransformUtility, "WorldToScreenPoint", 2, true);
    return g_screen.getWidth && g_screen.getHeight;
}

bool ReadScreenSize(std::int32_t& width, std::int32_t& height) {
    width = 0;
    height = 0;
    if (!EnsureScreenRuntime()) return false;
    return InvokeInt32(g_screen.getWidth, nullptr, width) &&
           InvokeInt32(g_screen.getHeight, nullptr, height) &&
           width > 0 && height > 0;
}

bool TryObjectGetterNames(Il2CppObject* object, Il2CppClass* klass,
                          const char* const* names, std::size_t count,
                          Il2CppObject*& out) {
    out = nullptr;
    for (std::size_t i = 0; i < count; ++i) {
        if (ObjectGetter(object, klass, names[i], out) && out) return true;
    }
    return false;
}

bool TryReadNormalizedScreenPosition(Il2CppObject* object, Il2CppClass* klass,
                                     float& normalizedX, float& normalizedY) {
    normalizedX = 0.0f;
    normalizedY = 0.0f;
    if (!object || !klass) return false;

    Il2CppObject* transform = nullptr;
    const char* directTransformGetters[] = {"get_transform", "get_Transform", "get_RectTransform"};
    (void)TryObjectGetterNames(object, klass, directTransformGetters,
                               _countof(directTransformGetters), transform);

    if (!transform) {
        Il2CppObject* gameObject = nullptr;
        const char* gameObjectGetters[] = {"get_gameObject", "get_GameObject", "get_CoreGameObject"};
        if (TryObjectGetterNames(object, klass, gameObjectGetters,
                                 _countof(gameObjectGetters), gameObject) && gameObject) {
            Il2CppClass* gameObjectClass = g_api.object_get_class(gameObject);
            const char* transformGetters[] = {"get_transform", "get_Transform"};
            if (gameObjectClass)
                (void)TryObjectGetterNames(gameObject, gameObjectClass, transformGetters,
                                           _countof(transformGetters), transform);
        }
    }
    if (!transform) return false;

    Il2CppClass* transformClass = g_api.object_get_class(transform);
    if (!transformClass) return false;
    const MethodInfo* getPosition = FindMethod(transformClass, "get_position", 0);
    if (!getPosition) getPosition = FindMethod(transformClass, "get_Position", 0);
    Float3 world{};
    if (!getPosition || !InvokeVector3(getPosition, transform, world)) return false;

    std::int32_t width = 0;
    std::int32_t height = 0;
    if (!ReadScreenSize(width, height)) return false;

    float screenX = world.x;
    float screenY = world.y;
    auto plausible = [width, height](float x, float y) {
        return std::isfinite(x) && std::isfinite(y) &&
               x >= -0.05f * width && x <= 1.05f * width &&
               y >= -0.05f * height && y <= 1.05f * height;
    };

    if (!plausible(screenX, screenY) && g_screen.worldToScreenPoint) {
        Il2CppObject* camera = nullptr;
        void* args[] = {&camera, &world};
        Float2 screen{};
        if (InvokeVector2(g_screen.worldToScreenPoint, nullptr, args, screen)) {
            screenX = screen.x;
            screenY = screen.y;
        }
    }
    if (!plausible(screenX, screenY)) return false;

    normalizedX = screenX / static_cast<float>(width);
    normalizedY = screenY / static_cast<float>(height);
    return std::isfinite(normalizedX) && std::isfinite(normalizedY);
}
'''

s = replace_once(
    s,
    '''bool ObjectGetter(Il2CppObject* object, Il2CppClass* klass, const char* getter, Il2CppObject*& out) {\n    out = nullptr;\n    const MethodInfo* method = FindMethod(klass, getter, 0);\n    return method && InvokeObject(method, object, out);\n}\n''',
    '''bool ObjectGetter(Il2CppObject* object, Il2CppClass* klass, const char* getter, Il2CppObject*& out) {\n    out = nullptr;\n    const MethodInfo* method = FindMethod(klass, getter, 0);\n    return method && InvokeObject(method, object, out);\n}\n''' + position_helpers,
    "bridge position helpers",
)

single_pass = r'''

struct SettingsSemanticMatch {
    SemanticControlKind controlKind = SemanticControlKind::None;
    RuntimeButton button{};
    RuntimeToggle toggle{};
    auto_menu_ui_logic::Candidate candidate{};
};

bool SettingsLabelFast(Il2CppObject* object, Il2CppClass* klass,
                       std::wstring& text, std::wstring& descendants) {
    text.clear();
    descendants.clear();
    (void)ReadStringMember(object, klass, "Text", text);
    if (auto_menu_ui_logic::Key(text) == L"thietlap") return true;
    CollectDescendantLabels(object, descendants);
    return auto_menu_ui_logic::ContainsExactSegment(descendants, L"thietlap");
}

bool SelectSettingsSemanticControlSinglePass(std::vector<SettingsSemanticMatch>& matches,
                                             int& selectedIndex,
                                             auto_menu_ui_logic::SelectionKind& kind,
                                             wchar_t* detail, std::size_t cap) {
    matches.clear();
    selectedIndex = -1;
    kind = auto_menu_ui_logic::SelectionKind::None;
    if (!EnsureUiDiscovery(detail, cap)) return false;

    Il2CppObject* dictionary = nullptr;
    g_api.field_static_get_value(g_ui.instances, &dictionary);
    Il2CppObject* entries = nullptr;
    std::int32_t count = 0;
    std::uintptr_t capacity = 0;
    if (!dictionary || !ReadLocal(dictionary, 0x18, entries) || !entries ||
        !ReadLocal(dictionary, 0x20, count) || count < 0 || count > 32768 ||
        !ReadLocal(entries, 0x18, capacity) || capacity > 32768) {
        SetText(detail, cap, L"UIObject.instances dictionary không hợp lệ khi quét Thiết lập");
        return false;
    }

    for (std::uintptr_t i = 0; i < capacity; ++i) {
        Il2CppObject* object = nullptr;
        const std::size_t entry = 0x20 + static_cast<std::size_t>(i) * 0x18;
        if (!ReadLocal(entries, entry + 0x10, object) || !object) continue;
        Il2CppClass* klass = g_api.object_get_class(object);
        if (!klass) continue;
        const bool isButton = g_api.class_is_assignable_from(g_ui.button, klass);
        const bool isToggle = g_api.class_is_assignable_from(g_ui.toggle, klass);
        if (!isButton && !isToggle) continue;

        bool active = false;
        if (!ReadToggleBool(object, klass, "get_ActiveInHierarchy", active) || !active) continue;

        std::wstring text;
        std::wstring descendants;
        if (!SettingsLabelFast(object, klass, text, descendants)) continue;

        SettingsSemanticMatch match{};
        match.candidate.text = text;
        match.candidate.descendants = descendants;
        (void)ReadStringMember(object, klass, "Name", match.candidate.name);
        CollectAncestorLabels(object, match.candidate.ancestors);
        if (auto_menu_ui_logic::InAutoFightUi(match.candidate)) continue;

        if (isButton) {
            match.controlKind = SemanticControlKind::Button;
            match.button.object = object;
            match.button.klass = klass;
            match.button.interactable = true;
            bool interactable = true;
            if (ReadToggleBool(object, klass, "get_Interactable", interactable))
                match.button.interactable = interactable;
            match.button.hasClickHandler = ExactMethod(klass, "HandleClickEvent", 0, false) != nullptr;
            match.button.candidate.name = match.candidate.name;
            match.button.candidate.text = match.candidate.text;
            match.button.candidate.descendants = match.candidate.descendants;
            match.button.candidate.ancestors = match.candidate.ancestors;
        } else {
            match.controlKind = SemanticControlKind::Toggle;
            match.toggle.object = object;
            match.toggle.klass = klass;
            bool interactable = false;
            (void)ReadToggleBool(object, klass, "get_Interactable", interactable);
            match.toggle.interactable = interactable;
            bool selected = false;
            if (!ReadToggleBool(object, klass, "get_Selected", selected)) continue;
            match.toggle.candidate.selected = selected ? 1 : 0;
            match.toggle.hasSetSelected = ExactMethod(klass, "set_Selected", 1, false, "System.Boolean") != nullptr;
            match.toggle.hasSelectHandler = ExactMethod(klass, "HandleSelectEvent", 1, false, "System.Boolean") != nullptr;
            match.toggle.candidate.name = match.candidate.name;
            match.toggle.candidate.text = match.candidate.text;
            match.toggle.candidate.descendants = match.candidate.descendants;
            match.toggle.candidate.ancestors = match.candidate.ancestors;
        }
        matches.push_back(std::move(match));
    }

    if (matches.empty()) return true;

    std::vector<auto_menu_ui_logic::Candidate> candidates;
    candidates.reserve(matches.size());
    for (const auto& match : matches) candidates.push_back(match.candidate);

    auto selection = auto_menu_ui_logic::SelectSettingsChoice(candidates);
    if (selection.kind == auto_menu_ui_logic::SelectionKind::Ambiguous) {
        // Only duplicate exact-label candidates pay the RectTransform/Screen cost.
        for (std::size_t i = 0; i < matches.size(); ++i) {
            float nx = 0.0f;
            float ny = 0.0f;
            Il2CppObject* object = matches[i].controlKind == SemanticControlKind::Button
                ? matches[i].button.object : matches[i].toggle.object;
            Il2CppClass* klass = matches[i].controlKind == SemanticControlKind::Button
                ? matches[i].button.klass : matches[i].toggle.klass;
            if (TryReadNormalizedScreenPosition(object, klass, nx, ny)) {
                matches[i].candidate.hasNormalizedPosition = true;
                matches[i].candidate.normalizedX = nx;
                matches[i].candidate.normalizedY = ny;
                candidates[i] = matches[i].candidate;
            }
        }
        selection = auto_menu_ui_logic::SelectSettingsChoiceSpatial(candidates, 0.55f);
    }

    kind = selection.kind;
    selectedIndex = selection.index;
    return true;
}

void AppendSettingsMatchDiagnostic(wchar_t* detail, std::size_t cap,
                                   const SettingsSemanticMatch& match) {
    AppendText(detail, cap, L" [N=");
    AppendText(detail, cap, match.candidate.name.c_str());
    AppendText(detail, cap, L" T=");
    AppendText(detail, cap, match.candidate.text.c_str());
    AppendText(detail, cap, L" A=");
    AppendText(detail, cap, match.candidate.ancestors.c_str());
    if (match.candidate.hasNormalizedPosition) {
        AppendText(detail, cap, L" NX=");
        AppendFloat(detail, cap, match.candidate.normalizedX);
        AppendText(detail, cap, L" NY=");
        AppendFloat(detail, cap, match.candidate.normalizedY);
    } else {
        AppendText(detail, cap, L" POS=?");
    }
    AppendText(detail, cap, L"]");
}
'''

s = replace_once(
    s,
    '''struct SemanticControlSelection {\n    auto_menu_ui_logic::SelectionKind kind = auto_menu_ui_logic::SelectionKind::None;\n    SemanticControlKind controlKind = SemanticControlKind::None;\n    int index = -1;\n};\n''',
    '''struct SemanticControlSelection {\n    auto_menu_ui_logic::SelectionKind kind = auto_menu_ui_logic::SelectionKind::None;\n    SemanticControlKind controlKind = SemanticControlKind::None;\n    int index = -1;\n};\n''' + single_pass,
    "bridge single-pass settings selector",
)

old_choose = r'''    std::vector<RuntimeButton> buttons;
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
'''

new_choose = r'''    std::vector<SettingsSemanticMatch> matches;
    int settingsIndex = -1;
    auto_menu_ui_logic::SelectionKind settingsKind{};
    if (!SelectSettingsSemanticControlSinglePass(matches, settingsIndex, settingsKind,
                                                  detail, cap)) return false;
    if (settingsKind != auto_menu_ui_logic::SelectionKind::Unique || settingsIndex < 0 ||
        static_cast<std::size_t>(settingsIndex) >= matches.size()) {
        SetText(detail, cap, settingsKind == auto_menu_ui_logic::SelectionKind::Ambiguous
            ? L"AUTO STEP2 BLOCKED: nhiều 'Thiết lập'; upper-region tie-break chưa UNIQUE"
            : L"AUTO STEP2 BLOCKED: menu AUTO đã mở nhưng chưa tìm thấy 'Thiết lập'");
        for (const auto& match : matches) AppendSettingsMatchDiagnostic(detail, cap, match);
        return true;
    }

    SettingsSemanticMatch& selected = matches[static_cast<std::size_t>(settingsIndex)];
    wchar_t invokeDetail[512]{};
    bool invoked = false;
    if (selected.controlKind == SemanticControlKind::Button) {
        invoked = InvokeSemanticButton(selected.button, L"AUTO STEP2", invokeDetail, _countof(invokeDetail));
    } else if (selected.controlKind == SemanticControlKind::Toggle) {
        invoked = InvokeSemanticToggle(selected.toggle, L"AUTO STEP2", invokeDetail, _countof(invokeDetail));
    }
'''

s = replace_once(s, old_choose, new_choose, "bridge ChooseAutoSettings selector")

# Add spatial proof to successful log without touching pickup logic.
s = replace_once(
    s,
    '''    SetText(detail, cap, invokeDetail);\n    AppendText(detail, cap, L" | OPEN PASS: AutoFightUI active + TogglePickUpTab UNIQUE");\n    return true;\n}\n\n\nbool SelectRuntimePickup''',
    '''    SetText(detail, cap, invokeDetail);\n    AppendText(detail, cap, L" | SETTINGS PICK: single-pass exact label; duplicate tie-break=normalized upper region");\n    AppendSettingsMatchDiagnostic(detail, cap, selected);\n    AppendText(detail, cap, L" | OPEN PASS: AutoFightUI active + TogglePickUpTab UNIQUE");\n    return true;\n}\n\n\nbool SelectRuntimePickup''',
    "bridge success spatial log",
)

p.write_text(s, encoding="utf-8")

# Update only the visible version label; no pickup behavior changes.
p = Path("src/main.cpp")
s = p.read_text(encoding="utf-8-sig")
s = replace_once(
    s,
    'L"Thần Long - Auto Settings Probe v0.3 Semantic Open"',
    'L"Thần Long - Auto Settings Probe v0.4 Upper Region"',
    "main version title",
)
p.write_text(s, encoding="utf-8")
