#pragma once

#include "core/config.hpp"
#include "core/game_state.hpp"
#include "core/i18n.hpp"
#include "media/asset_manager.hpp"
#include "media/audio_engine.hpp"
#include "media/sdl_compat.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
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

// Locate an OS-installed sans-serif font with complete Latin + Vietnamese diacritics.
// Uses system fontconfig or standard OS font directories only (never scans $HOME).
inline SystemFontMatch find_system_latin_font(bool bold = false,
                                              std::string_view lang = "vi",
                                              bool require_index_zero = false) {
    (void)require_index_zero;
    std::string fc_lang = lang.empty() ? std::string("vi") : std::string(lang);

#if defined(LLK2_HAVE_FONTCONFIG)
    if (FcConfig* cfg = FcInitLoadConfigAndFonts()) {
        FcPattern* pat = FcPatternCreate();
        FcPatternAddString(pat, FC_FAMILY, reinterpret_cast<const FcChar8*>("sans-serif"));
        FcPatternAddString(pat, FC_LANG, reinterpret_cast<const FcChar8*>(fc_lang.c_str()));
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

                FcLangSet* ls = nullptr;
                if (FcPatternGetLangSet(f, FC_LANG, 0, &ls) == FcResultMatch && ls) {
                    const FcChar8* req = reinterpret_cast<const FcChar8*>(fc_lang.c_str());
                    if (FcLangSetHasLang(ls, req) == FcLangDifferentLang) {
                        continue;
                    }
                }

                std::string p = reinterpret_cast<const char*>(file);
                if (!std::filesystem::exists(p)) continue;

                int fweight = FC_WEIGHT_REGULAR;
                FcPatternGetInteger(f, FC_WEIGHT, 0, &fweight);
                const bool weight_ok = bold ? (fweight >= FC_WEIGHT_SEMIBOLD)
                                            : (fweight >= FC_WEIGHT_NORMAL && fweight <= FC_WEIGHT_MEDIUM);

                if (!first_valid.valid() && (!require_index_zero || index == 0)) {
                    first_valid = {p, index};
                }
                if (weight_ok && (!require_index_zero || index == 0)) {
                    const auto ext = std::filesystem::path(p).extension().string();
                    if (!require_index_zero || ext == ".ttf" || ext == ".otf" || ext == ".TTF" || ext == ".OTF") {
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

    std::vector<const char*> candidates;
#if defined(_WIN32)
    candidates = bold
        ? std::vector<const char*>{"C:/Windows/Fonts/segoeuib.ttf", "C:/Windows/Fonts/arialbd.ttf", "C:/Windows/Fonts/segoeui.ttf"}
        : std::vector<const char*>{"C:/Windows/Fonts/segoeui.ttf", "C:/Windows/Fonts/arial.ttf", "C:/Windows/Fonts/tahoma.ttf"};
#elif defined(__APPLE__)
    candidates = {
        "/System/Library/Fonts/SFNS.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "/Library/Fonts/Arial Unicode.ttf"
    };
#else
    candidates = bold
        ? std::vector<const char*>{
            "/usr/share/fonts/noto/NotoSans-Bold.ttf",
            "/usr/share/fonts/truetype/noto/NotoSans-Bold.ttf",
            "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
            "/usr/share/fonts/liberation/LiberationSans-Bold.ttf"}
        : std::vector<const char*>{
            "/usr/share/fonts/noto/NotoSans-Regular.ttf",
            "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
            "/usr/share/fonts/TTF/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/liberation/LiberationSans-Regular.ttf"};
#endif

    for (const char* path : candidates) {
        if (std::filesystem::exists(path)) {
            return {path, 0};
        }
    }
    return {};
}

inline std::vector<std::string> ordered_languages(const core::I18n& i18n) {
    static const char* kPrimaryOrder[] = {"zh-TW", "zh-CN", "en", "vi"};
    auto avail = i18n.available_languages();
    std::vector<std::string> out;
    for (const char* code : kPrimaryOrder) {
        if (std::find(avail.begin(), avail.end(), code) != avail.end()) {
            out.emplace_back(code);
        }
    }
    for (const auto& code : avail) {
        if (std::find(out.begin(), out.end(), code) == out.end()) {
            out.push_back(code);
        }
    }
    return out;
}

inline std::string language_display_name(const core::I18n& i18n,
                                         const std::string& lang_code,
                                         bool bui_single_font_safe = false) {
    std::string suffix = lang_code;
    std::replace(suffix.begin(), suffix.end(), '-', '_');
    std::string key = (bui_single_font_safe ? "lang.bui." : "lang.") + suffix;
    std::string val = i18n.tr(key);
    if (val != key) return val;
    std::string fallback_key = "lang." + suffix;
    val = i18n.tr(fallback_key);
    if (val != fallback_key) return val;
    return lang_code;
}

// Single-source-of-truth pixel coordinates for overlaying localized rules text on top of
// the original 640x376 Form_Explain_Picture graphic (keeping all tile diagrams & red border intact).
struct ExplainRect {
    float x, y, w, h;
};

struct ExplainOverlayLayout {
    static constexpr int kImageW = 640;
    static constexpr int kImageH = 376;

    // Exact black-background mask rectangles covering the 4 original vertical Chinese labels
    static constexpr ExplainRect kMaskTurn0    = {  4.0f,  30.0f,  72.0f,  76.0f};
    static constexpr ExplainRect kMaskTurn1    = {  4.0f, 200.0f,  72.0f,  78.0f};
    static constexpr ExplainRect kMaskTurn2Top = {212.0f,  26.0f,  82.0f,  84.0f};
    static constexpr ExplainRect kMaskTurn2Bot = {216.0f, 200.0f,  72.0f,  82.0f};

    // Inner black rectangle inside the iconic 4px red border (outer red frame is x=476..639, y=0..375)
    static constexpr ExplainRect kMaskRulesBox  = {480.0f,   4.0f, 156.0f, 368.0f};
    static constexpr ExplainRect kTextRulesBody = {485.0f,   8.0f, 146.0f, 314.0f};
    static constexpr ExplainRect kTextControls  = {485.0f, 326.0f, 146.0f,  42.0f};
};

struct ExplainOverlayContent {
    bool enabled = false;
    std::string turn_0;
    std::string turn_1;
    std::string turn_2;
    std::string body;
    std::string controls;
};

inline ExplainOverlayContent build_explain_overlay(const media::AssetManager& assets,
                                                   const core::I18n& i18n) {
    ExplainOverlayContent out;
    if (assets.has_locale_explain_override(i18n.language())) {
        return out; // Per-locale image override replaces both image and text
    }
    if (i18n.tr("rules.use_overlay") != "1") {
        return out; // Original Chinese Form_Explain_Picture is already native
    }
    out.enabled = true;
    out.turn_0 = i18n.tr("rules.turn_0");
    out.turn_1 = i18n.tr("rules.turn_1");
    out.turn_2 = i18n.tr("rules.turn_2");
    out.body = i18n.tr("rules.body");
    out.controls = i18n.tr("rules.controls");
    return out;
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
    std::string controller;
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

inline std::string format_endless_stage_label(int level_idx, const core::I18n& i18n) {
    int clamped = std::clamp(level_idx, 0, 6);
    std::string shift = i18n.tr("inspector.shift." + std::to_string(clamped));
    return i18n.format("dialog.endless.stage_fmt", {
        {"num", std::to_string(clamped + 1)},
        {"shift", shift}
    });
}

inline std::string format_stage_clear_body(const core::GameEvent& ev,
                                           bool no_time_limit,
                                           const core::I18n& i18n) {
    if (ev.is_endless) {
        std::string shift = i18n.tr("inspector.shift." + std::to_string(std::clamp(ev.next_shift_level, 0, 6)));
        if (no_time_limit) {
            return i18n.format("msg.endless_clear_notime.body", {
                {"stage", std::to_string(ev.next_level_display)},
                {"shift", shift}
            });
        }
        return i18n.format("msg.endless_clear.body", {
            {"stage", std::to_string(ev.next_level_display)},
            {"shift", shift},
            {"bonus", std::to_string(ev.bonus)}
        });
    }
    return i18n.format("msg.stage_clear.body", {
        {"level", std::to_string(ev.next_level_display)},
        {"bonus", std::to_string(ev.bonus)}
    });
}

inline std::string format_timeout_body(const core::GameEvent& ev,
                                       bool unlimited_shuffle,
                                       const core::I18n& i18n) {
    int total_sc = static_cast<int>(std::lround(ev.total_score));
    if (ev.is_endless && unlimited_shuffle) {
        return i18n.format("msg.timeout_unlim_shuffle.body", {
            {"score", std::to_string(total_sc)}
        });
    }
    return i18n.format("msg.timeout.body", {
        {"bonus", std::to_string(ev.bonus)},
        {"score", std::to_string(total_sc)}
    });
}

inline InspectorSnapshot build_inspector_snapshot(const core::GameState& gs,
                                                  const media::AudioEngine& audio,
                                                  const core::Config& cfg,
                                                  const core::I18n& i18n,
                                                  bool reveal_hint = false,
                                                  const std::string& controller_name = {}) {
    InspectorSnapshot snap;
    if (gs.in_title_screen()) {
        snap.mode = i18n.tr("inspector.mode.title");
    } else if (gs.is_endless()) {
        snap.mode = i18n.tr("inspector.mode.endless");
    } else {
        snap.mode = i18n.tr("inspector.mode.playing");
    }

    std::string shift_key = "inspector.shift." + std::to_string(std::clamp(gs.level(), 0, 6));
    if (gs.is_endless() && !gs.in_title_screen()) {
        snap.stage = "#" + std::to_string(std::max(1, gs.endless_stage_count())) +
                     " (" + i18n.tr(shift_key) + ")";
    } else {
        snap.stage = std::to_string(gs.level() + 1) + " / 7 (" + i18n.tr(shift_key) + ")";
    }
    snap.tiles_left = std::to_string(gs.board().tiles_left()) + " / 144";
    std::string lives_str = gs.unlimited_shuffle() ? i18n.tr("inspector.lives_unlimited")
                                                   : std::to_string(gs.lives());
    snap.lives_score = i18n.tr("inspector.lives") + ": " + lives_str +
                       "   " + i18n.tr("inspector.score") + ": " +
                       std::to_string(static_cast<int>(std::lround(gs.score())));
    snap.time_left = gs.no_time_limit() ? i18n.tr("inspector.time_unlimited")
                                        : (std::to_string(gs.remaining_seconds()) + " s");
    snap.audio_backend = localized_backend_name(cfg.midi_backend, i18n);
    snap.soundfont = audio.active_soundfont_name();

    int track_id = (audio.current_midi_track() >= 0) ? (audio.current_midi_track() + 101) : 101;
    snap.track = std::to_string(track_id) + " (" +
                 (audio.is_midi_playing() ? i18n.tr("inspector.playing") : i18n.tr("inspector.stopped")) + ")";
    snap.controller = controller_name.empty() ? i18n.tr("inspector.controller_none") : controller_name;

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
        if (reveal_hint) {
            snap.next_hint = "(" + std::to_string(c1) + ", " + std::to_string(r1) + ") <-> (" +
                             std::to_string(c2) + ", " + std::to_string(r2) + ")";
        } else {
            snap.next_hint = i18n.tr("inspector.hint_hidden");
        }
    } else {
        snap.next_hint = "-";
    }
    return snap;
}

inline bool open_external_url(const std::string& url) {
    if (url.empty()) return false;
    return media::sdl_open_url(url.c_str());
}

// Shared About dialog geometry: original Form_Author_Picture.jpg (370x310, pure black) on the left,
// subtle vertical divider in the middle margin, and localized colored credits with clickable links on the right.
struct AboutLayout {
    static constexpr int kImageW = 370;
    static constexpr int kImageH = 310;
    static constexpr int kDividerX = 376;
    static constexpr int kDividerTop = 22;
    static constexpr int kDividerBottom = 288;
    static constexpr int kTextLeft = 398;
    static constexpr int kTextRightPad = 20;
    static constexpr int kTotalW = 684;
    static constexpr int kTotalH = 310;
    static constexpr int kTextWidth = kTotalW - kTextLeft - kTextRightPad; // 266px
};

struct AboutColor {
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
};

struct AboutSpan {
    std::string text;
    std::string url; // empty for plain text, non-empty for clickable hyperlink
};

struct AboutParagraph {
    AboutColor text_color;
    AboutColor link_color;
    AboutColor hover_color;
    std::vector<AboutSpan> spans;
};

struct AboutContent {
    std::vector<AboutParagraph> paragraphs;
};

// Parse inline Markdown links `[Label](https://...)` into ordered plain/link spans.
inline std::vector<AboutSpan> parse_about_markdown_spans(std::string_view src) {
    std::vector<AboutSpan> out;
    auto push_plain = [&](std::string_view sv) {
        if (sv.empty()) return;
        if (!out.empty() && out.back().url.empty()) {
            out.back().text.append(sv);
        } else {
            out.push_back(AboutSpan{std::string(sv), {}});
        }
    };

    size_t pos = 0;
    while (pos < src.size()) {
        size_t lb = src.find('[', pos);
        if (lb == std::string_view::npos) {
            push_plain(src.substr(pos));
            break;
        }
        size_t rb = src.find("](", lb + 1);
        if (rb == std::string_view::npos) {
            push_plain(src.substr(pos));
            break;
        }
        size_t rp = src.find(')', rb + 2);
        if (rp == std::string_view::npos) {
            push_plain(src.substr(pos));
            break;
        }
        std::string_view label = src.substr(lb + 1, rb - (lb + 1));
        std::string_view url = src.substr(rb + 2, rp - (rb + 2));
        bool valid_url = (url.rfind("http://", 0) == 0 || url.rfind("https://", 0) == 0);
        if (label.empty() || !valid_url) {
            push_plain(src.substr(pos, (lb + 1) - pos));
            pos = lb + 1;
            continue;
        }
        push_plain(src.substr(pos, lb - pos));
        out.push_back(AboutSpan{std::string(label), std::string(url)});
        pos = rp + 1;
    }
    return out;
}

inline AboutContent build_about_content(const core::I18n& i18n) {
    AboutContent content;

    // 1. Original program (Blue)
    content.paragraphs.push_back(AboutParagraph{
        AboutColor{88, 166, 255},
        AboutColor{138, 198, 255},
        AboutColor{196, 228, 255},
        parse_about_markdown_spans(i18n.tr("about.credits.original")),
    });

    // 2. Simplified Chinese translation (Green)
    content.paragraphs.push_back(AboutParagraph{
        AboutColor{62, 232, 88},
        AboutColor{126, 245, 144},
        AboutColor{188, 255, 198},
        parse_about_markdown_spans(i18n.tr("about.credits.sc_trans")),
    });

    // 3. Unofficial Remastered edition (Yellow)
    content.paragraphs.push_back(AboutParagraph{
        AboutColor{255, 232, 74},
        AboutColor{255, 244, 146},
        AboutColor{255, 251, 206},
        parse_about_markdown_spans(i18n.tr("about.credits.remaster")),
    });

    return content;
}

struct LaidOutAboutChunk {
    std::string text;
    std::string url;
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    AboutColor color;
    AboutColor hover_color;
    bool is_link = false;
};

struct AboutTextLayout {
    std::vector<LaidOutAboutChunk> chunks;
    float total_height = 0.0f;
};

namespace detail {

inline uint32_t decode_utf8_at(std::string_view s, size_t pos, size_t& out_len) {
    unsigned char c0 = static_cast<unsigned char>(s[pos]);
    if (c0 < 0x80) {
        out_len = 1;
        return c0;
    }
    if ((c0 & 0xE0) == 0xC0 && pos + 1 < s.size()) {
        out_len = 2;
        return ((c0 & 0x1Fu) << 6) | (static_cast<unsigned char>(s[pos + 1]) & 0x3Fu);
    }
    if ((c0 & 0xF0) == 0xE0 && pos + 2 < s.size()) {
        out_len = 3;
        return ((c0 & 0x0Fu) << 12) |
               ((static_cast<unsigned char>(s[pos + 1]) & 0x3Fu) << 6) |
               (static_cast<unsigned char>(s[pos + 2]) & 0x3Fu);
    }
    if ((c0 & 0xF8) == 0xF0 && pos + 3 < s.size()) {
        out_len = 4;
        return ((c0 & 0x07u) << 18) |
               ((static_cast<unsigned char>(s[pos + 1]) & 0x3Fu) << 12) |
               ((static_cast<unsigned char>(s[pos + 2]) & 0x3Fu) << 6) |
               (static_cast<unsigned char>(s[pos + 3]) & 0x3Fu);
    }
    out_len = 1;
    return c0;
}

inline bool is_cjk_opening_punct(uint32_t cp) {
    return cp == 0xFF08u || cp == 0x300Cu || cp == 0x300Eu || cp == 0x300Au || cp == 0x3010u;
}

inline bool is_cjk_closing_punct(uint32_t cp) {
    return cp == 0xFF09u || cp == 0x3002u || cp == 0xFF0Cu || cp == 0x3001u ||
           cp == 0xFF1Au || cp == 0xFF1Bu || cp == 0x300Du || cp == 0x300Fu ||
           cp == 0x300Bu || cp == 0x3011u;
}

inline bool is_cjk_breakable(uint32_t cp) {
    return (cp >= 0x2E80u && cp <= 0x9FFFu) ||
           (cp >= 0xF900u && cp <= 0xFAFFu) ||
           (cp >= 0xFF00u && cp <= 0xFFEFu);
}

} // namespace detail

// Shared UTF-8 rich-text line wrapper supporting Latin/Vietnamese word wrapping, CJK wrapping,
// explicit '\n' breaks, and seamless multi-word clickable hyperlinks across ImGui and BUI.
inline AboutTextLayout layout_about_paragraphs(
    const AboutContent& content,
    float max_width,
    float line_height,
    float para_gap,
    const std::function<float(std::string_view)>& measure_width,
    bool center_align = true) {
    AboutTextLayout result;
    if (max_width <= 8.0f || content.paragraphs.empty()) return result;

    struct Piece {
        std::string text;
        std::string url;
    };
    struct Cluster {
        enum class Kind { Text, Space, Newline };
        Kind kind = Kind::Text;
        std::vector<Piece> pieces;
    };

    float cur_y = 0.0f;

    for (size_t p_idx = 0; p_idx < content.paragraphs.size(); ++p_idx) {
        const auto& para = content.paragraphs[p_idx];
        if (p_idx > 0) {
            cur_y += para_gap;
        }

        // 1. Tokenize spans into unbreakable word/CJK clusters
        std::vector<Cluster> clusters;
        Cluster cur;
        bool cur_ends_with_open_cjk = false;
        bool cur_ends_with_cjk = false;

        auto append_piece = [&](Cluster& cl, std::string_view sv, const std::string& url) {
            if (!cl.pieces.empty() && cl.pieces.back().url == url) {
                cl.pieces.back().text.append(sv);
            } else {
                cl.pieces.push_back(Piece{std::string(sv), url});
            }
        };
        auto flush_cur = [&]() {
            if (!cur.pieces.empty()) {
                clusters.push_back(std::move(cur));
                cur = Cluster{};
                cur_ends_with_open_cjk = false;
                cur_ends_with_cjk = false;
            }
        };

        for (const auto& span : para.spans) {
            std::string_view sv = span.text;
            size_t i = 0;
            while (i < sv.size()) {
                size_t len = 1;
                uint32_t cp = detail::decode_utf8_at(sv, i, len);
                std::string_view ch = sv.substr(i, len);
                i += len;

                if (cp == '\r') continue;
                if (cp == '\n') {
                    flush_cur();
                    Cluster nl;
                    nl.kind = Cluster::Kind::Newline;
                    clusters.push_back(std::move(nl));
                    continue;
                }
                if (cp == ' ' || cp == '\t') {
                    flush_cur();
                    Cluster sp;
                    sp.kind = Cluster::Kind::Space;
                    sp.pieces.push_back(Piece{" ", span.url});
                    clusters.push_back(std::move(sp));
                    continue;
                }
                if (detail::is_cjk_closing_punct(cp)) {
                    append_piece(cur, ch, span.url);
                    cur_ends_with_open_cjk = false;
                    cur_ends_with_cjk = false;
                    flush_cur();
                    continue;
                }
                if (detail::is_cjk_opening_punct(cp)) {
                    flush_cur();
                    append_piece(cur, ch, span.url);
                    cur_ends_with_open_cjk = true;
                    cur_ends_with_cjk = false;
                    continue;
                }
                if (detail::is_cjk_breakable(cp)) {
                    if (!cur_ends_with_open_cjk) {
                        flush_cur();
                    }
                    append_piece(cur, ch, span.url);
                    cur_ends_with_open_cjk = false;
                    cur_ends_with_cjk = true;
                    continue;
                }
                // ASCII / Latin / Vietnamese character — break after preceding CJK ideograph
                if (cur_ends_with_cjk) {
                    flush_cur();
                }
                append_piece(cur, ch, span.url);
                cur_ends_with_open_cjk = false;
                cur_ends_with_cjk = false;
            }
        }
        flush_cur();

        // 2. Wrap clusters onto lines and emit merged LaidOutAboutChunks
        std::vector<Piece> line_pieces;
        float line_w = 0.0f;

        auto measure_cluster = [&](const Cluster& cl) -> float {
            std::string full;
            for (const auto& pc : cl.pieces) full.append(pc.text);
            return measure_width(full);
        };

        auto flush_line = [&]() {
            // Trim trailing spaces so underlines/widths stay crisp
            while (!line_pieces.empty()) {
                auto& back = line_pieces.back().text;
                while (!back.empty() && back.back() == ' ') back.pop_back();
                if (back.empty()) {
                    line_pieces.pop_back();
                } else {
                    break;
                }
            }
            // Merge adjacent pieces sharing the same URL for continuous kerning & underline
            std::vector<Piece> merged;
            for (auto& pc : line_pieces) {
                if (pc.text.empty()) continue;
                if (!merged.empty() && merged.back().url == pc.url) {
                    merged.back().text.append(pc.text);
                } else {
                    merged.push_back(std::move(pc));
                }
            }
            float x = 0.0f;
            size_t row_start = result.chunks.size();
            for (auto& pc : merged) {
                float w = measure_width(pc.text);
                LaidOutAboutChunk ch;
                ch.text = std::move(pc.text);
                ch.url = std::move(pc.url);
                ch.x = x;
                ch.y = cur_y;
                ch.w = w;
                ch.h = line_height;
                ch.is_link = !ch.url.empty();
                ch.color = ch.is_link ? para.link_color : para.text_color;
                ch.hover_color = para.hover_color;
                result.chunks.push_back(std::move(ch));
                x += w;
            }
            if (center_align && x < max_width) {
                float offset = std::floor((max_width - x) * 0.5f);
                for (size_t k = row_start; k < result.chunks.size(); ++k) {
                    result.chunks[k].x += offset;
                }
            }
            cur_y += line_height;
            line_pieces.clear();
            line_w = 0.0f;
        };

        for (const auto& cl : clusters) {
            if (cl.kind == Cluster::Kind::Newline) {
                flush_line();
                continue;
            }
            if (cl.kind == Cluster::Kind::Space) {
                if (line_pieces.empty()) continue; // never start a wrapped line with a space
                float sw = measure_cluster(cl);
                line_pieces.push_back(cl.pieces[0]);
                line_w += sw;
                continue;
            }
            float cw = measure_cluster(cl);
            if (!line_pieces.empty() && line_w + cw > max_width) {
                flush_line();
            }
            for (const auto& pc : cl.pieces) {
                line_pieces.push_back(pc);
            }
            line_w += cw;
        }
        if (!line_pieces.empty()) {
            flush_line();
        }
    }

    result.total_height = cur_y;
    return result;
}

} // namespace llk2::ui::common
