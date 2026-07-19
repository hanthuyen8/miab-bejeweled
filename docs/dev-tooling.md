# Công cụ dev

Phím tắt debug và cách đo hiệu năng. Khác với các `stepN-*-report.md` (báo cáo
lịch sử), file này mô tả **trạng thái hiện tại** — sửa code thì sửa cả đây.

## Phím tắt nhảy màn hình

Bấm được từ bất kỳ đâu. Xử lý ở `Game::handleCheatKey()` (`src/Game.cpp`),
chặn **trước** khi phím được dispatch xuống state hiện tại và nuốt luôn phím
đó để state không nhận được nữa.

| Phím | Tác dụng |
|---|---|
| F1 | Main menu |
| F2 | Endless |
| F3 | Time trial |
| F4 | How to play |
| F5 | Options |
| F6 | +1337 điểm giả vào HUD (bấm nhiều lần để xem số dài ra) |
| F7 | Mở thẳng màn "GAME OVER" với điểm hiện tại |
| F9 | Capture Spector.js (xem mục dưới) |

F6/F7 chỉ có tác dụng khi đang trong ván chơi; ở menu bị bỏ qua. Chúng cần API
riêng vì `StateGame::increaseScore` vốn `protected` — xem
`StateGame::cheatAddScore()` / `cheatShowScoreTable()`.

Lý do tồn tại: màn "GAME OVER" chỉ tới được sau khi chơi hết 2 phút Time trial
(`StateGameTimetrial.cpp:39`), không thể lặp lại nhanh để chỉnh layout.

### F7 cố ý KHÔNG đi qua `endGame()`

`GameBoard::endGame()` gọi `MiabSDK::SubmitHighscore()` → đẩy điểm lên
leaderboard **thật** của host page. Một phím cheat bắn điểm giả lên bảng xếp
hạng thật là không chấp nhận được, nên F7 dùng
`GameBoard::showScoreTableForTesting()` — chỉ dựng `ScoreTable` rồi chuyển
state, không submit.

**Đánh đổi:** F7 không test được luồng submit highscore. Muốn test luồng đó thì
vẫn phải chơi thật.

### Tắt trước khi lên production

Toàn bộ nằm sau `#ifdef SEAJEWELED_CHEATS`, điều khiển bằng
`option(ENABLE_CHEATS ...)` trong `CMakeLists.txt`, **mặc định ON**.

```sh
emcmake cmake -S . -B build-web -DENABLE_CHEATS=OFF
```

Khi OFF thì `handleCheatKey()` chỉ còn một hàm trả `false`, mọi API cheat biến
mất khỏi binary.

## Đo draw call bằng Spector.js

Bấm **F9** để capture frame kế tiếp. Cần thiết vì bấm nút trên UI của extension
quá chậm để bắt hiệu ứng ngắn — particle chỉ sống ~0.8s.

- Extension chỉ inject `window.spector` **sau khi được kích hoạt trên tab đó**:
  bấm icon extension → **reload**. Không có nó thì F9 chỉ log cảnh báo.
- Hotkey nằm trong `platform/web/shell.html` (shell mặc định của Emscripten +
  một listener). Bind trên `window` ở **capture phase** để chạy trước khi SDL
  nuốt event bàn phím trên canvas.
- `CMakeLists.txt` khai báo `LINK_DEPENDS` cho shell — `--shell-file` không
  được CMake coi là dependency, thiếu dòng đó thì sửa shell sẽ **không relink**.

### Đọc số liệu: "copies" khác "batches"

Đây là chỗ **rất dễ hiểu nhầm**, đã mất thời gian vì nó một lần rồi.

- **Số lệnh `SDL_RenderCopyEx`** (số phần tử trong `DrawingQueue`): in-game
  khoảng **79** lúc đứng yên, vọt lên **160+** lúc nổ chuỗi. Con số này nhìn
  đáng sợ nhưng **atlas chưa bao giờ làm giảm nó** — bàn cờ luôn có 64 gem phải
  vẽ, mỗi particle là một lệnh.
- **Số draw call thật (`drawArrays`)**: đây mới là tải GPU, và là con số mà
  [step2](step2-texture-atlas-report.md) đưa từ 73 xuống **13**. Đo được hiện
  tại là **9-10**, gần như không đổi giữa lúc idle và lúc nổ chuỗi.

Batch chỉ nối các draw **liên tiếp dùng chung con trỏ `SDL_Texture`**. Nên quy
tắc là: thứ gì tạo texture **riêng** mới phá batch, còn thêm bao nhiêu lệnh vẽ
từ cùng một atlas cũng gần như miễn phí.

Ví dụ thực tế đã gặp: ~150 lệnh vẽ particle mỗi frame chỉ tốn ~1 draw call (đều
từ atlas), trong khi **2 dòng floating score lại tốn 4 draw call** vì mỗi dòng
tự rasterize texture riêng qua SDL_ttf. Đó là lý do `FloatingScore` được chuyển
sang glyph atlas (xem [ui-visual-polish-report.md](ui-visual-polish-report.md)
mục 4d).

## Chạy bản web ở local

```sh
./build.sh
python3 -m http.server 8765 --directory build-web
# http://localhost:8765/seajeweled.html
```

**Bật "Disable cache" trong tab Network của DevTools.** `python -m http.server`
không gửi header no-cache, browser rất dễ dùng lại `.wasm` cũ — triệu chứng là
sửa code, build lại, mà game không đổi gì.

## Bẫy build cần nhớ

- **Chỉ sửa file trong `media/` → không tự relink.** `--preload-file` là link
  flag nhưng CMake không coi `media/` là dependency. Phải xoá
  `build-web/seajeweled.{data,js,wasm,html}` trước khi `./build.sh`.
- **Sửa `assets/sprites/` hoặc `assets/glyphs/` chưa vào game** — phải Publish
  lại qua TexturePacker (`texture-packer/atlas.tps`).
- Sửa `platform/web/shell.html` thì relink bình thường nhờ `LINK_DEPENDS`.
- Bản native hiện **không link được** trên máy dev (thiếu `SDL2main`); dùng
  target web để verify.

## Xem thêm

- [ui-visual-polish-report.md](ui-visual-polish-report.md) — quyết định về
  hình ảnh, hiệu ứng glint, khoá 60fps.
- [repo-structure.md](repo-structure.md) — asset nằm ở đâu, quy ước code.
- [step2-texture-atlas-report.md](step2-texture-atlas-report.md) — cơ chế batch.
