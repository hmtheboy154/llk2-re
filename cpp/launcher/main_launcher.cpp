#include "core/app_paths.hpp"
#include "core/config.hpp"
#include "core/hst_score.hpp"
#include "core/i18n.hpp"
#include "launcher/launcher_window.hpp"

#include <QApplication>
#include <QPixmap>
#include <QTimer>

#include <cstdio>
#include <filesystem>
#include <string>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    std::filesystem::path custom_data_dir;
    std::filesystem::path screenshot_path;
    int screenshot_tab = 0;
    std::string forced_lang;
    bool smoke_test = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--data-dir" && i + 1 < argc) {
            custom_data_dir = argv[++i];
        } else if (arg == "--smoke-test") {
            smoke_test = true;
        } else if (arg == "--lang" && i + 1 < argc) {
            forced_lang = argv[++i];
        } else if (arg == "--screenshot" && i + 1 < argc) {
            screenshot_path = argv[++i];
        } else if (arg == "--screenshot-tab" && i + 1 < argc) {
            try {
                screenshot_tab = std::stoi(argv[++i]);
            } catch (...) {
                screenshot_tab = 0;
            }
        }
    }

    auto paths = llk2::core::AppPaths::resolve(argc > 0 ? argv[0] : nullptr, custom_data_dir);

    llk2::core::Config config;
    config.load(paths.config_file());
    if (!forced_lang.empty()) {
        config.language = forced_lang;
    }

    llk2::core::I18n i18n;
    if (auto bundled_loc = paths.find_resource("locales"); !bundled_loc.empty()) {
        i18n.load_locales_dir(bundled_loc);
    }
    if (std::filesystem::exists(paths.locales_dir())) {
        i18n.load_locales_dir(paths.locales_dir());
    }
    i18n.set_language(config.language);

    llk2::core::HighScoreTable scores;
    if (!scores.load_json(paths.scores_file())) {
        if (!scores.load_legacy_hst(paths.legacy_hst_file())) {
            scores.reset_empty();
        }
    }

    llk2::launcher::LauncherWindow win(paths, config, i18n, scores);
    win.show();

    if (smoke_test) {
        QTimer::singleShot(30, [&]() {
            if (win.tab_count() != 5) {
                std::fprintf(stderr, "[FAIL] llk2-launcher smoke_test: expected 5 tabs, got %d\n", win.tab_count());
                app.exit(2);
                return;
            }

            // Exercise live language switching across all 4 built-in languages
            for (const char* lang : {"zh-CN", "en", "vi", "zh-TW"}) {
                win.set_language(lang);
                if (i18n.language() != lang) {
                    std::fprintf(stderr, "[FAIL] llk2-launcher smoke_test: language switch to %s failed\n", lang);
                    app.exit(3);
                    return;
                }
            }
            if (!forced_lang.empty()) {
                win.set_language(forced_lang);
            }

            // Cycle through all 5 tabs and verify rendering
            for (int t = 0; t < win.tab_count(); ++t) {
                win.select_tab(t);
                win.repaint();
            }

            if (!screenshot_path.empty()) {
                win.select_tab(screenshot_tab);
                win.repaint();
                win.grab().save(QString::fromStdString(screenshot_path.string()));
            }

            std::printf("[PASS] llk2-launcher smoke test\n");
            app.exit(0);
        });
    }

    return app.exec();
}
