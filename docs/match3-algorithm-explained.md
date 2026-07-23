# Thuật toán match-3 hoạt động thế nào

Giải thích luồng xử lý match-3 thuần thuật toán (không phải rendering) trong
`Board` (`include/Board.h`, `src/Board.cpp`) và state machine điều phối nó ở
`GameBoard` (`include/GameBoard.h`, `src/GameBoard.cpp`). Viết ra vì đây là nơi
sẽ cần mở rộng nếu sau này thêm special gem theo shape hoặc đổi cách tính điểm
(xem [Hướng mở rộng](#hướng-mở-rộng-sau-này) ở cuối).

## Board lưu trữ

`Board::squares` là mảng cố định `std::array<std::array<Square, 8>, 8>`,
index `[x][y]`. Mỗi `Square` (`include/Square.h:20`) gồm một enum loại gem
(`tSquare`: `sqEmpty, sqWhite, sqRed, sqPurple, sqOrange, sqGreen, sqYellow,
sqBlue` — 7 màu) cộng thêm 3 trường phục vụ animation rơi: `mustFall`, `origY`,
`destY`. Board 8x8 cố định, không có khái niệm kích thước động.

## State machine điều phối (`GameBoard`)

`GameBoard::tState` (`GameBoard.h:84-100`):

```
eNoBoard → eBoardAppearing → eSteady ⇄ eGemSelected → eGemSwitching
                                              ↓
eBoardDisappearing ← eTimeFinished    eGemDisappearing ⇄ eBoardFilling
                                              ↑______________|
                                          (lặp nếu cascade)
```

Người chơi chọn 2 gem → `eGemSwitching` (animate swap) → nếu `Board::check()`
sau swap không rỗng thì sang `eGemDisappearing`; nếu rỗng thì swap ngược lại,
quay về `eSteady`. Từ `eGemDisappearing` → xoá gem, gọi
`Board::calcFallMovements()`, sang `eBoardFilling` (animate rơi) → hết
animation thì `check()` lại; còn match thì tăng `mMultiplier`, quay lại
`eGemDisappearing` (đây là vòng lặp cascade); hết match thì gọi
`Board::solutions()`, nếu rỗng thì tạo lại board, ngược lại về `eSteady`.

Toàn bộ logic **và** animation đều nằm chung trong state này — không tách
riêng lớp "thuật toán thuần" khỏi lớp "trình chiếu". Muốn thêm state mới (vd
hiệu ứng đặc biệt khi tạo special gem) sẽ phải chèn thêm case vào
`GameBoard::update()`.

## Phát hiện match — `Board::check()` (`Board.cpp:130-187`)

**Không phải flood-fill.** Chỉ là quét đường thẳng, 2 vòng độc lập:

1. Quét từng hàng trái→phải: gom chuỗi gem giống nhau liên tiếp; chuỗi dài > 2
   thì push vào kết quả.
2. Quét từng cột trên→dưới, y hệt logic trên.

Trả về `MultipleMatch` — một `vector<Match>`, mỗi `Match` là `vector<Coord>`
của các ô thuộc một chuỗi thẳng.

**Hệ quả quan trọng:** thuật toán này **chỉ nhận diện đường thẳng ngang/dọc**,
không nhận diện hình dạng gộp:

- Một match hình chữ L hoặc T (3 ngang + 3 dọc giao nhau) bị trả về thành
  **2 `Match` tách biệt** — mỗi cái từ một vòng quét — không có bước gộp thành
  một vùng liên thông duy nhất.
- Hình vuông 2x2 **không được tính là match** — mỗi hàng/cột trong đó chỉ có
  2 gem liên tiếp, không đạt ngưỡng `> 2`.
- Match dài (4, 5 gem) chỉ được biết qua `Match.size()`, không có thông tin
  "đây là hình chữ L" hay "đây là đường thẳng 5 gem" một cách tường minh —
  phải tự suy luận từ toạ độ trong `Match`.

## Xoá gem matched

Trong state `eGemDisappearing` (`GameBoard.cpp:250-276`), sau animation biến
mất, duyệt từng `Coord` trong `mGroupedSquares` (kết quả `check()` gần nhất)
và gọi `Board::del(x, y)` (`Board.cpp:61`) — chỉ đơn giản set ô đó thành
`sqEmpty`.

## Gravity + Refill — `Board::calcFallMovements()` (`Board.cpp:203-272`)

Xử lý theo từng cột độc lập, 3 pass:

1. **Tính khoảng cách rơi**: quét từ dưới lên, gặp ô rỗng thì mọi ô phía trên
   được đánh dấu `mustFall = true` và cộng dồn `destY++`.
2. **Dịch chuyển vật lý**: quét lại từ dưới lên, ô nào `mustFall` thì chuyển
   xuống `squares[x][y + destY]`, ô cũ set về `sqEmpty`.
3. **Refill**: đếm số ô rỗng còn lại ở đầu cột (trên cùng), rồi gán gem mới
   ngẫu nhiên `static_cast<tSquare>(rand() % 7 + 1)` — phân bố đều trên 7 màu,
   không có trọng số hay điều kiện gì. `origY`/`destY` được set âm để gem
   animate rơi từ ngoài màn hình xuống.

## Cascade — kiểm tra lại sau khi rơi xong

Trong `eBoardFilling` (`GameBoard.cpp:279-334`): sau khi animation rơi hoàn
tất, gọi lại `Board::check()`. Còn match → tăng `mMultiplier`, quay lại
`eGemDisappearing` (lặp lại toàn bộ chu trình xoá → rơi → refill → check).
Hết match → gọi `Board::solutions()` để đảm bảo board còn nước đi hợp lệ.

## Kiểm tra board có nước đi — `Board::solutions()` (`Board.cpp:66-128`)

Brute-force: với **mỗi ô trong 64 ô**, thử swap lần lượt với tối đa 4 ô lân
cận (trên/dưới/trái/phải, có kiểm tra biên), gọi `check()`, rồi swap ngược
lại. Không thử swap chéo hay xa hơn 1 ô.

Chi phí: tối đa 64 × 4 = 256 lần gọi `check()`, mỗi `check()` là O(64) với
early-break trong vòng lặp gom chuỗi (`Board.cpp:159,182`: `x = k - 1` /
`y = k - 1` nhảy qua chuỗi đã gom thay vì quét lại từng ô). Với board 8x8 cố
định (giới hạn bởi kích thước màn hình, không có kế hoạch tăng), độ phức tạp
này không phải bottleneck — chỉ chạy khi tạo board mới (`generate()`,
`Board.cpp:13-51`) hoặc sau khi hết cascade, không chạy mỗi frame. Nếu sau
này board được phép lớn hơn nhiều (vd 20x20), chi phí sẽ tăng theo kiểu
O(n²) trên số ô và cần cân nhắc lại (vd giới hạn kiểm tra quanh vùng vừa
thay đổi thay vì toàn bộ board).

## Flow tổng quát

```
swap gem (eGemSwitching)
  → check() có match?
      không → swap ngược lại → eSteady
      có → eGemDisappearing: del() từng ô matched + calcFallMovements()
         → eBoardFilling: chờ animation rơi
         → check() lại
             có match → tăng multiplier → lặp lại eGemDisappearing (cascade)
             không → solutions() rỗng? → tạo lại board : eSteady
```

## Hướng mở rộng sau này

Ghi lại vì đã bàn tới việc thêm **special gem theo shape combo** và **cách
tính điểm khác** — hai điểm chạm vào đúng chỗ yếu của thiết kế hiện tại:

- **Special gem theo shape** (kiểu Bejeweled: match 4 thẳng → gem sọc, match
  chữ L/T → gem bomb, match 5 → gem màu tuỳ chọn) cần `check()` phân biệt
  được *hình dạng* của match, không chỉ độ dài. Hiện `check()` trả về các
  `Match` hàng/cột **tách rời** — muốn phát hiện L/T phải thêm bước gộp các
  `Match` có chung toạ độ (giao nhau) thành một nhóm, rồi phân loại nhóm đó
  theo shape (thẳng-4, thẳng-5, L, T, hình vuông nếu muốn hỗ trợ luôn). Đây
  là thay đổi trong `check()` hoặc một hàm mới build trên kết quả của nó,
  không đụng tới gravity/refill.
- **Tính điểm kiểu khác** hiện không có trong `Board` — điểm được tính ở phía
  `GameBoard`/`ScoreTable` dựa trên `mMultiplier` và số ô trong `Match`. Muốn
  tính điểm theo shape (vd L/T được nhiều điểm hơn đường thẳng cùng độ dài)
  cần thông tin shape từ bước trên truyền qua, vì hiện `Match` chỉ là danh
  sách toạ độ, không tự mô tả được nó là hình gì.
- `solutions()` brute-force vẫn ổn cho 8x8 kể cả sau khi thêm shape detection,
  vì nó chỉ cần biết "còn match hay không" (gọi `check().empty()`), không cần
  biết shape — không phải sửa khi làm tính năng trên.
