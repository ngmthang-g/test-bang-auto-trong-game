#include <windows.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include "pickup_ui_logic.h"
#include "protocol.h"

using namespace autosettings_probe;

namespace {
using Il2CppDomain = void;
using Il2CppAssembly = void;
using Il2CppImage = void;
using Il2CppClass = void;
using MethodInfo = void;
using FieldInfo = void;
using Il2CppType = void;
using Il2CppObject = void;
using Il2CppString = void;

HANDLE g_mapping = nullptr;
SharedBlock* g_shared = nullptr;

template <typename T>
bool Resolve(HMODULE module, const char* name, T& out) {
    out = nullptr;
    const FARPROC p = GetProcAddress(module, name);
    if (!p) return false;
    static_assert(sizeof(p) == sizeof(out), "pointer size mismatch");
    std::memcpy(&out, &p, sizeof(out));
    return out != nullptr;
}

bool Eq(const char* a, const char* b) {
    if (!a || !b) return false;
    while (*a && *b) {
        if (*a++ != *b++) return false;
    }
    return *a == *b;
}

void SetText(wchar_t* out, std::size_t cap, const wchar_t* text) {
    if (!out || cap == 0) return;
    if (!text) { out[0] = 0; return; }
    wcsncpy_s(out, cap, text, _TRUNCATE);
}

void AppendText(wchar_t* out, std::size_t cap, const wchar_t* text) {
    if (!out || !text || cap == 0) return;
    const std::size_t used = wcsnlen_s(out, cap);
    if (used >= cap - 1) return;
    wcsncat_s(out, cap, text, _TRUNCATE);
}

void AppendInt(wchar_t* out, std::size_t cap, int value) {
    wchar_t buffer[32]{};
    swprintf_s(buffer, _countof(buffer), L"%d", value);
    AppendText(out, cap, buffer);
}

struct Api {
    HMODULE module = nullptr;
    Il2CppDomain* (__cdecl* domain_get)() = nullptr;
    const Il2CppAssembly* (__cdecl* domain_assembly_open)(Il2CppDomain*, const char*) = nullptr;
    const Il2CppImage* (__cdecl* assembly_get_image)(const Il2CppAssembly*) = nullptr;
    Il2CppClass* (__cdecl* class_from_name)(const Il2CppImage*, const char*, const char*) = nullptr;
    const MethodInfo* (__cdecl* class_get_method_from_name)(Il2CppClass*, const char*, int) = nullptr;
    Il2CppClass* (__cdecl* class_get_parent)(Il2CppClass*) = nullptr;
    std::uint32_t (__cdecl* method_get_flags)(const MethodInfo*, std::uint32_t*) = nullptr;
    std::uint32_t (__cdecl* method_get_param_count)(const MethodInfo*) = nullptr;
    const Il2CppType* (__cdecl* method_get_param)(const MethodInfo*, std::uint32_t) = nullptr;
    const Il2CppType* (__cdecl* method_get_return_type)(const MethodInfo*) = nullptr;
    char* (__cdecl* type_get_name)(const Il2CppType*) = nullptr;
    void (__cdecl* free_fn)(void*) = nullptr;
    Il2CppObject* (__cdecl* runtime_invoke)(const MethodInfo*, void*, void**, void**) = nullptr;
    void* (__cdecl* object_unbox)(Il2CppObject*) = nullptr;
    Il2CppClass* (__cdecl* object_get_class)(Il2CppObject*) = nullptr;
    FieldInfo* (__cdecl* class_get_field_from_name)(Il2CppClass*, const char*) = nullptr;
    const Il2CppType* (__cdecl* field_get_type)(FieldInfo*) = nullptr;
    void (__cdecl* field_get_value)(Il2CppObject*, FieldInfo*, void*) = nullptr;
    void (__cdecl* field_static_get_value)(FieldInfo*, void*) = nullptr;
    Il2CppClass* (__cdecl* class_from_type)(const Il2CppType*) = nullptr;
    bool (__cdecl* class_is_valuetype)(const Il2CppClass*) = nullptr;
    bool (__cdecl* class_is_assignable_from)(Il2CppClass*, Il2CppClass*) = nullptr;
    std::int32_t (__cdecl* string_length)(Il2CppString*) = nullptr;
    const wchar_t* (__cdecl* string_chars)(Il2CppString*) = nullptr;
    std::size_t (__cdecl* image_get_class_count)(const Il2CppImage*) = nullptr;
    Il2CppClass* (__cdecl* image_get_class)(const Il2CppImage*, std::size_t) = nullptr;
    const char* (__cdecl* class_get_name)(Il2CppClass*) = nullptr;
    bool uiDiscoveryLoaded = false;

