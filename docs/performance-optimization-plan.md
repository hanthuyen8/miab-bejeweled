# Kế hoạch tối ưu hiệu năng render — Seajeweled

Kế hoạch này tổng hợp từ điều tra thực tế (Instruments Time Profiler trên
bản desktop + Spector.js trên bản Web/WASM chạy Chrome). Thực hiện theo thứ
tự dưới đây, **đo lại bằng Spector.js sau mỗi bước** trước khi làm bước
tiếp theo — không làm gộp nhiều thay đổi cùng lúc để biết chính xác thay
đổi nào có tác dụng.

Không cần xây dựng module Profiler runtime riêng (từng bàn tới) — Instruments
(desktop) + Spector.js (Web/WebGL) đã đủ cho giai đoạn này.

## Baseline đã đo (đứng yên, không thao tác, bản Web qua Chrome)

- **73 draw call thật** (`drawArrays`) — khớp với tính toán từ code
  (`~82` lệnh `Image::draw()` logic khi đứng yên: 64 gem + 1 board + 1
  selector + 9 nút (3 nút × 3 draw/nút) + 6 score/time + 1 con trỏ chuột).
  **Số draw call bản thân không phải vấn đề**, không cần cố giảm bằng mọi giá.
- **367 WebGL command tổng** ≈ 73 draw × (3×`vertexAttribPointer` +
  1×`bindTexture` + 1×`drawArrays`) — tức trung bình ~5 lệnh WebGL cho mỗi
  draw call. `bindTexture` lặp lại vì 7 màu gem xen kẽ theo bố cục bàn cờ
  (do `DrawingQueue` chỉ sort theo z, không sort theo texture — xem mục 1).
- Trace desktop (Instruments) cho thấy code C++ của chính game chỉ chiếm
  ~1% self-time CPU — bottleneck không nằm ở logic gameplay, mà ở cách dữ
  liệu vẽ được tổ chức trước khi xuống GPU.

## Thứ tự thực hiện

### 1. Tối ưu sort của `DrawingQueue`

**File**: `include/go_drawingqueue.h`, `src/go_window.cpp`
(hàm `gameLoop()`, vòng lặp `for (qIt = mDrawingQueue.begin()...)`)

- Hiện tại: `DrawingQueue` kế thừa `std::multimap<float z, DrawingQueueOperation>`
  — mỗi `enqueueDraw()` gọi `insert()` → cấp phát heap node + rebalance cây
  đỏ-đen (`__tree_balance_after_insert`, xác nhận có mặt trong Instruments
  trace, xem "Phát hiện phụ" ở lịch sử điều tra).
- Đổi sang `std::vector<std::pair<float, DrawingQueueOperation>>`:
  - `draw(z, op)` → `push_back()` thay vì `insert()` — O(1) amortized,
    không cấp phát node riêng, contiguous memory (cache-friendly).
  - Thêm bước `std::sort()` theo `z` (dùng `std::stable_sort` nếu thứ tự
    giữa các phần tử cùng z cần giữ nguyên như hành vi `multimap` hiện tại)
    **ngay trước** vòng lặp render trong `gameLoop()`.
- **Cải thiện thêm (secondary sort key)**: trong cùng 1 giá trị z (ví dụ
  toàn bộ 64 gem đều vẽ ở `z=3`, xem `GameBoard.cpp` dòng 473), thứ tự vẽ
  giữa chúng **không ảnh hưởng hình ảnh cuối cùng** (gem không chồng lấn
  nhau, không cần alpha-blend theo thứ tự) — an toàn để sort phụ theo
  con trỏ texture (`op.mTexture`) nhằm gom các gem cùng màu vẽ liên tiếp,
  giảm số lần `bindTexture` thật sự cần thiết.
  ⚠️ Cẩn thận: `FloatingScore`/`ParticleSystem` có thể chồng lấn và cần
  alpha-blend đúng thứ tự — **kiểm tra bằng mắt sau khi đổi** để đảm bảo
  không có regression hình ảnh ở các hiệu ứng combo/particle.
- Đảm bảo API `DrawingQueue::draw()` giữ nguyên chữ ký để không phải sửa
  chỗ gọi (`GoSDL::Image::draw()`/`Window::enqueueDraw()`).

### 2. Texture atlas cho gem

**File mới**: gộp 7 file `media/gem*.png` (65×65 mỗi file: White, Red,
Purple, Orange, Green, Yellow, Blue) thành 1 texture atlas (ví dụ lưới
1 hàng × 7 cột = 455×65, hoặc lưới vuông hơn nếu cần mipmap-friendly).
Cân nhắc gộp cả `selector.png` (cùng kích thước 65×65) vào atlas luôn.

