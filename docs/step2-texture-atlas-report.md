# Báo cáo Step 2 — Texture atlas cho sprite

Tham chiếu kế hoạch: [`performance-optimization-plan.md`](performance-optimization-plan.md) — mục **2**.
Tiếp nối [Step 1](step1-drawingqueue-report.md) (sort `DrawingQueue` + secondary texture key).

## Ý tưởng cốt lõi

Renderer SDL2 **gộp (batch)** các lệnh `SDL_RenderCopyEx` **liên tiếp dùng chung một `SDL_Texture`** thành 1 `drawArrays` (color/alpha mod nướng vào vertex color nên không phá batch; chỉ đổi texture mới phá). Vì vậy gom nhiều sprite vào 1 atlas → 1 texture → giảm mạnh số draw call.

**Phát hiện quyết định:** khóa batch là **con trỏ `SDL_Texture`, không phải file PNG**. `go_image.cpp` trước đây gọi `IMG_LoadTexture` mỗi lần → 2 Image cùng file vẫn ra 2 texture khác con trỏ → không gộp. Do đó chỉ đưa sprite vào chung file atlas là **chưa đủ**; các Image phải **share cùng một con trỏ texture** → cần thêm texture cache.

## Thay đổi

### Hạ tầng dùng chung
- **`include/go_textureatlas.h` + `src/go_textureatlas.cpp`** (mới): đọc file TexturePacker **"JSON (Hash)"** bằng jsoncpp → `map<tên sprite → SDL_Rect>`; `setImage(img, "gemRed.png")` cấu hình một `Image` trỏ vào vùng con của atlas.
- **Texture cache** (`src/go_image.cpp`): `static map<string, weak_ptr<SDL_Texture>>`. `loadTexture()` tra cache trước — cùng path → **cùng con trỏ texture**. Điều kiện tiên quyết để batch. Bonus độc lập: 3 nút cùng `buttonBackground.png` tự gộp kể cả chưa atlas.
- **`Image::setSrcRect()` + `mHasSrcRect`**: Image mang vùng atlas; `getWidth/getHeight` trả về kích thước vùng con và `draw()` dựng `dstRect` theo vùng con → **mọi chỗ gọi cũ không phải sửa**. Copy/move/assign mang theo `mSrcRect`/`mHasSrcRect`.
- **Đường ống `srcRect`**: `Image::draw` → `Window::enqueueDraw(..., const SDL_Rect* srcRect = nullptr)` → `DrawingQueueOperation.mSrcRect/mHasSrcRect` → tham số `srcrect` của `SDL_RenderCopyEx` (NULL = vẽ toàn texture, tương thích ngược).

### Wiring (chuyển sang atlas)
- **2a — board layer** (`GameBoard::loadResources`): 7 gem + selector + 2 particle.
- **2b — UI** (`BaseButton::set`, `GameIndicators::loadResources`): buttonBackground + 3 icon + scoreBackground + timeBackground.
- **Khép nốt** (`GameHint::setWindow`): selector của hint (sót lại từ 2a) — giờ cũng từ atlas, không còn texture riêng phá batch khi hint chạy.

Atlas: `media/atlas.png` (244×431, RGBA8888) + `media/atlas.json`, gộp 16 sprite nhỏ. Không gộp `board.png`/`howtoScreen.png` (full-screen, vẽ 1 lần, sẽ phình atlas) và text render bằng font (để Step 4).

## Kết quả đo (Spector.js, Web/Chrome, đứng yên)

| Bước | draw call (`drawArrays`) | tổng command |
|---|:---:|:---:|
| Baseline | 73 | 367 |
| Step 1 (sort vector + texture) | 27 | 150 |
| Step 2a (atlas gem/selector/particle) | 19 | 110 |
| **Step 2b (atlas UI bg/icon)** | **13** | **83** |

→ **Giảm 82% draw call, 77% command** so với baseline.

## Ghi chú z-order (giới hạn còn lại)

Batch chỉ nối khi các draw **liên tiếp** cùng texture. Theo painter's z-order, giữa các dải z của atlas có draw **text (font)** chen vào (header/number ở z=3, caption ở z=4) khiến texture atlas phải **bind lại** → vài draw call tách ra. 13 draw còn lại gần như toàn bộ là **text** + `board.png` (z=0) + con trỏ chuột (z=999).

Không "vặn z" để ép atlas liền mạch (mong manh, dễ tái diễn regression kiểu text-thời-gian ở Step 1). Cách gốc: **Step 4 (bitmap font)** đưa glyph số `0-9`/`:` vào **chính atlas này** → number trở thành cùng texture atlas → không còn chen texture lạ → vấn đề z-order tự tan.

## Trạng thái

- [x] Texture cache + atlas loader + đường ống `srcRect`
- [x] Wiring 2a (gem/selector/particle), 2b (UI bg/icon), GameHint selector
- [x] Build Web (Emscripten) compile + link OK
- [x] Đo Spector.js: 13 draw / 83 command
- [x] Kiểm tra mắt: gem/selector/nút/icon/score/time hiển thị đúng
- [x] **Tổ chức lại thư mục `media/`** — tách source art sang `assets/sprites/`, gom sound/font/ảnh vào thư mục con, gom media-string vào `include/Assets.h`, `.tps` xuất thẳng vào `media/`. Xem [repo-structure.md](repo-structure.md).
- [ ] Build + smoke test target khác (desktop macOS vẫn lỗi link `SDL2main` sẵn có của môi trường; Windows/Vita chưa test).

## Follow-up

- Các PNG source (`gem*.png`, `selector.png`, `icon*.png`, `*Background.png`, `partc*.png`) **không còn được nạp lúc runtime** nhưng vẫn là input của project TexturePacker (`texture-packer/atlas.tps` trỏ `../media/*.png`) → **không xóa**, sẽ dời sang thư mục source khi tổ chức lại `media/`.
- Tiếp theo: **Step 4 — bitmap font** cho số điểm/thời gian.
