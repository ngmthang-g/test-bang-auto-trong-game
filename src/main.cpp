#include <windows.h>
#include <tlhelp32.h>
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>
#include "auto_settings_parser.h"
#include "protocol.h"
#include "ui_logic.h"

using namespace autosettings_probe;

namespace {
constexpr int IDC_GAME = 1001;
constexpr int IDC_REFRESH = 1002;
constexpr int IDC_READ = 1003;
constexpr int IDC_PROBE = 1004;
constexpr int IDC_ENABLE = 1005;
constexpr int IDC_RUNTIME = 1006;
constexpr int IDC_OUTPUT = 1007;
constexpr int IDC_LOG = 1008;
constexpr int IDC_OPEN_AUTO_SETTINGS = 1009;

struct GameWindow {
    HWND hwnd = nullptr;
    DWORD pid = 0;
    DWORD threadId = 0;
    std::wstring title;
};

HWND g_main = nullptr;
HWND g_combo = nullptr;
HWND g_readButton = nullptr;
HWND g_probeButton = nullptr;
HWND g_enableButton = nullptr;
HWND g_runtime = nullptr;
HWND g_output = nullptr;
HWND g_log = nullptr;
HWND g_openAutoSettingsButton = nullptr;
std::vector<GameWindow> g_games;
bool g_lastVersionMatches = false;

std::wstring ErrorCodeText(const wchar_t* prefix, DWORD code) {
    return std::wstring(prefix) + L" (Win32=" + std::to_wstring(code) + L")";
}

void AppendLog(const std::wstring& text) {
    if (!g_log) return;
    const int len = GetWindowTextLengthW(g_log);
    SendMessageW(g_log, EM_SETSEL, len, len);
    const std::wstring line = text + L"\r\n";
    SendMessageW(g_log, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(line.c_str()));
}

bool ProcessHasGameAssembly(DWORD pid) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap == INVALID_HANDLE_VALUE) return false;
    MODULEENTRY32W me{};
    me.dwSize = sizeof(me);
    bool found = false;
    if (Module32FirstW(snap, &me)) {
        do {
            if (_wcsicmp(me.szModule, L"GameAssembly.dll") == 0) { found = true; break; }
        } while (Module32NextW(snap, &me));
    }
    CloseHandle(snap);
    return found;
}

BOOL CALLBACK EnumGameWindowProc(HWND hwnd, LPARAM) {
    if (!IsWindowVisible(hwnd) || GetWindow(hwnd, GW_OWNER) != nullptr) return TRUE;
    const int n = GetWindowTextLengthW(hwnd);
    if (n <= 0) return TRUE;
    std::wstring title(static_cast<std::size_t>(n) + 1, L'\0');
    GetWindowTextW(hwnd, title.data(), n + 1);
    title.resize(wcslen(title.c_str()));
    DWORD pid = 0;
    const DWORD tid = GetWindowThreadProcessId(hwnd, &pid);
    if (!pid || pid == GetCurrentProcessId()) return TRUE;
    if (!ProcessHasGameAssembly(pid)) return TRUE;
    g_games.push_back({hwnd, pid, tid, title});
    return TRUE;
}

std::wstring ExeDirectory() {
    wchar_t path[MAX_PATH]{};
    const DWORD n = GetModuleFileNameW(nullptr, path, _countof(path));
    if (!n || n >= _countof(path)) return L".";
    std::wstring s(path, n);
    const auto pos = s.find_last_of(L"\\/");
    return pos == std::wstring::npos ? L"." : s.substr(0, pos);
}

class BridgeSession {
public:
    ~BridgeSession() { Close(); }

    bool Matches(const GameWindow& g) const { return block_ && pid_ == g.pid && threadId_ == g.threadId; }

