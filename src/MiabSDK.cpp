#include "MiabSDK.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <memory>

EM_JS(void, miab_sdk_submit_highscore, (int score, const char* mode, int elapsedMs), {
    var msg = {
        type: 'miab:highscore',
        score: score,
        mode: UTF8ToString(mode),
    };
    if (elapsedMs >= 0) msg.elapsedMs = elapsedMs;
    window.parent.postMessage(msg, window.location.origin);
});

void MiabSDK::SubmitHighscore(int score, const std::string& mode, int elapsedMs) {
    miab_sdk_submit_highscore(score, mode.c_str(), elapsedMs);
}

// Callback lưu ở heap (không có cách map trực tiếp std::function vào EM_JS), giải phóng ngay khi
// message đầu tiên (và duy nhất — host chỉ trả lời 1 lần) về, hoặc không bao giờ nếu response
// không tới (chấp nhận rò rỉ 1 std::function nhỏ trong trường hợp hiếm này — vòng đời game ngắn).
using HighscoreCallback = std::function<void(const std::string&, int, int)>;

extern "C" EMSCRIPTEN_KEEPALIVE
void miab_sdk_on_highscore_data(HighscoreCallback* cb, const char* mode, int score, int elapsedMs) {
    (*cb)(mode, score, elapsedMs);
}

EM_JS(void, miab_sdk_request_highscore, (HighscoreCallback* cb), {
    function handler(event) {
        if (event.origin !== window.location.origin) return;
        if (!event.data || event.data.type !== 'miab:highscoreData') return;
        window.removeEventListener('message', handler);
        var scores = event.data.scores || {};
        for (var mode in scores) {
            if (Object.prototype.hasOwnProperty.call(scores, mode)) {
                var entry = scores[mode];
                var elapsedMs = (entry.elapsedMs === null || entry.elapsedMs === undefined) ? -1 : entry.elapsedMs;
                Module.ccall('miab_sdk_on_highscore_data', 'void', ['number', 'string', 'number', 'number'],
                    [cb, mode, entry.score, elapsedMs]);
            }
        }
    }
    window.addEventListener('message', handler);
    window.parent.postMessage({type: 'miab:requestHighscore'}, window.location.origin);
});

void MiabSDK::RequestHighscore(std::function<void(const std::string&, int, int)> onReceived) {
    auto* cb = new HighscoreCallback(std::move(onReceived));
    miab_sdk_request_highscore(cb);
}

#else

void MiabSDK::SubmitHighscore(int, const std::string&, int) {
    // no-op: build native/platform khác không có host page (MIAB) để báo về.
}

void MiabSDK::RequestHighscore(std::function<void(const std::string&, int, int)>) {
    // no-op: onReceived đơn giản không bao giờ được gọi — game giữ nguyên default (thường 0).
}

#endif