- **Cần sửa API**: `GoSDL::Image::draw()` (`go_image.h/cpp`) hiện luôn vẽ
  toàn bộ texture (`SDL_RenderCopyEx(..., srcrect=NULL, ...)`). Cần thêm
  tham số `SDL_Rect* srcRect` (mặc định `nullptr` = vẽ toàn bộ, giữ tương
  thích ngược cho mọi chỗ gọi khác) để vẽ đúng vùng con trong atlas.
- `DrawingQueueOperation` (`go_drawingqueue.h`) cần thêm field `SDL_Rect
  mSrcRect` để mang thông tin vùng atlas qua tới `SDL_RenderCopyEx` trong
  `go_window.cpp`.
- `GameBoard.cpp` (dòng ~330-473): đổi từ 7 `GoSDL::Image` field riêng
  (`mImgWhite`, `mImgRed`...) sang 1 texture atlas dùng chung + tính
  `srcRect` theo loại gem (`tSquare` trong `Square.h`).
- Sau bước này, kết hợp với mục 1: toàn bộ khối gem chia sẻ 1 texture —
  không còn cần sort theo texture cho riêng gem nữa (dù sort z/texture ở
  mục 1 vẫn có ích cho phần UI/button khác dùng texture riêng).

### 3. Nâng cấp SDL2 → SDL3 (rủi ro cao nhất — cần research trước khi làm)

**Ghi chú quan trọng**: trace Instruments đã bắt được cả `libSDL2-2.0.0.dylib`
**và** `libSDL3.0.dylib` cùng load trong process trên desktop — rất có thể
do Homebrew's `sdl2_image`/`sdl2_ttf` formula hiện tại dùng SDL3 làm lớp
compat nội bộ. Cần xác nhận lại trước khi kết luận gì thêm.

Trước khi migrate thật, **research riêng** (không code ngay):

1. Kiểm tra VitaSDK có SDL3 port ổn định chưa (`platform/vita`,
   `CMakeLists.txt` mục `VITA` — đây là target rủi ro nhất, có thể chưa
   sẵn sàng và trở thành blocker cho toàn bộ migration).
2. Kiểm tra Emscripten SDL3 port (`-sUSE_SDL=3` có được hỗ trợ ổn định
   chưa, so với `-sUSE_SDL=2` hiện dùng trong `CMakeLists.txt`).
3. SDL_image/SDL_mixer/SDL_ttf bản 3.x có API khác SDL2 tương ứng — không
   phải drop-in, cần rà lại toàn bộ `go_image.cpp`, `go_font.cpp`,
   `go_sound.cpp`, `go_music.cpp`.
4. Lý do cân nhắc: SDL3's renderer API có batching tốt hơn nội tại (giải
   quyết đúng vấn đề "5 lệnh WebGL/draw call" phát hiện ở mục baseline) —
   nhưng đây là thay đổi lớn, ảnh hưởng cả 4 platform target
   (Linux/macOS/Windows/Vita/Web), phải test kỹ trên từng target trước khi
   merge, không được làm 1 target bị crash/không build được.

### 4. Bitmap font cho số điểm/thời gian

**File**: `include/go_font.h`, `src/go_font.cpp`, `src/GameIndicators.cpp`

- Chỉ áp dụng cho `mImgScore`/`mImgTime` (chuỗi số + dấu `:`, charset nhỏ) —
  **không cần đổi** text menu/tiêu đề (`StateMainMenu`, `StateOptions`,
  `BaseButton` caption) vì render ít lần, không đáng tối ưu.
- `GameIndicators::regenerateScoreTexture()`/`updateTime()` (dòng 85-113)
  **đã có sẵn throttle tốt** (chỉ render lại khi giá trị đổi, không phải
  mỗi frame) — nên đây là mục có tác động thấp nhất trong 4 mục, đặt cuối
  cùng đúng như đề xuất.
- Cách làm: dựng sẵn 1 texture atlas chứa các glyph `0-9` + `:` (bitmap
  font), thay `SDL_ttf`/`renderText()` bằng tra cứu `srcRect` theo ký tự
  (cần API atlas từ mục 2 — có thể tái dùng cùng cơ chế `srcRect` đã thêm
  vào `Image::draw()`).

## Cách đo sau mỗi bước

1. Build lại target Web: `emcmake cmake -S . -B build-web && emmake cmake
   --build build-web -- -j"$(sysctl -n hw.ncpu)"`.
2. Mở trong Chrome, đứng yên không thao tác (cùng điều kiện với baseline).
3. Spector.js: capture 1 frame, ghi lại số `drawArrays` (draw call thật)
   và tổng số command.
4. So với baseline (73 draw / 367 command) — ghi lại kết quả từng bước để
   biết thay đổi nào thực sự có tác dụng.
5. Sau mỗi bước cũng build + chạy thử **cả 3 target còn lại** (desktop,
   Windows nếu có máy, Vita nếu có SDK) để đảm bảo không phá build khác —
   đặc biệt quan trọng ở mục 2 (đổi API `Image::draw()`) và mục 3 (đổi cả
   SDL version).
