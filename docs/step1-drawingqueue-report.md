# Báo cáo Step 1 — Tối ưu sort của `DrawingQueue`

Tham chiếu kế hoạch: [`performance-optimization-plan.md`](performance-optimization-plan.md) — mục **1**.

## Mục tiêu

Loại bỏ chi phí `std::multimap::insert()` (cấp phát heap node + rebalance cây
đỏ-đen mỗi lần enqueue) và gom các lệnh vẽ cùng texture để giảm số lần
`bindTexture`/draw call thật xuống GPU.

## Thay đổi

### `include/go_drawingqueue.h`
- `DrawingQueue` đổi từ `private std::multimap<float, DrawingQueueOperation>`
  sang `private std::vector<std::pair<float, DrawingQueueOperation>>`
  → bộ nhớ liền mạch (cache-friendly), không cấp phát node riêng.
- `draw(z, op)`: giữ nguyên chữ ký, đổi `insert()` → `push_back()`
  (O(1) amortized).
- Thêm `sort()`: `std::stable_sort` với **khóa chính = z**, **khóa phụ =
  con trỏ texture** (`op.mTexture`) để gom các draw cùng texture nằm liền
  nhau. Dùng `stable_sort` để giữ thứ tự chèn gốc giữa các phần tử có cùng
  `(z, texture)` → bảo toàn thứ tự alpha-blend cho hiệu ứng chồng lấn cùng
  texture.
- `DrawingQueueIterator` cập nhật sang iterator của vector.

### `src/go_window.cpp` (`gameLoop()`)
- Gọi `mDrawingQueue.sort()` một lần ngay trước vòng lặp render.
- `begin()/end()/size()/clear()` và `qIt->second` không đổi → không phải
  sửa chỗ gọi khác.

## Kết quả đo (Spector.js, Web/Chrome, đứng yên — cùng điều kiện baseline)

| Chỉ số               | Baseline | Sau Step 1 | Thay đổi |
|----------------------|:--------:|:----------:|:--------:|
| Draw call (`drawArrays`) | 73   | **27**     | −63%     |
| Tổng WebGL command       | 367  | **150**    | −59%     |

Draw call giảm mạnh (không chỉ giảm command như dự đoán ban đầu) vì renderer
của SDL2 **tự batch** các lệnh vẽ liên tiếp cùng texture thành 1 `drawArrays`.
Khóa sort phụ theo con trỏ texture đã gom 64 gem (7 màu, vốn rải rác theo bố
cục bàn cờ) thành các cụm cùng màu liền nhau → nhiều gem cùng màu nhập được
vào chung một draw call.

## Regression đã gặp và cách xử lý

**Hiện tượng:** text đếm ngược thời gian nhấp nháy / bị `timeBackground.png`
đè lên (giống z-fighting).

**Nguyên nhân:** `GameIndicators` vẽ background và số **cùng z=2**
(`mImgTimeBackground` và `mImgTime`; tương tự cặp score). Trước đây thứ tự
đúng nhờ `multimap` giữ thứ tự chèn (background chèn trước → vẽ trước → số
đè lên). Khóa sort phụ theo texture ở Step 1 sắp lại theo địa chỉ con trỏ
texture nên thứ tự background/số bị hoán đổi ở một số frame.

**Sửa** (`src/GameIndicators.cpp`): cho số điểm/thời gian dùng **z=3** (trên
background z=2). Đây đúng là quy ước codebase đã dùng sẵn ở `BaseButton`
(background `z`, icon `z+1`, caption `z+2`) để xử lý các lớp chồng nhau —
không phụ thuộc thứ tự chèn.

**Phạm vi kiểm tra:** `BaseButton` (mỗi nút 3 draw) đã layer đúng bằng
z/z+1/z+2 nên không bị. Cặp bg/số ở `GameIndicators` là chỗ duy nhất vi phạm,
đã sửa cả score lẫn time. Fix không ảnh hưởng thắng lợi 27 draw / 150 command
(2 số ở z=3 chỉ tạo nhóm texture nhỏ riêng, không đụng batching của khối gem).

## Bài học rút ra

Sau Step 1, thứ tự vẽ giữa các phần tử **cùng z** không còn là thứ tự chèn —
bất kỳ hai phần tử chồng lấn, khác texture nào cũng **phải** có z khác nhau.
Đây là invariant `BaseButton` đã giả định sẵn; các code vẽ UI mới cần tuân theo.

## Trạng thái

- [x] Đổi `multimap` → `vector` + `stable_sort`
- [x] Build Web (Emscripten) compile + link thành công
- [x] Đo lại bằng Spector.js: 27 draw / 150 command
- [x] Sửa regression z-order ở score/time indicator
- [x] Kiểm tra mắt: hiệu ứng combo/particle/FloatingScore bình thường;
      text thời gian hết nhấp nháy
- [ ] Build + smoke test các target còn lại (desktop macOS hiện lỗi link
      `SDL2main` sẵn có của môi trường — không liên quan thay đổi này; Windows/Vita
      chưa test do thiếu máy/SDK)

Sẵn sàng chuyển sang **Step 2 — texture atlas cho gem**.
