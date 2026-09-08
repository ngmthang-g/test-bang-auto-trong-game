from pathlib import Path


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly one match, got {count}")
    return text.replace(old, new, 1)

# main.cpp: do not tear down a healthy bridge just because the window list is refreshed.
p = Path("src/main.cpp")
s = p.read_text(encoding="utf-8-sig")
s = replace_once(
    s,
    "void RefreshWindows() {\n    g_session.Close();\n    g_games.clear();",
    "void RefreshWindows() {\n    // Refreshing the window list is read-only discovery. Keep an already attached\n    // bridge session alive; Open() will close it only if the selected PID/TID changes.\n    g_games.clear();",
    "main RefreshWindows lifecycle",
)
s = replace_once(
    s,
    "    void Close() {\n        if (hook_) { UnhookWindowsHookEx(hook_); hook_ = nullptr; }",
    "    void Close() {\n        // Ask the in-game bridge to release its named mapping before removing the\n        // Windows hook. Without this handshake, the remote DLL can keep\n        // Local\\\\ThanLongAutoSettings_<PID> alive and the next attach is mistaken\n        // for a second Probe instance.\n        if (block_ && hook_) {\n            Response ignored{};\n            std::wstring ignoredError;\n            (void)Send(Command::ShutdownBridge, ignored, ignoredError);\n        }\n        if (hook_) { UnhookWindowsHookEx(hook_); hook_ = nullptr; }",
    "main BridgeSession::Close handshake",
)
p.write_text(s, encoding="utf-8")

# bridge.cpp: explicit shutdown ACK then release the bridge-side mapping handle/view.
p = Path("src/bridge.cpp")
s = p.read_text(encoding="utf-8-sig")
s = replace_once(
    s,
    "void ProcessRequest() {\n    if (!EnsureMapping() || !g_shared) return;",
    "void CleanupMapping();\n\nvoid ProcessRequest() {\n    if (!EnsureMapping() || !g_shared) return;",
    "bridge CleanupMapping forward declaration",
)
s = replace_once(
    s,
    "    const Command command = static_cast<Command>(g_shared->request.command);\n    ResultCode code = ResultCode::None;\n\n    if (command == Command::ReadAutoSettings) {",
    "    const Command command = static_cast<Command>(g_shared->request.command);\n    ResultCode code = ResultCode::None;\n\n    if (command == Command::ShutdownBridge) {\n        g_shared->response.ok = 1;\n        g_shared->response.resultCode = static_cast<std::int32_t>(ResultCode::Ok);\n        SetText(g_shared->response.detail, kDetailCapacity, L\"Bridge shutdown ACK; releasing mapping\");\n        // Publish completion before unmapping. Controller owns a second mapping\n        // handle, so its view stays valid long enough to observe completedSeq.\n        InterlockedExchange(&g_shared->bridgeBusy, 0);\n        MemoryBarrier();\n        InterlockedExchange(&g_shared->completedSeq, seq);\n        CleanupMapping();\n        return;\n    }\n\n    if (command == Command::ReadAutoSettings) {",
    "bridge ShutdownBridge command",
)
p.write_text(s, encoding="utf-8")

print("bridge lifecycle patch applied")