    bool Load(wchar_t* detail, std::size_t cap) {
        if (module) return true;
        module = GetModuleHandleW(L"GameAssembly.dll");
        if (!module) { SetText(detail, cap, L"GameAssembly.dll chưa sẵn sàng"); return false; }
#define NEED(symbol) do { if (!Resolve(module, "il2cpp_" #symbol, symbol)) { SetText(detail, cap, L"Thiếu IL2CPP export bắt buộc"); return false; } } while (0)
        NEED(domain_get);
        NEED(domain_assembly_open);
        NEED(assembly_get_image);
        NEED(class_from_name);
        NEED(class_get_method_from_name);
        NEED(class_get_parent);
        NEED(method_get_flags);
        NEED(method_get_param_count);
        NEED(method_get_param);
        NEED(method_get_return_type);
        NEED(type_get_name);
        NEED(runtime_invoke);
        NEED(object_unbox);
        NEED(object_get_class);
        NEED(class_get_field_from_name);
        NEED(field_get_type);
        NEED(field_get_value);
        NEED(class_from_type);
        NEED(class_is_valuetype);
        NEED(string_length);
        NEED(string_chars);
#undef NEED
        if (!Resolve(module, "il2cpp_free", free_fn)) {
            SetText(detail, cap, L"Thiếu il2cpp_free");
            return false;
        }
        return true;
    }

