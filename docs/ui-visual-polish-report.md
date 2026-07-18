# Báo cáo — Polish giao diện main menu & in-game (phiên làm việc 2026-07-18/19)

Chuỗi thay đổi trực quan (không phải performance) qua nhiều vòng lặp
xem-và-chỉnh với artwork mới của user. Ghi lại ở đây để phiên sau tiếp tục
polish mà không phải dò lại quyết định cũ. Xem cấu trúc asset & quy ước ở
[repo-structure.md](repo-structure.md).

## 1. Main menu

- **Background/logo** (`media/menu/mainMenuBackground.png`, `mainMenuLogo.png`):
  ảnh đứng riêng (không qua atlas), load trực tiếp — xem `Assets::MenuBackground/MenuLogo`.
- **Logo**: ảnh gốc export to hơn canvas (1067px > 800px). Quyết định **scale
  qua code** (`factorX/Y` trong `Image::draw()`) thay vì resize file, để dễ
  chỉnh nhanh lúc còn thử nghiệm. Scale hiện tại `628/1067 ≈ 0.588`, vị trí
  `x=86` (canh giữa `(800-628)/2`), top margin `y=24`.
- **Font menu** (`Assets::FontMenu`, `media/fonts/fuenteMenu.ttf`): thử qua
  nhiều family (EB Garamond → Philosopher → Akt → Jost → Quicksand) trước khi
  chốt **Quicksand-SemiBold**, size 26.
- **Text menu**: viết hoa toàn bộ (literal, sửa thẳng string vì đây là
  literal tĩnh, không qua gettext runtime concat). Màu chữ `#e5e2e9`.
- **Drop-cap**: chữ cái đầu mỗi dòng menu to hơn (`mFontDropCap`, size 31,
  cùng family), canh baseline khớp với phần chữ còn lại qua `Font::getAscent()`
  (mới thêm vào `GoSDL::Font`) — xem `StateMainMenu::firstUtf8CharLen()` (tách
  theo byte UTF-8, tránh cắt đôi ký tự đa byte khi sau này có bản dịch khác).
- **Highlight** (`menuHighlight.png`): tint màu qua code (`#8e7b9f`) dùng
  tham số `color` sẵn có của `Image::draw()` (`SDL_SetTextureColorMod`) —
  không cần sửa ảnh.
- **Canh giữa**: text + highlight canh giữa động theo `getHeight()` thực tế
  của ảnh đã render, thay vì offset cứng (offset cứng cũ chỉ đúng với font cũ,
  vỡ khi đổi font/size).

## 2. Board & gems

- **`board.png`**: thay bằng `game-scene.png` — nguồn 1448×1086 (tỷ lệ đúng
  4:3) resize thẳng xuống 800×600 (canvas cố định, không scale runtime).
- **7 gem** (`gemBlue/Green/Orange/Purple/Red/White/Yellow.png`): nguồn mới
  512×512 vuông, resize xuống đúng 65×65 (kích thước ô bàn cờ — code vẽ gem
  ở `factorX/Y=1`, không tự scale, xem `GameBoard.cpp` dùng hằng `65` khắp
  nơi). `gemYellow` ban đầu thiếu trong export, đã bổ sung.
- **Selector** (xem mục 4).

## 3. Panel bên trái (buttons + score/time)

### Buttons (Hint/Reset/Exit)

- `buttonBackground.png` + 3 icon redesign theo phong cách banner viền hoa
  văn. Qua vài vòng resize (banner gốc 267×102 → chốt **140×53**; icon gốc
  63×63 → **33×33**, cùng scale factor với banner).
- `BaseButton.cpp` được generalize để tự thích ứng theo size ảnh thật thay vì
  số cứng:
  - icon inset & canh giữa dọc: `(bgHeight - iconHeight) / 2`
  - text: offset trái = icon width, canh giữa phần còn lại, canh giữa dọc
    theo `(bgHeight - captionHeight) / 2`
  - caption viết hoa qua code (`std::transform` + `toupper`, ASCII-only) vì
    caption được ghép động (`_("Show hint")` + suffix riêng cho Vita — suffix
    này đã bị xóa, xem mục 5) nên không thể sửa thẳng literal như menu.
  - Font riêng **`Assets::FontButton`** (Quicksand-SemiBold, `media/fonts/fuenteButton.ttf`)
    tách khỏi `FontNormal` dùng chung ở ScoreTable/Howtoplay — tránh đổi
    font 1 chỗ mà vỡ chỗ khác.
- Layout: `horizButPos=45`, `vertButStart=356`, spacing giữa các nút tính
  động qua `BaseButton::getHeight()` (mới thêm accessor) thay vì hằng số.

### Score / time panel

- `scoreBackground.png` redesign (140×58); `timeBackground.png` **dùng chung
  y hệt ảnh này** (copy trực tiếp, không cần asset riêng).
- Bố cục dọc: label "SCORE" → nền score → label "TIME LEFT" → nền time, xếp
  từ `y=170` xuống, mỗi phần tử cách nhau `groupGap`, tính bằng
  `getHeight()` thực tế của từng ảnh — tự thích ứng nếu asset đổi size sau
  này. Label canh giữa theo **panel width cố định 150px** (không phải theo
  width riêng của background).
- Label "score"/"time left": đổi font sang `Assets::FontButton`, viết hoa
  literal (`_("SCORE")`, `_("TIME LEFT")` — ở đây literal tĩnh nên sửa thẳng
  string được, khác case của button caption).
