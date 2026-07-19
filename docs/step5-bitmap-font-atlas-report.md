# Step 5 — Toàn bộ font sang bitmap glyph atlas

## Vấn đề

Step 4 mới chuyển **chữ số** score/time sang glyph atlas. Mọi chữ còn lại vẫn
đi qua `SDL_ttf`:

- Mỗi chuỗi render ra **một `SDL_Texture` riêng** (thêm một texture nữa cho
  shadow) → mỗi đoạn chữ tự phá batch, vì `DrawingQueue` gom theo texture.
- `BaseButton::setText()` **mở lại file `.ttf`** mỗi lần đổi caption
  (`TTF_OpenFont` + rasterize), không chỉ khi khởi tạo.
- Ba file `.ttf` (~217KB) bị nhồi vào `seajeweled.data` qua `--preload-file`.

## Cách làm

### Bake glyph
`texture-packer/generate_glyphs.py` viết lại: bake nhiều **face × size** thay
vì một bộ chữ số.

| face | nguồn (`assets/fonts/`) | size bake | glyph |
|---|---|---|---|
| `menu` | Quicksand-SemiBold.ttf | 40, 72 | 83 |
| `normal` | Miso-Regular.ttf | 40 | 83 |
| `lcd` | Quicksand-Bold.ttf | 40, 72 | 11 |

Ba điểm thiết kế đáng lưu ý:

- **Hai size mỗi face, không phải một.** Game vẽ chữ từ 20px đến 72px. Nếu chỉ
  bake 72px thì caption nút 20px phải thu nhỏ 3.6× → nhoè. Runtime chọn sheet
  40px cho size ≤ 40, 72px cho size lớn hơn → thu nhỏ tối đa ~1.55×, không bao
  giờ phóng to.
- **Canvas nới ra khi mực tràn advance.** Canvas rộng = advance, nhưng `j` (left
  bearing âm), `Q` (đuôi thò xuống dưới descender) và `_` của Miso sẽ bị **cắt
  mất** nếu theo đúng advance. Script tự phát hiện qua `getbbox()`, nới canvas
  và ghi lại `xoffset`/`yoffset` để runtime vẫn bước đúng advance.
- **Tên file theo codepoint** (`menu72_u0041.png`), không theo ký tự: filesystem
  macOS không phân biệt hoa thường nên `a.png` và `A.png` sẽ đè nhau.

Output: `assets/glyphs/<face><size>/` + `media/fonts.json` (metric).

### Pack atlas
`atlas.tps` trỏ vào 5 thư mục glyph mới. `texture-packer/build_atlas.sh` chạy
bake + repack bằng **TexturePacker CLI** — không cần mở GUI như trước.

Atlas: `248×543`, 16 frame → **`1297×512`, 287 frame** (188 KB), vẫn dưới mức
2048 nên còn chỗ trống.

### Runtime
- **`include/BitmapFont.h` + `src/BitmapFont.cpp`**
  - `BitmapFontAtlas` — load atlas + `fonts.json` **một lần**, giữ toàn bộ glyph.
    `Game` sở hữu nó và expose qua `getFonts()` (giống `getGameSounds()`), nạp
    **trước** `changeState()` đầu tiên vì các state dựng font trong constructor.
  - `BitmapFont` — view `(face, size)`, copy rẻ, không giữ tài nguyên. API bám
    sát `GoSDL::Font` cũ nên call site đổi ít: `getTextWidth()`, `getHeight()`,
    `getAscent()`, `draw()`, `drawWithShadow()`, `wrapText()`.
  - `(x, y)` = góc trái trên của line box, **trùng hệ toạ độ cũ** → không phải
    dịch lại vị trí ở bất kỳ call site nào.
- **`BitmapNumber` bị xoá** — nó là tập con của `BitmapFont`.
- **`GoSDL::Image::draw()` thành `const`** (nó chỉ enqueue, không sửa gì), nhờ
  vậy vẽ glyph dùng chung không phải copy `Image` → tiết kiệm một cặp thao tác
  atomic refcount cho **mỗi glyph mỗi frame**.

13 call site chuyển từ *rasterize-một-lần-ra-`Image`* sang *giữ chuỗi, vẽ glyph
mỗi frame*. `StateHowtoplay` wrap body text **một lần lúc dựng** (`wrapText`),
không wrap lại mỗi frame.

### Điểm tinh tế: shadow và z-order
`drawWithShadow()` vẽ 2 lượt **cùng z**. An toàn *vì* hai lượt dùng chung
texture atlas: `DrawingQueue::sort()` là `stable_sort` theo `(z, texture)`, nên
tie giữ nguyên thứ tự insert → shadow vào trước, luôn nằm dưới. Glyph bake màu
trắng nên shadow chỉ là cùng một run tô đen bằng `colorMod`; alpha phải lấy từ
`shadowColor.a` vì `colorMod` chỉ nhân RGB.

## Kết quả

- Không còn rasterize lúc chạy, không còn cấp phát texture cho chữ.
- Mọi chữ batch chung với gem/UI — 1 texture bind cho toàn bộ text.
- `BaseButton::setText()` không còn đụng tới file `.ttf`.
- `media/` không còn font → `.data` nhẹ đi ~217KB.
- Build sạch với `-Wall -Wextra -pedantic`; C++ standard nâng lên **C++17**
  (`CMakeLists.txt` dùng `CMAKE_CXX_STANDARD` thay cho `-std=` thủ công).

Đã verify chạy thật trên web build: menu chính (drop-cap + baseline), how-to-play
(wrap + xuống dòng), options (đổi text động vẫn canh giữa đúng), HUD in-game,
score table, và floating score `+20` lúc ăn điểm.

## Phạm vi & giới hạn

- **Chỉ ASCII trong `CHARSET`.** Ký tự ngoài danh sách bị bỏ qua khi vẽ. Text
  xử lý theo byte chứ không phải UTF-8, nên bản dịch gettext có dấu sẽ không
  hiện — hiện chỉ ảnh hưởng build Linux (`_()` là no-op trên macOS/Windows).
- **`SDL_ttf` vẫn được link** và `go_font.h/cpp` vẫn còn, nhưng không còn code
  nào gọi. Vì TTF đã rời `media/`, đường dẫn runtime cũ cũng không load được
  nữa — tức đây là fallback ở mức source, không phải fallback chạy được.
- **Wrap khác SDL_ttf đôi chút.** `wrapText()` là greedy word wrap tự viết, ngắt
  dòng không nhất thiết trùng `TTF_RenderUTF8_Blended_Wrapped`, nên đoạn body
  how-to-play xuống dòng hơi khác bản cũ.
- **Advance làm tròn theo từng glyph** (SDL_ttf cộng dồn subpixel), nên bề rộng
  chuỗi dài có thể lệch vài px. Chữ canh giữa vẫn canh giữa; canh trái không đổi.

## Checklist

- [x] Bake 271 glyph / 5 sheet / 3 face, pack vào atlas
- [x] `build_atlas.sh` — pipeline scripted, bỏ bước GUI
- [x] `BitmapFont` + `BitmapFontAtlas`, gộp `BitmapNumber` vào
- [x] Chuyển 13 call site SDL_ttf
- [x] Dời `.ttf` khỏi `media/` sang `assets/fonts/`
- [x] Verify từng màn hình trên web build
