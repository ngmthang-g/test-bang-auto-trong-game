# HƯỚNG DẪN KỸ THUẬT — AUTO → THIẾT LẬP → NHẶT ĐỒ → NHẶT VẬT PHẨM → LƯU

## 1. Mục tiêu tài liệu

Tài liệu này mô tả đầy đủ cơ chế đã nghiên cứu và triển khai trong `ThanLongAutoSettingsProbe` để một AI/kỹ sư khác có thể tiếp tục phát triển mà không phải nghiên cứu lại từ đầu.

Mục tiêu cuối của chuỗi là thao tác bảng AUTO của client Thần Long **không dùng tọa độ click, không OCR, không quét pixel và không chiếm chuột**:

```text
AUTO ngoài HUD
  → Thiết lập trong menu AUTO
  → bảng Thiết Lập AUTO
  → tab Nhặt đồ
  → Nhặt vật phẩm = ON
  → Lưu thiết lập
  → proof runtime ON + proof persisted ON
```

Baseline ổn định của phần Nhặt đồ là **v0.2 Save Proof**. Các bản sau chỉ được phép thêm tầng mở bảng ở phía trước, không được phá route Nhặt đồ đã live-prove.

---

## 2. Điều quan trọng nhất: đây KHÔNG phải quét màn hình

Tool không chụp ảnh game, không OCR chữ, không tìm icon theo pixel.

Bridge chạy trong process game và đọc trực tiếp các object UI managed/IL2CPP đang sống từ:

```text
FGStudio.LuaSystem.Base.UIObject.instances
```

Từ mỗi UI object, tool có thể đọc hoặc suy ra:

- class runtime (`UIButton`, `UIToggle`, ...)
- `Name`
- `Text`
- descendants qua `get_CoreChildren` / `get_Children`
- ancestor qua `get_Parent`
- `get_ActiveInHierarchy()`
- `get_Interactable()`
- với toggle: `get_Selected()`
- với toggle: `set_Selected(Boolean)`
- với toggle: `HandleSelectEvent(Boolean)`
- với button: `HandleClickEvent()`

Do đó cơ chế là **semantic UI object discovery + method invocation**, không phải auto-click màn hình.

---

## 3. Kiến trúc tổng thể

### 3.1 Controller EXE

`ThanLongAutoSettingsProbe.exe` làm các việc:

- quét cửa sổ game/PID có `GameAssembly.dll`
- tạo/kết nối shared mapping cho PID
- gắn bridge DLL vào thread cửa sổ game
- gửi command tới bridge
- nhận response/log
- chờ giữa các bước UI khi cần

Controller có thể `Sleep(500)` giữa bước mở `AUTO` và bước chọn `Thiết lập`.

**Không Sleep trong bridge/game thread.**

### 3.2 Bridge DLL

`ThanLongAutoSettingsBridge.dll` chạy trong process game, resolve IL2CPP exports từ `GameAssembly.dll`, sau đó thực hiện read/probe/invoke trên thread game.

Các action semantic phải được thực hiện qua object/method game đã resolve, không dùng `SendInput`, `mouse_event`, tọa độ desktop hay OCR.

### 3.3 Shared mapping

Controller và bridge trao đổi request/response bằng named shared mapping theo PID.

Bridge lifecycle phải được shutdown sạch trước khi đổi PID/thoát tool để tránh mapping cũ gây false error "đã có một Probe khác".

---

## 4. Cách đọc AutoSettings

Tool đã live-prove đọc được cấu hình từ `LeaderRoleData`.

Các route đọc được thử theo hướng fail-safe:

```text
LeaderRoleData.get_AutoSettings
LeaderRoleData.AutoSettings
LeaderRoleData.<AutoSettings>k__BackingField
LeaderRoleData.autoSettings
```

Nếu không có trực tiếp thì thử `LeaderRoleData.roleData` rồi tìm AutoSettings trên backing object.

Schema client đã xác nhận:

```text
version#AUTOTRAIN#PICKITEM#UTILITIES#REGENE#PET#AUTOPK#FUBEN
```

Version hiện dùng:

```text
4.1
```

Segment delimiter:

```text
#
```

Field delimiter:

```text
|
```

---

## 5. Schema PICKITEM quan trọng

`PICKITEM` được serialize theo thứ tự:

1. `IsOn`
2. `PickRanger`
3. `IsFilterItem`
4. `FilterItemSettings`
5. `AutoEatX2`
6. `AutoUsingItem`
7. `UsingItemList`
8. `IsAutoDropItem`
9. `DropItemSettings`

