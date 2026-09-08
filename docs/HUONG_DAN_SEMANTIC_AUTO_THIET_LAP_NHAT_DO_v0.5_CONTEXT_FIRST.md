# HƯỚNG DẪN KỸ THUẬT v0.5 — AUTO → THIẾT LẬP (CONTEXT-FIRST) → NHẶT ĐỒ → LƯU

## 1. Mục tiêu

Bản v0.5 sửa đúng lỗi live của v0.4 khi trên màn hình có **hai nút cùng chữ `Thiết lập`**:

```text
AUTO→THIẾT LẬP STEP2 BLOCKED:
[N=Button_-16406 A=Image_-16368/AutoFightGroup/TopIcon/MainUI POS=?]
[N=ButSetting A=IconTab/BottomIcon/MainUI POS=?]
```

Điểm quan trọng: hai candidate đã tự mang ngữ cảnh ancestor đủ mạnh để phân biệt, nên **không cần dựa vào position trong case live này**.

## 2. Đây vẫn không phải quét màn hình

Tool không screenshot, không OCR, không pixel matching, không SendInput và không click tọa độ.

Bridge đọc trực tiếp managed/IL2CPP UI object từ:

```text
FGStudio.LuaSystem.Base.UIObject.instances
```

Mỗi candidate có thể có:

```text
Name
Text
descendants
ancestors
ActiveInHierarchy
Interactable
Selected
```

Button dùng `HandleClickEvent()`; toggle dùng `set_Selected(true)` hoặc fallback `HandleSelectEvent(true)`.

## 3. Root cause v0.4

v0.4 đã làm đúng phần semantic exact-label và chỉ đọc position khi duplicate. Nhưng trên client thật, hai wrapper `UIButton` nói trên không expose được transform theo các getter thử nghiệm, nên:

```text
POS=?
```

Khi cả hai candidate đều thiếu position proof, spatial tie-break phải fail-closed.

Đây là hành vi an toàn, không phải crash.

## 4. Dấu vân tay semantic live đã chứng minh

Candidate đúng trong menu AUTO:

```text
Name      = Button_-16406   (tên số động, KHÔNG được hard-code)
Ancestors = Image_-16368/AutoFightGroup/TopIcon/MainUI
```

Candidate sai ở giao diện dưới:

```text
Name      = ButSetting
Ancestors = IconTab/BottomIcon/MainUI
```

Không hard-code `Button_-16406` hay `Image_-16368` vì suffix số có thể đổi sau restart.

Dấu vân tay ổn định cần dùng là:

```text
AutoFightGroup
TopIcon
```

và candidate sai đặc trưng bởi:

```text
BottomIcon
```

## 5. Selection policy v0.5

Thứ tự mới:

```text
1. Exact semantic label = "Thiết lập"
2. Nếu chỉ 1 candidate -> dùng ngay
3. Nếu duplicate -> context tie-break:
      candidate phải có ancestor segment exact:
      AutoFightGroup
      AND TopIcon
4. Nếu đúng 1 candidate context -> chọn ngay
5. Chỉ nếu context vẫn ambiguous -> mới thử normalized position fallback v0.4
6. Nếu vẫn không UNIQUE -> BLOCKED, không chọn đại
```

Pure logic nằm trong:

```text
src/auto_menu_ui_logic.h
```

Function mới:

```cpp
IsAutoFightMenuSettingsContext(...)
SelectSettingsChoiceContext(...)
```

`SelectSettingsChoiceSpatial(...)` giờ gọi context trước, spatial sau.

## 6. Vì sao cách này nhẹ hơn v0.4

Ancestor labels đã được thu thập cho chính candidate `Thiết lập` trong single-pass scan. Vì vậy case live chỉ cần string segment compare trên dữ liệu đã có.

Không phát sinh thêm:

```text
RectTransform getter
Screen.width/height
WorldToScreenPoint
snapshot trước/sau
```

Position code vẫn giữ làm fallback cho layout khác, nhưng **case hiện tại không phải trả chi phí position**.

## 7. Không dùng substring mù

Context dùng segment exact theo chuỗi ancestor `/`-separated, không chỉ tìm substring tự do.

Ví dụ accepted:

```text
Image_x/AutoFightGroup/TopIcon/MainUI
```

Không suy luận từ tên động `Button_-xxxxx`.

Nếu có hai candidate đều mang `AutoFightGroup/TopIcon`, selector vẫn trả Ambiguous.

## 8. Chuỗi cuối

```text
AUTO HUD
→ semantic exact AUTO
→ HandleClickEvent()
→ chờ menu hiện
→ single-pass UIObject.instances
→ exact "Thiết lập"
→ duplicate?
    → AutoFightGroup + TopIcon context
    → position chỉ là fallback
→ semantic invoke Thiết lập
→ proof AutoFightUI bằng TogglePickUpTab
→ chọn tab Nhặt đồ
→ TogPickUpEquipment = ON
→ Lưu thiết lập
→ runtime proof ON
→ persisted PICKITEM.IsOn = ON
```

## 9. Phần Nhặt đồ không thay đổi

v0.5 chỉ sửa tầng chọn `Thiết lập`. Không thay đổi route đã live-prove:

```text
TogglePickUpTab
TogPickUpEquipment
ButtonSaveSettings / Lưu thiết lập
AutoSettings schema 4.1
PICKITEM.IsOn
```

Nguyên tắc fail-closed và không ghi/click lần hai vẫn giữ nguyên.

## 10. Regression tests bắt buộc

Test live-case:

```text
Thiết lập A -> AutoFightGroup/TopIcon/MainUI
Thiết lập B -> BottomIcon/MainUI
=> UNIQUE A
```

Test fail-closed:

```text
Thiết lập A -> AutoFightGroup/TopIcon
Thiết lập B -> Other/TopIcon
=> không được suy đoán rộng chỉ bằng TopIcon
```

Spatial fallback vẫn có test riêng với context không rõ và normalizedY trên/dưới.

## 11. Quy tắc cho AI/kỹ sư tiếp theo

- Không thay logic Nhặt đồ đang ổn nếu task chỉ liên quan mở AUTO/Thiết lập.
- Không hard-code suffix số runtime như `-16406`.
- Ưu tiên semantic context đã live-prove trước geometry.
- Geometry chỉ dùng fallback.
- Không OCR/pixel nếu managed UI object vẫn đọc được.
- Không chọn candidate đầu tiên khi ambiguous.
- Sau invoke phải có proof trạng thái tiếp theo.
