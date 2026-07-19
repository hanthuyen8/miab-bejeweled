# Báo cáo — Tối ưu kích thước build (phiên 2026-07-19)

Giảm `seajeweled.data` từ **5422 KB xuống 4826 KB (−11%)** mà **không mất một
pixel nào**. Ghi lại ở đây vì phần lớn nội dung là quy trình dễ bị xoá âm thầm
khi export lại asset. Xem cấu trúc asset ở [repo-structure.md](repo-structure.md).

## Nguyên tắc đã chốt: chỉ lossless

Đã đo cả phương án lossy và **cố ý không dùng**:

| Phương án | 3 ảnh nền | Kết luận |
|---|---:|---|
| Gốc | 2352 KB | |
| oxipng + bỏ alpha (**đang dùng**) | 1896 KB | lossless |
| pngquant | 690 KB | palette 256 màu → banding gradient |
| JPEG q3 | 265 KB | mất chất lượng thấy rõ |
| WebP q90 | 223 KB | chưa rõ port SDL2_image của Emscripten có hỗ trợ |

Art của game nhiều gradient, nén palette làm lộ banding. Con số nhỏ hơn không
bù được. **Đừng "tối ưu thêm" bằng pngquant/JPEG ở các phiên sau** — đó là
quyết định có cân nhắc, không phải chỗ bỏ sót.

## Nguồn dung lượng

Đo trước khi làm — hai nhóm chiếm 88% toàn bộ `.data`:

- `music.ogg` **2466 KB = 45%** — một file duy nhất.
- 3 ảnh nền 800×600 (`board`, `mainMenuBackground`, `howtoScreen`) = 2352 KB = 43%.

Phần còn lại (atlas, logo, sfx, json) cộng lại chưa tới 12%. Bài học: đo trước
khi tối ưu — cảm giác "atlas chắc nặng" là sai, nó chỉ có 187 KB.

## 1. Bỏ kênh alpha chết (lossless)

Photoshop export RGBA kể cả khi không có gì trong suốt. Cả 3 ảnh nền có
**toàn bộ 480000 pixel đều alpha=255** — kênh alpha chiếm 25% dung lượng file
mà không mang thông tin nào.

Đã kiểm chứng lossless bằng cách so từng pixel kênh RGB trước/sau: giống hệt.

Ảnh thật sự dùng alpha (logo 81.7% pixel trong suốt, atlas 84.7%,
`menuHighlight`, `handCursor`) thì giữ nguyên.

## 2. `oxipng` (lossless)

Chỉ chọn lại bộ lọc PNG và mã hoá lại luồng deflate, **không đụng dữ liệu ảnh**.
Dùng `-o max --strip safe`.

## 3. `mainMenuLogo.png` — ship đúng kích thước hiển thị

Trước: file 1067×239 nhưng `StateMainMenu::draw()` vẽ ở scale `628/1067`. Tức
ship gấp ~2.9 lần số pixel cần, rồi resample xuống **mỗi frame** lúc chạy.

Nay: ship 628×140, vẽ 1:1 (`factorX/Y = 1`). **204 KB → 88 KB**, và nét hơn
(resample offline bằng LANCZOS thay vì bộ lọc runtime).

Bản gốc 1067px cất ở `assets/photoshop/mainMenuLogo.png`. Trước đó logo và
`mainMenuBackground` **không có bản gốc nào trong `assets/`** — file trong
`media/` là bản duy nhất; nay đã đưa cả hai về đúng quy ước.

> Đây là thứ đáng kiểm tra khi thêm ảnh mới: **kích thước file có khớp kích
> thước vẽ ra không?** Không có gì cảnh báo khi lệch.

## 4. Hai chỗ tối ưu bị quy trình xoá âm thầm

Đây là phần quan trọng nhất của báo cáo này.

### `media/atlas.png` → nằm trong `build_atlas.sh`

Mỗi lần Publish TexturePacker **ghi đè** `media/atlas.png`. Nén thủ công sẽ
biến mất ở lần Publish kế tiếp mà không ai biết. Nên bước `oxipng` đã được đưa
thẳng vào `texture-packer/build_atlas.sh`.

### Ảnh rời → `optimize_media.sh`

Export lại bất kỳ ảnh nào từ Photoshop là mất cả alpha-strip lẫn oxipng. Không
có gì bắt được việc đó — build vẫn chạy, chỉ là `.data` phình lại.

**Sau khi export lại ảnh, chạy `./optimize_media.sh`.** Script tự quyết định
file nào bỏ alpha được: chỉ bỏ khi **mọi pixel đều alpha=255**. Không có danh
sách cứng nào để lỗi thời khi thêm ảnh mới.

- Idempotent — chạy lại không đổi gì thêm, không sợ nén chồng nén.
- **Không resize** — thu nhỏ là quyết định riêng của từng ảnh (xem mục 3),
  tự động hoá sẽ nguy hiểm.
- Nhắc xoá `build-web/seajeweled.*` khi có file đổi (xem gotcha bên dưới).

Cần `oxipng` (`brew install oxipng`) và Pillow.

## 5. `music.ogg` — cố ý giữ nguyên

2466 KB, vorbis 160kb/s stereo, 2:32. Đã đo phương án nén:

| oggenc | Size |
|---|---:|
| `-q1` (80kb/s) | 1357 KB |
| `-q2` (96kb/s) | 1584 KB |
| `-q3` (112kb/s) | 1831 KB |

**Quyết định giữ nguyên** — re-encode là lossy, và 2 MB được coi là chấp nhận
được. Sau khi đã tối ưu ảnh, music giờ chiếm **51%** của `.data`, nên đây là
chỗ duy nhất còn dư địa đáng kể nếu sau này thật sự cần giảm nữa.

Lưu ý toolchain: `ffmpeg` của Homebrew **không có `libvorbis`**, encoder
`vorbis` native thì bỏ qua `-b:a` (mọi bitrate ra cùng một size) và chỉ nhận
stereo. Phải dùng `oggenc` (`brew install vorbis-tools`) mới điều khiển được
bitrate. Xem thêm ở [ui-visual-polish-report.md](ui-visual-polish-report.md).

## Kết quả

| File | Trước | Sau |
|---|---:|---:|
| `board.png` | 821 KB | 589 KB |
| `mainMenuBackground.png` | 799 KB | 688 KB |
| `howtoScreen.png` | 732 KB | 619 KB |
| `mainMenuLogo.png` | 204 KB | 88 KB |
| `atlas.png` | 187 KB | 163 KB |
| `menuHighlight.png` | 5 KB | 3 KB |
| `handCursor.png` | 2 KB | 1 KB |
| `music.ogg` | 2466 KB | *giữ nguyên* |
| **`seajeweled.data`** | **5422 KB** | **4826 KB** |

## Gotcha

- **Chỉ sửa `media/` → `build.sh` không tự relink.** `--preload-file` là link
  flag, CMake không coi `media/` là dependency của bước link. Phải xoá
  `build-web/seajeweled.{data,js,wasm,html}` trước khi `./build.sh`, nếu không
  `.data` vẫn là bản cũ. `optimize_media.sh` có nhắc lại điều này khi nó đổi file.
- `wasm` (1401 KB) và `js` (177 KB) **không** nằm trong `.data` — nếu quan tâm
  tổng dung lượng tải về thì phải cộng thêm.
- `atlas.json` (61 KB) + `fonts.json` (36 KB) là text, nén rất tốt qua
  gzip/brotli của web server — không cần tối ưu ở phía asset.
