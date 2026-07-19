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
  nơi). `gemYellow` ban đầu thiếu trong export, đã bổ sung. Sau đó user
  re-export lại vài lần (từng viên riêng lẻ, rồi cả bộ 7 viên) — cùng một
  quy trình resize 65×65, không có gì đổi về mặt kỹ thuật.
- **Selector** (xem mục 4).

## 1b. Con trỏ chuột (`handCursor.png`)

- Redesign lại (viên đá hình mũi tên), nguồn export 750×750 vuông → resize
  xuống **32×32** (giữ nguyên size cũ, hotspot ở góc trên-trái khớp với cách
  `Game.cpp` vẽ: `mMouseCursor.draw(getMouseX(), getMouseY(), Z::Cursor)`,
  không có offset).
- **Quyết định giữ standalone, không đưa vào atlas** dù kích thước nhỏ (đủ
  chỗ trong atlas 248×543 hiện tại). Lý do: cursor vẽ **cuối cùng mỗi frame ở
  z=999**, sau các draw text (không thuộc atlas) — gộp vào atlas sẽ không
  tạo thêm batch nào (đúng giới hạn "text chen texture" đã ghi ở
  [step2-texture-atlas-report.md](step2-texture-atlas-report.md)), chỉ bớt
  được 1 texture object riêng. Không đáng đánh đổi việc sửa `atlas.tps` +
  code (`Game.cpp` từ `setPath` sang `atlas.setImage`) cho lợi ích nhỏ vậy —
  nên **để ở `media/images/handCursor.png` như cũ**.

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
  glyph, không ship runtime) thay vì `fuentelcd.ttf`. Lúc đó chỉ ảnh hưởng
  `GameIndicators` (score/time HUD); về sau `FloatingScore` cũng chuyển sang
  dùng bộ glyph này (mục 4d) và `fuentelcd.ttf` bị gỡ hẳn (mục 4e).

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

## 4b. Hiệu ứng tia sáng lướt qua gem (`GemShine`)

Vệt sáng chéo quét qua từng viên gem, `include/GemShine.h` + `src/GemShine.cpp`.

- **Không dùng sprite** — vệt sáng sinh bằng code (gradient gaussian 2 dải:
  một dải rộng mềm + một dải mảnh sáng đi kèm). Không có asset nào phải export.
- **Mask theo silhouette gem**: đây là vấn đề cốt lõi. Vẽ đè một sprite chữ
  nhật lên ô 65px sẽ lòi ra nền bàn cờ vì gem không lấp đầy ô. Giải pháp:
  lúc load, bake sẵn từng gem × 24 frame vào một sheet qua render target, dùng
  `SDL_ComposeCustomBlendMode(DST_ALPHA, ONE, ADD, ZERO, ONE, ADD)` →
  `màu = vệt × alphaGem + màuGem`, `alpha` giữ nguyên. Vệt bị nhân với alpha
  của gem nên tắt đúng ở mép gem.
  - Vì hệ số màu nguồn là `DST_ALPHA` chứ không phải `SRC_ALPHA`, **cường độ
    vệt phải nằm ở kênh RGB, alpha để 255**. Đây là chỗ dễ sai nhất.
  - Gem được vẽ vào sheet bằng `SDL_BLENDMODE_NONE` để alpha gốc sang nguyên
    vẹn — cả hiệu ứng phụ thuộc vào alpha đó.
  - Có fallback sang `SDL_BLENDMODE_ADD` nếu backend từ chối custom blend
    (khi đó vệt sẽ lem ra ngoài viền). GLES2 của Emscripten **chấp nhận**, đã
    kiểm chứng.
- **Chi phí draw = 0**: gem vẫn vẽ đúng 1 lần như cũ, chỉ đổi `srcRect` sang
  frame khác. Gem tách khỏi `atlas.png` sang sheet riêng nên +1 texture, nhưng
  đo thực tế `batches/frame` chỉ 9-10 (báo cáo cũ ghi 13) → không đáng kể.
  Sheet 24×65 × 7×65 ≈ 2.8MB.
