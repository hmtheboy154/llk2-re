#include "core/config.hpp"
#include "core/hst_score.hpp"
#include "core/i18n.hpp"
#include "media/asset_manager.hpp"
#include "media/audio_engine.hpp"
#include "ui/qt/image_dialogs.hpp"
#include "ui/qt/main_window.hpp"
#include "ui/qt/table_dialog.hpp"

#include <QApplication>
#include <QMessageBox>
#include <QTimer>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    std::filesystem::path work_dir = std::filesystem::current_path();
    std::filesystem::path custom_assets_dir;
    bool smoke_test = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--assets" && i + 1 < argc) {
            custom_assets_dir = argv[++i];
        } else if (arg == "--smoke-test") {
            smoke_test = true;
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
        if (!smoke_test) {
            QMessageBox::critical(
                nullptr,
                QString::fromStdString(i18n.tr("msg.missing_assets.title")),
                QString::fromStdString(i18n.tr("msg.missing_assets.body")) + "\n\n" + QString::fromStdString(err));
        } else {
            std::fprintf(stderr, "[llk2-qt] Failed to load assets: %s\n", err.c_str());
        }
        return 1;
    }

    llk2::media::AudioEngine audio;
    audio.init(assets, config, &err);

    llk2::ui::qt::MainWindow win(assets, audio, config, i18n, scores, work_dir);
    win.show();

    if (smoke_test) {
        // Automated end-to-end Qt 6 UI verification sequence
        QTimer::singleShot(30, [&]() {
            // 1. Start a new game and verify board state
            win.trigger_new_game();
            auto& gs = win.game_state();
            if (gs.in_title_screen() || !gs.board_active() || gs.board().tiles_left() != 144) {
                std::fprintf(stderr, "[FAIL] smoke_test: new game failed to initialize board\n");
                app.exit(2);
                return;
            }

            // 2. Test live language switching (zh-TW -> zh-CN -> zh-TW)
            win.set_language("zh-CN");
            if (win.windowTitle().toStdString() != i18n.tr("app.title") || i18n.language() != "zh-CN") {
                std::fprintf(stderr, "[FAIL] smoke_test: zh-CN live switch failed\n");
                app.exit(3);
                return;
            }
            win.set_language("zh-TW");

            // 3. Test live scaling & filter changes
            win.set_scale_mode(llk2::core::WindowScaleMode::Scale1_5x);
            win.set_scale_filter(llk2::core::ScaleFilter::Bilinear);
            win.set_scale_mode(llk2::core::WindowScaleMode::Scale1x);
            win.set_scale_filter(llk2::core::ScaleFilter::Nearest);

            // 4. Test MIDI track toggle & backend hot-swap
            win.set_midi_track_enabled(101, false);
            win.set_midi_backend(llk2::core::MidiBackend::NativeOS);
            win.set_midi_backend(llk2::core::MidiBackend::EmbeddedSoundFont);
            win.set_midi_track_enabled(101, true);

            // 5. Instantiate and render TableDialog, ExplainDialog, AuthorDialog
            llk2::ui::qt::TableDialog table_dlg(scores, i18n, 0, "Tester", &win);
            table_dlg.show();
            table_dlg.close();

            llk2::ui::qt::ImageSplashDialog explain_dlg(assets.form_explain(), "Explain", &win);
            explain_dlg.show();
            explain_dlg.close();

            llk2::ui::qt::ImageSplashDialog author_dlg(assets.form_author(), "Author", &win);
            author_dlg.show();
            author_dlg.close();

            // 6. Test matching a pair on the board
            int c1 = static_cast<int>(std::lround(gs.board().hint_x1()));
            int r1 = static_cast<int>(std::lround(gs.board().hint_y1()));
            int c2 = static_cast<int>(std::lround(gs.board().hint_x2()));
            int r2 = static_cast<int>(std::lround(gs.board().hint_y2()));
            gs.on_mouse_down(1, 21.0f + c1 * 40 + 10, 6.0f + r1 * 50 + 10, 2000);
            gs.on_mouse_down(1, 21.0f + c2 * 40 + 10, 6.0f + r2 * 50 + 10, 2010);

            QTimer::singleShot(60, [&]() {
                win.trigger_give_up();
                std::printf("[PASS] llk2-qt smoke test\n");
                app.exit(0);
            });
        });
    }

    return app.exec();
}
