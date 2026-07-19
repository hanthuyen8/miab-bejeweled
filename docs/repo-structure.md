# Cấu trúc thư mục repo

Tài liệu này mô tả cách tổ chức asset và các quy ước sau khi tối ưu render
(xem [Step 2](step2-texture-atlas-report.md)).

## Nguyên tắc: tách "runtime asset" khỏi "source art"

- **`media/`** — chỉ chứa thứ game **nạp lúc chạy** và **ship kèm** (được
  preload vào web bundle qua `--preload-file media@media`, và symlink cho
  desktop). Không để file thừa ở đây.
- **`assets/`** — **source art**, *không* ship, *không* preload. Đây là input
  của TexturePacker để tạo ra `media/atlas.png`. Tách ra khỏi `media/` giúp
  web bundle không phải tải các sprite lẻ đã nằm sẵn trong atlas.

## Cây thư mục

```
media/                      ← runtime assets (shipped / preloaded)
    atlas.png               ← texture atlas (output của TexturePacker)
    atlas.json              ← bảng frame "JSON (Hash)" của atlas
    images/                 ← ảnh đứng riêng, không vào atlas
        board.png           (800×600, vẽ 1 lần)
        howtoScreen.png     (800×600, màn hướng dẫn)
        handCursor.png      (con trỏ chuột)
    fonts.json              ← metric glyph bitmap (advance/offset/baseline)
                              cho từng face+size — BitmapFont đọc file này
    sounds/
        match1.ogg, match2.ogg, match3.ogg
        select.ogg, fall.ogg, music.ogg
    menu/                   ← ảnh màn hình menu (đổi tên từ stateMainMenu/)
        mainMenuBackground.png
        mainMenuLogo.png
        menuHighlight.png

assets/                     ← source art (NOT shipped, TexturePacker inputs)
    sprites/
        gemBlue/Green/Orange/Purple/Red/White/Yellow.png  (65×65)
        selector.png                                       (65×65)
        iconHint/Restart/Exit.png                          (40×41)
        buttonBackground.png, scoreBackground.png, timeBackground.png
        partc1.png, partc2.png                             (108×108)
    glyphs/                 ← glyph bitmap sinh ra cho atlas, 1 thư mục mỗi
                              face+size (generate_glyphs.py tự xoá & tạo lại)
        menu40/, menu72/    (83 glyph — UI, nút, tiêu đề)
        normal40/           (83 glyph — body text)
        lcd40/, lcd72/      (11 glyph 0-9 + ':' — số score/time)
    fonts/                  ← CHỈ để bake glyph, KHÔNG ship runtime
        Quicksand-SemiBold.ttf  (face "menu")
        Miso-Regular.ttf        (face "normal")
        Quicksand-Bold.ttf      (face "lcd")
    photoshop/              ← file export gốc trước khi resize

platform/
    web/shell.html          ← shell HTML của Emscripten (--shell-file), thêm
                              hotkey F9 gọi Spector.js capture
    unix/, windows/         ← icon & metadata đóng gói desktop

texture-packer/
    atlas.tps               ← project TexturePacker:
                              source = ../assets/sprites/ + ../assets/glyphs/*/
                              output = ../media/atlas.png + ../media/atlas.json
    generate_glyphs.py      ← bake glyph từ assets/fonts/*.ttf ra PNG + fonts.json
    build_atlas.sh          ← chạy generate_glyphs.py rồi repack atlas bằng
                              TexturePacker CLI (không cần mở GUI)
```

## Quy ước code

### Đường dẫn asset — dùng hằng số, không hard-code
Mọi đường dẫn `media/...` và mọi tên sprite trong atlas được khai báo tập
trung ở **`include/Assets.h`** (namespace `Assets`, và `Assets::Sprite` cho
tên frame atlas). **Không** viết chuỗi `"media/..."` rải rác trong code —
dùng hằng số để dễ grep usage và đổi path một chỗ.

```cpp
#include "Assets.h"
mImgBoard.setWindowAndPath(mGame, Assets::Board);
atlas.setImage(mImgRed, Assets::Sprite::GemRed);
```

### Draw depth (z-order) — dùng hằng số, không hard-code
Mọi độ sâu vẽ (z) được khai báo ở **`include/ZOrder.h`** (namespace `Z`, và
các namespace con `Z::Menu`, `Z::Howto`, `Z::Button`). z lớn vẽ đè lên trên.
`DrawingQueue` chỉ đảm bảo thứ tự theo z — hai phần tử **chồng nhau và cần
thứ tự trước/sau cố định phải dùng z khác nhau** (xem Step 1/2). Đừng viết số
z trực tiếp trong `draw()`.

