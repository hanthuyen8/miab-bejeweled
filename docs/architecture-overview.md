# Kiến trúc & kỹ thuật của game Freegemas

Ghi chú kỹ thuật tổng hợp về cách game (Bejeweled clone C++/SDL2) này được xây
dựng — dùng để tra cứu lại, không phải hướng dẫn thay đổi code.

## 1. Tổng thể

- Ngôn ngữ: C++11. Thư viện đồ họa/âm thanh: **SDL2** (+ SDL2_image,
  SDL2_mixer, SDL2_ttf). JsonCpp để lưu config/highscore.
- Build system: **CMake**, một `CMakeLists.txt` build ra 4 target khác nhau:
  Linux/macOS (desktop), Windows, PlayStation Vita, và Web (Emscripten/WASM).
- Không dùng engine có sẵn (Unity/Godot/...), không ECS, không physics
  engine — kiến trúc OOP truyền thống, mỗi hệ thống là 1 class riêng
  (composition, không global singleton).

## 2. SDL là gì

SDL (Simple DirectMedia Layer) là lớp trung gian (abstraction layer) giữa
code game và OS: tạo window, đọc input (bàn phím/chuột/gamepad), vẽ 2D
(texture, rect, xoay, alpha blend), phát âm thanh. Mỗi OS có API native
riêng và khác nhau hoàn toàn; SDL cho code C++ gọi 1 API duy nhất, tự "dịch"
xuống OS bên dưới.

**SDL chỉ cung cấp API vẽ mức rất thấp** — không có khái niệm "update/draw
loop", "scene", hay animation/tweening:

```
SDL_RenderClear(renderer)          // xóa màn hình
SDL_RenderCopy / SDL_RenderCopyEx  // copy 1 texture lên renderer
SDL_RenderPresent(renderer)        // hiện buffer đã vẽ lên màn hình thật
```

Nhờ SDL, game build được cho cả 4 platform từ cùng 1 bộ source — kể cả Web,
vì Emscripten có sẵn "SDL2 port" giả lập API SDL bằng WebGL/Web Audio.

## 3. CMake thật sự làm gì

CMake không phải trình biên dịch — nó *sinh ra* file cấu hình cho build tool
thật (Makefile, Visual Studio project...). Trong `CMakeLists.txt` của
project, CMake làm 4 việc:

1. Gom danh sách file `.cpp` tự động (`file(GLOB ...)`).
2. Tìm thư viện (SDL2, JsonCpp...) trên máy đang build qua pkg-config, tự
   set include path/link flags — khỏi phải gõ tay đường dẫn khác nhau giữa
   Homebrew/apt/...
3. Rẽ nhánh theo target build: `if(WIN32)/if(VITA)/if(EMSCRIPTEN)/else()` —
   cùng 1 file xử lý 4 trường hợp hoàn toàn khác nhau (link static, đóng gói
   `.vpk` cho Vita, `--preload-file` nhúng `media/` cho Web...).
4. Định nghĩa cách cài đặt (`install(...)`) cho Linux theo chuẩn FHS.

Tóm gọn: **SDL abstraction ở runtime, CMake abstraction ở build-time**.

## 4. Vòng lặp chính (game loop) — `update()`/`draw()` là do tác giả tự viết

**Quan trọng**: `update()` và `draw()` **không phải hàm SDL cung cấp để
override**. Đây là 2 hàm ảo (`virtual`) do tác giả tự khai báo trong class
`GoSDL::Window` (`include/go_window.h`) — một class tự viết bọc quanh SDL2
thật, namespace `GoSDL` là tên riêng của tác giả, không phải SDL chính hãng.
SDL2 thật (`<SDL.h>`) không có class `Window`, không có gì để "override".

Pattern "tách update (logic) / draw (render)" là convention rất phổ biến
trong lập trình game nói chung (Unity `Update()`, XNA/MonoGame
`Update()`/`Draw()`, LÖVE2D `love.update()`/`love.draw()`...) — tác giả tự
implement lại pattern này trên nền SDL2 vì SDL không có sẵn.

Chuỗi gọi mỗi frame:

```
GoSDL::Window::show()
  → gameLoop()  (mỗi frame)
      → update()   // virtual, Game override → mCurrentState->update() → ...
                    //   MUTATE state: animation step, vị trí gem, điểm số...
      → draw()     // virtual, Game override → mCurrentState->draw() → ...
                    //   READ state đã tính, đẩy lệnh vẽ vào DrawingQueue
                    //   (KHÔNG gọi SDL trực tiếp, chỉ enqueueDraw())
      → SDL_RenderClear(mRenderer)
      → for (mỗi operation trong DrawingQueue, sắp theo z-order):
            SDL_RenderCopyEx(...)   // ← SDL THẬT, chỉ gọi ở đây, cuối gameLoop()
      → SDL_RenderPresent(mRenderer)
```

