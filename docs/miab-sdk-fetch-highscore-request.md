# Đề xuất bổ sung Highscore SDK: chiều host → game (fetch highscore lúc start)

Gửi cho agent phụ trách React (`miab-v1`) để review + implement. Đây là **đề xuất**, không phải
spec đã chốt — nếu OK thì người phụ trách `docs/games/highscore-sdk.md` bên `miab-v1` merge nội
dung này vào (file đó chỉ agent React được sửa, nên không tự động cập nhật ở đây).

## Vấn đề

Sau khi ván chơi kết thúc, UI game hiển thị `Latest high score: 0` — vì SDK hiện tại
(`docs/games/highscore-sdk.md` mục 2, "Message contract") chỉ có 1 chiều: game → host (submit điểm
mới qua `postMessage({type: 'miab:highscore', ...})`). Game không có cách nào biết highscore hiện
tại của user để hiển thị.

## Giải pháp: game chủ động fetch lúc start, async, không block

Game gửi request lúc khởi động; host trả lời bất kỳ lúc nào fetch xong (không đồng bộ). Nếu response
chưa về kịp lúc cần hiển thị, game cứ dùng mặc định `0` — không có cơ chế chờ/timeout/retry ở tầng
SDK. Vì chính game chủ động gửi request, không có race-condition timing như phương án host tự đẩy
data xuống lúc mount iframe (JS glue code của game chắc chắn đã load xong tại thời điểm gửi request,
còn phía host thì listener đã được React gắn từ lúc mount iframe, trước khi game kịp postMessage).

### 1. Game → host

```js
window.parent.postMessage({
    type: 'miab:requestHighscore',
}, window.location.origin);
```

Gọi 1 lần lúc WASM module init xong, không cần chờ user vào màn chơi.

### 2. Host → game

```js
iframe.contentWindow.postMessage({
    type: 'miab:highscoreData',
    scores: {
        endless: 1200,
        timetrial: 340,
        // key = đúng chuỗi `mode` mà game dùng khi submit (mục 2 spec gốc). Mode nào user CHƯA
        // từng có record thì KHÔNG xuất hiện trong object này — không phải lỗi, game tự hiểu là
        // "chưa có" và giữ giá trị mặc định.
    },
}, window.location.origin);
```

Quy ước:
- Request/response **bất đồng bộ, không timeout, không retry** ở tầng SDK. Response có thể đến sau
  vài trăm ms (network round-trip), hoặc không bao giờ đến (lỗi mạng, session hết hạn). Đây là
  quyết định có chủ đích — `Latest high score` chỉ mang tính hiển thị tham khảo, không phải logic
  chơi game, nên chấp nhận hiển thị `0` nếu response chưa kịp về (vd user thua ngay ván đầu).
- Chỉ gọi 1 lần lúc start — mỗi lần user vào lại game là 1 lần load trang mới (iframe reload), state
  cũ (kể cả MEMFS phía WASM) đã mất theo nên không cần request lại giữa ván.
- `scores` là object phẳng `mode -> score` để mở rộng nhiều mode mà không cần đổi contract. Không
  cần field nào khác (không cần `elapsedMs` ở chiều này — đây là điểm cao nhất từng đạt, không phải
  1 lần chơi cụ thể).

## Việc cần làm ở phía React/server (miab-v1)

- `<Game>Screen.tsx`: thêm nhánh trong listener `message` đã có sẵn (cùng validate
  `event.origin === window.location.origin` + `event.source === iframe.contentWindow` như luồng
  submit) — khi `event.data.type === 'miab:requestHighscore'`, gọi 1 API mới lấy toàn bộ highscore
  của user cho `gameId` này, rồi `iframe.contentWindow.postMessage({type: 'miab:highscoreData',
  scores: {...}}, window.location.origin)`.
- Server: thêm `GET /api/games/:gameId/highscore` (`verifySession`, cookie-only, cùng pattern với
  route `POST` hiện có) → `SELECT mode, score FROM game_highscore WHERE uid = ? AND game_id = ?`,
  trả về map `mode -> score`. Dùng lại bảng `game_highscore`
  (`sql/2026-07-15-game-highscore.sql`) đã có sẵn, không cần migration mới.
- Cùng origin nên không cần cấu hình CORS cho endpoint này.

## Phía C++ (đã implement trong repo game, tham khảo — không cần đụng)

`include/MiabSDK.h` / `src/MiabSDK.cpp` sẽ có thêm hàm `RequestHighscore`, gọi lúc `Game` khởi tạo,
callback cập nhật `OptionsManager` (chỗ `ScoreTable` đang đọc `Latest high score` từ đó):

```cpp
// MiabSDK.h — thêm cạnh SubmitHighscore
namespace MiabSDK {
    void SubmitHighscore(int score, const std::string& mode, int elapsedMs = -1);

    // Xin highscore hiện tại của user cho từng mode. Gọi 1 lần lúc game khởi động.
    // `onReceived(mode, score)` được gọi mỗi khi có 1 mode trong response — CÓ THỂ không bao giờ
    // được gọi (network lỗi, session hết hạn...), game phải tự chọn giá trị mặc định (thường 0) và
    // không được block chờ callback này.
    void RequestHighscore(std::function<void(const std::string& mode, int score)> onReceived);
}
```

```cpp
// Gọi trong Game::Game() (src/Game.cpp), trước khi vào state đầu tiên
MiabSDK::RequestHighscore([this](const std::string& mode, int score) {
    if (mode == "endless") mOptions.setHighscoreEndless(score);
    else if (mode == "timetrial") mOptions.setHighscoreTimetrial(score);
});
```

Chưa implement phần C++ này — sẽ làm sau khi phía React/server ở trên xong, để test end-to-end được
luôn.
