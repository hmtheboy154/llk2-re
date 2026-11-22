#pragma once

#include "core/config.hpp"
#include "core/game_state.hpp"
#include "core/i18n.hpp"
#include "media/audio_engine.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#if defined(LLK2_HAVE_FONTCONFIG)
#include <fontconfig/fontconfig.h>
#endif

namespace llk2::ui::common {

inline uint64_t now_steady_ms() noexcept {
    using clock = std::chrono::steady_clock;
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(clock::now().time_since_epoch()).count());
}

// Strips Win32/Qt '&' mnemonic markers (e.g. "遊戲(&G)" -> "遊戲(G)", "A&&B" -> "A&B").
inline std::string strip_mnemonic(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '&') {
            if (i + 1 < text.size() && text[i + 1] == '&') {
                out.push_back('&');
                ++i;
            }
            continue;
        }
        out.push_back(text[i]);
    }
    return out;
}

inline std::vector<std::string> split_lines(std::string_view text) {
    std::vector<std::string> lines;
    size_t start = 0;
    while (start <= text.size()) {
        size_t nl = text.find('\n', start);
        if (nl == std::string_view::npos) {
            lines.emplace_back(text.substr(start));
            break;
        }
        std::string_view seg = text.substr(start, nl - start);
        if (!seg.empty() && seg.back() == '\r') {
            seg.remove_suffix(1);
        }
        lines.emplace_back(seg);
        start = nl + 1;
    }
    return lines;
}

inline std::vector<uint8_t> read_file_bytes(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return {};
    std::streamsize sz = f.tellg();
    if (sz <= 0) return {};
    std::vector<uint8_t> buf(static_cast<size_t>(sz));
    f.seekg(0, std::ios::beg);
    if (!f.read(reinterpret_cast<char*>(buf.data()), sz)) {
        return {};
    }
    return buf;
}

struct SystemFontMatch {
    std::string path;
    int face_index = 0;

    bool valid() const noexcept { return !path.empty(); }
};