- Số điểm/thời gian (`mNumbers`, bitmap font từ atlas): canh giữa cả ngang
  lẫn dọc trong nền — cần thêm `BitmapNumber::height(fontSize)` (trước đó chỉ
  có `width()`).
- Màu chữ (label + số) đồng nhất: `#e5d8c4`.
- **Digit bitmap font đổi sang Bold**: `texture-packer/generate_glyphs.py`
  trỏ sang `assets/fonts/Quicksand-Bold.ttf` (thêm mới, chỉ dùng để bake
  glyph, không ship runtime) thay vì `fuentelcd.ttf`. Chỉ ảnh hưởng
  `GameIndicators` (score/time HUD) — **không** ảnh hưởng `ScoreTable`/
  `FloatingScore`, vốn vẫn render trực tiếp bằng `fuentelcd.ttf` qua SDL_ttf,
  không đi qua glyph bitmap này.

## 4. Selector (ô chọn gem)

- Ảnh mới: hình khuyên tròn (ring), chủ đích **to hơn ô gem 65px** để không
  bị cắt viền. Chốt kích thước **85×85** (~30% lớn hơn), canh giữa ô bằng
  `selectorInset = (mImgSelector.getWidth() - 65) / 2`.
- **Xoay liên tục**: `mSelectorAngle` tăng dần, truyền vào tham số `angle`
  sẵn có của `Image::draw()` (dùng `SDL_RenderCopyEx`, xoay quanh tâm rect
  mặc định khi `center=NULL`).
- **Pulse mờ/rõ**: `mSelectorPulsePhase` dùng `sin()` để ra alpha dao động
  nhẹ (200±55).
- **Tốc độ theo delta-time thực** (`SDL_GetTicks()`), không phải bước cố
  định mỗi lần gọi `update()` — vì trên web, `update()` chạy theo
  `requestAnimationFrame` của browser (không đều tuyệt đối), bước cố định
  từng gây tốc độ animation không ổn định. Có clamp delta tối đa 100ms để
  tránh nhảy vọt khi tab bị background.
- **Đã thử rồi bỏ: hiệu ứng "breathing" scale**. Lý do: `Image::draw()` build
  `SDL_Rect` (số nguyên) cho `destRect.w/h`, nên mỗi frame width/height bị
  làm tròn về pixel nguyên → với biên độ scale nhỏ (±5%), nhiều frame liên
  tiếp ra cùng 1 giá trị rồi nhảy 1px một lúc → nhìn giật kiểu "step".
  Rotation không bị vấn đề này vì `angle` là `double` và không đổi kích
  thước rect, chỉ xoay — phép biến đổi hình học liên tục, không lượng tử hoá
  theo pixel.
  - **Fix đúng gốc (chưa làm)**: chuyển `go_image.cpp`/`go_window.cpp` sang
    `SDL_FRect`/`SDL_RenderCopyExF` (SDL ≥ 2.0.10) để có toạ độ/kích thước
    subpixel. Ảnh hưởng **mọi** `Image::draw()` trong game (gem, nút, board…),
    không chỉ selector — cần cân nhắc kỹ trước khi làm, rủi ro cao hơn một
    tính năng nhỏ.

## 5. Gỡ bỏ hỗ trợ PlayStation Vita

Không còn build target Vita. Đã xóa toàn bộ:
- `#ifdef __vita__` trong `inter.h`, `OptionsManager.h`, `Util.cpp`,
  `GameIndicators.cpp`, `StateOptions.cpp`, `go_window.cpp`, `Game.cpp`.
- Target `VITA` trong `CMakeLists.txt` (vita.cmake include, `vita_create_self`,
  `vita_create_vpk`).
- Thư mục `platform/vita/` (sce_sys assets).
- Mục "Installation on Playstation Vita" + nhắc tới Vita trong `README.md`.

Docs lịch sử khác (`step1/2/4-*-report.md`, `performance-optimization-plan.md`)
vẫn còn nhắc Vita — **giữ nguyên**, vì đó là báo cáo ghi lại quyết định tại
thời điểm viết, không phải code sống cần đồng bộ.

## Gotcha quan trọng cần nhớ

- **Chỉ sửa file trong `media/` (không đụng code) → `build.sh` không tự
  relink.** `--preload-file` là link flag; CMake không coi `media/` là
  dependency của bước link, nên Make/Ninja thấy object file không đổi và bỏ
  qua relink → `seajeweled.data` vẫn là bản cũ dù đã sửa ảnh/font. Phải xóa
  `build-web/seajeweled.{data,js,wasm,html}` trước khi `./build.sh` mỗi lần
  chỉ đổi asset.
- **Asset đổi trong `assets/sprites/`/`assets/glyphs/` chưa tự vào game** —
  phải Publish lại qua TexturePacker (`texture-packer/atlas.tps`) để cập
  nhật `media/atlas.png`/`atlas.json` trước khi build.

## Việc còn dang dở / có thể polish tiếp

- `timeBackground` đang dùng chung ảnh với `scoreBackground` — nếu muốn ảnh
  riêng biệt cho time, cần export thêm.
- Cân nhắc `SDL_FRect`/`RenderCopyExF` nếu muốn thêm hiệu ứng scale mượt cho
  selector hoặc chỗ khác sau này (xem mục 4).
- Chưa kiểm tra lại toàn bộ layout trên các state khác (`StateOptions`,
  `StateHowtoplay`, `ScoreTable`) sau khi đổi `FontNormal`/màu — các màn đó
  không nằm trong scope phiên này nhưng dùng chung một số asset/font.
