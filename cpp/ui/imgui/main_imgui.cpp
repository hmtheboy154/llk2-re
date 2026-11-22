#include "core/config.hpp"
#include "core/hst_score.hpp"
#include "core/i18n.hpp"
#include "media/asset_manager.hpp"
#include "media/audio_engine.hpp"
#include "ui/common/ui_helpers.hpp"
#include "ui/imgui/imgui_app.hpp"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>

int main(int argc, char* argv[]) {
    std::filesystem::path work_dir = std::filesystem::current_path();
    std::filesystem::path custom_assets_dir;
    std::filesystem::path screenshot_path;
    std::filesystem::path explain_screenshot_path;
    std::string forced_lang;
    bool smoke_test = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--assets" && i + 1 < argc) {
            custom_assets_dir = argv[++i];
        } else if (arg == "--smoke-test") {
            smoke_test = true;
        } else if (arg == "--screenshot" && i + 1 < argc) {
            screenshot_path = argv[++i];
        } else if (arg == "--explain-screenshot" && i + 1 < argc) {
            explain_screenshot_path = argv[++i];
        } else if (arg == "--lang" && i + 1 < argc) {
            forced_lang = argv[++i];
        }
    }

    llk2::core::Config config;
    config.load(work_dir / "config.json");

    if (!forced_lang.empty()) {
        config.language = forced_lang;
    }
    llk2::core::I18n i18n;
    if (std::filesystem::exists(work_dir / "locales")) {
        i18n.load_locales_dir(work_dir / "locales");
    }
    i18n.set_language(config.language);

    llk2::core::HighScoreTable scores;
    if (!scores.load_json(work_dir / "scores.json")) {
        if (!scores.load_legacy_hst(work_dir / "d4s.hst")) {
            scores.reset_empty();
        }
    }

    llk2::media::AssetManager assets;
    std::string err;
    bool assets_ok = false;
    if (!custom_assets_dir.empty()) {
        assets_ok = assets.load_from_dir(custom_assets_dir, &err);
    } else {
        assets_ok = assets.auto_load(work_dir, &err);
    }

    if (!assets_ok) {
        std::fprintf(stderr, "[llk2-imgui] %s\n%s\n%s\n",
                     i18n.tr("msg.missing_assets.title").c_str(),
                     i18n.tr("msg.missing_assets.body").c_str(),
                     err.c_str());
        return 1;
    }

    llk2::media::AudioEngine audio;
    audio.init(assets, config, &err);

    llk2::ui::imgui::ImGuiApp app(assets, audio, config, i18n, scores, work_dir);
    if (!app.init(smoke_test, &err)) {
        std::fprintf(stderr, "[llk2-imgui] Failed to initialize UI: %s\n", err.c_str());
        audio.shutdown();
        return 1;
    }

    if (smoke_test) {
        // 1. Render initial title frame
        app.render_frame();

        // 2. Start a new game and verify 144-tile board initialization
        app.trigger_new_game();
        auto& gs = app.game_state();
        if (gs.in_title_screen() || !gs.board_active() || gs.board().tiles_left() != 144) {
            std::fprintf(stderr, "[FAIL] llk2-imgui smoke_test: new game failed to initialize board\n");
            app.shutdown();
            audio.shutdown();
            return 2;
        }
        app.render_frame();

        // 3. Test live language switching (zh-TW -> zh-CN -> en -> vi -> zh-TW)
        for (const char* lang : {"zh-CN", "en", "vi", "zh-TW"}) {
            app.set_language(lang);
            app.render_frame();
            if (app.window_title() != i18n.tr("app.title") || i18n.language() != lang) {
                std::fprintf(stderr, "[FAIL] llk2-imgui smoke_test: %s live switch failed\n", lang);
                app.shutdown();
                audio.shutdown();
                return 3;
            }
        }
        if (!forced_lang.empty()) {
            app.set_language(forced_lang);
            app.render_frame();
        }

        // 4. Test live scaling & filter presets
        app.set_scale_mode(llk2::core::WindowScaleMode::Scale1_5x);
        app.set_scale_filter(llk2::core::ScaleFilter::Bilinear);
        app.render_frame();
        app.set_scale_mode(llk2::core::WindowScaleMode::Scale1x);
        app.set_scale_filter(llk2::core::ScaleFilter::Nearest);
        app.render_frame();

        // 5. Test MIDI track toggle & backend hot-swap
        app.set_midi_track_enabled(101, false);
        app.set_midi_backend(llk2::core::MidiBackend::NativeOS);
        app.set_midi_backend(llk2::core::MidiBackend::EmbeddedSoundFont);
        app.set_midi_track_enabled(101, true);

        // 6. Test matching a pair via hint coordinates
        int c1 = static_cast<int>(std::lround(gs.board().hint_x1()));
        int r1 = static_cast<int>(std::lround(gs.board().hint_y1()));
        int c2 = static_cast<int>(std::lround(gs.board().hint_x2()));
        int r2 = static_cast<int>(std::lround(gs.board().hint_y2()));
        uint64_t t = llk2::ui::common::now_steady_ms();
        gs.on_mouse_down(1, 21.0f + c1 * 40 + 10, 6.0f + r1 * 50 + 10, t);
        gs.on_mouse_down(1, 21.0f + c2 * 40 + 10, 6.0f + r2 * 50 + 10, t + 10);
        app.render_frame();

        if (!screenshot_path.empty()) {
            app.set_inspector_visible(true);
            app.render_frame(); // 1st frame computes AutoResize window bounds
            app.render_frame(); // 2nd frame renders visible Inspector + board
            app.save_screenshot(screenshot_path);
        }

        // 7. Test Records / Explain (with localized rules overlay) / Author modals
        app.open_records_modal(-1);
        app.render_frame();
        app.open_explain_modal();
        app.render_frame();
        app.render_frame();
        if (!explain_screenshot_path.empty()) {
            app.save_screenshot(explain_screenshot_path);
        }
        app.open_author_modal();
        app.render_frame();

        app.trigger_give_up();
        app.render_frame();

        std::printf("[PASS] llk2-imgui smoke test\n");
        app.shutdown();
        audio.shutdown();
        return 0;
    }

    int rc = app.run();
    app.shutdown();
    audio.shutdown();
    return rc;
}