    bool Open(const GameWindow& g, std::wstring& error) {
        if (Matches(g)) return true;
        Close();
        pid_ = g.pid;
        threadId_ = g.threadId;

        wchar_t mappingName[128]{};
        MappingName(pid_, mappingName, _countof(mappingName));
        mapping_ = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,
                                      static_cast<DWORD>(sizeof(SharedBlock)), mappingName);
        if (!mapping_) { error = ErrorCodeText(L"CreateFileMapping thất bại", GetLastError()); Close(); return false; }
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            error = L"Đã có một Auto Settings Probe khác đang gắn vào PID này";
            Close();
            return false;
        }
        block_ = static_cast<SharedBlock*>(MapViewOfFile(mapping_, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedBlock)));
        if (!block_) { error = ErrorCodeText(L"MapViewOfFile thất bại", GetLastError()); Close(); return false; }
        ZeroMemory(block_, sizeof(*block_));
        block_->magic = kMagic;
        block_->protocolVersion = kProtocolVersion;
        block_->targetPid = pid_;
        block_->targetWindowThreadId = threadId_;
        block_->response.runtimePickupState = -1;

        const std::wstring dllPath = ExeDirectory() + L"\\ThanLongAutoSettingsBridge.dll";
        module_ = LoadLibraryW(dllPath.c_str());
        if (!module_) { error = ErrorCodeText(L"Không load được ThanLongAutoSettingsBridge.dll", GetLastError()); Close(); return false; }
        FARPROC proc = GetProcAddress(module_, "TlAutoSettingsHook");
        if (!proc) { error = ErrorCodeText(L"DLL thiếu export TlAutoSettingsHook", GetLastError()); Close(); return false; }
        hook_ = SetWindowsHookExW(WH_GETMESSAGE, reinterpret_cast<HOOKPROC>(proc), module_, threadId_);
        if (!hook_) { error = ErrorCodeText(L"SetWindowsHookEx thất bại", GetLastError()); Close(); return false; }
        if (!PostThreadMessageW(threadId_, kWakeMessage, 0, 0)) {
            error = ErrorCodeText(L"PostThreadMessage bootstrap thất bại", GetLastError());
            Close();
            return false;
        }
        const ULONGLONG deadline = GetTickCount64() + 3000;
        while (GetTickCount64() < deadline) {
            if (block_->bridgeLoaded == 1) return true;
            Sleep(10);
        }
        error = L"Timeout chờ bridge load trong game process";
        Close();
        return false;
    }

    bool Send(Command command, Response& response, std::wstring& error) {
        if (!block_ || !hook_) { error = L"Bridge chưa mở"; return false; }
        ZeroMemory(&block_->response, sizeof(block_->response));
        block_->response.runtimePickupState = -1;
        block_->request.command = static_cast<std::uint32_t>(command);
        block_->request.arg0 = 0;
        block_->request.arg1 = 0;
        const LONG seq = InterlockedIncrement(&block_->requestSeq);
        MemoryBarrier();
        if (!PostThreadMessageW(threadId_, kWakeMessage, 0, 0)) {
            error = ErrorCodeText(L"PostThreadMessage command thất bại", GetLastError());
            return false;
        }
        const ULONGLONG deadline = GetTickCount64() + 4000;
        while (GetTickCount64() < deadline) {
            if (block_->completedSeq == seq) {
                MemoryBarrier();
                response = block_->response;
                return true;
            }
            Sleep(10);
        }
        error = L"Timeout chờ bridge trả lời";
        return false;
    }

    void Close() {
        // Ask the in-game bridge to release its named mapping before removing the
        // Windows hook. Without this handshake, the remote DLL can keep
        // Local\\ThanLongAutoSettings_<PID> alive and the next attach is mistaken
        // for a second Probe instance.
        if (block_ && hook_) {
            Response ignored{};
            std::wstring ignoredError;
            (void)Send(Command::ShutdownBridge, ignored, ignoredError);
        }
        if (hook_) { UnhookWindowsHookEx(hook_); hook_ = nullptr; }
        if (block_) { UnmapViewOfFile(block_); block_ = nullptr; }
        if (mapping_) { CloseHandle(mapping_); mapping_ = nullptr; }
        if (module_) { FreeLibrary(module_); module_ = nullptr; }
        pid_ = 0;
        threadId_ = 0;
    }

