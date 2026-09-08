#include <windows.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
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

struct Api {
    HMODULE module = nullptr;
    Il2CppDomain* (__cdecl* domain_get)() = nullptr;
    const Il2CppAssembly* (__cdecl* domain_assembly_open)(Il2CppDomain*, const char*) = nullptr;
    const Il2CppImage* (__cdecl* assembly_get_image)(const Il2CppAssembly*) = nullptr;
    Il2CppClass* (__cdecl* class_from_name)(const Il2CppImage*, const char*, const char*) = nullptr;
    const MethodInfo* (__cdecl* class_get_method_from_name)(Il2CppClass*, const char*, int) = nullptr;
    Il2CppClass* (__cdecl* class_get_parent)(Il2CppClass*) = nullptr;
    const Il2CppType* (__cdecl* method_get_return_type)(const MethodInfo*) = nullptr;
    char* (__cdecl* type_get_name)(const Il2CppType*) = nullptr;
    void (__cdecl* free_fn)(void*) = nullptr;
    Il2CppObject* (__cdecl* runtime_invoke)(const MethodInfo*, void*, void**, void**) = nullptr;
    Il2CppClass* (__cdecl* object_get_class)(Il2CppObject*) = nullptr;
    FieldInfo* (__cdecl* class_get_field_from_name)(Il2CppClass*, const char*) = nullptr;
    const Il2CppType* (__cdecl* field_get_type)(FieldInfo*) = nullptr;
    void (__cdecl* field_get_value)(Il2CppObject*, FieldInfo*, void*) = nullptr;
    Il2CppClass* (__cdecl* class_from_type)(const Il2CppType*) = nullptr;
    bool (__cdecl* class_is_valuetype)(const Il2CppClass*) = nullptr;
    std::int32_t (__cdecl* string_length)(Il2CppString*) = nullptr;
    const wchar_t* (__cdecl* string_chars)(Il2CppString*) = nullptr;

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
        NEED(method_get_return_type);
        NEED(type_get_name);
        NEED(runtime_invoke);
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
};

Api g_api;

const Il2CppImage* AssemblyCSharp() {
    Il2CppDomain* domain = g_api.domain_get ? g_api.domain_get() : nullptr;
    if (!domain) return nullptr;
    const Il2CppAssembly* assembly = g_api.domain_assembly_open(domain, "Assembly-CSharp");
    if (!assembly) assembly = g_api.domain_assembly_open(domain, "Assembly-CSharp.dll");
    return assembly ? g_api.assembly_get_image(assembly) : nullptr;
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

bool TypeIsString(const Il2CppType* type) {
    if (!type) return false;
    char* name = g_api.type_get_name(type);
    if (!name) return false;
    const bool ok = Eq(name, "System.String");
    g_api.free_fn(name);
    return ok;
}

bool InvokeObject(const MethodInfo* method, void* instance, Il2CppObject*& out) {
    out = nullptr;
    if (!method) return false;
    void* exc = nullptr;
    out = g_api.runtime_invoke(method, instance, nullptr, &exc);
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

bool TryStringGetter(Il2CppObject* object, Il2CppClass* klass, const char* getter,
                     Il2CppString*& out) {
    out = nullptr;
    const MethodInfo* m = FindMethod(klass, getter, 0);
    if (!m || !TypeIsString(g_api.method_get_return_type(m))) return false;
    Il2CppObject* value = nullptr;
    if (!InvokeObject(m, object, value) || !value) return false;
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
    const MethodInfo* getter = FindMethod(shared, "get_LeaderRoleData", 0);
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
        (void)ReadAutoSettings(g_shared->response.autoSettings, kAutoSettingsCapacity,
                               g_shared->response.detail, kDetailCapacity, code);
        g_shared->response.ok = 1;
        g_shared->response.resultCode = static_cast<std::int32_t>(ResultCode::RuntimeUnknown);
        g_shared->response.runtimePickupState = -1;
        g_shared->response.mutationAvailable = 0;
        SetText(g_shared->response.detail, kDetailCapacity,
                L"Runtime pickup chưa có nguồn độc lập được chứng minh; persisted AutoSettings không được dùng làm proof");
    } else if (command == Command::EnsurePickupOn) {
        g_shared->response.ok = 0;
        g_shared->response.resultCode = static_cast<std::int32_t>(ResultCode::MutationBlocked);
        g_shared->response.runtimePickupState = -1;
        g_shared->response.mutationAvailable = 0;
        SetText(g_shared->response.detail, kDetailCapacity,
                L"BLOCKED: chưa có runtime read-back độc lập nên v0.1 không ghi/toggle Nhặt vật phẩm");
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
