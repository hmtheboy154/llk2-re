#include "core/config.hpp"
#include "core/hst_score.hpp"
#include "core/i18n.hpp"
#include "media/asset_manager.hpp"
#include "media/audio_engine.hpp"
#include "ui/bui/bui_app.hpp"
#include "ui/common/ui_helpers.hpp"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>

int main(int argc, char* argv[]) {
    std::filesystem::path work_dir = std::filesystem::current_path();
    std::filesystem::path custom_assets_dir;
    std::filesystem::path screenshot_path;
    std::filesystem::path inspector_screenshot_path;
    bool smoke_test = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--assets" && i + 1 < argc) {
            custom_assets_dir = argv[++i];
        } else if (arg == "--smoke-test") {
            smoke_test = true;
        } else if (arg == "--screenshot" && i + 1 < argc) {
            screenshot_path = argv[++i];
        } else if (arg == "--inspector-screenshot" && i + 1 < argc) {
            inspector_screenshot_path = argv[++i];
        }
    }

    llk2::core::Config config;
    config.load(work_dir / "config.json");

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
        std::fprintf(stderr, "[llk2-bui] %s\n%s\n%s\n",
                     i18n.tr("msg.missing_assets.title").c_str(),
                     i18n.tr("msg.missing_assets.body").c_str(),
                     err.c_str());
        return 1;
    }

    llk2::media::AudioEngine audio;
    audio.init(assets, config, &err);

    llk2::ui::bui::BuiApp app(assets, audio, config, i18n, scores, work_dir);
    if (!app.init(smoke_test, &err)) {
        std::fprintf(stderr, "[llk2-bui] Failed to initialize UI: %s\n", err.c_str());
        audio.shutdown();
        return 1;
    }

    if (smoke_test) {
        // 1. Render title screen into offscreen BuiRaster (works with zero display server!)
        if (!app.render_headless_frame()) {
            std::fprintf(stderr, "[FAIL] llk2-bui smoke_test: title frame render failed\n");
            app.shutdown();
            audio.shutdown();
            return 2;
        }

        // 2. Start a new game and verify 144-tile board initialization
        app.trigger_new_game();
        auto& gs = app.game_state();
        if (gs.in_title_screen() || !gs.board_active() || gs.board().tiles_left() != 144) {
            std::fprintf(stderr, "[FAIL] llk2-bui smoke_test: new game failed to initialize board\n");
            app.shutdown();
            audio.shutdown();
            return 3;
        }
        app.render_headless_frame();

        // 3. Test live language switching (zh-TW -> zh-CN -> zh-TW)
        app.set_language("zh-CN");
        app.render_headless_frame();
        if (app.current_title() != i18n.tr("app.title") || i18n.language() != "zh-CN") {
            std::fprintf(stderr, "[FAIL] llk2-bui smoke_test: zh-CN live switch failed\n");
            app.shutdown();
            audio.shutdown();
            return 4;
        }
        app.set_language("zh-TW");
        app.render_headless_frame();

        // 4. Test Blender Dark/Light theme switching + CJK font preservation
        app.set_blender_theme("light");
        if (app.current_theme() != "light") {
            std::fprintf(stderr, "[FAIL] llk2-bui smoke_test: Blender light theme switch failed\n");
            app.shutdown();
            audio.shutdown();
            return 5;
        }
        app.render_headless_frame();
        app.set_blender_theme("dark");
        app.render_headless_frame();

        // 5. Test scaling & filter presets
        app.set_scale_mode(llk2::core::WindowScaleMode::Scale1_5x);
        app.set_scale_filter(llk2::core::ScaleFilter::Bilinear);
        app.render_headless_frame();
        app.set_scale_mode(llk2::core::WindowScaleMode::Scale1x);
        app.set_scale_filter(llk2::core::ScaleFilter::Nearest);
        app.render_headless_frame();

        // 6. Test MIDI track toggle & backend hot-swap
        app.set_midi_track_enabled(101, false);
        app.set_midi_backend(llk2::core::MidiBackend::NativeOS);
        app.set_midi_backend(llk2::core::MidiBackend::EmbeddedSoundFont);
        app.set_midi_track_enabled(101, true);

        // 7. Test F12 Inspector + Board Hint Pair highlight
        app.set_inspector_visible(true);
        app.set_hint_highlight(true);
        app.render_headless_frame();
        if (!app.render_inspector_headless_frame(inspector_screenshot_path)) {
            std::fprintf(stderr, "[FAIL] llk2-bui smoke_test: inspector headless render failed\n");
            app.shutdown();
            audio.shutdown();
            return 6;
        }

        // 8. Test matching a pair via hint coordinates
        int c1 = static_cast<int>(std::lround(gs.board().hint_x1()));
        int r1 = static_cast<int>(std::lround(gs.board().hint_y1()));
        int c2 = static_cast<int>(std::lround(gs.board().hint_x2()));
        int r2 = static_cast<int>(std::lround(gs.board().hint_y2()));
        uint64_t t = llk2::ui::common::now_steady_ms();
        gs.on_mouse_down(1, 21.0f + c1 * 40 + 10, 6.0f + r1 * 50 + 10, t);
        gs.on_mouse_down(1, 21.0f + c2 * 40 + 10, 6.0f + r2 * 50 + 10, t + 10);
        app.render_headless_frame();

        // 9. Regression test: open Explain & Author modals and simulate clicking directly on
        // the image/modal (must NOT segfault and must close cleanly via deferred frame action!)
        app.open_explain_modal();
        app.render_headless_frame(screenshot_path);
        app.simulate_click(384, 300); // center of Explain image
        app.render_headless_frame();
        if (app.is_modal_open()) {
            std::fprintf(stderr, "[FAIL] llk2-bui smoke_test: Explain modal did not close on click\n");
            app.shutdown();
            audio.shutdown();
            return 7;
        }

        app.open_author_modal();
        app.render_headless_frame();
        app.simulate_click(384, 300); // center of Author image
        app.render_headless_frame();
        if (app.is_modal_open()) {
            std::fprintf(stderr, "[FAIL] llk2-bui smoke_test: Author modal did not close on click\n");
            app.shutdown();
            audio.shutdown();
            return 8;
        }

        app.open_records_modal(-1);
        app.render_headless_frame();
        app.close_active_modal();
        app.set_hint_highlight(false);
        app.set_inspector_visible(false);

        app.trigger_give_up();
        app.render_headless_frame();

        std::printf("[PASS] llk2-bui smoke test\n");
        app.shutdown();
        audio.shutdown();
        return 0;
    }

    int rc = app.run();
    app.shutdown();
    audio.shutdown();
    return rc;
}
