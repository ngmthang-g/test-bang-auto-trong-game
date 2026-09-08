# Thần Long Auto Settings Probe v0.2

Tool chẩn đoán Windows x64 để đọc cấu hình **Thiết lập AUTO** trực tiếp từ runtime client Thần Long, không OCR và không click tọa độ.

## Chức năng

- Quét các cửa sổ process có `GameAssembly.dll`.
- Đọc `Game.RoleData.AutoSettings` qua `LuaSystemSharedData.get_LeaderRoleData` -> `AutoSettings` / `roleData.AutoSettings`.
- Parse và hiển thị AUTOTRAIN, PICKITEM, UTILITIES, REGENE, PET, AUTOPK và FUBEN.
- Hiển thị raw AutoSettings và cảnh báo version/schema mismatch.
- Quét `UIObject.instances` để nhận diện riêng **tab Nhặt đồ** (`TogglePickUpTab`) và **checkbox Nhặt vật phẩm** (`TogPickUpEquipment`).
- Khi bảng Thiết Lập AUTO đã mở, probe tự chọn tab `Nhặt đồ` nếu cần: ưu tiên `set_Selected(true)` theo donor `InvokeControl`, fallback `HandleSelectEvent(true)` chỉ khi setter không tồn tại, rồi bắt buộc `get_Selected()` read-back = 1.
- Sau proof tab, quét lại UI mới và chỉ nhận checkbox `Nhặt vật phẩm` trong `TabPickUp`; navigation tab không còn được tính là candidate pickup.
- Đọc `TogPickUpEquipment.get_Selected()` làm runtime proof độc lập với persisted `PICKITEM.IsOn`.
- Chỉ mở route `BẬT NHẶT VẬT PHẨM` khi tìm đúng **một** checkbox, control interactable, có `HandleSelectEvent(Boolean)` và schema persisted là 4.1.
- Khi bật pickup: gọi đúng một lần `HandleSelectEvent(true)`, sau đó bắt buộc `get_Selected()` read-back = ON; không thử ghi lần hai nếu proof thất bại.
- Sau action, đọc lại AutoSettings để log so sánh runtime với persisted state.

## Fail-safe của v0.2

`PICKITEM.IsOn` trong AutoSettings vẫn chỉ là cấu hình persisted; nó không được dùng thay cho runtime proof.

Nếu bảng Thiết Lập AUTO chưa mở, tab `Nhặt đồ` không unique/không interactable, chọn tab không có read-back, nội dung `TabPickUp` chưa materialize sau khi tab đã Selected=1, checkbox pickup mơ hồ, hoặc pickup read-back không ON thì tool **không ghi mù**. Sau một route tab đã invoke thành công nhưng nội dung chưa xuất hiện, tool cũng không thử callback tab lần hai.

v0.2 không dùng `SendInput`, `mouse_event`, OCR, tọa độ màn hình, packet `CMD_SHARED_PARAMETER=200024`, không sửa trực tiếp chuỗi `Game.RoleData.AutoSettings`, và không gọi Lua gameplay action để bật Nhặt vật phẩm.

## Cách test live v0.2

1. Tải artifact `ThanLongAutoSettingsProbe-v0.2-win-x64` từ GitHub Actions; giữ EXE và DLL cùng thư mục.
2. Mở client, vào nhân vật và chạy tool cùng integrity level với game.
3. Chọn PID rồi bấm `ĐỌC TOÀN BỘ AUTO` để xác nhận schema 4.1 và persisted `PICKITEM.IsOn`.
4. **Chỉ cần mở bảng Thiết Lập AUTO trong game. Cố ý để ở một tab khác như Đánh quái; không cần tự bấm Nhặt đồ.**
5. Bấm `KIỂM TRA RUNTIME`. Tool phải tự chọn dòng `Nhặt đồ`, proof `TogglePickUpTab.Selected=1`, quét lại UI và sau đó đọc đúng `TogPickUpEquipment`:
   - checkbox đang bật -> `Runtime Nhặt vật phẩm: ON`;
   - checkbox đang tắt -> `Runtime Nhặt vật phẩm: OFF`;
   - nếu `CHƯA XÁC ĐỊNH`, gửi nguyên dòng `RUNTIME PROBE`; log sẽ nói rõ route tab và candidate/runtime nào chưa được chứng minh.
6. Tắt checkbox `Nhặt vật phẩm` bằng tay rồi bấm `KIỂM TRA RUNTIME` để xác nhận `OFF`. Chỉ khi nút `BẬT NHẶT VẬT PHẨM` sáng thì bấm nút đó.
7. PASS live cho mutation khi đồng thời thấy checkbox trong game chuyển sang ON, tool báo `get_Selected read-back=ON`, và lần probe tiếp theo vẫn báo runtime ON.
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
