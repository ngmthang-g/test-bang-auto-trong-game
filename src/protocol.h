#pragma once
#include <windows.h>
#include <stdlib.h>
#include <cstdint>
#include <cstddef>
#include <cwchar>

namespace autosettings_probe {

constexpr std::uint32_t kMagic = 0x41535042u;
constexpr std::uint32_t kProtocolVersion = 0x00010000u;
constexpr UINT kWakeMessage = WM_APP + 0x5A3;
constexpr wchar_t kMappingPrefix[] = L"Local\\ThanLongAutoSettings_";
constexpr std::size_t kAutoSettingsCapacity = 8192;
constexpr std::size_t kDetailCapacity = 512;

enum class Command : std::uint32_t {
    None = 0,
    ReadAutoSettings = 1,
    ProbePickupRuntime = 2,
    EnsurePickupOn = 3,
};

enum class ResultCode : std::int32_t {
    None = 0,
    Ok = 1,
    RuntimeUnknown = 2,
    MutationBlocked = 3,
    Il2CppUnavailable = 10,
    RoleDataUnavailable = 11,
    AutoSettingsUnavailable = 12,
    AutoSettingsTooLong = 13,
    ProtocolMismatch = 20,
};

struct Request {
    std::uint32_t command = 0;
    std::int32_t arg0 = 0;
    std::int32_t arg1 = 0;
};

struct Response {
    std::int32_t ok = 0;
    std::int32_t resultCode = 0;
    std::int32_t runtimePickupState = -1;
    std::int32_t mutationAvailable = 0;
    wchar_t autoSettings[kAutoSettingsCapacity]{};
    wchar_t detail[kDetailCapacity]{};
};

struct SharedBlock {
    std::uint32_t magic = kMagic;
    std::uint32_t protocolVersion = kProtocolVersion;
    std::uint32_t targetPid = 0;
    std::uint32_t targetWindowThreadId = 0;
    volatile LONG requestSeq = 0;
    volatile LONG completedSeq = 0;
    volatile LONG bridgeLoaded = 0;
    volatile LONG bridgeBusy = 0;
    Request request{};
    Response response{};
};

inline void MappingName(DWORD pid, wchar_t* output, std::size_t count) {
    if (!output || count == 0) return;
    const int written = swprintf_s(output, count, L"%ls%lu", kMappingPrefix, static_cast<unsigned long>(pid));
    if (written < 0) output[0] = 0;
}

static_assert(sizeof(Request) == 12, "protocol request layout changed");
static_assert(kAutoSettingsCapacity >= 4096, "AutoSettings buffer must be diagnostic-sized");
static_assert(kDetailCapacity >= 256, "detail buffer too small");

} // namespace autosettings_probe
