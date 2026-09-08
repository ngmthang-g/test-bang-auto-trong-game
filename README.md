# Thần Long Auto Settings Probe v0.2

Tool chẩn đoán Windows x64 để đọc cấu hình **Thiết lập AUTO** trực tiếp từ runtime client Thần Long, không OCR và không click tọa độ.

## Chức năng

- Quét các cửa sổ process có `GameAssembly.dll`.
- Đọc `Game.RoleData.AutoSettings` qua `LuaSystemSharedData.get_LeaderRoleData` -> `AutoSettings` / `roleData.AutoSettings`.
- Parse và hiển thị AUTOTRAIN, PICKITEM, UTILITIES, REGENE, PET, AUTOPK và FUBEN.
- Hiển thị raw AutoSettings và cảnh báo version/schema mismatch.
- Quét `UIObject.instances` để tìm **UIToggle Nhặt vật phẩm** đang sống trong bảng AUTO.
- Đọc `UIToggle.get_Selected()` làm runtime proof độc lập với persisted `PICKITEM.IsOn`.
- Chỉ mở route `BẬT NHẶT VẬT PHẨM` khi tìm đúng **một** candidate, control interactable, có `HandleSelectEvent(Boolean)` và schema persisted là 4.1.
- Khi bật: gọi đúng một lần `HandleSelectEvent(true)`, sau đó bắt buộc `get_Selected()` read-back = ON; không thử ghi lần hai nếu proof thất bại.
- Sau action, đọc lại AutoSettings để log so sánh runtime với persisted state.

## Fail-safe của v0.2

`PICKITEM.IsOn` trong AutoSettings vẫn chỉ là cấu hình persisted; nó không được dùng thay cho runtime proof.

Nếu bảng AUTO/tab Nhặt đồ chưa mở, không tìm thấy toggle, có nhiều candidate giống nhau, control không interactable, thiếu `HandleSelectEvent(Boolean)`, hoặc read-back không ON thì tool **không ghi mù** và giữ/đưa route về trạng thái khóa.

v0.2 không dùng `SendInput`, `mouse_event`, OCR, tọa độ màn hình, packet `CMD_SHARED_PARAMETER=200024`, không sửa trực tiếp chuỗi `Game.RoleData.AutoSettings`, và không gọi Lua gameplay action để bật Nhặt vật phẩm.

## Cách test live v0.2

1. Tải artifact `ThanLongAutoSettingsProbe-v0.2-win-x64` từ GitHub Actions; giữ EXE và DLL cùng thư mục.
2. Mở client, vào nhân vật và chạy tool cùng integrity level với game.
3. Chọn PID rồi bấm `ĐỌC TOÀN BỘ AUTO` để xác nhận schema 4.1 và persisted `PICKITEM.IsOn`.
4. **Mở bảng AUTO trong game và chọn tab Nhặt đồ**, để checkbox `Nhặt vật phẩm` tồn tại/active trong UI runtime.
5. Bấm `KIỂM TRA RUNTIME`. Kết quả cần đối chiếu trực tiếp với checkbox đang nhìn thấy:
   - checkbox đang bật -> `Runtime Nhặt vật phẩm: ON`;
   - checkbox đang tắt -> `Runtime Nhặt vật phẩm: OFF`;
   - nếu `CHƯA XÁC ĐỊNH`, gửi nguyên dòng `RUNTIME PROBE` vì log có Name/Text/descendant/ancestor của candidate để khóa tiếp.
6. Chỉ khi probe báo candidate UNIQUE và nút `BẬT NHẶT VẬT PHẨM` sáng: tắt checkbox Nhặt vật phẩm bằng tay, probe lại để xác nhận `OFF`, rồi bấm nút bật của tool.
7. PASS live cho mutation khi đồng thời thấy:
   - checkbox trong game chuyển sang ON;
   - tool báo `get_Selected read-back=ON`;
   - lần probe kế tiếp vẫn báo runtime ON.
8. Dòng `PERSISTED PICKITEM.IsOn` được log riêng để so sánh. Nếu runtime ON nhưng persisted chưa đổi ngay, không coi đó là runtime failure; game có thể chỉ persist khi lưu thiết lập.

## Build

```powershell
cmake -S . -B build -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Output:

- `build/Release/ThanLongAutoSettingsProbe.exe`
- `build/Release/ThanLongAutoSettingsBridge.dll`

## Test suite

- `auto_settings_parser_tests`
- `ui_logic_tests`
- `pickup_ui_logic_tests`
- `protocol_layout_tests`
