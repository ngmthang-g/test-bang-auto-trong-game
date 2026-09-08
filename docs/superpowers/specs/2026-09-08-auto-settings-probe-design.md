# Thiết kế v0.1 — Auto Settings Probe

Ngày: 2026-09-08

## Mục tiêu

Tạo một tool Windows x64 độc lập để kiểm tra trạng thái bảng **Thiết lập AUTO** của client Thần Long mà không cần OCR, không cần click tọa độ và không làm thay đổi game trong luồng đọc.

Repo đích: `ngmthang-g/test-bang-auto-trong-game`.

Repo donor kỹ thuật: `ngmthang-g/Auto-APK-than-long`.

Repo dữ liệu/reverse: `ngmthang-g/clinent-game-than-long-DATA-2222`.

## Phạm vi v0.1

### 1. Đọc toàn bộ AutoSettings

Tool đọc `Game.RoleData.AutoSettings` từ runtime client và parse cấu trúc đã xác minh:

```text
version#AUTOTRAIN#PICKITEM#UTILITIES#REGENE#PET#AUTOPK#FUBEN
```

Mỗi segment được parse theo schema đã materialize trong `database/AUTO_SETTINGS_SCHEMA.md`.

Tool hiển thị tên field và giá trị dễ đọc thay vì chỉ in chuỗi thô.

Các nhóm v0.1:

- Đánh quái / AUTOTRAIN
- Nhặt đồ / PICKITEM
- Tiện ích / UTILITIES
- Hồi phục / REGENE
- Pet / PET
- PK / AUTOPK
- Phụ bản / FUBEN

Tool vẫn hiển thị chuỗi raw AutoSettings ở một vùng chẩn đoán để đối chiếu khi schema/runtime có lệch.

### 2. Nguyên tắc read-only

Luồng `Đọc toàn bộ AUTO` tuyệt đối không:

- gọi Lua action thay đổi gameplay;
- gửi packet lưu cấu hình;
- ghi trực tiếp field runtime;
- click UI;
- chiếm chuột.

Đây là probe read-only.

### 3. Probe riêng cho Nhặt vật phẩm runtime

Ngoài persisted AutoSettings, tool có một probe riêng để tìm trạng thái runtime thật của `PICKITEM.IsOn`.

Thứ tự ưu tiên:

1. đọc field/property managed trực tiếp nếu donor/runtime surface cho phép;
2. nếu không có field trực tiếp, kiểm tra control `UIToggle` tương ứng bằng cơ chế UI discovery hiện có;
3. nếu vẫn không chứng minh được thì trả `CHƯA XÁC ĐỊNH`.

Không được suy diễn runtime ON chỉ từ persisted `AutoSettings`.

### 4. Nút Bật Nhặt vật phẩm

Nút `BẬT NHẶT VẬT PHẨM` chỉ được enable khi tool đã xác định một route runtime có thể đọc lại và verify.

Luồng bắt buộc:

```text
READ runtime IsOn
 -> nếu true: PASS, không ghi
 -> nếu false: set true bằng route đã chứng minh
 -> READ AGAIN
 -> chỉ PASS nếu true
```

Không dùng toggle mù.

Không dùng thao tác bán đồ/vứt đồ làm donor action.

Nếu chỉ có persisted route mà chưa có runtime proof thì nút bật phải bị khóa.

### 5. Giao diện

Tool tối giản gồm:

- combobox chọn cửa sổ/PID game;
- nút `Quét lại`;
- nút `ĐỌC TOÀN BỘ AUTO`;
- tree/list hiển thị các group và field;
- vùng `Runtime Nhặt vật phẩm: ON/OFF/CHƯA XÁC ĐỊNH`;
- nút `KIỂM TRA RUNTIME`;
- nút `BẬT NHẶT VẬT PHẨM` có safety gate;
- vùng log PASS/FAIL và detail kỹ thuật.

## Kiến trúc

### Controller EXE

Trách nhiệm:

- enumerate cửa sổ game/PID;
- cài bridge vào đúng game process theo donor pattern;
- gửi command qua shared memory;
- nhận snapshot AutoSettings/raw runtime probe;
- parse và render UI;
- không tự sửa memory từ process controller.

### Bridge DLL

Tái sử dụng pattern donor:

- `SetWindowsHookExW(WH_GETMESSAGE, ...)`;
- shared memory per PID;
- request/response command;
- IL2CPP metadata resolution;
- chỉ thao tác managed/runtime trong context hợp lệ.

V0.1 thêm command read-only cho AutoSettings và command runtime pickup probe.

Mutation command `EnsurePickupOn` chỉ được thêm sau khi runtime read proof tồn tại.

### Parser thuần C++

Tách parser AutoSettings thành module thuần C++ không phụ thuộc Windows/IL2CPP để test dễ dàng.

Parser phải:

- split top-level `#`;
- split segment `|`;
- parse bool `0/1`;
- giữ nguyên giá trị string/list;
- chấp nhận thiếu field mà không crash;
- đánh dấu schema mismatch;
- tránh copy bug `DropItemSettings` index đã biết trong donor Lua.

## Error handling

Tool phải fail-safe:

- GameAssembly chưa sẵn sàng -> báo lỗi, không retry mutation.
- Không đọc được AutoSettings -> không hiện dữ liệu giả.
- Version khác `4.1` -> vẫn hiển thị raw, đánh dấu `VERSION MISMATCH`, không cho mutation.
- Runtime pickup probe không chắc chắn -> `CHƯA XÁC ĐỊNH`, khóa nút bật.
- Managed exception -> ghi log, không tiếp tục action.
- PID/window mất -> dừng command và yêu cầu quét lại.

## Kiểm thử

### Unit tests

Tối thiểu:

1. parse AutoSettings 4.1 đủ field;
2. parse PICKITEM đúng 9 field;
3. bool 0/1 -> OFF/ON;
4. dữ liệu thiếu field không crash;
5. dữ liệu dư field vẫn giữ raw;
6. version mismatch;
7. không nhầm `IsAutoDropItem` với `DropItemSettings`;
8. parser FUBEN schedule không out-of-range.

### Integration/build tests

GitHub Actions Windows x64:

- configure CMake;
- build Release;
- chạy unit tests;
- artifact gồm EXE + bridge DLL;
- chỉ publish artifact khi tests pass.

## Tiêu chí hoàn thành v0.1

v0.1 đạt khi:

1. build Windows x64 thành công trên GitHub Actions;
2. unit tests pass;
3. tool chọn được PID/cửa sổ game;
4. đọc được raw `Game.RoleData.AutoSettings` hoặc báo lỗi rõ ràng;
5. parse và hiển thị đầy đủ các group đã biết;
6. pickup runtime probe không bao giờ trả ON/OFF nếu chưa có bằng chứng;
7. nút bật pickup mặc định khóa cho đến khi runtime route được verify;
8. không có đường click chuột/SendInput cho feature này.

## Ngoài phạm vi v0.1

- chỉnh sửa toàn bộ các checkbox/giá trị AUTO khác;
- auto lưu toàn bộ AutoSettings;
- bán/vứt/use item;
- tự mở bảng AUTO;
- OCR/pixel detection;
- production integration vào tool chính.

Sau khi v0.1 chứng minh runtime pickup route ổn định mới ghép `ENSURE_PICKUP_ON` vào luồng giao dịch -> ra bãi của tool chính.
