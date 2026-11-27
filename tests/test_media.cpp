#include "core/config.hpp"
#include "core/game_state.hpp"
#include "core/i18n.hpp"
#include "media/asset_manager.hpp"
#include "media/audio_engine.hpp"
#include "media/board_renderer.hpp"
#include "media/gamepad_input.hpp"
#include "ui/common/ui_helpers.hpp"

#undef NDEBUG
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

using namespace llk2::core;
using namespace llk2::media;

static std::filesystem::path ensure_test_assets(bool& created_temp) {
    created_temp = false;
    std::filesystem::path src_dir(LLK2_SOURCE_DIR);

    // Check if assets already exist in standard locations
    for (const auto& candidate : {src_dir / "assets", src_dir / "src" / "assets", src_dir / "src-llkcn" / "assets"}) {
        if (std::filesystem::exists(candidate / "images") && std::filesystem::exists(candidate / "sfx")) {
            return candidate;
        }
    }

    // Extract from original D4s.exe into a temporary test directory if available
    std::filesystem::path exe_candidate = src_dir.parent_path() / "D4s.exe";
    std::filesystem::path extractor = src_dir / "tools" / "extract_assets.py";
    if (std::filesystem::exists(exe_candidate) && std::filesystem::exists(extractor)) {
        auto tmp_assets = std::filesystem::temp_directory_path() / "llk2_test_extracted_assets";
#if defined(_WIN32)
        std::string cmd = "\"python \"" + extractor.string() + "\" \"" + exe_candidate.string() +
                          "\" -o \"" + tmp_assets.string() + "\" > NUL 2>&1\"";
#else
        std::string cmd = "python3 \"" + extractor.string() + "\" \"" + exe_candidate.string() +
                          "\" -o \"" + tmp_assets.string() + "\" > /dev/null 2>&1";
#endif
        if (std::system(cmd.c_str()) == 0 && std::filesystem::exists(tmp_assets / "images")) {
            created_temp = true;
            return tmp_assets;
        }
    }
    return {};
}