// Locate an OS-installed sans-serif font with Traditional & Simplified Chinese glyphs.
// Uses system fontconfig or standard OS font directories only (never scans $HOME).
inline SystemFontMatch find_system_cjk_font(bool bold = false,
                                            std::string_view lang = "zh-TW",
                                            bool require_index_zero = false) {
    (void)require_index_zero;
    const bool prefer_sc = (lang == "zh-CN" || lang == "zh-cn");

#if defined(LLK2_HAVE_FONTCONFIG)
    if (FcConfig* cfg = FcInitLoadConfigAndFonts()) {
        FcPattern* pat = FcPatternCreate();
        FcPatternAddString(pat, FC_FAMILY, reinterpret_cast<const FcChar8*>("sans-serif"));
        FcPatternAddString(pat, FC_LANG, reinterpret_cast<const FcChar8*>(prefer_sc ? "zh-cn" : "zh-tw"));
        FcPatternAddInteger(pat, FC_WEIGHT, bold ? FC_WEIGHT_BOLD : FC_WEIGHT_REGULAR);
        FcPatternAddInteger(pat, FC_SLANT, FC_SLANT_ROMAN);
        FcConfigSubstitute(cfg, pat, FcMatchPattern);
        FcDefaultSubstitute(pat);

        SystemFontMatch first_valid;
        FcResult res = FcResultMatch;
        if (FcFontSet* fs = FcFontSort(cfg, pat, FcFalse, nullptr, &res)) {
            for (int i = 0; i < fs->nfont; ++i) {
                FcPattern* f = fs->fonts[i];
                FcChar8* file = nullptr;
                int index = 0;
                if (FcPatternGetString(f, FC_FILE, 0, &file) != FcResultMatch || !file) {
                    continue;
                }
                FcPatternGetInteger(f, FC_INDEX, 0, &index);

                // Ensure the matched face actually supports the target Chinese script.
                FcLangSet* ls = nullptr;
                if (FcPatternGetLangSet(f, FC_LANG, 0, &ls) == FcResultMatch && ls) {
                    const FcChar8* req = reinterpret_cast<const FcChar8*>(prefer_sc ? "zh-cn" : "zh-tw");
                    if (FcLangSetHasLang(ls, req) == FcLangDifferentLang) {
                        continue;
                    }
                }

                std::string p = reinterpret_cast<const char*>(file);
                if (!std::filesystem::exists(p)) continue;

                // Prefer fonts matching requested weight closely when scanning FcFontSort.
                int fweight = FC_WEIGHT_REGULAR;
                FcPatternGetInteger(f, FC_WEIGHT, 0, &fweight);
                const bool weight_ok = bold ? (fweight >= FC_WEIGHT_SEMIBOLD)
                                            : (fweight >= FC_WEIGHT_NORMAL && fweight <= FC_WEIGHT_MEDIUM);

                if (!first_valid.valid() && (!require_index_zero || index == 0)) {
                    first_valid = {p, index};
                }

                if (weight_ok && (!require_index_zero || index == 0)) {
                    // Prefer single-face .otf/.ttf files when index 0 is required
                    const auto ext = std::filesystem::path(p).extension().string();
                    if (!require_index_zero || ext == ".otf" || ext == ".ttf" || ext == ".OTF" || ext == ".TTF") {
                        FcFontSetDestroy(fs);
                        FcPatternDestroy(pat);
                        FcConfigDestroy(cfg);
                        return {p, index};
                    }
                }
            }
            FcFontSetDestroy(fs);
        }
        FcPatternDestroy(pat);
        FcConfigDestroy(cfg);
        if (first_valid.valid()) {
            return first_valid;
        }
    }
#endif

    // Deterministic OS system font fallback list (no user directory scanning)
    std::vector<const char*> candidates;
#if defined(_WIN32)
    if (prefer_sc) {
        candidates = bold
            ? std::vector<const char*>{"C:/Windows/Fonts/msyhbd.ttc", "C:/Windows/Fonts/msyh.ttc", "C:/Windows/Fonts/simhei.ttf"}
            : std::vector<const char*>{"C:/Windows/Fonts/msyh.ttc", "C:/Windows/Fonts/msjh.ttc", "C:/Windows/Fonts/simhei.ttf"};
    } else {
        candidates = bold
            ? std::vector<const char*>{"C:/Windows/Fonts/msjhbd.ttc", "C:/Windows/Fonts/msjh.ttc", "C:/Windows/Fonts/msyh.ttc"}
            : std::vector<const char*>{"C:/Windows/Fonts/msjh.ttc", "C:/Windows/Fonts/msyh.ttc", "C:/Windows/Fonts/mingliu.ttc"};
    }
#elif defined(__APPLE__)
    candidates = {
        "/System/Library/Fonts/PingFang.ttc",
        "/System/Library/Fonts/STHeiti Medium.ttc",
        "/System/Library/Fonts/Hiragino Sans GB.ttc",
        "/Library/Fonts/Arial Unicode.ttf"
    };
#else
    if (prefer_sc) {
        candidates = bold
            ? std::vector<const char*>{
                "/usr/share/fonts/adobe-source-han-sans/SourceHanSansCN-Bold.otf",
                "/usr/share/fonts/adobe-source-han-sans/SourceHanSansTW-Bold.otf",
                "/usr/share/fonts/noto-cjk/NotoSansCJK-Bold.ttc",
                "/usr/share/fonts/opentype/noto/NotoSansCJK-Bold.ttc"}
            : std::vector<const char*>{
                "/usr/share/fonts/adobe-source-han-sans/SourceHanSansCN-Regular.otf",
                "/usr/share/fonts/adobe-source-han-sans/SourceHanSansTW-Regular.otf",
                "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
                "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
                "/usr/share/fonts/truetype/wqy/wqy-microhei.ttc",
                "/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc",
                "/usr/share/fonts/adobe-source-han-sans/SourceHanSans.ttc"};
    } else {
        candidates = bold
            ? std::vector<const char*>{
                "/usr/share/fonts/adobe-source-han-sans/SourceHanSansTW-Bold.otf",
                "/usr/share/fonts/adobe-source-han-sans/SourceHanSansCN-Bold.otf",
                "/usr/share/fonts/noto-cjk/NotoSansCJK-Bold.ttc",
                "/usr/share/fonts/opentype/noto/NotoSansCJK-Bold.ttc"}
            : std::vector<const char*>{
                "/usr/share/fonts/adobe-source-han-sans/SourceHanSansTW-Regular.otf",
                "/usr/share/fonts/adobe-source-han-sans/SourceHanSansCN-Regular.otf",
                "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
                "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
                "/usr/share/fonts/truetype/wqy/wqy-microhei.ttc",
                "/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc",
                "/usr/share/fonts/adobe-source-han-sans/SourceHanSans.ttc"};
    }
#endif

    for (const char* path : candidates) {
        if (std::filesystem::exists(path)) {
            return {path, 0};
        }
    }
    return {};
}