Vì vậy:

```text
PICKITEM field[0] == 1  → persisted Nhặt vật phẩm ON
PICKITEM field[0] == 0  → persisted Nhặt vật phẩm OFF
```

Default radius quan sát được là `500`, khớp UI game.

Không dùng persisted AutoSettings làm runtime proof. Runtime checkbox và persisted config là hai lớp khác nhau.

---

## 6. Runtime proof cho Nhặt vật phẩm

Control live đã xác định trên client thật:

```text
Name = TogPickUpEquipment
Text = Nhặt vật phẩm
Ancestor chứa TabPickUp / AutoFightUI
```

Tab điều hướng bên trái là object khác:

```text
Name = TogglePickUpTab
Text = Nhặt đồ
```

Hai object này tuyệt đối không được gom chung.

### 6.1 Chọn tab Nhặt đồ

Tìm UNIQUE:

```text
TogglePickUpTab / Nhặt đồ
```

Nếu `get_Selected() == true`:

```text
NO-OP
```

Nếu OFF:

```text
ưu tiên set_Selected(true)
```

Nếu setter không tồn tại thì mới fallback:

```text
HandleSelectEvent(true)
```

Sau invoke bắt buộc:

```text
get_Selected() == true
```

Nếu read-back không ON thì dừng, không thử ghi lần hai.

### 6.2 Bật Nhặt vật phẩm

Sau khi tab Nhặt đồ đã selected, re-enumerate UI và tìm đúng:

```text
TogPickUpEquipment
Text = Nhặt vật phẩm
Ancestor = .../TabPickUp/.../AutoFightUI
```

Nếu runtime đã ON:

```text
NO-OP
```

Nếu OFF:

```text
set_Selected(true)
```

Fallback chỉ khi setter không có:

```text
HandleSelectEvent(true)
```

Sau đó bắt buộc `get_Selected() == ON`.

---

## 7. Tại sao phải bấm Lưu thiết lập

Live test chứng minh tick checkbox runtime chưa đủ để persisted `PICKITEM.IsOn` đổi.

Sau khi runtime Nhặt vật phẩm ON, tool phải tìm đúng nút:

```text
Text = Lưu thiết lập
Ancestor chứa AutoFightUI
Class = UIButton
```

Sau đó invoke:

```text
UIButton.HandleClickEvent()
```

Rồi đọc lại AutoSettings.

PASS cuối chỉ khi đồng thời:

```text
runtime TogPickUpEquipment = ON
PERSISTED PICKITEM.IsOn = ON
```

Nếu Save đã dispatch nhưng persisted vẫn OFF:

```text
BLOCKED
không click Save lần hai trong cùng command
```

Đây là nguyên tắc fail-closed quan trọng.

---

## 8. Mở bảng AUTO bằng semantic UI

### 8.1 Bước AUTO ngoài HUD

Tool quét active `UIButton` và `UIToggle`, tìm exact semantic label:

```text
AUTO
```

Không nhận label AUTO nằm trong `AutoFightUI`.

Nếu control là `UIButton`:

```text
HandleClickEvent()
```

Nếu là `UIToggle`:

```text
set_Selected(true)
```

fallback:

```text
HandleSelectEvent(true)
```

Sau step 1 controller chờ khoảng `500 ms` để menu AUTO hiện đầy đủ.

### 8.2 Bước Thiết lập

Sau khi menu AUTO hiện, trên client có thể tồn tại đồng thời hai control cùng chữ:

```text
Thiết lập trong menu AUTO phía trên
Thiết lập giao diện chính ở góc dưới
```

Bản v0.3 chỉ match exact chữ `Thiết lập`, nên có thể chọn nhầm hoặc ambiguous.

v0.4 sửa đúng vấn đề này bằng **single-pass semantic scan + spatial tie-break**.

---

## 9. v0.4 — cơ chế chọn đúng Thiết lập nhẹ và nhanh

### 9.1 Không snapshot trước/sau

Không dùng snapshot toàn bộ UI trước khi bấm AUTO rồi diff sau.

Lý do:

- cần hai lượt snapshot
- phải lưu nhiều object hơn
- không cần thiết cho case này

### 9.2 Một lượt duyệt dictionary

Sau khi AUTO đã mở, bridge duyệt `UIObject.instances` một lần để tìm candidate `Thiết lập`.

Luồng:

```text
UIObject.instances
  → chỉ UIButton / UIToggle
  → chỉ ActiveInHierarchy
  → đọc Text
  → nếu Text chưa exact thì tìm exact label trong descendants
  → chỉ giữ label exact "Thiết lập"
  → đọc ancestor cho đúng candidate
  → loại candidate thuộc AutoFightUI
```