- **Một tham số tốc độ duy nhất** (`kGlintCellTravelMs` trong `GameBoard.cpp`):
  thời gian ánh sáng đi hết bề ngang 1 ô. `kGlintSweepMs` được **suy ra** từ
  nó. Lý do phải suy ra chứ không chỉnh tay: tốc độ vệt *bên trong* một gem và
  tốc độ sóng *từ gem này sang gem kia* là cùng một tốc độ vật lý. Lúc để hai
  hằng số độc lập thì chúng lệch nhau 2.6× → mắt đọc ra "từng viên tự loé",
  không phải "một tia sáng quét qua bàn cờ".
- **Pha có thành phần hàng**: `phaseCells = (7-i) + |lean| * (7-j)`. Vệt nghiêng
  thì đầu dưới trễ hơn đầu trên, nên hàng dưới phải sáng muộn hơn đúng bằng độ
  lệch đó, nếu không đường sáng thành hình bậc thang. Ở góc gần 90° sai số này
  ~0 nên không lộ; càng nghiêng càng lộ.
- **`GemShine` expose `kStreakAngleDegrees` / `kStreakWidth` / `streakLean()`**
  ra header thay vì giấu trong `.cpp`, để `GameBoard` tính pha từ **chính** con
  số dùng lúc bake. Hai nơi giữ hai bản sao thì đổi góc một chỗ là lệch ngay và
  không có gì báo lỗi.
- Số cột sáng cùng lúc **không phải tham số chỉnh tay** — nó là hệ quả hình học
  của góc nghiêng (60° trên bàn cao 520px → đường thẳng trải ngang ~4.6 cột).
  Muốn ít cột sáng hơn thì dựng vệt đứng hơn.
- Glint tắt khi bàn cờ đang động (`eSteady`/`eGemSelected` mới bật), và frame
  luôn được set kể cả lúc tắt để gem không bị đóng băng ở giữa sweep.

## 4c. Khoá 60fps trên web

Phát hiện khi truy vấn nghi vấn "tụt fps lúc sóng sáng quét qua" — hoá ra
không có tụt fps nào cả.

- `emscripten_set_main_loop_arg(..., 0, 1)` bám `requestAnimationFrame` → chạy
  đúng refresh rate màn hình (120Hz trên máy ProMotion). `mUpdateInterval`
  (17ms) **chỉ có tác dụng ở bản native**, web bỏ qua hoàn toàn.
- Hệ quả nghiêm trọng: `update()` chạy animation bằng `mAnimationCurrentStep++`
  — bước cố định **mỗi lần gọi**, không theo thời gian thực. Nên toàn bộ
  animation bàn cờ chạy nhanh theo refresh rate. Selector không dính vì đã
  chuyển sang delta-time (mục 4), phần bàn cờ thì chưa.
- Cách khoá: **không** truyền `60` vào `emscripten_set_main_loop_arg` — khi
  fps > 0 Emscripten chuyển sang `setTimeout`, mất đồng bộ vsync, pacing tệ
  hơn. Thay vào đó giữ rAF và bỏ nhịp thừa bằng accumulator
  (`Window::frameIsDue()`), có clamp 100ms cho trường hợp tab xuống nền.
- `runFrame()` được tách thành `pollEvents()` + `gameLoop()`; callback web
  drain event **mỗi nhịp rAF** (input vẫn nhạy ở 120Hz) nhưng chỉ vẽ ở 60fps.
- Đường native giữ nguyên `SDL_Delay`, **cố ý không** gate trong `runFrame()`
  — gate ở đó sẽ chồng lên bộ giới hạn sẵn có của native và tụt xuống 30fps.
- Cảm giác "glint giật" ban đầu chính là do glint đổi frame ở 60Hz trên màn
  120Hz. Khoá 60fps làm khớp 1:1 và hết giật.

## 4d. `FloatingScore` chuyển sang glyph atlas

Phát hiện khi soi draw call bằng Spector: mỗi số điểm bay lên tốn **2 draw
call riêng** (một cho chữ trắng, một cho bóng), nên nổ 2 chuỗi là +4 draw call.

