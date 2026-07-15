# Tích hợp Seajeweled (WASM) vào React app

Tài liệu này mô tả cách nhúng bản build web của game Seajeweled (C++/SDL2 compile
sang WebAssembly qua Emscripten) vào một React app đã có sẵn, dùng cho agent
phụ trách phần React.

## 1. Trạng thái hiện tại

- Game gốc: https://github.com/josetomastocino/freegemas (GPLv2 — xem mục
  License bên dưới).
- Đã build thành công target Web bằng Emscripten. Build tạo ra 4 file tĩnh:

  | File | Vai trò | Kích thước hiện tại |
  |---|---|---|
  | `seajeweled.html` | Shell HTML mặc định của Emscripten (React không dùng file này, xem mục 3.4) | ~20 KB |
  | `seajeweled.js` | JS glue code (khởi tạo module, quản lý canvas, filesystem ảo...) | ~185 KB |
  | `seajeweled.wasm` | Binary WASM chứa toàn bộ game logic | ~2.5 MB |
  | `seajeweled.data` | Toàn bộ thư mục `media/` (ảnh, font, âm thanh) được đóng gói sẵn, nạp vào filesystem ảo lúc khởi động | ~3.9 MB |

- **Highscore local** vẫn lưu trong game như trước (file ảo `options.json` qua
  `SDL_GetPrefPath`, nằm trong MEMFS của WASM — **không** persist qua session
  mới trên web vì MEMFS reset mỗi lần load trang).
- **Highscore online**: đã tích hợp `MiabSDK::SubmitHighscore` (xem
  `include/MiabSDK.h` / `src/MiabSDK.cpp`, gọi từ `GameBoard::endGame`). Khi
  build web, mỗi lần kết thúc mode `endless` hoặc `timetrial` game tự
  `postMessage({type: 'miab:highscore', score, mode, elapsedMs}, ...)` ra
  `window.parent` theo đúng contract mô tả trong
  `docs/games/highscore-sdk.md` (repo `miab-v1`). Phần nhận + forward lên
  server là trách nhiệm của React host (`SeajeweledScreen.tsx`), đã implement
  sẵn ở phía `miab-v1`, không cần đụng gì thêm bên C++.

## 2. Cách build (nếu cần build lại)

Yêu cầu: đã cài Emscripten (`brew install emscripten` trên macOS).

```bash
emcmake cmake -S . -B build-web
emmake cmake --build build-web -- -j"$(sysctl -n hw.ncpu)"
```

Output nằm trong `build-web/seajeweled.{html,js,wasm,data}`. Thư mục
`build-web/` đã có trong `.gitignore`, không commit vào repo.

## 3. Cách nhúng vào React — khuyến nghị dùng iframe

**Không cần embed trực tiếp / không cần thay đổi gì phía WASM để chạy được
trong iframe.** Lý do chọn iframe thay vì mount canvas trực tiếp vào React
tree: cô lập vòng đời (game chạy vòng lặp vô hạn qua
`requestAnimationFrame`, không có cách dọn dẹp sạch khi component unmount),
và tránh đụng namespace global mà Emscripten JS glue tạo ra (`Module`, v.v.).

### 3.1. Đặt file tĩnh

Copy 3 file `seajeweled.js`, `seajeweled.wasm`, `seajeweled.data` vào thư mục
static của React app, ví dụ:

```
public/games/seajeweled/seajeweled.js
public/games/seajeweled/seajeweled.wasm
public/games/seajeweled/seajeweled.data
```

(Không cần copy `seajeweled.html` — React tự viết HTML/loader riêng, xem mục
3.4 cho các yêu cầu bắt buộc khi làm vậy.)

Vì đây là file tĩnh, không cần route/server logic riêng — deploy như asset
bình thường, cache CDN dài hạn được (nội dung game không đổi giữa các lần
người dùng chơi, chỉ đổi khi bạn build lại và deploy).

### 3.2. Component iframe

```jsx
function SeajeweledGame() {
  return (
    <iframe
      src="/games/seajeweled/index.html"
      title="Seajeweled"
      allow="pointer-lock; fullscreen"
      style={{ width: '100%', height: '100%', border: 0 }}
    />
  );
}
```

Điểm quan trọng: thuộc tính `allow="pointer-lock; fullscreen"` **bắt buộc**
phải có — game dùng Pointer Lock API (ẩn/khoá con trỏ chuột) và có nút
Fullscreen; thiếu attribute này thì 2 tính năng đó sẽ âm thầm không hoạt
động bên trong iframe (không báo lỗi rõ ràng, dễ nhầm là bug game).

### 3.3. Same-origin, không có giới hạn CORS