Nếu chỉ còn đúng một candidate:

```text
chọn ngay
không đọc vị trí
```

Do đó case bình thường vẫn cực nhẹ.

### 9.3 Chỉ khi có duplicate mới đọc vị trí

Nếu có >= 2 `Thiết lập`, bridge mới đọc position của chính các candidate đó.

Không đọc RectTransform/position của toàn bộ UI.

Đây là điểm tối ưu chính.

---

## 10. Cách lấy vị trí mà không quét pixel

Position vẫn được đọc từ managed UI object, không liên quan ảnh màn hình.

Bridge thử lấy transform bằng các getter an toàn/read-only:

```text
get_transform
get_Transform
get_RectTransform
```

Nếu wrapper không trả transform trực tiếp, thử lấy GameObject:

```text
get_gameObject
get_GameObject
get_CoreGameObject
```

rồi lấy transform từ GameObject.

Từ transform đọc:

```text
get_position()
```

Kết quả là `UnityEngine.Vector3`.

---

## 11. Chuẩn hóa vị trí theo độ phân giải

Không dùng pixel cố định như `x=500, y=200`.

Bridge resolve:

```text
UnityEngine.CoreModule
UnityEngine.Screen
Screen.get_width()
Screen.get_height()
```

Sau đó:

```text
normalizedX = screenX / Screen.width
normalizedY = screenY / Screen.height
```

Unity screen coordinate dùng gốc dưới:

```text
Y = 0.0  → đáy màn hình
Y = 1.0  → đỉnh màn hình
```

v0.4 dùng ngưỡng upper region:

```text
normalizedY >= 0.55
```

Ý nghĩa: khi có hai `Thiết lập`, chỉ candidate nằm ở vùng trên màn hình được ưu tiên.

Nút `Thiết lập` góc dưới có normalizedY thấp nên bị loại.

Ngưỡng là normalized ratio nên thay đổi độ phân giải không làm mất ý nghĩa như pixel tuyệt đối.

---

## 12. Fallback chuyển world position sang screen position

Trong phần lớn Screen Space Overlay UI, `Transform.position` đã gần screen pixel.

Nếu giá trị không nằm trong khoảng hợp lý so với `Screen.width/height`, bridge thử fallback:

```text
UnityEngine.RectTransformUtility.WorldToScreenPoint(null, worldPosition)
```

Rồi mới normalize.

Nếu cả hai route không tạo được vị trí hợp lệ:

```text
POS = UNKNOWN
```

Tool không đoán.

---

## 13. Spatial selection policy

Pure selection logic nằm trong:

```text
src/auto_menu_ui_logic.h
```

Candidate có thêm:

```cpp
bool hasNormalizedPosition;
float normalizedX;
float normalizedY;
```

Logic:

```text
1. SelectSettingsChoice bằng semantic exact label.
2. Nếu None → None.
3. Nếu Unique → dùng ngay, không spatial cost.
4. Nếu Ambiguous:
     chỉ candidate có position proof
     và normalizedY >= 0.55
5. Nếu đúng 1 candidate upper → Unique.
6. Nếu 0 hoặc >1 → vẫn Ambiguous, fail-closed.
```

Không "chọn đại candidate đầu tiên".

---

## 14. Vì sao cơ chế này nhẹ

Chi phí bình thường:

```text
1 dictionary pass sau AUTO mở
+ semantic text matching
```

Chi phí position chỉ xảy ra khi duplicate:

```text
2 candidate Thiết lập
→ đọc transform/position cho đúng 2 candidate
```

Không có:

```text
screenshot
OCR
pixel scanning
image matching
full before/after snapshot
mouse movement
SendInput
```

So với render game mỗi frame, vài managed getter trên 1–2 candidate là rất nhỏ.

---

## 15. UIObject.instances — cấu trúc runtime hiện đang dùng

Bridge đọc dictionary managed bằng layout đã live-test trong client hiện tại.

Các offset đang dùng:

```text
dictionary + 0x18 → entries array
dictionary + 0x20 → count
entries + 0x18    → capacity/length
entry base        → 0x20 + i * 0x18
entry + 0x10      → UIObject pointer
```

Có hard limits để tránh đọc object layout hỏng:

```text
count <= 32768
capacity <= 32768
```

Nếu dictionary không hợp lệ, fail ngay.

Không nên copy các offset này sang client/version khác mà không verify.

---