`update()` luôn chạy trước `draw()` trong cùng 1 frame, để `draw()` có dữ
liệu mới nhất để vẽ. `draw()` về nguyên tắc là read-only/idempotent — ngoại
lệ duy nhất: `StateGame::draw()` tự đổi `mState` khi ở `eInitial`, để đảm
bảo màn hình "Loading..." được vẽ 1 frame trước khi bắt đầu load resource
nặng (tránh đứng hình không phản hồi).

**DrawingQueue** (`go_drawingqueue.h`): `multimap<float z, DrawingQueueOperation>`
— mọi lệnh vẽ trong 1 frame được gom vào đây theo độ sâu z, chỉ render thật
1 lần ở cuối frame theo đúng thứ tự z.

Trên Web: không có vòng `while` chặn — dùng
`emscripten_set_main_loop_arg`, trình duyệt gọi lại `runFrame()` qua
`requestAnimationFrame`.

## 5. State Machine — điều phối màn hình

`Game` (kế thừa `GoSDL::Window`) không tự chứa logic màn hình, ủy quyền cho
`State` hiện tại (`mCurrentState`, `shared_ptr<State>`):

- `State` là lớp trừu tượng (`update()`, `draw()`, input callbacks).
- Cụ thể: `StateMainMenu`, `StateOptions`, `StateHowToPlay`, `StateGame`
  (trừu tượng) → `StateGameEndless`, `StateGameTimetrial`.
- `Game::changeState(string)` tạo state mới, gán vào `mCurrentState`.

Điểm hay: `StateGameEndless`/`StateGameTimetrial` mỗi cái override
`update()` riêng (logic khác nhau: có/không đếm giờ) nhưng **dùng chung 1
hàm `StateGame::draw()`** — vì hình ảnh hiển thị giống hệt nhau giữa các
mode, chỉ logic khác. Tách Logic (thay đổi theo mode) khỏi Presentation
(giống nhau mọi mode).

## 6. Model/View tách biệt: `Board` vs `GameBoard`

- **`Board`** (`Board.h/cpp`) — model thuần túy, không phụ thuộc SDL. Ma
  trận `8x8` `Square`. Thuật toán: `swap`, `check()` (tìm match, trả về
  `MultipleMatch`), `calcFallMovements()`, `solutions()` (tìm nước đi hợp
  lệ — dùng cho hint và detect hết nước đi). Vì tách khỏi rendering, có thể
  copy `Board` để thử nước đi giả lập trước khi commit thật (dùng ở
  `GameHint`).
- **`GameBoard`** (`GameBoard.h/cpp`, ~715 dòng, file lớn nhất) — view/
  controller, sở hữu 1 `Board`, vẽ nó, xử lý input, chạy animation. Có state
  machine con riêng (`eBoardAppearing → eSteady → eGemSelected →
  eGemSwitching → eGemDisappearing...`).

## 7. Layout: toàn bộ tọa độ là hardcode (magic number)

Không có layout engine/hệ tọa độ tương đối nào — mọi vị trí là pixel tuyệt
đối viết chết trong code, giả định canvas cố định:

```cpp
GoSDL::Window(800, 600, "Freegemas")   // Game.cpp — logical resolution cố định
int posX = 241, posY = 41;             // GameBoard.cpp — góc bàn chơi
... i * 65 ...                          // mỗi ô gem cách nhau 65px
std::round(800 / 2 - text.getWidth()/2) // StateMainMenu/Options.cpp — canh giữa
```

Kích thước file ảnh khớp chính xác với các con số này — bằng chứng cho quy
trình thiết kế: `board.png` là 800×600, `gemBlue.png`/`selector.png` là
65×65. Nhiều khả năng: **thiết kế toàn bộ UI trong Photoshop/GIMP ở canvas
800×600 trước, đo tọa độ trên đó rồi gõ tay vào code** — không có liên kết
máy móc giữa file thiết kế gốc và code.

**Hệ quả**: không có tooling hỗ trợ (không hot-reload, không debug overlay
hiện tọa độ, không config file tách layout khỏi code) — quy trình chỉnh UI
là **sửa số → compile → chạy → nhìn bằng mắt → lặp lại** thủ công hoàn
toàn.

### `SDL_RenderSetLogicalSize` — phân biệt "kích thước hiển thị" vs "độ phân giải logic"

```cpp
SDL_RenderSetLogicalSize(mRenderer, width, height);   // go_window.cpp
```

Mọi lệnh vẽ luôn hiểu theo không gian logic 800×600 cố định; SDL tự
scale/letterbox lên kích thước cửa sổ/canvas thật.

