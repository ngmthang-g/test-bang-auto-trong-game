# Thần Long Auto Settings Probe v0.1

Tool chẩn đoán Windows x64 để đọc cấu hình **Thiết lập AUTO** trực tiếp từ runtime client Thần Long, không OCR và không click tọa độ.

## Chức năng

- Quét các cửa sổ process có `GameAssembly.dll`.
- Đọc `Game.RoleData.AutoSettings` thông qua donor runtime đã xác minh (`LuaSystemSharedData.get_LeaderRoleData` -> `AutoSettings`/`roleData.AutoSettings`).
- Parse và hiển thị các nhóm AUTOTRAIN, PICKITEM, UTILITIES, REGENE, PET, AUTOPK và FUBEN.
- Hiển thị raw AutoSettings và cảnh báo version/schema mismatch.
- Probe riêng `Runtime Nhặt vật phẩm` theo nguyên tắc fail-safe.

## Giới hạn an toàn của v0.1

`PICKITEM.IsOn` trong chuỗi AutoSettings là cấu hình persisted. Nó **không được coi là bằng chứng độc lập rằng engine nhặt đồ runtime hiện tại đang ON**.

Vì chưa có runtime read-back độc lập được chứng minh trong repo test này, v0.1 sẽ:

- hiển thị `Runtime Nhặt vật phẩm: CHƯA XÁC ĐỊNH`;
- giữ nút `BẬT NHẶT VẬT PHẨM` bị khóa;
- không gọi Lua action;
- không gửi packet `CMD_SHARED_PARAMETER`;
- không sửa `Game.RoleData.AutoSettings`;
- không dùng `SendInput`, `mouse_event`, OCR hay click màn hình.

Đây là hành vi cố ý theo safety gate của spec. Khi một runtime source độc lập được live-prove, route `EnsurePickupOn` mới được phép mở.

## Cách test

1. Tải artifact `ThanLongAutoSettingsProbe-v0.1-win-x64` từ GitHub Actions.
2. Giữ `ThanLongAutoSettingsProbe.exe` và `ThanLongAutoSettingsBridge.dll` cùng thư mục.
3. Mở client game và vào nhân vật.
4. Chạy EXE cùng integrity level với game; nếu game chạy Administrator thì tool cũng cần chạy Administrator.
5. Chọn cửa sổ/PID và bấm `ĐỌC TOÀN BỘ AUTO`.
6. Gửi phần LOG + RAW AutoSettings nếu một field/version khác schema dự kiến.

## Build

```powershell
cmake -S . -B build -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Output:

- `build/Release/ThanLongAutoSettingsProbe.exe`
- `build/Release/ThanLongAutoSettingsBridge.dll`