## 16. IL2CPP exports bridge đang sử dụng

Nhóm chính:

```text
il2cpp_domain_get
il2cpp_domain_assembly_open
il2cpp_assembly_get_image
il2cpp_class_from_name
il2cpp_class_get_method_from_name
il2cpp_class_get_parent
il2cpp_method_get_flags
il2cpp_method_get_param_count
il2cpp_method_get_param
il2cpp_method_get_return_type
il2cpp_type_get_name
il2cpp_runtime_invoke
il2cpp_object_unbox
il2cpp_object_get_class
il2cpp_class_get_field_from_name
il2cpp_field_get_type
il2cpp_field_get_value
il2cpp_field_static_get_value
il2cpp_class_from_type
il2cpp_class_is_valuetype
il2cpp_class_is_assignable_from
il2cpp_string_length
il2cpp_string_chars
il2cpp_image_get_class_count
il2cpp_image_get_class
il2cpp_class_get_name
il2cpp_free
```

Method invocation luôn kiểm tra exception trả về từ `il2cpp_runtime_invoke`.

---

## 17. Exact method validation

Helper `ExactMethod` kiểm tra:

- method name
- parameter count
- static/instance flag
- type parameter 0 khi cần

Ví dụ toggle mutation:

```text
set_Selected(Boolean)
HandleSelectEvent(Boolean)
```

Không gọi method chỉ vì trùng tên nhưng signature sai.

---

## 18. Nguyên tắc fail-closed

Đây là nguyên tắc xuyên suốt tool.

Không mutate khi:

- không tìm thấy candidate
- có nhiều candidate mà không phân giải được
- control không active/interactable
- thiếu method semantic đã verify
- read-back không khớp
- schema AutoSettings mismatch
- Save không tạo persisted proof
- position duplicate không đọc được
- spatial filter vẫn trả nhiều candidate

Mỗi command chỉ dispatch tối đa một action tương ứng; không spam click/callback lần hai để "thử vận may".

---

## 19. Chuỗi hoàn chỉnh mong muốn

Sau khi v0.4 live-prove phần `AUTO → Thiết lập`, chuỗi hoàn chỉnh là:

```text
STATE 1: Ensure AutoSettings panel
  AutoFightUI đã mở?
    YES → skip AUTO/menu
    NO  → semantic AUTO
          → wait controller ~500ms
          → single-pass exact Thiết lập
          → duplicate? spatial upper-region tie-break
          → HandleClickEvent / toggle semantic route
          → proof AutoFightUI active

STATE 2: Ensure Nhặt đồ tab
  TogglePickUpTab UNIQUE
  → Selected ON hoặc set_Selected(true)
  → get_Selected read-back ON

STATE 3: Ensure Nhặt vật phẩm
  TogPickUpEquipment UNIQUE dưới TabPickUp
  → ON thì no-op
  → OFF thì set_Selected(true)
  → get_Selected read-back ON

STATE 4: Persist
  nếu PICKITEM.IsOn đã ON và không mutation → no-op
  nếu cần lưu:
      Button Lưu thiết lập UNIQUE
      → HandleClickEvent()
      → read AutoSettings
      → PICKITEM.IsOn == ON

PASS cuối:
  runtime pickup ON
  persisted pickup ON
```

---

## 20. Tích hợp vào luồng sau giao dịch

Khi ghép vào tool lớn, không chạy ngay khi vừa gửi `Trade.Done`.

Nên chờ:

```text
trade session đóng thật
→ fresh bag snapshot
→ ENSURE AUTO SETTINGS / PICKUP ON
→ proof runtime + persisted
→ đóng/để bảng tùy logic
→ đi map train
→ Auto Train
```

Mục đích: mọi transition phải dựa trên state proof, không dựa trên click đã gửi.

---

## 21. Những cách KHÔNG nên quay lại

### Không dùng OCR/screenshot

Không cần, chậm hơn và phụ thuộc resolution/theme.

### Không dùng tọa độ click

Không cần. Position trong v0.4 chỉ dùng **phân loại candidate**, không dùng để click.

### Không gửi `SendInput`

Mục tiêu là không chiếm chuột người dùng.

### Không patch AutoSettings string rồi coi là runtime ON

Persisted config không phải runtime checkbox proof.

### Không gọi Lua gameplay action mù

Các lỗi sell/drop trước đây cho thấy Lua route có thể phụ thuộc context/live object/main thread. UI toggle/button semantic route đã live-prove đơn giản hơn cho setting này.

---

## 22. Regression tests quan trọng