struct InspectorSnapshot {
    std::string mode;
    std::string stage;
    std::string tiles_left;
    std::string lives_score;
    std::string time_left;
    std::string audio_backend;
    std::string soundfont;
    std::string track;
    std::string next_hint;
    bool has_valid_hint = false;
    int hint_c1 = -1;
    int hint_r1 = -1;
    int hint_c2 = -1;
    int hint_r2 = -1;
};

inline std::string localized_backend_name(core::MidiBackend backend, const core::I18n& i18n) {
    switch (backend) {
        case core::MidiBackend::OstAudio:
            return i18n.tr("menu.settings.midi_ost");
        case core::MidiBackend::NativeOS:
            return i18n.tr("menu.settings.midi_native");
        case core::MidiBackend::EmbeddedSoundFont:
            return i18n.tr("menu.settings.midi_embedded");
    }
    return "?";
}

inline InspectorSnapshot build_inspector_snapshot(const core::GameState& gs,
                                                  const media::AudioEngine& audio,
                                                  const core::Config& cfg,
                                                  const core::I18n& i18n) {
    InspectorSnapshot snap;
    snap.mode = gs.in_title_screen() ? i18n.tr("inspector.mode.title")
                                     : i18n.tr("inspector.mode.playing");

    std::string shift_key = "inspector.shift." + std::to_string(std::clamp(gs.level(), 0, 6));
    snap.stage = std::to_string(gs.level() + 1) + " / 7 (" + i18n.tr(shift_key) + ")";
    snap.tiles_left = std::to_string(gs.board().tiles_left()) + " / 144";
    snap.lives_score = i18n.tr("inspector.lives") + ": " + std::to_string(gs.lives()) +
                       "   " + i18n.tr("inspector.score") + ": " +
                       std::to_string(static_cast<int>(std::lround(gs.score())));
    snap.time_left = std::to_string(gs.remaining_seconds()) + " s";
    snap.audio_backend = localized_backend_name(cfg.midi_backend, i18n);
    snap.soundfont = audio.active_soundfont_name();

    int track_id = (audio.current_midi_track() >= 0) ? (audio.current_midi_track() + 101) : 101;
    snap.track = std::to_string(track_id) + " (" +
                 (audio.is_midi_playing() ? i18n.tr("inspector.playing") : i18n.tr("inspector.stopped")) + ")";

    int c1 = static_cast<int>(std::lround(gs.board().hint_x1()));
    int r1 = static_cast<int>(std::lround(gs.board().hint_y1()));
    int c2 = static_cast<int>(std::lround(gs.board().hint_x2()));
    int r2 = static_cast<int>(std::lround(gs.board().hint_y2()));
    if (!gs.in_title_screen() && gs.board_active() &&
        c1 >= 1 && c1 <= core::Board::kPlayCols && r1 >= 1 && r1 <= core::Board::kPlayRows &&
        c2 >= 1 && c2 <= core::Board::kPlayCols && r2 >= 1 && r2 <= core::Board::kPlayRows &&
        gs.board().cell(c1, r1).occupied && gs.board().cell(c2, r2).occupied) {
        snap.has_valid_hint = true;
        snap.hint_c1 = c1;
        snap.hint_r1 = r1;
        snap.hint_c2 = c2;
        snap.hint_r2 = r2;
        snap.next_hint = "(" + std::to_string(c1) + ", " + std::to_string(r1) + ") <-> (" +
                         std::to_string(c2) + ", " + std::to_string(r2) + ")";
    } else {
        snap.next_hint = "-";
    }
    return snap;
}

} // namespace llk2::ui::common