private:
    DWORD pid_ = 0;
    DWORD threadId_ = 0;
    HANDLE mapping_ = nullptr;
    SharedBlock* block_ = nullptr;
    HMODULE module_ = nullptr;
    HHOOK hook_ = nullptr;
};

BridgeSession g_session;

void RefreshWindows() {
    // Refreshing the window list is read-only discovery. Keep an already attached
    // bridge session alive; Open() will close it only if the selected PID/TID changes.
    g_games.clear();
    SendMessageW(g_combo, CB_RESETCONTENT, 0, 0);
    EnumWindows(EnumGameWindowProc, 0);
    std::sort(g_games.begin(), g_games.end(), [](const GameWindow& a, const GameWindow& b) {
        if (a.title != b.title) return a.title < b.title;
        return a.pid < b.pid;
    });
    for (const auto& g : g_games) {
        const std::wstring label = g.title + L"  [PID " + std::to_wstring(g.pid) + L"]";
        SendMessageW(g_combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
    }
    if (!g_games.empty()) SendMessageW(g_combo, CB_SETCURSEL, 0, 0);
    AppendLog(L"Quét thấy " + std::to_wstring(g_games.size()) + L" cửa sổ có GameAssembly.dll");
}

const GameWindow* SelectedGame() {
    const LRESULT index = SendMessageW(g_combo, CB_GETCURSEL, 0, 0);
    if (index == CB_ERR || index < 0 || static_cast<std::size_t>(index) >= g_games.size()) return nullptr;
    return &g_games[static_cast<std::size_t>(index)];
}

bool EnsureSession() {
    const GameWindow* game = SelectedGame();
    if (!game) { AppendLog(L"Chưa chọn cửa sổ game"); return false; }
    std::wstring error;
    if (!g_session.Open(*game, error)) { AppendLog(L"BRIDGE FAIL: " + error); return false; }
    return true;
}

std::wstring RenderDocument(const autosettings::Document& d) {
    std::wstring out;
    out += L"VERSION: " + d.version + (d.versionMatches ? L"  [OK]\r\n" : L"  [VERSION MISMATCH]\r\n");
    out += L"Schema: ";
    out += d.schemaMismatch ? L"có lệch/thiếu/dư field\r\n" : L"khớp schema đã biết\r\n";
    for (const auto& group : d.groups) {
        out += L"\r\n=== " + group.name + L" ===\r\n";
        for (const auto& field : group.fields)
            out += field.name + L" = " + autosettings::DisplayValue(field) + L"\r\n";
        if (group.truncated) out += L"[TRUNCATED GROUP]\r\n";
        for (std::size_t i = 0; i < group.extras.size(); ++i)
            out += L"Extra[" + std::to_wstring(i) + L"] = " + group.extras[i] + L"\r\n";
    }
    if (!d.topLevelExtras.empty()) {
        out += L"\r\n=== TOP LEVEL EXTRAS ===\r\n";
        for (std::size_t i = 0; i < d.topLevelExtras.size(); ++i)
            out += L"ExtraSegment[" + std::to_wstring(i) + L"] = " + d.topLevelExtras[i] + L"\r\n";
    }
    out += L"\r\n=== RAW AutoSettings ===\r\n" + d.raw;
    return out;
}

void UpdatePersistedPickupProof(const Response& response) {
    if (!response.autoSettings[0]) {
        AppendLog(L"PERSISTED PICKITEM.IsOn = UNKNOWN (không có AutoSettings snapshot)");
        return;
    }
    const autosettings::Document doc = autosettings::Parse(response.autoSettings);
    g_lastVersionMatches = doc.versionMatches;
    const autosettings::Group* pick = autosettings::FindGroup(doc, autosettings::GroupKind::PickItem);
    if (!pick || pick->fields.empty()) {
        AppendLog(L"PERSISTED PICKITEM.IsOn = UNKNOWN (không parse được PICKITEM)");
        return;
    }
    AppendLog(L"PERSISTED PICKITEM.IsOn = " + autosettings::DisplayValue(pick->fields[0]) +
              (doc.versionMatches ? L" [schema 4.1]" : L" [VERSION MISMATCH]"));
}

void UpdateRuntimeUi(const Response& response) {
    const std::wstring state = ui_logic::PickupStateText(response.runtimePickupState);
    SetWindowTextW(g_runtime, (L"Runtime Nhặt vật phẩm: " + state).c_str());
    const bool enable = ui_logic::CanEnablePickup(response.mutationAvailable == 1, g_lastVersionMatches);
    EnableWindow(g_enableButton, enable ? TRUE : FALSE);
}

void DoReadAll() {
    if (!EnsureSession()) return;
    Response response{};
    std::wstring error;
    if (!g_session.Send(Command::ReadAutoSettings, response, error)) { AppendLog(L"READ FAIL: " + error); return; }
    if (!response.ok) {
        AppendLog(L"READ FAIL code=" + std::to_wstring(response.resultCode) + L": " + response.detail);
        SetWindowTextW(g_output, L"");
        g_lastVersionMatches = false;
        EnableWindow(g_enableButton, FALSE);
        return;
    }
    const autosettings::Document doc = autosettings::Parse(response.autoSettings);
    g_lastVersionMatches = doc.versionMatches;
    SetWindowTextW(g_output, RenderDocument(doc).c_str());
    AppendLog(L"READ PASS: " + std::wstring(response.detail));
    EnableWindow(g_enableButton, FALSE);
}

void DoProbe() {
    if (!EnsureSession()) return;
    Response response{};
    std::wstring error;
    if (!g_session.Send(Command::ProbePickupRuntime, response, error)) { AppendLog(L"PROBE FAIL: " + error); return; }
    UpdatePersistedPickupProof(response);
    UpdateRuntimeUi(response);
    AppendLog(L"RUNTIME PROBE: " + std::wstring(response.detail));
}

void DoEnsurePickup() {
    if (!IsWindowEnabled(g_enableButton)) {
        AppendLog(L"ENSURE PICKUP bị khóa: chưa có runtime read-back proof UNIQUE/interactable/schema 4.1");
        return;
    }
    if (!EnsureSession()) return;
    Response response{};
    std::wstring error;
    if (!g_session.Send(Command::EnsurePickupOn, response, error)) { AppendLog(L"ENSURE FAIL: " + error); return; }
    UpdatePersistedPickupProof(response);
    UpdateRuntimeUi(response);
    AppendLog(std::wstring(response.ok ? L"ENSURE PASS: " : L"ENSURE BLOCKED: ") + response.detail);
}

void DoOpenAutoSettingsSemantic() {
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

void Layout(HWND hwnd) {
    RECT r{};
    GetClientRect(hwnd, &r);
    const int w = r.right - r.left;
    const int h = r.bottom - r.top;
    MoveWindow(g_combo, 12, 12, w - 330, 300, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_REFRESH), w - 306, 12, 92, 28, TRUE);
    MoveWindow(g_readButton, w - 206, 12, 194, 28, TRUE);
    MoveWindow(g_probeButton, 12, 48, 180, 28, TRUE);
    MoveWindow(g_enableButton, 200, 48, 210, 28, TRUE);
    MoveWindow(g_runtime, 430, 52, w - 442, 24, TRUE);
    MoveWindow(g_openAutoSettingsButton, 12, 80, 270, 28, TRUE);
    const int logH = 130;
    MoveWindow(g_output, 12, 116, w - 24, h - 116 - logH - 20, TRUE);
    MoveWindow(g_log, 12, h - logH - 8, w - 24, logH, TRUE);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    switch (msg) {
        case WM_CREATE: {
            g_combo = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                                    0,0,0,0, hwnd, reinterpret_cast<HMENU>(IDC_GAME), nullptr, nullptr);
            CreateWindowW(L"BUTTON", L"Quét lại", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                          0,0,0,0, hwnd, reinterpret_cast<HMENU>(IDC_REFRESH), nullptr, nullptr);
            g_readButton = CreateWindowW(L"BUTTON", L"ĐỌC TOÀN BỘ AUTO", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                         0,0,0,0, hwnd, reinterpret_cast<HMENU>(IDC_READ), nullptr, nullptr);
            g_probeButton = CreateWindowW(L"BUTTON", L"KIỂM TRA RUNTIME", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                          0,0,0,0, hwnd, reinterpret_cast<HMENU>(IDC_PROBE), nullptr, nullptr);
            g_enableButton = CreateWindowW(L"BUTTON", L"BẬT NHẶT VẬT PHẨM", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                           0,0,0,0, hwnd, reinterpret_cast<HMENU>(IDC_ENABLE), nullptr, nullptr);
            g_openAutoSettingsButton = CreateWindowW(L"BUTTON", L"TEST AUTO → THIẾT LẬP", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                           0,0,0,0, hwnd, reinterpret_cast<HMENU>(IDC_OPEN_AUTO_SETTINGS), nullptr, nullptr);
            g_runtime = CreateWindowW(L"STATIC", L"Runtime Nhặt vật phẩm: CHƯA XÁC ĐỊNH", WS_CHILD | WS_VISIBLE,
                                      0,0,0,0, hwnd, reinterpret_cast<HMENU>(IDC_RUNTIME), nullptr, nullptr);
            g_output = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                0,0,0,0, hwnd, reinterpret_cast<HMENU>(IDC_OUTPUT), nullptr, nullptr);
            g_log = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                0,0,0,0, hwnd, reinterpret_cast<HMENU>(IDC_LOG), nullptr, nullptr);
            EnableWindow(g_enableButton, FALSE);
            RefreshWindows();
            Layout(hwnd);
            return 0;
        }
        case WM_SIZE: Layout(hwnd); return 0;
        case WM_COMMAND: {
            const int id = LOWORD(wParam);
            if (id == IDC_REFRESH && HIWORD(wParam) == BN_CLICKED) RefreshWindows();
            else if (id == IDC_READ && HIWORD(wParam) == BN_CLICKED) DoReadAll();
            else if (id == IDC_PROBE && HIWORD(wParam) == BN_CLICKED) DoProbe();
            else if (id == IDC_ENABLE && HIWORD(wParam) == BN_CLICKED) DoEnsurePickup();
            else if (id == IDC_OPEN_AUTO_SETTINGS && HIWORD(wParam) == BN_CLICKED) DoOpenAutoSettingsSemantic();
            else if (id == IDC_GAME && HIWORD(wParam) == CBN_SELCHANGE) {
                g_session.Close();
                g_lastVersionMatches = false;
                EnableWindow(g_enableButton, FALSE);
                SetWindowTextW(g_runtime, L"Runtime Nhặt vật phẩm: CHƯA XÁC ĐỊNH");
            }
            return 0;
        }
        case WM_DESTROY:
            g_session.Close();
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    const wchar_t kClassName[] = L"ThanLongAutoSettingsProbeWindow";
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.hInstance = instance;
    wc.lpfnWndProc = WndProc;
    wc.lpszClassName = kClassName;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    if (!RegisterClassExW(&wc)) return 1;

    g_main = CreateWindowExW(0, kClassName, L"Thần Long - Auto Settings Probe v0.4 Upper Region",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1020, 760,
        nullptr, nullptr, instance, nullptr);
    if (!g_main) return 2;
    ShowWindow(g_main, show);
    UpdateWindow(g_main);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}