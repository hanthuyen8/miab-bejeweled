# Báo cáo Step 4 — Bitmap font cho số điểm/thời gian

> **Đã bị thay thế bởi [step5-bitmap-font-atlas-report.md](step5-bitmap-font-atlas-report.md).**
> `BitmapNumber`, `Assets::Sprite::Digit*`/`Colon` và thư mục `assets/glyphs/lcd/`
> mô tả dưới đây **không còn tồn tại** — toàn bộ font (không riêng chữ số) nay đi
> qua `BitmapFont` + `media/fonts.json`. Giữ lại làm ghi chép lịch sử.

Tham chiếu kế hoạch: [`performance-optimization-plan.md`](performance-optimization-plan.md) — mục **4**.
Tiếp nối [Step 2](step2-texture-atlas-report.md) (texture atlas) và bản
[refactor Assets/ZOrder](refactor-assets-zorder-report.md).

## Ý tưởng

Số điểm/thời gian trước đây render bằng `SDL_ttf` → **mỗi chuỗi một texture
riêng** → phá batch và là phần lớn draw call "text" còn lại sau Step 2. Thay
bằng **bitmap font trong chính atlas**: mỗi glyph `0-9`/`:` là một sprite của
`atlas.png`, vẽ ghép lại thành số → cùng texture với gem/UI → **batch chung,
không rasterize mỗi frame, hết z-order break do số gây ra**.

Chỉ áp dụng cho `fuentelcd` (số) — `fuenteNormal`/`fuenteMenu` (text đa ngôn
ngữ qua gettext) **giữ SDL_ttf**, xem "Phạm vi & giới hạn".

## Cách làm

### Generate glyph
- **`texture-packer/generate_glyphs.py`** (Pillow): render `0-9` và `:` từ
  `media/fonts/fuentelcd.ttf` ra PNG trắng, **1 size 72px**, canvas cao cố
  định (ascent+descent) + baseline chung → các glyph thẳng hàng. Output:
  `assets/glyphs/lcd/digit0.png…digit9.png, colon.png` (đồng đều **36×73**,
  font LCD monospace).
- **Trắng + 1 size** là chủ ý: màu áp lúc vẽ qua `colorMod` (nhân — trắng ra
  được mọi màu, kể cả bóng đen của FloatingScore); size khác nhau (33/62/…)
  scale xuống từ 72 qua `destRect` (chỉ downscale, linear filter đã bật → không
  mờ). Không cần bitmap riêng cho từng size/màu.
- Pack vào `atlas.png` qua TexturePacker (`prependSmartFolderName=false` nên
  key giữ basename `digit0.png…`). Atlas repack `244×431` → `609×228`.

### Code
- **`Assets::Sprite::Digit0..Digit9, Colon`** — key glyph.
- **`include/BitmapNumber.h`** (header-only): nạp 11 glyph atlas Image; `draw(text,
  x, y, z, fontSize, color, alpha)` vẽ từng glyph cạnh nhau (advance = bề rộng
  glyph × scale, `scale = fontSize/72`); `width()` để canh phải. Monospace nên
  layout đơn giản.
- **`GameIndicators`**: bỏ `mFontScore/mFontTime` + `mImgScore/mImgTime` (SDL_ttf);
  thay bằng `BitmapNumber mNumbers` + chuỗi `mScoreText/mTimeText`.
  `regenerateScoreTexture()`/`updateTime()` chỉ cập nhật chuỗi (không rasterize).
  `draw()` vẽ số bằng `mNumbers` — canh phải x=197 (score, size 33) / x=190
  (time, size 62), màu `{78,193,190}`, `Z::UIText`.

## Kết quả đo (Spector.js, Web/Chrome, đứng yên)

| Bước | draw call | command |
|---|:---:|:---:|
| Baseline | 73 | 367 |
| Step 1 (sort) | 27 | 150 |
| Step 2a (atlas gem) | 19 | 110 |
| Step 2b (atlas UI) | 13 | 83 |
| **Step 4 (bitmap số)** | **11** | **55** |

→ **−85% draw call, −85% command** so với baseline.

Phân rã 11 draw call còn lại:
- 1 board (texture riêng), 1 cursor (texture riêng) — cố ý không atlas.
- **1 khối atlas lớn** = gem + nút + icon + **số bitmap** (batch làm một). ⭐
- 5 text TTF: `score`, `time left`, `Exit`, `Reset game`, `Show hint`
  (header + caption nút, `fuenteNormal`) — giữ SDL_ttf.
- 1 selector (cùng atlas nhưng bị caption TTF chen z=4 nên tách batch).

Đây gần như là **sàn** với hướng hiện tại.

## Kết quả thực tế

**Lỗi freeze/stall game trên bản nhúng ReactJs đã hết** sau chuỗi tối ưu này —
đúng triệu chứng gốc mà kế hoạch nhắm tới (bottleneck không ở gameplay logic
mà ở cách dữ liệu vẽ tổ chức trước khi xuống GPU).

## Phạm vi & giới hạn

- Chỉ số (`fuentelcd`). `fuenteNormal`/`fuenteMenu` render **text đa ngôn ngữ
  (gettext)** → bitmap hoá phải bake full charset + kerning, rủi ro locale,
  mà vẽ hiếm (không phải bottleneck) → **giữ SDL_ttf**.
- FloatingScore/ScoreTable dùng `fuentelcd` nhưng vẫn TTF (cỡ khác, vẽ hiếm) —
  chưa chuyển.

## Đòn bẩy còn lại (tùy chọn, chưa làm)

"Bake cả câu UI": các chuỗi cố định (`Show hint`/`Reset game`/`Exit`/`score`/
`time left`) pre-render mỗi câu thành 1 sprite atlas → batch luôn vào khối lớn,
selector nhập lại được → còn ~5-6 draw. Đánh đổi: **khoá English** (mất khả năng
đổi ngôn ngữ tự do), lời hiệu năng ít (text vẽ 1 lần). Để dành nếu cần gọn thêm.

## Trạng thái

- [x] Generate 11 glyph LCD, pack vào atlas
- [x] `BitmapNumber` + wiring `GameIndicators`
- [x] Build Web OK; đo Spector.js 11 draw / 55 command
- [x] Kiểm tra mắt: số điểm/thời gian khớp vị trí/cỡ/màu
- [x] Xác nhận lỗi freeze ReactJs đã hết
- [ ] (tùy chọn) bake câu UI; chuyển FloatingScore/ScoreTable sang bitmap