`tests/auto_menu_ui_logic_test.cpp` phải giữ case:

```text
Candidate A: Thiết lập menu AUTO, normalizedY ~0.82
Candidate B: Thiết lập góc dưới, normalizedY ~0.08
```

Expected:

```text
SelectSettingsChoiceSpatial(..., 0.55) → Candidate A UNIQUE
```

Và case không có position:

```text
2 exact labels + no position proof
→ Ambiguous
```

Không được đổi thành "first wins".

Các test khác phải tiếp tục pass:

- AutoSettings parser
- ui_logic
- pickup_ui_logic
- auto_menu_ui_logic
- protocol layout

---

## 23. Log live cần quan sát

Khi có duplicate `Thiết lập`, log v0.4 nên cho biết candidate position dạng:

```text
[N=... T=Thiết lập A=... NX=0.xxx NY=0.xxx]
```

Candidate đúng phía trên cần có `NY >= 0.55`.

Candidate góc dưới dự kiến `NY` thấp.

PASS cuối bước mở bảng có dạng ý nghĩa:

```text
AUTO STEP2 DISPATCH ...
SETTINGS PICK: single-pass exact label; duplicate tie-break=normalized upper region
...
OPEN PASS: AutoFightUI active + TogglePickUpTab UNIQUE
```

Nếu thấy `POS=?`, route position của class đó chưa được prove; không nên hạ safety gate bằng cách chọn bừa.

---

## 24. Build Windows x64

Repository dùng CMake.

```powershell
cmake -S . -B build -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Output chính:

```text
build/Release/ThanLongAutoSettingsProbe.exe
build/Release/ThanLongAutoSettingsBridge.dll
```

EXE và DLL phải đi cùng một build/version.

Nếu game chạy Administrator, tool cũng nên chạy cùng integrity level.

---

## 25. Source files quan trọng

```text
src/protocol.h
    shared protocol/commands

src/bridge.cpp
    IL2CPP resolution
    UIObject.instances scan
    semantic invoke
    runtime/persisted proof
    spatial position resolver

src/main.cpp
    Windows controller UI
    session/bridge lifecycle
    AUTO→Thiết lập two-step orchestration

src/auto_menu_ui_logic.h
    pure semantic + spatial selection logic

src/pickup_ui_logic.h
    Nhặt đồ / Nhặt vật phẩm / Save candidate logic
    PHẢI GIỮ ỔN ĐỊNH nếu không có lý do rõ ràng

src/auto_settings_parser.*
    schema/parser AutoSettings
```

---

## 26. Baseline/version lineage

### v0.2 Save Proof

Đã live-prove:

```text
mở sẵn Thiết Lập AUTO
→ Nhặt đồ
→ Nhặt vật phẩm ON
→ Lưu thiết lập
→ runtime + persisted proof
```

Đây là baseline không được phá.

### v0.3 Semantic Open

Thêm:

```text
AUTO HUD → Thiết lập
```

Nhưng exact label `Thiết lập` có thể đụng nút góc dưới.

### v0.4 Upper Region

Giữ nguyên pickup v0.2, giữ semantic AUTO v0.3 và sửa duy nhất ambiguity `Thiết lập` bằng:

```text
single pass
→ exact semantic label
→ spatial read chỉ khi duplicate
→ normalized upper-region tie-break
→ fail-closed
```

---

## 27. Quy tắc cho AI tiếp tục phát triển

1. Không sửa `pickup_ui_logic` chỉ để giải quyết lỗi mở menu.
2. Không thay semantic method bằng click tọa độ khi chưa chứng minh semantic route thất bại.
3. Mọi selector có nhiều candidate phải fail-closed hoặc có proof phân giải rõ ràng.
4. Position chỉ là metadata để chọn object, không phải tọa độ click.
5. Không quét position toàn bộ UI khi semantic filter có thể thu hẹp trước.
6. Nếu thêm candidate context, ưu tiên `Name/Text/ancestor/sibling` trước khi tăng scan cost.
7. Mỗi mutation phải có read-back/state proof.
8. Persisted và runtime là hai lớp proof khác nhau.
9. Build EXE và DLL cùng commit.
10. Test live mới là bằng chứng cuối cho UI runtime behavior.

---

## 28. Tóm tắt một câu

Cơ chế đúng là:

```text
đọc object UI thật trong game → lọc semantic → chỉ khi trùng tên mới đọc vị trí normalized của vài candidate → invoke method của đúng object → đọc state lại để chứng minh
```

Nó **không quét màn hình**, không OCR, không click pixel và không chiếm chuột.
