# Auto Settings Probe Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a Windows x64 diagnostic tool that reads and parses the complete built-in `Game.RoleData.AutoSettings` configuration for a selected Thần Long client, with a conservative pickup-runtime probe and a mutation gate that stays disabled until a live runtime route is proven.

**Architecture:** A small Win32 controller enumerates game windows, creates a per-PID shared-memory block, and installs a WH_GETMESSAGE hook from a companion DLL. The bridge executes only read/probe commands in the game window thread, resolves IL2CPP metadata dynamically, copies the current managed `AutoSettings` string into shared memory, and never returns client object pointers. A portable C++ parser turns the serialized settings into named groups/fields for display and unit testing.

**Tech Stack:** C++17, Win32 API, IL2CPP exported metadata/runtime functions, CMake 3.24+, MSVC x64, GitHub Actions windows-latest.

**Spec:** `docs/superpowers/specs/2026-09-08-auto-settings-probe-design.md`

## Global Constraints

- Windows x64 only.
- `Đọc toàn bộ AUTO` is read-only: no gameplay Lua action, packet send, memory write, UI click, SendInput, or mouse ownership.
- Serialized schema version expected: `4.1`; version mismatch remains readable/raw but disables mutation.
- Pickup runtime state must be `UNKNOWN` unless an independent runtime source is actually proven.
- `EnsurePickupOn` must never toggle blindly and must remain unavailable when runtime read-back is unproven.
- Do not reproduce the shipped `DropItemSettings` indexing bug; PICKITEM fields 8 and 9 are distinct.

---

### Task 1: Portable AutoSettings parser with TDD

**Files:**
- Create: `src/auto_settings_parser.h`
- Create: `src/auto_settings_parser.cpp`
- Create: `tests/auto_settings_parser_test.cpp`

**Interfaces:**
- Produces: `autosettings::Document Parse(const std::wstring&)`
- Produces: `const Group* FindGroup(const Document&, GroupKind)`
- Produces: `std::wstring DisplayValue(const Field&)`

- [ ] **Step 1: Write failing parser tests** covering complete 4.1 data, PICKITEM 9-field ordering, bool formatting, truncated data, extra data, version mismatch, DropItemSettings index, and FUBEN schedule bounds.
- [ ] **Step 2: Build/run the test target and confirm RED** because parser symbols do not yet exist.
- [ ] **Step 3: Implement the minimal parser** with schema tables, bounds-safe splitting, raw-value preservation, and version diagnostics.
- [ ] **Step 4: Re-run parser tests and confirm GREEN.**
- [ ] **Step 5: Commit parser + tests.**

### Task 2: Minimal controller/bridge protocol

**Files:**
- Create: `src/protocol.h`
- Create: `src/bridge.cpp`

**Interfaces:**
- Commands: `ReadAutoSettings`, `ProbePickupRuntime`, `EnsurePickupOn`
- Shared response: `ok`, `runtimePickupState`, `mutationAvailable`, `autoSettings[8192]`, `detail[512]`

- [ ] **Step 1: Add compile-time protocol layout tests/static assertions** for fixed-size shared-memory POD fields.
- [ ] **Step 2: Confirm test/compile failure before protocol exists.**
- [ ] **Step 3: Implement protocol and bridge bootstrap** using `Local\\ThanLongAutoSettings_<PID>`, `WH_GETMESSAGE`, and a private wake message.
- [ ] **Step 4: Implement `ReadAutoSettings`** by resolving `Assembly-CSharp`, `FGStudio.LuaSystem.API.LuaSystemAPI_Game`, the static `get_RoleData` path (with conservative SessionData fallback), then `AutoSettings` field/property; copy UTF-16 values only.
- [ ] **Step 5: Implement `ProbePickupRuntime` conservatively**: report `UNKNOWN` unless a distinct managed runtime member/control can be resolved and read; persisted PICKITEM.IsOn alone is not proof.
- [ ] **Step 6: Implement `EnsurePickupOn` safety gate** so it returns blocked when no independently readable runtime route exists; no fallback packet/Lua/toggle is allowed in v0.1.
- [ ] **Step 7: Build bridge with /W4 and fix warnings/errors.**

### Task 3: Win32 diagnostic controller UI

**Files:**
- Create: `src/main.cpp`

**Interfaces:**
- Consumes: parser API and `SharedBlock` protocol.
- Produces: selectable game-process UI and diagnostic output.

- [ ] **Step 1: Add pure helper tests where practical** for window-label formatting and state-to-text mapping.
- [ ] **Step 2: Implement game-window enumeration** using visible top-level windows with process/thread IDs; show title + PID and support refresh.
- [ ] **Step 3: Implement bridge session lifecycle**: create/open mapping, load companion DLL locally, install `TlAutoSettingsHook` into selected window thread, send command, wait with timeout, and unhook/cleanup on selection change/exit.
- [ ] **Step 4: Implement `ĐỌC TOÀN BỘ AUTO`** to request raw settings, parse them, and render every known group/field plus raw diagnostics.
- [ ] **Step 5: Implement runtime pickup status display** (`ON/OFF/CHƯA XÁC ĐỊNH`) and keep `BẬT NHẶT VẬT PHẨM` disabled unless `mutationAvailable==1` and version is 4.1.
- [ ] **Step 6: Implement logs** for bridge load, request timeout, IL2CPP resolution error, version mismatch, and probe result.

### Task 4: Build system and CI artifact

**Files:**
- Create: `CMakeLists.txt`
- Create: `.github/workflows/build.yml`
- Create: `README.md`

**Interfaces:**
- Produces: `ThanLongAutoSettingsProbe.exe`, `ThanLongAutoSettingsBridge.dll`, and `auto_settings_parser_tests.exe`.

- [ ] **Step 1: Add CMake targets** for bridge DLL, GUI EXE, and parser tests with x64/Windows guards.
- [ ] **Step 2: Add GitHub Actions windows-latest workflow** to configure Release x64, build, run CTest, and upload EXE+DLL artifact only after tests pass.
- [ ] **Step 3: Document usage and safety behavior** including that pickup mutation is intentionally unavailable until a separate live runtime source is proven.
- [ ] **Step 4: Push and inspect GitHub Actions.** If CI fails, use logs to fix and rerun until green.

### Task 5: Verification and integration

**Files:**
- Modify only files required by failures discovered in verification.

- [ ] **Step 1: Run/confirm all unit tests in CI.**
- [ ] **Step 2: Confirm Release x64 EXE and DLL artifacts exist.**
- [ ] **Step 3: Audit source for forbidden feature paths:** no `SendInput`, `mouse_event`, direct screen click, network packet send, or AutoSettings write in the read flow.
- [ ] **Step 4: Open PR from `feat/auto-settings-probe-v0.1` to `main` with implementation/test summary.**
- [ ] **Step 5: Merge/fast-forward only after green CI, then verify the main-branch build artifact/release path is available.
