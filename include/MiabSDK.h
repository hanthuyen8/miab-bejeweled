#pragma once
#include <string>
#include <functional>

namespace MiabSDK {
    // Báo 1 highscore về host page (MIAB). Gọi khi 1 mode/ván chơi kết thúc.
    // `mode` là bucket leaderboard ổn định (vd "endless", "timetrial").
    // `elapsedMs` = thời gian hoàn thành mode tính bằng mili-giây; truyền -1 nếu mode không có
    // khái niệm "hoàn thành" (vd endless).
    // No-op ngoài build Emscripten/web — an toàn gọi thẳng từ code chung, không cần #ifdef ở call site.
    void SubmitHighscore(int score, const std::string& mode, int elapsedMs = -1);

    // Xin highscore hiện tại của user cho từng mode. Gọi 1 lần lúc game khởi động.
    // `onReceived(mode, score, elapsedMs)` được gọi mỗi khi có 1 mode trong response (elapsedMs =
    // -1 nếu mode đó không có khái niệm "hoàn thành", giống quy ước ở SubmitHighscore) — CÓ THỂ
    // không bao giờ được gọi (network lỗi, session hết hạn, user chưa có record...), game phải tự
    // chọn giá trị mặc định (thường 0) và không được block chờ callback này.
    void RequestHighscore(std::function<void(const std::string& mode, int score, int elapsedMs)> onReceived);
}
