#include "MiabSDK.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

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

#else

void MiabSDK::SubmitHighscore(int, const std::string&, int) {
    // no-op: build native/platform khác không có host page (MIAB) để báo về.
}

#endif
