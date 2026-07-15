#pragma once
#include <string>

namespace MiabSDK {
    // Báo 1 highscore về host page (MIAB). Gọi khi 1 mode/ván chơi kết thúc.
    // `mode` là bucket leaderboard ổn định (vd "endless", "timetrial").
    // `elapsedMs` = thời gian hoàn thành mode tính bằng mili-giây; truyền -1 nếu mode không có
    // khái niệm "hoàn thành" (vd endless).
    // No-op ngoài build Emscripten/web — an toàn gọi thẳng từ code chung, không cần #ifdef ở call site.
    void SubmitHighscore(int score, const std::string& mode, int elapsedMs = -1);
}
