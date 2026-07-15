# Báo cáo refactor — Tổ chức lại asset + hằng số `Assets.h` / `ZOrder.h`

Refactor dọn dẹp sau [Step 2](step2-texture-atlas-report.md). Không đổi hành vi
game — chỉ tổ chức lại file và gom chuỗi/hằng số. Xem cấu trúc & quy ước ở
[repo-structure.md](repo-structure.md).

## Động cơ

1. Sau Step 2, 16 PNG sprite lẻ **không còn nạp lúc runtime** (đã gộp vào
   `atlas.png`) nhưng vẫn bị preload vào web bundle → thừa. Không xóa được vì
   project TexturePacker (`.tps`) trỏ tới chúng.
2. Đường dẫn `"media/..."` **hard-code rải rác** khắp code → khó tìm usage,
   dễ typo, đổi path phải sửa nhiều chỗ.
3. Giá trị **z-order** cũng là số ma thuật nằm rải trong các lệnh `draw()` →
   khó nắm layer stack, khó chỉnh.

## Việc đã làm

### 1. Tách "source art" khỏi "runtime asset"
- **`assets/sprites/`** (mới): 16 PNG source — input TexturePacker, **không**
  ship, **không** preload web.
- **`media/`**: chỉ còn runtime asset, chia thư mục con `images/ fonts/
  sounds/ menu/` (+ `atlas.png`/`atlas.json` ở gốc). Đổi tên `stateMainMenu/`
  → `menu/`. Dùng `git mv` để giữ history.
- Kết quả: sprite source biến mất khỏi web bundle (xác nhận bằng grep
  `seajeweled.js`), `media/` gọn.

### 2. `include/Assets.h` — gom media-string
- Namespace `Assets` (đường dẫn file) + `Assets::Sprite` (tên frame atlas).
- Thay **toàn bộ** chuỗi `"media/..."` trong code (grep xác nhận chỉ còn
  trong `Assets.h`).

### 3. `include/ZOrder.h` — gom z-order
- Namespace `Z` + con `Z::Menu` / `Z::Howto` / `Z::Button`, tên ngữ nghĩa
  cho từng lớp. Thay toàn bộ z literal trong `draw()` khắp 10 file.
- Layer stack in-game: `Board(0)` → `UIPanel(2)` →
  `Gem/UIText/ScoreTable(3)` → `Selector(4)` → `Particle(7)` → `Hint(10)`
  → `FloatingScore(80)` → `Cursor(999)`.

### 4. Khép migration atlas
- `JewelGroupAnim` (gem animation ở menu chính) trước còn nạp gem PNG lẻ →
  nay lấy từ atlas. Sau bước này **không còn code nào nạp sprite source lúc
  runtime**, an toàn để dời chúng sang `assets/`.

### 5. Cập nhật TexturePacker `.tps`
- Source: `../media/*.png` → `../assets/sprites/*.png`.
- Output: ghi thẳng `../media/atlas.png` + `../media/atlas.json` (lần Publish
  sau khỏi copy tay).

## Verify

- [x] Build Web (Emscripten) compile + link OK sau mỗi nhịp.
- [x] Web bundle chứa path mới (`media/sounds/…`, `media/fonts/…`, …), không
      còn path phẳng cũ, không còn sprite source.
- [x] Kiểm tra mắt: mọi màn hình (menu, in-game, options, how-to-play, score
      table) nạp asset đúng; thứ tự lớp (z-order) đúng — số điểm/thời gian
      trên nền panel, selector/hint/particle/floating score/cursor đúng lớp.

## Quy ước mới (bắt buộc theo)

- **Không** hard-code `"media/..."` → dùng `Assets::` / `Assets::Sprite::`.
- **Không** viết số z trong `draw()` → dùng `Z::...`.
- Sprite atlas cấu hình qua `GoSDL::TextureAtlas` + `Assets::Sprite::`.
- Thêm sprite mới: bỏ vào `assets/sprites/`, Publish `.tps`, thêm hằng số vào
  `Assets::Sprite`.

Chi tiết đầy đủ: [repo-structure.md](repo-structure.md).

## Còn lại

- 2 file `texture-packer/atlas_output.{png,json}` (export cũ, stale) nên xóa —
  `.tps` giờ ghi thẳng vào `media/`, `media/atlas.*` là bản chính.
- Lộ trình chính tiếp theo: **Step 4 — bitmap font** cho số điểm/thời gian.