```cpp
#include "ZOrder.h"
img->draw(x, y, Z::Gem);
mImgSelector.draw(x, y, Z::Selector);
```

Layer stack in-game (dưới → trên): `Board(0)` → `UIPanel(2)` →
`Gem/UIText/ScoreTable(3)` → `Selector(4)` → `Particle(7)` → `Hint(10)` →
`FloatingScore(80)` → `Cursor(999)`.

### Sprite trong atlas — dùng `TextureAtlas`
Sprite thuộc atlas được cấu hình qua `GoSDL::TextureAtlas`
(`include/go_textureatlas.h`): `atlas.load(window, Assets::AtlasImage,
Assets::AtlasData)` rồi `atlas.setImage(image, Assets::Sprite::X)`. Nhờ
texture cache trong `GoSDL::Image`, mọi Image nạp cùng file atlas chia sẻ 1
`SDL_Texture` → renderer batch được các draw. Xem [Step 2](step2-texture-atlas-report.md).

## Quy trình cập nhật atlas

1. Sửa/thêm sprite trong `assets/sprites/`.
2. Mở `texture-packer/atlas.tps` bằng TexturePacker, **Publish** — file
   `media/atlas.png` + `media/atlas.json` được ghi đè trực tiếp (không cần
   copy tay). Giữ **Data format: JSON (Hash)**, **Rotation: OFF**,
   **Trim: None**, **Texture format: PNG-32** (xem lý do trong Step 2 report).
3. Nếu thêm sprite mới: thêm hằng số tên frame vào `Assets::Sprite` và dùng
   `atlas.setImage(...)` ở nơi cần.
4. Build lại (`emmake cmake --build build-web`) và đo lại bằng Spector.js.

## Ghi chú

- Thư mục `docs/` chứa kế hoạch/báo cáo tối ưu (user tracking công việc ở
  đây). Xem `performance-optimization-plan.md` và các `stepN-*-report.md`.
- `docs/images/` chứa hình minh hoạ. Hình của GemShine **sinh bằng script**
  `docs/images/gemshine_figures.py` (chép lại công thức trong `GemShine.cpp`,
  đọc sprite thật từ atlas) — sửa hằng số trong `GemShine.cpp` thì chạy lại
  script, không thì hình sẽ lệch khỏi code.
- Hiệu ứng vệt sáng trên gem: [gemshine-explained.md](gemshine-explained.md).
  Đáng đọc trước khi động vào z-order, vì gem dùng texture riêng chứ không vẽ
  từ atlas.
- Phím tắt debug, cách đo draw call, bẫy build: [dev-tooling.md](dev-tooling.md).
- Z-order layering đã gom vào `include/ZOrder.h` (xem mục "Draw depth" ở trên).
- **Không còn TTF nào được ship.** Toàn bộ chữ trong game vẽ bằng glyph
  bitmap trong atlas (`BitmapFont` + `media/fonts.json`). Ba file `.ttf` chỉ
  còn nằm ở `assets/fonts/` làm input lúc bake, nên `media/` không có font và
  gói `.data` nhẹ đi ~217KB. `SDL_ttf` vẫn được link và `go_font.h/cpp` vẫn
  còn trong repo, nhưng **không còn code nào gọi tới** — và vì TTF không nằm
  trong `media/` nữa nên đường dẫn runtime cũ cũng không load được.
- **Đổi charset thì phải bake lại.** `CHARSET` trong `generate_glyphs.py` quyết
  định glyph nào tồn tại; ký tự ngoài danh sách sẽ bị bỏ qua khi vẽ (không
  advance). Text được xử lý theo byte, không phải UTF-8 — bản dịch gettext có
  ký tự ngoài ASCII sẽ không hiện. Sau khi sửa, chạy `texture-packer/build_atlas.sh`.
- **Sửa `platform/web/shell.html` thì phải relink.** Shell là link input nhưng
  CMake không tự suy ra từ flag `--shell-file`, nên `CMakeLists.txt` khai báo
  `LINK_DEPENDS` cho nó. Cùng loại bẫy với `media/` (xem
  [ui-visual-polish-report.md](ui-visual-polish-report.md)).
```