static void test_renderer_and_audio_with_assets(const std::filesystem::path& assets_dir) {
    AssetManager assets;
    std::string err;
    bool ok = assets.load_from_dir(assets_dir, &err);
    if (!ok) {
        std::fprintf(stderr, "AssetManager::load_from_dir failed: %s\n", err.c_str());
    }
    assert(ok);
    assert(assets.is_loaded());
    assert(assets.pic_title().width == 500 && assets.pic_title().height == 375);
    assert(assets.pic_data().width == 510 && assets.pic_data().height == 240);
    assert(assets.pic_mjdata().width == 360 && assets.pic_mjdata().height == 612);
    assert(assets.form_explain().width == 640 && assets.form_explain().height == 376);
    assert(assets.form_author().width == 370 && assets.form_author().height == 310);
    for (int i = 0; i < 7; ++i) {
        assert(assets.pic_level(i).width == 600 && assets.pic_level(i).height == 450);
    }

    // 1. Verify BoardRenderer title screen & active game board rendering
    GameState gs;
    BoardRenderer renderer(assets);
    renderer.render(gs);
    assert(renderer.canvas().width == 768 && renderer.canvas().height == 550);

    // Title screen should have non-black pixels inside the centered 500x375 title area
    size_t non_black_title = 0;
    for (size_t i = 0; i < renderer.canvas().pixels.size(); i += 4) {
        if (renderer.canvas().pixels[i] != 0 || renderer.canvas().pixels[i + 1] != 0 || renderer.canvas().pixels[i + 2] != 0) {
            ++non_black_title;
        }
    }
    assert(non_black_title > 50000);

    // Start new game and render active board + hover + cursor + pair selection
    gs.start_new_game(1000);
    gs.on_mouse_move(21.0f + 2 * 40 + 10, 6.0f + 2 * 50 + 10);
    renderer.render(gs);

    // Activate keyboard/gamepad cursor and verify high-contrast 3-ring Cyan/White/Cyan frame renders
    assert(gs.move_cursor(1, 0));
    assert(gs.cursor_active());
    renderer.render(gs);
    {
        int base_x = gs.cursor_c() * 40 + 21;
        int base_y = gs.cursor_r() * 50 + 6;
        size_t idx1 = (static_cast<size_t>(base_y + 1) * 768 + (base_x + 1)) * 4;
        size_t idx2 = (static_cast<size_t>(base_y + 2) * 768 + (base_x + 2)) * 4;
        size_t idx3 = (static_cast<size_t>(base_y + 3) * 768 + (base_x + 3)) * 4;
        assert(renderer.canvas().pixels[idx1 + 0] == 0 && renderer.canvas().pixels[idx1 + 1] == 220 && renderer.canvas().pixels[idx1 + 2] == 255);
        assert(renderer.canvas().pixels[idx2 + 0] == 255 && renderer.canvas().pixels[idx2 + 1] == 255 && renderer.canvas().pixels[idx2 + 2] == 255);
        assert(renderer.canvas().pixels[idx3 + 0] == 0 && renderer.canvas().pixels[idx3 + 1] == 170 && renderer.canvas().pixels[idx3 + 2] == 255);
    }

    // Use Board::hint_x1..y2 (cached by start_new_game -> has_valid_move) to click a guaranteed linkable pair
    int c1 = static_cast<int>(std::lround(gs.board().hint_x1()));
    int r1 = static_cast<int>(std::lround(gs.board().hint_y1()));
    int c2 = static_cast<int>(std::lround(gs.board().hint_x2()));
    int r2 = static_cast<int>(std::lround(gs.board().hint_y2()));

    gs.on_mouse_down(1, 21.0f + c1 * 40 + 10, 6.0f + r1 * 50 + 10, 1010);
    gs.on_mouse_down(1, 21.0f + c2 * 40 + 10, 6.0f + r2 * 50 + 10, 1020);
    // Immediate 0ms evaluation inside on_mouse_down (no need to wait for 30ms tick!)
    assert(gs.animating_link());
    renderer.render(gs);

    // Check center of (c1, r1) has the inner yellow core (255, 255, 0) of the 3-pass lightning beam
    int cx = c1 * 40 + 21 + 20;
    int cy = r1 * 50 + 6 + 25;
    size_t c_idx = (static_cast<size_t>(cy) * 768 + cx) * 4;
    assert(renderer.canvas().pixels[c_idx + 0] == 255);
    assert(renderer.canvas().pixels[c_idx + 1] == 255);
    assert(renderer.canvas().pixels[c_idx + 2] == 0);

    // Test nearest & bilinear scaling
    RgbaSurface scaled_2x;
    renderer.scale_to(1536, 1100, ScaleFilter::Nearest, scaled_2x);
    assert(scaled_2x.width == 1536 && scaled_2x.height == 1100);

    RgbaSurface scaled_bilinear;
    renderer.scale_to(1152, 825, ScaleFilter::Bilinear, scaled_bilinear);
    assert(scaled_bilinear.width == 1152 && scaled_bilinear.height == 825);

    // 2. Verify AudioEngine (WAV SFX + Embedded SoundFont MIDI + Native OS MIDI + Track Rotation)
#if defined(_WIN32)
    _putenv_s("SDL_AUDIODRIVER", "dummy");
#else
    setenv("SDL_AUDIODRIVER", "dummy", 1);
#endif

    Config cfg;
    cfg.midi_backend = MidiBackend::EmbeddedSoundFont;
    AudioEngine audio;
    assert(audio.init(assets, cfg, &err));
    assert(audio.is_initialized());
    std::printf("[info] Active SoundFont: %s | OST Available: %s\n",
                audio.active_soundfont_name().c_str(),
                audio.has_ost_available() ? "YES" : "NO");
    assert(audio.current_midi_track() == 0); // 101.mid started automatically!

    // Mix 4096 frames of embedded MIDI + SFX 203.wav
    audio.play_sfx(203);
    std::vector<float> pcm(4096 * 2, 0.0f);
    audio.mix_audio_frames(pcm.data(), 4096);
    float max_amp = 0.0f;
    for (float s : pcm) {
        max_amp = std::max(max_amp, std::fabs(s));
    }
    assert(max_amp > 0.001f);

    // Untick track 0 (101.mid) -> update() must immediately advance to track 1 (102.mid)
    cfg.set_track_enabled(101, false);
    audio.update(cfg);
    assert(audio.current_midi_track() == 1);

    // Switch live to NativeOS MIDI backend (and back to EmbeddedSoundFont)
    cfg.midi_backend = MidiBackend::NativeOS;
    audio.update(cfg);
    assert(audio.current_midi_track() == 1);

    // If lossless OST tracks are available, test streaming FLAC OST audio!
    if (audio.has_ost_available()) {
        cfg.midi_backend = MidiBackend::OstAudio;
        audio.update(cfg);
        assert(audio.current_midi_track() == 1);
        std::vector<float> ost_pcm(4096 * 2, 0.0f);
        audio.mix_audio_frames(ost_pcm.data(), 4096);
        float ost_amp = 0.0f;
        for (float s : ost_pcm) {
            ost_amp = std::max(ost_amp, std::fabs(s));
        }
        assert(ost_amp > 0.0001f);
    }

    cfg.midi_backend = MidiBackend::EmbeddedSoundFont;
    audio.update(cfg);
    assert(audio.current_midi_track() == 1);

    // Disable BGM -> stops playback immediately
    cfg.music_on = false;
    audio.update(cfg);
    assert(audio.current_midi_track() == -1);
    assert(!audio.is_midi_playing());

    audio.shutdown();

    // 3. Verify 4-language i18n JSON catalogs (zh-TW, zh-CN, en, vi) and Rules overlay config
    I18n i18n;
    std::filesystem::path locales_dir = std::filesystem::path(LLK2_SOURCE_DIR) / "locales";
    assert(i18n.load_locales_dir(locales_dir));
    auto langs = llk2::ui::common::ordered_languages(i18n);
    assert(langs.size() >= 4);
    assert(langs[0] == "zh-TW" && langs[1] == "zh-CN" && langs[2] == "en" && langs[3] == "vi");

    const char* required_keys[] = {
        "app.title", "menu.game", "menu.game.endless", "menu.audio", "menu.settings", "menu.about",
        "menu.settings.language", "menu.settings.fullscreen", "menu.settings.rumble", "menu.settings.inspector", "menu.settings.theme",
        "dialog.explain.title", "dialog.btn.close", "dialog.endless.title", "dialog.endless.note",
        "dialog.endless.all_levels", "dialog.endless.shuffle", "dialog.endless.specific_levels",
        "dialog.endless.no_time_limit", "dialog.endless.unlimited_shuffle", "dialog.endless.stage_fmt",
        "inspector.title", "inspector.mode.endless", "inspector.time_unlimited", "inspector.lives_unlimited",
        "inspector.controller", "inspector.controller_none",
        "inspector.hint_hidden", "inspector.highlight_hint",
        "about.credits.original", "about.credits.sc_trans", "about.credits.remaster",
        "msg.stage_clear.body", "msg.endless_clear.body", "msg.endless_clear_notime.body",
        "msg.timeout_unlim_shuffle.body", "msg.all_clear.body",
        "launcher.title", "launcher.tab.play", "launcher.tab.assets", "launcher.tab.settings",
        "launcher.tab.scores", "launcher.tab.diagnostics", "launcher.play.btn_launch",
        "launcher.assets.btn_extract", "launcher.assets.btn_check_pack",
        "launcher.settings.rumble",
        "launcher.diag.mode_user", "launcher.diag.mode_portable"
    };
    for (const char* lang : {"zh-TW", "zh-CN", "en", "vi"}) {
        assert(i18n.set_language(lang));
        for (const char* key : required_keys) {
            std::string val = i18n.tr(key);
            assert(!val.empty() && val != key);
        }
        assert(assets.form_explain_for_lang(lang).valid());
        auto ov = llk2::ui::common::build_explain_overlay(assets, i18n);
        if (std::string(lang) == "en" || std::string(lang) == "vi") {
            assert(ov.enabled);
            assert(!ov.turn_0.empty() && !ov.turn_1.empty() && !ov.turn_2.empty());
            assert(!ov.body.empty() && !ov.controls.empty());
        } else {
            assert(!ov.enabled);
        }

        auto about = llk2::ui::common::build_about_content(i18n);
        assert(about.paragraphs.size() == 3);
        int link_count = 0;
        for (const auto& p : about.paragraphs) {
            for (const auto& sp : p.spans) {
                if (!sp.url.empty()) ++link_count;
            }
        }
        assert(link_count == 4);
        auto laid = llk2::ui::common::layout_about_paragraphs(
            about, static_cast<float>(llk2::ui::common::AboutLayout::kTextWidth),
            18.0f, 14.0f, [](std::string_view sv) { return static_cast<float>(sv.size()) * 6.5f; });
        assert(!laid.chunks.empty() && laid.total_height > 0.0f);
    }

    // Verify hint coordinates stay hidden until reveal_hint is enabled, and Controller field formats cleanly
    assert(i18n.set_language("en"));
    auto snap_hidden = llk2::ui::common::build_inspector_snapshot(gs, audio, cfg, i18n, false);
    auto snap_shown = llk2::ui::common::build_inspector_snapshot(gs, audio, cfg, i18n, true, "Xbox Series X Controller");
    assert(snap_hidden.has_valid_hint);
    assert(snap_hidden.next_hint == i18n.tr("inspector.hint_hidden"));
    assert(snap_hidden.controller == i18n.tr("inspector.controller_none"));
    assert(snap_shown.next_hint.find("<->") != std::string::npos);
    assert(snap_shown.controller == "Xbox Series X Controller");

    // 4. Verify GamepadInput lifecycle is safe in headless environments
    {
        GamepadInput pad;
        pad.init();
        auto acts = pad.poll(1000, false);
        assert(!acts.any());
        pad.rumble(0.2f, 0.4f, 50, true);
        pad.rumble_for_sfx(203, true);
        pad.rumble_for_sfx(205, false);
        pad.shutdown();
    }

    // Verify Inspector snapshot in Endless Mode (Unlimited time + Unlimited lives + Endless #1 stage)
    llk2::core::EndlessConfig ecfg;
    ecfg.type = llk2::core::EndlessModeType::SpecificLevels;
    ecfg.level_mask = static_cast<uint8_t>(1u << 4);
    ecfg.no_time_limit = true;
    ecfg.unlimited_shuffle = true;
    gs.start_endless_game(ecfg, 2000);
    auto snap_endless = llk2::ui::common::build_inspector_snapshot(gs, audio, cfg, i18n, false);
    assert(snap_endless.mode == i18n.tr("inspector.mode.endless"));
    assert(snap_endless.stage.rfind("#1 (", 0) == 0);
    assert(snap_endless.time_left == i18n.tr("inspector.time_unlimited"));
    assert(snap_endless.lives_score.find(i18n.tr("inspector.lives_unlimited")) != std::string::npos);
}

int main() {
    int vpx = 0, vpy = 0, vpw = 0, vph = 0;
    BoardRenderer::compute_aspect_viewport(1920, 1080, vpx, vpy, vpw, vph);
    assert(vph == 1080);
    assert(vpw == 1080 * 768 / 550);
    assert(vpx == (1920 - vpw) / 2 && vpy == 0);

    bool created_temp = false;
    auto assets_dir = ensure_test_assets(created_temp);
    assert(!assets_dir.empty() && "Failed to locate or extract test assets");

    test_renderer_and_audio_with_assets(assets_dir);

    if (created_temp) {
        std::filesystem::remove_all(assets_dir);
    }
    std::printf("[PASS] test_media\n");
    return 0;
}