- Nguyên nhân: `FloatingScore` gọi `Font::renderText()` trong constructor →
  mỗi instance tạo `SDL_Texture` **riêng**. Batch chỉ nối các draw liên tiếp
  cùng con trỏ texture (xem [step2](step2-texture-atlas-report.md)), nên texture
  riêng = bind riêng = draw call riêng.
- Sửa: dùng `BitmapNumber` (glyph 0-9 từ atlas) y như HUD đã làm ở Step 4.
  Bóng đổ giờ là **cùng bộ glyph** với `colorMod` đen thay vì texture thứ hai.
  Tất cả gộp vào run atlas sẵn có → gần như không thêm draw call nào.
- Lợi ích phụ quan trọng hơn cả draw call: constructor cũ còn gọi
  `Font::setAll()` → `go_font.cpp` gọi `TTF_OpenFont` **mỗi lần**, không có
  cache. Tức mỗi lần ăn điểm là mở + parse file font, rasterize 2 lần, upload
  2 texture — ngay giữa nhịp particle đang nổ. Nay constructor chỉ còn
  `std::to_string`.
- `BitmapNumber` được **inject qua constructor** (`FloatingScore` nhận
  `BitmapNumber *`), `GameBoard` sở hữu instance riêng thay vì dùng chung với
  `GameIndicators` — tránh `GameBoard` phải tham chiếu ngược sang nó. Hai
  instance chỉ là vài `Image` copy trỏ chung một atlas texture.
- Đổi diện mạo: số điểm bay lên từ LCD → Quicksand-Bold, trùng font với HUD.

## 4e. Gỡ bỏ font LCD

- `media/fonts/fuentelcd.ttf` đã xoá khỏi repo, cùng hằng `Assets::FontLcd`.
- Chỗ dùng cuối cùng là `ScoreTable` (số điểm màn "GAME OVER") → chuyển sang
  `Assets::FontButton` (Quicksand-SemiBold) @72. Tiện thể xoá `fntLcdSmall`
  vốn được khai báo nhưng chưa bao giờ dùng.
- Lưu ý còn lệch: số điểm ScoreTable là **SemiBold**, còn chữ số HUD/floating
  score là **Bold** — vì `Quicksand-Bold.ttf` chỉ nằm ở `assets/fonts/` để bake
  glyph, không ship trong `media/`. Muốn khớp tuyệt đối thì cho `ScoreTable`
  vẽ số bằng `BitmapNumber` (nó đang pre-render thành `Image` kèm shadow nên
  phải sửa cách dựng), hoặc ship thêm Quicksand-Bold.

## 4f. Công cụ dev

Hotkey capture Spector.js (F9) và phím tắt nhảy màn hình (F1-F7) — xem
[dev-tooling.md](dev-tooling.md).

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

- **Animation bàn cờ vẫn dùng bước cố định mỗi lần `update()`** (`mAnimationCurrentStep++`).
  Sau khi khoá 60fps (mục 4c) thì nó chạy đúng tốc độ, nhưng vẫn mong manh: ai
  đổi frame cap là tốc độ game đổi theo. Fix gốc là chuyển sang delta-time như
  selector đã làm. Đây là món đáng làm nhất trong danh sách này.
- **`go_font.cpp` không cache `TTF_OpenFont`** — mỗi `Font::setAll()` mở lại
  file font. Đã hết nóng sau khi `FloatingScore` bỏ TTF (mục 4d), nhưng
  `ScoreTable`/`StateMainMenu`/`BaseButton` vẫn gọi lúc load.
- `timeBackground` đang dùng chung ảnh với `scoreBackground` — nếu muốn ảnh
  riêng biệt cho time, cần export thêm.
- Cân nhắc `SDL_FRect`/`RenderCopyExF` nếu muốn thêm hiệu ứng scale mượt cho
  selector hoặc chỗ khác sau này (xem mục 4).
- Chưa kiểm tra lại toàn bộ layout trên các state khác (`StateOptions`,
  `StateHowtoplay`, `ScoreTable`) sau khi đổi `FontNormal`/màu — các màn đó
  không nằm trong scope phiên này nhưng dùng chung một số asset/font.