Miễn là file game được serve **cùng origin** (cùng domain/port) với React
app — đúng như trường hợp hiện tại — thì code bên trong iframe gọi
`fetch('/api/...')` hoạt động bình thường như gọi từ trang chính: không dính
CORS, cookie/session cũng được gửi kèm tự động. Không cần proxy hay cấu hình
gì thêm cho việc này.

### 3.4. HTML/loader riêng (React không dùng `seajeweled.html` mặc định)

Vì chỉ lấy `seajeweled.js` + `seajeweled.wasm` + `seajeweled.data` và tự viết
HTML/JS loader riêng, cần đảm bảo các điểm sau — thiếu cái nào cũng có thể
gây lỗi khó nhận ra (game load được nhưng im lặng không chạy, hoặc chạy
nhưng mất hết log debug):

1. **Canvas**: tạo sẵn 1 thẻ `<canvas id="canvas">` (đúng id `canvas`, đây
   là default Emscripten tìm) trong HTML *trước khi* load `seajeweled.js`,
   hoặc set `Module.canvas` trỏ tới canvas đó trước khi script chạy.
2. **`Module.locateFile`**: nếu `seajeweled.wasm`/`seajeweled.data` được host
   ở path khác thư mục chứa `seajeweled.js` (ví dụ bundler đổi tên/hash file,
   hoặc build tool tách asset ra CDN khác), PHẢI set:
   ```js
   window.Module = {
     canvas: document.getElementById('canvas'),
     locateFile: (path) => `/games/seajeweled/${path}`,
   };
   ```
   trước khi `<script src=".../seajeweled.js">` chạy. Nếu bỏ qua bước này mà
   đường dẫn thực tế khác thư mục gốc, module sẽ fetch sai path và fail
   silent hoặc báo lỗi mơ hồ.
3. **KHÔNG override `Module.print`/`Module.printErr` để nuốt mất output** —
   nếu có ý định custom logging (route log game vào UI riêng), nhớ vẫn gọi
   thêm `console.log`/`console.error` gốc bên trong, đừng thay thế hoàn
   toàn. Đây là kênh duy nhất để debug game từ phía C++ (xem mục 5 bên
   dưới). **Đã xác nhận: ở bản tích hợp hiện tại log `[DEBUG]` của game lên
   console bình thường**, nên điểm này không phải vấn đề — giữ nguyên như
   vậy khi sửa đổi thêm.

## 4. Xoay màn hình cho mobile (landscape-only game trên viewport portrait)

Game được thiết kế cho màn ngang. Trên mobile, nếu muốn giữ trang web ở chế
độ dọc (không ép OS xoay màn hình) nhưng vẫn cho phép chơi bằng cách xoay
điện thoại vật lý, dùng CSS transform xoay 90° container chứa iframe:

```css
.seajeweled-container {
  position: fixed;
  inset: 0;
  width: 100dvh;
  height: 100dvw;
  transform: rotate(90deg) translateY(-100%);
  transform-origin: top left;
}

@media (orientation: landscape) {
  .seajeweled-container {
    width: 100dvw;
    height: 100dvh;
    transform: none;
  }
}
```

Cơ chế: `@media (orientation: portrait/landscape)` xét theo **tỷ lệ viewport
thực tế** (width vs height), không phải theo cảm biến xoay vật lý của máy.
Nếu OS đang khoá xoay dọc, viewport không bao giờ đổi hình dạng dù người
dùng xoay điện thoại — nội dung đã bị xoay sẵn 90° bằng CSS sẽ hiển thị đúng
chiều trong mắt người dùng khi họ xoay điện thoại, mà không cần đổi setting
hệ điều hành. Nếu người dùng có auto-rotate bật sẵn, viewport tự chuyển
thành landscape thật và media query tự tắt phần xoay (tránh xoay chồng 2
lần).

Không cần sửa gì phía C++/SDL cho việc này — đây thuần là CSS wrapper bên
ngoài iframe. `SDL_RenderSetLogicalSize` phía trong đã tự letterbox theo tỷ
lệ cố định của game.

Lưu ý:
- Dùng `dvh`/`dvw` (dynamic viewport units) thay vì `vh`/`vw` để tránh sai
  kích thước khi thanh địa chỉ Safari mobile ẩn/hiện.
- Cần `overflow: hidden` trên `body` để tránh scrollbar do box bị xoay lớn
  hơn viewport thực.
- Browser tự hit-test touch/click đúng theo transform, toạ độ chạm vẫn map
  chính xác vào iframe bên trong — không cần tính toán lại thủ công.

## 5. Debug hiện tượng đứng hình (freeze) ~1 giây — đang điều tra

Có một hiện tượng: thỉnh thoảng (không đều đặn) UI đứng hình ~1 giây (chuột,
đồng hồ đếm ngược trong game đều dừng cùng lúc). Đã thử và loại trừ, theo
thứ tự:

1. Tắt nhạc nền → vẫn freeze (loại trừ audio decode làm nguyên nhân gốc).
2. Build với `-O3` (trước đó chỉ mặc định `-O1`) → vẫn freeze, không cải
   thiện rõ rệt.
3. Thêm đo thời gian bằng `std::chrono` ngay trong C++ (`src/go_window.cpp`,
   hàm `gameLoop()`), log `[PERF] slow frame: total=...ms update=...ms
   draw=...ms render=...ms (queue=N)` ra `stderr` mỗi khi 1 frame (từ lúc
   bắt đầu `update()` tới lúc `SDL_RenderPresent` xong) vượt quá 50ms →
   **log này không xuất hiện dù freeze vẫn xảy ra**.
4. Xác nhận log debug khác của game (`[DEBUG] [++ Constructor] ...`) vẫn
   lên console bình thường trong bản tích hợp React — tức console
   passthrough không phải vấn đề, `[PERF]` thực sự không fire.

**Kết luận đã xác nhận**: chỗ đứng hình **không nằm trong** logic game
(`update()`/`draw()`/render loop) — nếu nó nằm trong đó, log `[PERF]` đã
phải xuất hiện. Nghĩa là trình duyệt đơn giản **không gọi
`requestAnimationFrame` đúng lịch** trong khoảng ~1s đó. Nguyên nhân nằm ở
tầng JS/browser — GC pause, WASM JIT tier-up compilation, hoặc **script
khác trên cùng trang (kể cả chính React app) chiếm dụng main thread** — chứ
không phải ở C++ code của game.

**Bước tiếp theo, cần agent React đo giúp:**

Thêm tạm đoạn JS này vào đầu file HTML/loader riêng (trước khi load
`seajeweled.js`), để đo khoảng cách giữa các frame ở tầng JS — bắt được cả
những chỗ đứng hình xảy ra ngoài code C++ mà instrumentation trong
`gameLoop()` không thấy được:

```js
window.__stallMonitor = { gaps: [], last: performance.now() };
(function tick(ts) {
  const now = performance.now();
  const dt = now - window.__stallMonitor.last;
  window.__stallMonitor.last = now;
  if (dt > 150) window.__stallMonitor.gaps.push({ t: Math.round(now), dt: Math.round(dt) });
  requestAnimationFrame(tick);
})();
```

Chơi tới khi freeze, sau đó chạy trong console:
```js
JSON.stringify(window.__stallMonitor.gaps)
```

Gửi lại kết quả (mảng `{t, dt}`). Vì đã xác nhận `[PERF]` phía C++ không hề
fire, gap đo được ở đây gần như chắc chắn sẽ khớp thời điểm freeze mà không
có `[PERF]` đi kèm — lúc đó nên dùng Chrome DevTools **Performance tab**,
record đúng lúc freeze, xem thanh "Main" đang chạy hàm JS nào (không phải
C++/wasm) tại thời điểm đó. Rất có thể sẽ chỉ ra code của chính React app
hoặc một thư viện khác trên trang, không phải game — nếu vậy, hướng sửa nằm
ở phía React (ví dụ re-render nặng, effect chạy đồng bộ nặng, timer/polling
nào đó), không phải ở C++.

## 6. Việc cần làm thêm (chưa nằm trong bản build hiện tại)

- **Highscore online**: đã xong, xem mục 1. Còn lại là việc của phía
  `miab-v1` (đã implement — xem `docs/games/highscore-sdk.md` mục 3).
- **Âm thanh autoplay**: trình duyệt chặn autoplay audio cho tới khi có
  tương tác người dùng (click/tap). Game hiện tự phát nhạc nền ngay khi vào
  màn chơi (`GameIndicators::loadResources`, `src/GameIndicators.cpp`) — có
  thể bị trình duyệt chặn im lặng nếu người dùng chưa tương tác gì với
  iframe trước đó. Cần test thực tế; nếu bị chặn, cách xử lý chuẩn là chỉ
  gọi `Mix_FadeInMusic`/tương đương sau sự kiện click/tap đầu tiên.

## 7. License

Game gốc theo **GPLv2**. Nếu app React (bao gồm cả bản đã tích hợp) được
phân phối/public, cần:
- Giữ nguyên file `LICENSE` (GPLv2) kèm theo mã nguồn game.
- Ghi công tác giả gốc: José Tomás Tocino García.
- Mã nguồn (bao gồm phần đã chỉnh sửa) phải ở dạng có thể truy cập được cho
  người nhận bản phân phối.

Không ràng buộc gì nếu chỉ chạy nội bộ/không phân phối ra ngoài.