    bool LoadUiDiscovery(wchar_t* detail, std::size_t cap) {
        if (!Load(detail, cap)) return false;
        if (uiDiscoveryLoaded) return true;
        if (!Resolve(module, "il2cpp_class_is_assignable_from", class_is_assignable_from) ||
            !Resolve(module, "il2cpp_field_static_get_value", field_static_get_value)) {
            SetText(detail, cap, L"Thiếu IL2CPP export cần cho UIObject.instances");
            return false;
        }
        (void)Resolve(module, "il2cpp_image_get_class_count", image_get_class_count);
        (void)Resolve(module, "il2cpp_image_get_class", image_get_class);
        (void)Resolve(module, "il2cpp_class_get_name", class_get_name);
        uiDiscoveryLoaded = true;
        return true;
    }
};

Api g_api;

const Il2CppImage* AssemblyCSharp() {
    Il2CppDomain* domain = g_api.domain_get ? g_api.domain_get() : nullptr;
    if (!domain) return nullptr;
    const Il2CppAssembly* assembly = g_api.domain_assembly_open(domain, "Assembly-CSharp");
    if (!assembly) assembly = g_api.domain_assembly_open(domain, "Assembly-CSharp.dll");
    return assembly ? g_api.assembly_get_image(assembly) : nullptr;
}

bool StaticMethod(const MethodInfo* method) {
    if (!method || !g_api.method_get_flags) return false;
    constexpr std::uint32_t StaticFlag = 0x0010;
    std::uint32_t implFlags = 0;
    return (g_api.method_get_flags(method, &implFlags) & StaticFlag) != 0;
}

const MethodInfo* FindMethod(Il2CppClass* klass, const char* name, int argc) {
    for (Il2CppClass* c = klass; c; c = g_api.class_get_parent(c)) {
        if (const MethodInfo* m = g_api.class_get_method_from_name(c, name, argc)) return m;
    }
    return nullptr;
}

FieldInfo* FindField(Il2CppClass* klass, const char* name) {
    for (Il2CppClass* c = klass; c; c = g_api.class_get_parent(c)) {
        if (FieldInfo* f = g_api.class_get_field_from_name(c, name)) return f;
    }
    return nullptr;
}

bool ParamType(const MethodInfo* method, std::uint32_t index, const char* expected) {
    if (!method || !expected || index >= g_api.method_get_param_count(method)) return false;
    const Il2CppType* type = g_api.method_get_param(method, index);
    char* name = type ? g_api.type_get_name(type) : nullptr;
    if (!name) return false;
    const bool ok = Eq(name, expected);
    g_api.free_fn(name);
    return ok;
}

const MethodInfo* ExactMethod(Il2CppClass* klass, const char* name, int argc, bool isStatic,
                              const char* p0 = nullptr) {
    const MethodInfo* method = FindMethod(klass, name, argc);
    if (!method || StaticMethod(method) != isStatic) return nullptr;
    if (argc > 0 && p0 && !ParamType(method, 0, p0)) return nullptr;
    return method;
}

bool TypeIsString(const Il2CppType* type) {
    if (!type) return false;
    char* name = g_api.type_get_name(type);
    if (!name) return false;
    const bool ok = Eq(name, "System.String");
    g_api.free_fn(name);
    return ok;
}

bool InvokeObjectArgs(const MethodInfo* method, void* instance, void** args, Il2CppObject*& out) {
    out = nullptr;
    if (!method) return false;
    void* exc = nullptr;
    out = g_api.runtime_invoke(method, instance, args, &exc);
    return exc == nullptr;
}

bool InvokeObject(const MethodInfo* method, void* instance, Il2CppObject*& out) {
    return InvokeObjectArgs(method, instance, nullptr, out);
}

bool InvokeBool(const MethodInfo* method, void* instance, bool& out) {
    out = false;
    if (!method) return false;
    void* exc = nullptr;
    Il2CppObject* boxed = g_api.runtime_invoke(method, instance, nullptr, &exc);
    if (exc || !boxed) return false;
    const Il2CppType* type = g_api.method_get_return_type(method);
    char* name = type ? g_api.type_get_name(type) : nullptr;
    if (!name) return false;
    const bool typeOk = Eq(name, "System.Boolean");
    g_api.free_fn(name);
    if (!typeOk) return false;
    void* raw = g_api.object_unbox(boxed);
    if (!raw) return false;
    out = *reinterpret_cast<const std::uint8_t*>(raw) != 0;
    return true;
}

bool InvokeVoid(const MethodInfo* method, void* instance, void** args) {
    if (!method) return false;
    void* exc = nullptr;
    (void)g_api.runtime_invoke(method, instance, args, &exc);
    return exc == nullptr;
}

bool CopyManagedString(Il2CppString* value, wchar_t* out, std::size_t cap,
                       wchar_t* detail, std::size_t detailCap) {
    if (!value || !out || cap == 0) {
        SetText(detail, detailCap, L"AutoSettings string=null");
        return false;
    }
    const int len = g_api.string_length(value);
    const wchar_t* chars = g_api.string_chars(value);
    if (len < 0 || !chars) {
        SetText(detail, detailCap, L"Không đọc được nội dung AutoSettings");
        return false;
    }
    if (static_cast<std::size_t>(len) >= cap) {
        SetText(detail, detailCap, L"AutoSettings dài hơn buffer 8192; từ chối cắt cụt");
        return false;
    }
    for (int i = 0; i < len; ++i) out[i] = chars[i];
    out[len] = 0;
    return true;
}

bool CopyManagedString(Il2CppString* value, std::wstring& out) {
    out.clear();
    if (!value) return false;
    const int len = g_api.string_length(value);
    const wchar_t* chars = g_api.string_chars(value);
    if (len < 0 || len > 4096 || !chars) return false;
    out.assign(chars, chars + len);
    return true;
}

bool TryStringGetter(Il2CppObject* object, Il2CppClass* klass, const char* getter,
                     Il2CppString*& out) {
    out = nullptr;
    const MethodInfo* method = FindMethod(klass, getter, 0);
    if (!method || !TypeIsString(g_api.method_get_return_type(method))) return false;
    Il2CppObject* value = nullptr;
    if (!InvokeObject(method, object, value) || !value) return false;
    out = reinterpret_cast<Il2CppString*>(value);
    return true;
}

bool TryStringField(Il2CppObject* object, Il2CppClass* klass, const char* fieldName,
                    Il2CppString*& out) {
    out = nullptr;
    FieldInfo* field = FindField(klass, fieldName);
    if (!field || !TypeIsString(g_api.field_get_type(field))) return false;
    Il2CppString* value = nullptr;
    g_api.field_get_value(object, field, &value);
    if (!value) return false;
    out = value;
    return true;
}

bool ReadStringMember(Il2CppObject* object, Il2CppClass* klass, const char* member,
                      std::wstring& out) {
    out.clear();
    std::string getter = std::string("get_") + member;
    Il2CppString* value = nullptr;
    if (TryStringGetter(object, klass, getter.c_str(), value) ||
        TryStringField(object, klass, member, value)) {
        return CopyManagedString(value, out);
    }
    return false;
}

bool RoleDataBacking(Il2CppObject* leader, Il2CppClass* leaderClass,
                     Il2CppObject*& backing, Il2CppClass*& backingClass) {
    backing = nullptr;
    backingClass = nullptr;
    FieldInfo* field = FindField(leaderClass, "roleData");
    if (!field) return false;
    const Il2CppType* type = g_api.field_get_type(field);
    Il2CppClass* fieldClass = type ? g_api.class_from_type(type) : nullptr;
    if (!fieldClass || g_api.class_is_valuetype(fieldClass)) return false;
    g_api.field_get_value(leader, field, &backing);
    if (!backing) return false;
    backingClass = g_api.object_get_class(backing);
    return backingClass != nullptr;
}

bool GetLeader(Il2CppObject*& leader, Il2CppClass*& leaderClass,
               wchar_t* detail, std::size_t cap) {
    leader = nullptr;
    leaderClass = nullptr;
    if (!g_api.Load(detail, cap)) return false;
    const Il2CppImage* image = AssemblyCSharp();
    if (!image) { SetText(detail, cap, L"Không mở được Assembly-CSharp"); return false; }
    Il2CppClass* shared = g_api.class_from_name(image, "FGStudio.LuaSystem", "LuaSystemSharedData");
    if (!shared) { SetText(detail, cap, L"Không resolve LuaSystemSharedData"); return false; }
    const MethodInfo* getter = ExactMethod(shared, "get_LeaderRoleData", 0, true);
    if (!getter || !InvokeObject(getter, nullptr, leader) || !leader) {
        SetText(detail, cap, L"LeaderRoleData chưa sẵn sàng");
        return false;
    }
    leaderClass = g_api.object_get_class(leader);
    if (!leaderClass) { SetText(detail, cap, L"Không lấy được class LeaderRoleData"); return false; }
    return true;
}

bool ReadAutoSettings(wchar_t* out, std::size_t outCap, wchar_t* detail, std::size_t detailCap,
                      ResultCode& code) {
    code = ResultCode::Il2CppUnavailable;
    Il2CppObject* leader = nullptr;
    Il2CppClass* leaderClass = nullptr;
    if (!GetLeader(leader, leaderClass, detail, detailCap)) return false;

    Il2CppString* settings = nullptr;
    if (TryStringGetter(leader, leaderClass, "get_AutoSettings", settings) ||
        TryStringField(leader, leaderClass, "AutoSettings", settings) ||
        TryStringField(leader, leaderClass, "<AutoSettings>k__BackingField", settings) ||
        TryStringField(leader, leaderClass, "autoSettings", settings)) {
        code = ResultCode::AutoSettingsUnavailable;
        if (!CopyManagedString(settings, out, outCap, detail, detailCap)) {
            code = ResultCode::AutoSettingsTooLong;
            return false;
        }
        code = ResultCode::Ok;
        SetText(detail, detailCap, L"Đọc AutoSettings trực tiếp từ LeaderRoleData");
        return true;
    }

    Il2CppObject* backing = nullptr;
    Il2CppClass* backingClass = nullptr;
    if (!RoleDataBacking(leader, leaderClass, backing, backingClass)) {
        code = ResultCode::RoleDataUnavailable;
        SetText(detail, detailCap, L"Không resolve được roleData backing của LeaderRoleData");
        return false;
    }

    settings = nullptr;
    if (!(TryStringGetter(backing, backingClass, "get_AutoSettings", settings) ||
          TryStringField(backing, backingClass, "AutoSettings", settings) ||
          TryStringField(backing, backingClass, "<AutoSettings>k__BackingField", settings) ||
          TryStringField(backing, backingClass, "autoSettings", settings))) {
        code = ResultCode::AutoSettingsUnavailable;
        SetText(detail, detailCap, L"roleData có nhưng không resolve được AutoSettings string");
        return false;
    }
    if (!CopyManagedString(settings, out, outCap, detail, detailCap)) {
        code = ResultCode::AutoSettingsTooLong;
        return false;
    }
    code = ResultCode::Ok;
    SetText(detail, detailCap, L"Đọc AutoSettings từ LeaderRoleData.roleData");
    return true;
}

template <typename T>
bool ReadLocal(const void* base, std::size_t offset, T& value) {
    if (!base) return false;
    SIZE_T done = 0;
    const auto* address = reinterpret_cast<const unsigned char*>(base) + offset;
    return ReadProcessMemory(GetCurrentProcess(), address, &value, sizeof(value), &done) != FALSE && done == sizeof(value);
}

bool IsUiObjectClass(Il2CppClass* klass) {
    return klass && FindField(klass, "instances");
}

bool IsToggleClass(Il2CppClass* klass) {
    return klass &&
           ExactMethod(klass, "get_Selected", 0, false) &&
           (ExactMethod(klass, "HandleSelectEvent", 1, false, "System.Boolean") ||
            ExactMethod(klass, "set_Selected", 1, false, "System.Boolean"));
}

struct UiRuntime {
    bool ready = false;
    const Il2CppImage* image = nullptr;
    Il2CppClass* uiObject = nullptr;
    Il2CppClass* toggle = nullptr;
    FieldInfo* instances = nullptr;
};

UiRuntime g_ui;

void FindUiClassesByMetadata() {
    if (!g_ui.image || !g_api.image_get_class_count || !g_api.image_get_class || !g_api.class_get_name) return;
    const std::size_t count = g_api.image_get_class_count(g_ui.image);
    if (count == 0 || count > 65536) return;
    for (std::size_t i = 0; i < count; ++i) {
        Il2CppClass* klass = g_api.image_get_class(g_ui.image, i);
        const char* name = klass ? g_api.class_get_name(klass) : nullptr;
        if (!name) continue;
        if (!g_ui.uiObject && Eq(name, "UIObject") && IsUiObjectClass(klass)) g_ui.uiObject = klass;
        if (!g_ui.toggle && Eq(name, "UIToggle") && IsToggleClass(klass)) g_ui.toggle = klass;
        if (g_ui.uiObject && g_ui.toggle) return;
    }
}

bool EnsureUiDiscovery(wchar_t* detail, std::size_t cap) {
    if (g_ui.ready) return true;
    if (!g_api.LoadUiDiscovery(detail, cap)) return false;
    g_ui.image = AssemblyCSharp();
    if (!g_ui.image) { SetText(detail, cap, L"UI discovery: không mở được Assembly-CSharp"); return false; }
    g_ui.uiObject = g_api.class_from_name(g_ui.image, "FGStudio.LuaSystem.Base", "UIObject");
    g_ui.toggle = g_api.class_from_name(g_ui.image, "FGStudio.LuaSystem.GUI", "UIToggle");
    if (g_ui.uiObject && !IsUiObjectClass(g_ui.uiObject)) g_ui.uiObject = nullptr;
    if (g_ui.toggle && !IsToggleClass(g_ui.toggle)) g_ui.toggle = nullptr;
    FindUiClassesByMetadata();
    g_ui.instances = g_ui.uiObject ? FindField(g_ui.uiObject, "instances") : nullptr;
    if (!g_ui.uiObject || !g_ui.toggle || !g_ui.instances) {
        SetText(detail, cap, L"Không resolve đủ UIObject.instances/UIToggle đã validate");
        return false;
    }
    g_ui.ready = true;
    return true;
}

bool ReadManagedPointerArray(Il2CppObject* array, std::vector<Il2CppObject*>& values, std::size_t hardLimit) {
    values.clear();
    std::uintptr_t length = 0;
    if (!array || !ReadLocal(array, 0x18, length) || length > hardLimit) return false;
    values.reserve(static_cast<std::size_t>(length));
    for (std::uintptr_t i = 0; i < length; ++i) {
        Il2CppObject* value = nullptr;
        if (!ReadLocal(array, 0x20 + static_cast<std::size_t>(i) * sizeof(void*), value)) return false;
        if (value) values.push_back(value);
    }
    return true;
}

bool ObjectGetter(Il2CppObject* object, Il2CppClass* klass, const char* getter, Il2CppObject*& out) {
    out = nullptr;
    const MethodInfo* method = FindMethod(klass, getter, 0);
    return method && InvokeObject(method, object, out);
}

void AppendLabel(std::wstring& target, const std::wstring& value) {
    if (value.empty()) return;
    if (target.find(value) != std::wstring::npos) return;
    if (!target.empty()) target += L"/";
    target += value;
}

void CollectDescendantLabels(Il2CppObject* root, std::wstring& labels) {
    labels.clear();
    std::vector<Il2CppObject*> pending{root};
    std::vector<Il2CppObject*> visited;
    while (!pending.empty() && visited.size() < 96) {
        Il2CppObject* current = pending.back();
        pending.pop_back();
        if (!current || std::find(visited.begin(), visited.end(), current) != visited.end()) continue;
        visited.push_back(current);
        Il2CppClass* klass = g_api.object_get_class(current);
        if (!klass) continue;
        if (current != root) {
            std::wstring value;
            if (ReadStringMember(current, klass, "Name", value)) AppendLabel(labels, value);
            if (ReadStringMember(current, klass, "Text", value)) AppendLabel(labels, value);
        }
        Il2CppObject* children = nullptr;
        if (!ObjectGetter(current, klass, "get_CoreChildren", children) || !children)
            (void)ObjectGetter(current, klass, "get_Children", children);
        if (!children) continue;
        std::vector<Il2CppObject*> childValues;
        if (!ReadManagedPointerArray(children, childValues, 128)) continue;
        for (Il2CppObject* child : childValues) pending.push_back(child);
    }
}

void CollectAncestorLabels(Il2CppObject* root, std::wstring& labels) {
    labels.clear();
    Il2CppObject* current = root;
    std::vector<Il2CppObject*> seen;
    for (int depth = 0; depth < 14 && current; ++depth) {
        if (std::find(seen.begin(), seen.end(), current) != seen.end()) break;
        seen.push_back(current);
        Il2CppClass* klass = g_api.object_get_class(current);
        if (!klass) break;
        Il2CppObject* parent = nullptr;
        if (!ObjectGetter(current, klass, "get_Parent", parent) || !parent) break;
        Il2CppClass* parentClass = g_api.object_get_class(parent);
        if (!parentClass) break;
        std::wstring name;
        if (ReadStringMember(parent, parentClass, "Name", name)) AppendLabel(labels, name);
        current = parent;
    }
}

struct RuntimeToggle {
    Il2CppObject* object = nullptr;
    Il2CppClass* klass = nullptr;
    pickup_ui_logic::Candidate candidate{};
    bool interactable = false;
    bool hasSetSelected = false;
    bool hasSelectHandler = false;
};

bool ReadToggleBool(Il2CppObject* object, Il2CppClass* klass, const char* getter, bool& value) {
    const MethodInfo* method = ExactMethod(klass, getter, 0, false);
    return method && InvokeBool(method, object, value);
}

bool EnumerateActiveToggles(std::vector<RuntimeToggle>& toggles, wchar_t* detail, std::size_t cap) {
    toggles.clear();
    if (!EnsureUiDiscovery(detail, cap)) return false;
    Il2CppObject* dictionary = nullptr;
    g_api.field_static_get_value(g_ui.instances, &dictionary);
    Il2CppObject* entries = nullptr;
    std::int32_t count = 0;
    std::uintptr_t capacity = 0;
    if (!dictionary || !ReadLocal(dictionary, 0x18, entries) || !entries ||
        !ReadLocal(dictionary, 0x20, count) || count < 0 || count > 32768 ||
        !ReadLocal(entries, 0x18, capacity) || capacity > 32768) {
        SetText(detail, cap, L"UIObject.instances dictionary không hợp lệ");
        return false;
    }

    for (std::uintptr_t i = 0; i < capacity; ++i) {
        Il2CppObject* object = nullptr;
        const std::size_t entry = 0x20 + static_cast<std::size_t>(i) * 0x18;
        if (!ReadLocal(entries, entry + 0x10, object) || !object) continue;
        Il2CppClass* klass = g_api.object_get_class(object);
        if (!klass || !g_api.class_is_assignable_from(g_ui.toggle, klass)) continue;

        bool active = false;
        if (!ReadToggleBool(object, klass, "get_ActiveInHierarchy", active) || !active) continue;
        bool interactable = false;
        if (!ReadToggleBool(object, klass, "get_Interactable", interactable)) interactable = false;
        bool selected = false;
        if (!ReadToggleBool(object, klass, "get_Selected", selected)) continue;

        RuntimeToggle row{};
        row.object = object;
        row.klass = klass;
        row.interactable = interactable;
        row.hasSetSelected = ExactMethod(klass, "set_Selected", 1, false, "System.Boolean") != nullptr;
        row.hasSelectHandler = ExactMethod(klass, "HandleSelectEvent", 1, false, "System.Boolean") != nullptr;
        row.candidate.selected = selected ? 1 : 0;
        (void)ReadStringMember(object, klass, "Name", row.candidate.name);
        (void)ReadStringMember(object, klass, "Text", row.candidate.text);
        CollectDescendantLabels(object, row.candidate.descendants);
        CollectAncestorLabels(object, row.candidate.ancestors);
        toggles.push_back(std::move(row));
    }
    return true;
}

void AppendToggleDiagnostic(wchar_t* detail, std::size_t cap, const RuntimeToggle& toggle) {
    AppendText(detail, cap, L" [N=");
    AppendText(detail, cap, toggle.candidate.name.c_str());
    AppendText(detail, cap, L" T=");
    AppendText(detail, cap, toggle.candidate.text.c_str());
    AppendText(detail, cap, L" D=");
    AppendText(detail, cap, toggle.candidate.descendants.c_str());
    AppendText(detail, cap, L" A=");
    AppendText(detail, cap, toggle.candidate.ancestors.c_str());
    AppendText(detail, cap, L" S=");
    AppendInt(detail, cap, toggle.candidate.selected);
    AppendText(detail, cap, L"]");
}

bool SelectRuntimePickup(std::vector<RuntimeToggle>& toggles, int& selectedIndex,
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

void PopulatePersistedSnapshot(Response& response) {
    ResultCode ignoredCode = ResultCode::None;
    wchar_t ignoredDetail[160]{};
    (void)ReadAutoSettings(response.autoSettings, kAutoSettingsCapacity,
                           ignoredDetail, _countof(ignoredDetail), ignoredCode);
}

bool ProbePickupRuntime(Response& response, wchar_t* detail, std::size_t cap) {
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

bool EnsureMapping() {
    if (g_shared) return true;
    wchar_t name[128]{};
    MappingName(GetCurrentProcessId(), name, _countof(name));
    g_mapping = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, name);
    if (!g_mapping) return false;
    g_shared = static_cast<SharedBlock*>(MapViewOfFile(g_mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedBlock)));
    if (!g_shared) {
        CloseHandle(g_mapping);
        g_mapping = nullptr;
        return false;
    }
    if (g_shared->magic != kMagic || g_shared->protocolVersion != kProtocolVersion ||
        g_shared->targetPid != GetCurrentProcessId()) {
        UnmapViewOfFile(g_shared);
        CloseHandle(g_mapping);
        g_shared = nullptr;
        g_mapping = nullptr;
        return false;
    }
    InterlockedExchange(&g_shared->bridgeLoaded, 1);
    return true;
}

void ResetResponse(Response& response) {
    ZeroMemory(&response, sizeof(response));
    response.runtimePickupState = -1;
}

void ProcessRequest() {
    if (!EnsureMapping() || !g_shared) return;
    const LONG seq = g_shared->requestSeq;
    if (seq == 0 || g_shared->completedSeq == seq) return;
    if (InterlockedCompareExchange(&g_shared->bridgeBusy, 1, 0) != 0) return;

    ResetResponse(g_shared->response);
    const Command command = static_cast<Command>(g_shared->request.command);
    ResultCode code = ResultCode::None;

    if (command == Command::ReadAutoSettings) {
        const bool ok = ReadAutoSettings(g_shared->response.autoSettings, kAutoSettingsCapacity,
                                         g_shared->response.detail, kDetailCapacity, code);
        g_shared->response.ok = ok ? 1 : 0;
        g_shared->response.resultCode = static_cast<std::int32_t>(code);
    } else if (command == Command::ProbePickupRuntime) {
        const bool transportOk = ProbePickupRuntime(g_shared->response,
                                                    g_shared->response.detail, kDetailCapacity);
        if (!transportOk) {
            g_shared->response.ok = 0;
            g_shared->response.resultCode = static_cast<std::int32_t>(ResultCode::RuntimeUnknown);
            g_shared->response.runtimePickupState = -1;
            g_shared->response.mutationAvailable = 0;
        }
    } else if (command == Command::EnsurePickupOn) {
        const bool transportOk = EnsurePickupOn(g_shared->response,
                                                g_shared->response.detail, kDetailCapacity);
        if (!transportOk) {
            g_shared->response.ok = 0;
            g_shared->response.resultCode = static_cast<std::int32_t>(ResultCode::MutationBlocked);
            g_shared->response.runtimePickupState = -1;
            g_shared->response.mutationAvailable = 0;
        }
    } else {
        g_shared->response.ok = 0;
        g_shared->response.resultCode = static_cast<std::int32_t>(ResultCode::ProtocolMismatch);
        SetText(g_shared->response.detail, kDetailCapacity, L"Command không hợp lệ");
    }

    MemoryBarrier();
    InterlockedExchange(&g_shared->completedSeq, seq);
    InterlockedExchange(&g_shared->bridgeBusy, 0);
}

void CleanupMapping() {
    if (g_shared) { UnmapViewOfFile(g_shared); g_shared = nullptr; }
    if (g_mapping) { CloseHandle(g_mapping); g_mapping = nullptr; }
}

} // namespace

extern "C" __declspec(dllexport) LRESULT CALLBACK TlAutoSettingsHook(int code, WPARAM wParam, LPARAM lParam) {
    if (code >= 0) {
        (void)EnsureMapping();
        const MSG* msg = reinterpret_cast<const MSG*>(lParam);
        if (msg && msg->message == kWakeMessage) ProcessRequest();
    }
    return CallNextHookEx(nullptr, code, wParam, lParam);
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
    } else if (reason == DLL_PROCESS_DETACH) {
        CleanupMapping();
    }
    return TRUE;
}