- **Resize cửa sổ / canvas web** → KHÔNG break (SDL tự scale đồng đều, giữ
  tỷ lệ). Đây là cách xoay 90° trên mobile trong `react-integration.md` chỉ
  cần CSS transform, không đụng C++.
- **Đổi con số `800, 600` thật sự trong code** → SẼ break, vì `posX = 241`,
  `800 / 2 - ...` và mọi offset khác không tính lại theo kích thước mới —
  không có single source of truth cho layout.

## 8. Hệ tọa độ (anchor): top-left, Y hướng xuống — khác Unity

`Image::draw(x, y, ...)` (`go_image.cpp`) gán thẳng `destRect.x = x;
destRect.y = y;` — `SDL_Rect` định nghĩa `(x,y)` là **góc trên-trái**, không
phải tâm.

```
SDL (game này), HTML Canvas, CSS, DirectX texture space:
(0,0) ──► X          Unity (world/screen space):
  │                     ▲
  │                     │
  ▼                     │
  Y                   (0,0) ──► X
top-left, Y xuống     bottom-left, Y lên
```

- Photoshop mặc định anchor **transform** (scale/rotate) là center
  (0.5, 0.5) của layer — khác với tọa độ *đặt vị trí* (top-left) mà designer
  phải tự đọc/chuyển đổi khi giao cho code.
- SDL2's `SDL_RenderCopyEx` (dùng cho animation xoay gem) mặc định **xoay
  quanh tâm** của `destRect` nếu không truyền pivot riêng — tức 2 hệ anchor
  khác nhau cùng tồn tại tùy loại thao tác: đặt vị trí = top-left, xoay =
  center. Giống convention pivot vs position trong Unity/Godot.
- Ngoại lệ trong chính Unity: `OnGUI` (Legacy IMGUI) dùng top-left, Y xuống
  — giống SDL, khác World/Screen space chuẩn của Unity.
- Khi port giữa 2 hệ: `Y_unity = screenHeight - Y_sdl - height_object`
  (nguồn lỗi phổ biến: hình vẽ ngược, animation rơi từ dưới lên nếu quên
  đảo trục Y).

## 9. Animation — tự viết, 2 tầng không đồng bộ

SDL không có animation/tweening library. Animation trong game này **không
đảo trục hay đổi convention** — nội suy trực tiếp giá trị pixel X/Y (cùng hệ
top-left/Y-xuống ở mục 8), feed thẳng vào `Image::draw()` mỗi frame.

Có 2 tầng animation trong code, không đồng bộ với nhau:

- **Tầng "class đầy đủ" (`Animacion`, `Animation.h/cpp`)** — API instance-
  based (`update()`, `get()`, `finished()`...) đúng chuẩn thiết kế OOP, có
  vẻ dùng cho phiên bản trước (còn liên quan tới `Ecuaciones.cpp` — tiếng
  Tây Ban Nha nghĩa "equations"). **Không được instantiate ở đâu trong game
  hiện tại** — code debt còn sót lại.
- **Tầng thực dùng: hàm `static` easing** (`Animacion::easeOutQuad`,
  `easeOutCubic`, `easeOutQuart`...) — chữ ký chuẩn kiểu Robert Penner's
  easing equations (`t, b, c, d` = currentStep, begin, change, duration).
  Mỗi nơi cần animation (`GameBoard`, `ParticleSystem`, `JewelGroupAnim`)
  **tự quản lý bộ đếm bước riêng** (`mAnimationCurrentStep`) và tự gọi hàm
  static để tính tọa độ — không dùng object `Animacion` quản lý giúp.

Cơ chế: `update()` chỉ tăng `mAnimationCurrentStep++` và kiểm tra điều kiện
kết thúc để chuyển `mState`; `draw()` đọc step đó, feed vào hàm easing
static để tính tọa độ pixel cần vẽ ngay lúc đó. Ví dụ gem rơi khi bàn cờ
xuất hiện: `origY` âm (điểm bắt đầu nằm phía trên, ngoài khung nhìn),
`destY` dương (quãng cần đi) → Y tăng dần → rơi xuống đúng trực quan vì Y
hướng xuống.

## 10. Lưu trữ & bản Web

- `OptionsManager` dùng JsonCpp ghi `options.json` qua `SDL_GetPrefPath`.
- Bản Web (Emscripten): SDL2 qua "port" built-in, asset `media/` đóng gói
  vào `freegemas.data` qua `--preload-file`. `options.json` nằm trong MEMFS
  ảo — **mất khi reload trang** (không persist qua session mới). Chi tiết
  tích hợp React + highscore online: xem `docs/react-integration.md`.

## Tham khảo thêm

- `docs/react-integration.md` — cách nhúng bản Web/WASM vào React app,
  đang điều tra bug đứng hình ~1s (đã xác định không nằm trong C++
  update()/draw(), nghi ở tầng JS/browser).
