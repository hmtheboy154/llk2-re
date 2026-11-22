#include "ui/qt/main_window.hpp"
#include "ui/common/ui_helpers.hpp"
#include "ui/qt/image_dialogs.hpp"
#include "ui/qt/inspector_dialog.hpp"
#include "ui/qt/table_dialog.hpp"

#include <QApplication>
#include <QFileDialog>
#include <QIcon>
#include <QKeySequence>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPen>
#include <algorithm>
#include <chrono>
#include <cmath>

namespace llk2::ui::qt {

// ── BoardWidget ──────────────────────────────────────────────────────────────

BoardWidget::BoardWidget(media::BoardRenderer& renderer, const core::Config& config, QWidget* parent)
    : QWidget(parent),
      renderer_(renderer),
      config_(config),
      rgb32_buf_(media::BoardRenderer::WIDTH, media::BoardRenderer::HEIGHT, QImage::Format_RGB32) {
    rgb32_buf_.fill(Qt::black);
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setMinimumSize(core::GameState::kCanvasWidth / 2, core::GameState::kCanvasHeight / 2);
}

void BoardWidget::set_callbacks(MouseMoveCallback on_move, MouseDownCallback on_down) {
    on_move_ = std::move(on_move);
    on_down_ = std::move(on_down);
}

void BoardWidget::set_hint_overlay(bool enabled, int c1, int r1, int c2, int r2) {
    if (hint_enabled_ == enabled &&
        hint_c1_ == c1 && hint_r1_ == r1 &&
        hint_c2_ == c2 && hint_r2_ == r2) {
        return;
    }
    hint_enabled_ = enabled;
    hint_c1_ = c1;
    hint_r1_ = r1;
    hint_c2_ = c2;
    hint_r2_ = r2;
    update();
}

void BoardWidget::copy_rgba_rect_to_rgb32(int rx, int ry, int rw, int rh) {
    const auto& surf = renderer_.canvas();
    if (!surf.valid()) return;

    int x0 = std::clamp(rx, 0, surf.width);
    int y0 = std::clamp(ry, 0, surf.height);
    int x1 = std::clamp(rx + rw, 0, surf.width);
    int y1 = std::clamp(ry + rh, 0, surf.height);
    if (x0 >= x1 || y0 >= y1) return;

    for (int y = y0; y < y1; ++y) {
        const uint8_t* src = surf.pixels.data() + (static_cast<size_t>(y) * surf.width + x0) * 4;
        uint32_t* dst = reinterpret_cast<uint32_t*>(rgb32_buf_.scanLine(y)) + x0;
        int span = x1 - x0;
        for (int x = 0; x < span; ++x) {
            uint32_t r = src[x * 4 + 0];
            uint32_t g = src[x * 4 + 1];
            uint32_t b = src[x * 4 + 2];
            dst[x] = 0xFF000000u | (r << 16) | (g << 8) | b;
        }
    }
}

QRect BoardWidget::map_logical_rect_to_widget(int rx, int ry, int rw, int rh) const {
    int vp_x = 0, vp_y = 0, vp_w = 0, vp_h = 0;
    media::BoardRenderer::compute_aspect_viewport(width(), height(), vp_x, vp_y, vp_w, vp_h);
    if (vp_w <= 0 || vp_h <= 0) return rect();

    double sx = static_cast<double>(vp_w) / static_cast<double>(media::BoardRenderer::WIDTH);
    double sy = static_cast<double>(vp_h) / static_cast<double>(media::BoardRenderer::HEIGHT);

    int wx0 = vp_x + static_cast<int>(std::floor(rx * sx)) - 2;
    int wy0 = vp_y + static_cast<int>(std::floor(ry * sy)) - 2;
    int wx1 = vp_x + static_cast<int>(std::ceil((rx + rw) * sx)) + 2;
    int wy1 = vp_y + static_cast<int>(std::ceil((ry + rh) * sy)) + 2;

    wx0 = std::clamp(wx0, 0, width());
    wy0 = std::clamp(wy0, 0, height());
    wx1 = std::clamp(wx1, 0, width());
    wy1 = std::clamp(wy1, 0, height());
    return QRect(wx0, wy0, std::max(0, wx1 - wx0), std::max(0, wy1 - wy0));
}

void BoardWidget::sync_full(bool immediate_repaint) {
    copy_rgba_rect_to_rgb32(0, 0, media::BoardRenderer::WIDTH, media::BoardRenderer::HEIGHT);
    if (immediate_repaint) {
        repaint();
    } else {
        update();
    }
}

void BoardWidget::sync_tile_hover_change(int prev_c, int prev_r, int new_c, int new_r) {
    QRegion dirty;
    auto mark_tile = [&](int c, int r) {
        if (c < 0 || c >= core::Board::kCols || r < 0 || r >= core::Board::kRows) return;
        int px = core::GameState::kOffX + c * media::BoardRenderer::TILE_W;
        int py = core::GameState::kOffY + r * media::BoardRenderer::TILE_H;
        copy_rgba_rect_to_rgb32(px, py, 46, 56);
        dirty += map_logical_rect_to_widget(px, py, 46, 56);
    };
    mark_tile(prev_c, prev_r);
    mark_tile(new_c, new_r);

    if (!dirty.isEmpty()) {
        update(dirty);
    }
}

bool BoardWidget::map_to_logical(const QPointF& pos, float& out_lx, float& out_ly) const {
    int vp_x = 0, vp_y = 0, vp_w = 0, vp_h = 0;
    media::BoardRenderer::compute_aspect_viewport(width(), height(), vp_x, vp_y, vp_w, vp_h);
    if (vp_w <= 0 || vp_h <= 0) return false;

    float rx = static_cast<float>(pos.x()) - static_cast<float>(vp_x);
    float ry = static_cast<float>(pos.y()) - static_cast<float>(vp_y);
    if (rx < 0.0f || ry < 0.0f || rx >= static_cast<float>(vp_w) || ry >= static_cast<float>(vp_h)) {
        return false;
    }

    out_lx = rx * static_cast<float>(core::GameState::kCanvasWidth) / static_cast<float>(vp_w);
    out_ly = ry * static_cast<float>(core::GameState::kCanvasHeight) / static_cast<float>(vp_h);
    return true;
}

void BoardWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);

    int vp_x = 0, vp_y = 0, vp_w = 0, vp_h = 0;
    media::BoardRenderer::compute_aspect_viewport(width(), height(), vp_x, vp_y, vp_w, vp_h);
    QRect vp_rect(vp_x, vp_y, vp_w, vp_h);

    // Only fill letterbox/pillarbox regions outside the viewport (zero overdraw!)
    if (vp_rect != rect()) {
        QRegion bars = QRegion(event->rect()) - QRegion(vp_rect);
        for (const QRect& bar : bars) {
            painter.fillRect(bar, Qt::black);
        }
    }

    if (vp_w <= 0 || vp_h <= 0) return;

    QRect dirty_vp = event->rect().intersected(vp_rect);
    if (dirty_vp.isEmpty()) return;

    painter.setRenderHint(QPainter::SmoothPixmapTransform, config_.scale_filter == core::ScaleFilter::Bilinear);

    if (vp_w == media::BoardRenderer::WIDTH && vp_h == media::BoardRenderer::HEIGHT) {
        // 1:1 native pixel blit — zero scaling cost!
        QRect src_rect = dirty_vp.translated(-vp_x, -vp_y);
        painter.drawImage(dirty_vp.topLeft(), rgb32_buf_, src_rect);
    } else {
        // Partial sub-rectangle scaled blit: only scales the dirty tiles instead of the entire window!
        double inv_sx = static_cast<double>(media::BoardRenderer::WIDTH) / static_cast<double>(vp_w);
        double inv_sy = static_cast<double>(media::BoardRenderer::HEIGHT) / static_cast<double>(vp_h);

        QRectF src_f(
            (dirty_vp.left() - vp_x) * inv_sx,
            (dirty_vp.top() - vp_y) * inv_sy,
            dirty_vp.width() * inv_sx,
            dirty_vp.height() * inv_sy);
        painter.drawImage(QRectF(dirty_vp), rgb32_buf_, src_f);
    }

    if (hint_enabled_ && hint_c1_ >= 1 && hint_r1_ >= 1 && hint_c2_ >= 1 && hint_r2_ >= 1) {
        double sx = static_cast<double>(vp_w) / static_cast<double>(media::BoardRenderer::WIDTH);
        double sy = static_cast<double>(vp_h) / static_cast<double>(media::BoardRenderer::HEIGHT);
        QPen pen(QColor(255, 220, 40, 240));
        pen.setWidthF(3.0);
        pen.setJoinStyle(Qt::MiterJoin);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        auto draw_box = [&](int c, int r) {
            double x0 = vp_x + (core::GameState::kOffX + c * media::BoardRenderer::TILE_W) * sx;
            double y0 = vp_y + (core::GameState::kOffY + r * media::BoardRenderer::TILE_H) * sy;
            double w = media::BoardRenderer::TILE_W * sx;
            double h = media::BoardRenderer::TILE_H * sy;
            painter.drawRect(QRectF(x0 + 1.5, y0 + 1.5, std::max(1.0, w - 3.0), std::max(1.0, h - 3.0)));
        };
        draw_box(hint_c1_, hint_r1_);
        draw_box(hint_c2_, hint_r2_);
    }
}

void BoardWidget::mouseMoveEvent(QMouseEvent* event) {
    if (!on_move_) return;
    float lx = -1.0f, ly = -1.0f;
    if (map_to_logical(event->position(), lx, ly)) {
        on_move_(lx, ly);
    } else {
        on_move_(-1.0f, -1.0f);
    }
}

void BoardWidget::mousePressEvent(QMouseEvent* event) {
    if (!on_down_) return;
    float lx = -1.0f, ly = -1.0f;
    if (!map_to_logical(event->position(), lx, ly)) return;

    int btn = 0;
    if (event->button() == Qt::LeftButton) {
        btn = 1;
    } else if (event->button() == Qt::RightButton) {
        btn = 2;
    }
    if (btn != 0) {
        on_down_(btn, lx, ly);
    }
}

void BoardWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    mousePressEvent(event);
}

void BoardWidget::leaveEvent(QEvent* /*event*/) {
    if (on_move_) {
        on_move_(-1.0f, -1.0f);
    }
}

// ── MainWindow ───────────────────────────────────────────────────────────────

uint64_t MainWindow::now_ticks_ms() {
    using clock = std::chrono::steady_clock;
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(clock::now().time_since_epoch()).count());
}

MainWindow::MainWindow(media::AssetManager& assets,
                       media::AudioEngine& audio,
                       core::Config& config,
                       core::I18n& i18n,
                       core::HighScoreTable& scores,
                       const std::filesystem::path& work_dir,
                       QWidget* parent)
    : QMainWindow(parent),
      assets_(assets),
      audio_(audio),
      config_(config),
      i18n_(i18n),
      scores_(scores),
      work_dir_(work_dir),
      renderer_(assets) {
    if (!assets_.icon_path().empty() && std::filesystem::exists(assets_.icon_path())) {
        setWindowIcon(QIcon(QString::fromStdString(assets_.icon_path().string())));
    }

    board_widget_ = new BoardWidget(renderer_, config_, this);
    setCentralWidget(board_widget_);

    board_widget_->set_callbacks(
        [this](float lx, float ly) {
            int prev_hc = game_state_.hover_c();
            int prev_hr = game_state_.hover_r();
            game_state_.on_mouse_move(lx, ly);
            int new_hc = game_state_.hover_c();
            int new_hr = game_state_.hover_r();
            if (new_hc != prev_hc || new_hr != prev_hr) {
                renderer_.render(game_state_);
                board_widget_->sync_tile_hover_change(prev_hc, prev_hr, new_hc, new_hr);
            }
        },
        [this](int button, float lx, float ly) {
            uint64_t now = now_ticks_ms();
            game_state_.on_mouse_down(button, lx, ly, now);
            process_game_events(now);
            renderer_.render(game_state_);
            board_widget_->sync_full(false);
            refresh_inspector_and_hint();
        });

    inspector_dlg_ = new InspectorDialog(i18n_, this);
    inspector_dlg_->set_callbacks(
        [this](bool enabled) {
            set_hint_highlight(enabled);
        },
        [this]() {
            if (act_inspector_) {
                act_inspector_->blockSignals(true);
                act_inspector_->setChecked(false);
                act_inspector_->blockSignals(false);
            }
        });

    build_menus();
    retranslate_ui();
    sync_menu_states();
    apply_window_scale();
    refresh_inspector_and_hint();

    i18n_.add_change_listener([this](const std::string& /*new_lang*/) {
        retranslate_ui();
    });

    renderer_.render(game_state_);
    board_widget_->sync_full(false);

    tick_timer_ = new QTimer(this);
    tick_timer_->setTimerType(Qt::PreciseTimer);
    tick_timer_->setInterval(16); // 60 Hz precise tick for crisp 200ms link flash & countdown
    connect(tick_timer_, &QTimer::timeout, this, &MainWindow::on_game_tick);
    tick_timer_->start();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    save_config_and_scores();
    audio_.shutdown();
    QMainWindow::closeEvent(event);
}

void MainWindow::build_menus() {
    auto* mb = menuBar();

    // Order: 遊戲(&G) - 背景聲音(&B) - 設定(&S) - 關於(&A)
    menu_game_ = mb->addMenu("");
    menu_audio_ = mb->addMenu("");
    menu_settings_ = mb->addMenu("");
    menu_about_ = mb->addMenu("");

    // 1. 遊戲(&G)
    act_new_game_ = menu_game_->addAction("");
    act_give_up_ = menu_game_->addAction("");
    menu_game_->addSeparator();
    act_records_ = menu_game_->addAction("");
    menu_game_->addSeparator();
    act_exit_ = menu_game_->addAction("");

    connect(act_new_game_, &QAction::triggered, this, &MainWindow::trigger_new_game);
    connect(act_give_up_, &QAction::triggered, this, &MainWindow::trigger_give_up);
    connect(act_records_, &QAction::triggered, this, &MainWindow::on_show_records);
    connect(act_exit_, &QAction::triggered, this, &QWidget::close);

    // 2. 背景聲音(&B)
    act_music_ = menu_audio_->addAction("");
    act_music_->setCheckable(true);
    act_sound_ = menu_audio_->addAction("");
    act_sound_->setCheckable(true);

    connect(act_music_, &QAction::toggled, this, [this](bool checked) {
        config_.music_on = checked;
        audio_.update(config_);
        save_config_and_scores();
    });
    connect(act_sound_, &QAction::toggled, this, [this](bool checked) {
        config_.sound_on = checked;
        audio_.update(config_);
        save_config_and_scores();
    });

    // 3. 設定(&S)
    // 3a. 語言(&L)
    submenu_lang_ = menu_settings_->addMenu("");
    group_lang_ = new QActionGroup(this);
    group_lang_->setExclusive(true);
    act_lang_tw_ = submenu_lang_->addAction("");
    act_lang_tw_->setCheckable(true);
    act_lang_cn_ = submenu_lang_->addAction("");
    act_lang_cn_->setCheckable(true);
    group_lang_->addAction(act_lang_tw_);
    group_lang_->addAction(act_lang_cn_);

    connect(act_lang_tw_, &QAction::triggered, this, [this]() { set_language("zh-TW"); });
    connect(act_lang_cn_, &QAction::triggered, this, [this]() { set_language("zh-CN"); });

    menu_settings_->addSeparator();

    // 3b. 背景音樂音軌 (101, 102, 103)
    submenu_midi_tracks_ = menu_settings_->addMenu("");
    for (int i = 0; i < 3; ++i) {
        int track_id = 101 + i;
        act_midi_tracks_[i] = submenu_midi_tracks_->addAction("");
        act_midi_tracks_[i]->setCheckable(true);
        connect(act_midi_tracks_[i], &QAction::toggled, this, [this, track_id](bool checked) {
            set_midi_track_enabled(track_id, checked);
        });
    }

    // 3c. 音樂播放裝置 (Lossless OST vs Native OS MIDI vs Embedded SoundFont + Custom SF2 picker)
    submenu_midi_device_ = menu_settings_->addMenu("");
    group_midi_device_ = new QActionGroup(this);
    group_midi_device_->setExclusive(true);
    act_midi_ost_ = submenu_midi_device_->addAction("");
    act_midi_ost_->setCheckable(true);
    act_midi_native_ = submenu_midi_device_->addAction("");
    act_midi_native_->setCheckable(true);
    act_midi_embedded_ = submenu_midi_device_->addAction("");
    act_midi_embedded_->setCheckable(true);
    group_midi_device_->addAction(act_midi_ost_);
    group_midi_device_->addAction(act_midi_native_);
    group_midi_device_->addAction(act_midi_embedded_);

    submenu_midi_device_->addSeparator();
    act_select_sf2_ = submenu_midi_device_->addAction("");

    connect(act_midi_ost_, &QAction::triggered, this, [this]() {
        set_midi_backend(core::MidiBackend::OstAudio);
    });
    connect(act_midi_native_, &QAction::triggered, this, [this]() {
        set_midi_backend(core::MidiBackend::NativeOS);
    });
    connect(act_midi_embedded_, &QAction::triggered, this, [this]() {
        set_midi_backend(core::MidiBackend::EmbeddedSoundFont);
    });
    connect(act_select_sf2_, &QAction::triggered, this, &MainWindow::on_select_soundfont);

    menu_settings_->addSeparator();

    // 3d. 畫面縮放 (1x, 1.5x, 2x, 3x, Free + Nearest / Bilinear)
    submenu_scale_ = menu_settings_->addMenu("");
    group_scale_mode_ = new QActionGroup(this);
    group_scale_mode_->setExclusive(true);
    act_scale_1x_ = submenu_scale_->addAction("");
    act_scale_1x_->setCheckable(true);
    act_scale_1_5x_ = submenu_scale_->addAction("");
    act_scale_1_5x_->setCheckable(true);
    act_scale_2x_ = submenu_scale_->addAction("");
    act_scale_2x_->setCheckable(true);
    act_scale_3x_ = submenu_scale_->addAction("");
    act_scale_3x_->setCheckable(true);
    act_scale_free_ = submenu_scale_->addAction("");
    act_scale_free_->setCheckable(true);

    group_scale_mode_->addAction(act_scale_1x_);
    group_scale_mode_->addAction(act_scale_1_5x_);
    group_scale_mode_->addAction(act_scale_2x_);
    group_scale_mode_->addAction(act_scale_3x_);
    group_scale_mode_->addAction(act_scale_free_);

    connect(act_scale_1x_, &QAction::triggered, this, [this]() { set_scale_mode(core::WindowScaleMode::Scale1x); });
    connect(act_scale_1_5x_, &QAction::triggered, this, [this]() { set_scale_mode(core::WindowScaleMode::Scale1_5x); });
    connect(act_scale_2x_, &QAction::triggered, this, [this]() { set_scale_mode(core::WindowScaleMode::Scale2x); });
    connect(act_scale_3x_, &QAction::triggered, this, [this]() { set_scale_mode(core::WindowScaleMode::Scale3x); });
    connect(act_scale_free_, &QAction::triggered, this, [this]() { set_scale_mode(core::WindowScaleMode::FreeAspect); });

    submenu_scale_->addSeparator();

    group_scale_filter_ = new QActionGroup(this);
    group_scale_filter_->setExclusive(true);
    act_filter_nearest_ = submenu_scale_->addAction("");
    act_filter_nearest_->setCheckable(true);
    act_filter_bilinear_ = submenu_scale_->addAction("");
    act_filter_bilinear_->setCheckable(true);
    group_scale_filter_->addAction(act_filter_nearest_);
    group_scale_filter_->addAction(act_filter_bilinear_);

    connect(act_filter_nearest_, &QAction::triggered, this, [this]() { set_scale_filter(core::ScaleFilter::Nearest); });
    connect(act_filter_bilinear_, &QAction::triggered, this, [this]() { set_scale_filter(core::ScaleFilter::Bilinear); });

    menu_settings_->addSeparator();

    // 3e. Legacy d4s.hst Import / Export
    act_import_hst_ = menu_settings_->addAction("");
    act_export_hst_ = menu_settings_->addAction("");
    connect(act_import_hst_, &QAction::triggered, this, &MainWindow::on_import_hst);
    connect(act_export_hst_, &QAction::triggered, this, &MainWindow::on_export_hst);

    menu_settings_->addSeparator();

    // 3f. Inspector / Hint Assistant (F12)
    act_inspector_ = menu_settings_->addAction("");
    act_inspector_->setCheckable(true);
    act_inspector_->setShortcut(QKeySequence(Qt::Key_F12));
    connect(act_inspector_, &QAction::toggled, this, [this](bool checked) {
        set_inspector_visible(checked);
    });

    // 4. 關於(&A)
    act_explain_ = menu_about_->addAction("");
    act_author_ = menu_about_->addAction("");
    connect(act_explain_, &QAction::triggered, this, &MainWindow::on_show_explain);
    connect(act_author_, &QAction::triggered, this, &MainWindow::on_show_author);
}

void MainWindow::retranslate_ui() {
    auto qtr = [this](std::string_view key) {
        return QString::fromStdString(i18n_.tr(key));
    };

    setWindowTitle(qtr("app.title"));

    menu_game_->setTitle(qtr("menu.game"));
    act_new_game_->setText(qtr("menu.game.new"));
    act_give_up_->setText(qtr("menu.game.giveup"));
    act_records_->setText(qtr("menu.game.record"));
    act_exit_->setText(qtr("menu.game.exit"));

    menu_audio_->setTitle(qtr("menu.audio"));
    act_music_->setText(qtr("menu.audio.music"));
    act_sound_->setText(qtr("menu.audio.sound"));

    menu_settings_->setTitle(qtr("menu.settings"));
    submenu_lang_->setTitle(qtr("menu.settings.language"));
    act_lang_tw_->setText(qtr("lang.zh_TW"));
    act_lang_cn_->setText(qtr("lang.zh_CN"));

    submenu_midi_tracks_->setTitle(qtr("menu.settings.midi_tracks"));
    act_midi_tracks_[0]->setText(qtr("menu.settings.track_101"));
    act_midi_tracks_[1]->setText(qtr("menu.settings.track_102"));
    act_midi_tracks_[2]->setText(qtr("menu.settings.track_103"));

    submenu_midi_device_->setTitle(qtr("menu.settings.midi_device"));
    act_midi_ost_->setText(qtr("menu.settings.midi_ost"));
    act_midi_native_->setText(qtr("menu.settings.midi_native"));
    act_midi_embedded_->setText(qtr("menu.settings.midi_embedded"));
    act_select_sf2_->setText(qtr("menu.settings.select_soundfont"));

    submenu_scale_->setTitle(qtr("menu.settings.scale"));
    act_scale_1x_->setText(qtr("menu.settings.scale_1x"));
    act_scale_1_5x_->setText(qtr("menu.settings.scale_1_5x"));
    act_scale_2x_->setText(qtr("menu.settings.scale_2x"));
    act_scale_3x_->setText(qtr("menu.settings.scale_3x"));
    act_scale_free_->setText(qtr("menu.settings.scale_free"));
    act_filter_nearest_->setText(qtr("menu.settings.filter_nearest"));
    act_filter_bilinear_->setText(qtr("menu.settings.filter_bilinear"));

    act_import_hst_->setText(qtr("menu.settings.import_hst"));
    act_export_hst_->setText(qtr("menu.settings.export_hst"));
    act_inspector_->setText(qtr("menu.settings.inspector"));

    menu_about_->setTitle(qtr("menu.about"));
    act_explain_->setText(qtr("menu.about.explain"));
    act_author_->setText(qtr("menu.about.author"));

    if (inspector_dlg_) {
        inspector_dlg_->retranslate();
        refresh_inspector_and_hint();
    }
}

void MainWindow::sync_menu_states() {
    bool on_title = game_state_.in_title_screen();
    act_new_game_->setEnabled(on_title);
    act_give_up_->setEnabled(!on_title);

    act_music_->setChecked(config_.music_on);
    act_sound_->setChecked(config_.sound_on);

    if (config_.language == "zh-CN") {
        act_lang_cn_->setChecked(true);
    } else {
        act_lang_tw_->setChecked(true);
    }

    for (int i = 0; i < 3; ++i) {
        act_midi_tracks_[i]->setChecked(config_.is_track_enabled(101 + i));
    }

    act_midi_ost_->setEnabled(audio_.has_ost_available());
    if (config_.midi_backend == core::MidiBackend::OstAudio && audio_.has_ost_available()) {
        act_midi_ost_->setChecked(true);
    } else if (config_.midi_backend == core::MidiBackend::NativeOS) {
        act_midi_native_->setChecked(true);
    } else {
        act_midi_embedded_->setChecked(true);
    }

    switch (config_.scale_mode) {
        case core::WindowScaleMode::Scale1x:
            act_scale_1x_->setChecked(true);
            break;
        case core::WindowScaleMode::Scale1_5x:
            act_scale_1_5x_->setChecked(true);
            break;
        case core::WindowScaleMode::Scale2x:
            act_scale_2x_->setChecked(true);
            break;
        case core::WindowScaleMode::Scale3x:
            act_scale_3x_->setChecked(true);
            break;
        case core::WindowScaleMode::FreeAspect:
            act_scale_free_->setChecked(true);
            break;
    }

    if (config_.scale_filter == core::ScaleFilter::Bilinear) {
        act_filter_bilinear_->setChecked(true);
    } else {
        act_filter_nearest_->setChecked(true);
    }
}

void MainWindow::apply_window_scale() {
    if (config_.scale_mode == core::WindowScaleMode::FreeAspect) {
        board_widget_->setMinimumSize(core::GameState::kCanvasWidth / 2, core::GameState::kCanvasHeight / 2);
        board_widget_->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        setMinimumSize(core::GameState::kCanvasWidth / 2, core::GameState::kCanvasHeight / 2 + menuBar()->sizeHint().height());
        setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    } else {
        float factor = config_.scale_factor();
        int target_w = static_cast<int>(std::lround(core::GameState::kCanvasWidth * factor));
        int target_h = static_cast<int>(std::lround(core::GameState::kCanvasHeight * factor));
        board_widget_->setFixedSize(target_w, target_h);
        setMinimumSize(0, 0);
        setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        adjustSize();
        setFixedSize(sizeHint());
    }
    board_widget_->update();
}

void MainWindow::save_config_and_scores() {
    config_.save(work_dir_ / "config.json");
    scores_.save_json(work_dir_ / "scores.json");
    scores_.save_legacy_hst(work_dir_ / "d4s.hst");
}

void MainWindow::trigger_new_game() {
    uint64_t now = now_ticks_ms();
    game_state_.start_new_game(now);
    sync_menu_states();
    renderer_.render(game_state_);
    board_widget_->sync_full(false);
    refresh_inspector_and_hint();
}

void MainWindow::trigger_give_up() {
    game_state_.give_up_game();
    sync_menu_states();
    renderer_.render(game_state_);
    board_widget_->sync_full(false);
    refresh_inspector_and_hint();
}

void MainWindow::set_language(const std::string& lang_code) {
    if (i18n_.set_language(lang_code)) {
        config_.language = lang_code;
        sync_menu_states();
        save_config_and_scores();
    }
}

void MainWindow::set_scale_mode(core::WindowScaleMode mode) {
    config_.scale_mode = mode;
    sync_menu_states();
    apply_window_scale();
    save_config_and_scores();
}

void MainWindow::set_scale_filter(core::ScaleFilter filter) {
    config_.scale_filter = filter;
    sync_menu_states();
    board_widget_->update();
    save_config_and_scores();
}

void MainWindow::set_midi_backend(core::MidiBackend backend) {
    config_.midi_backend = backend;
    sync_menu_states();
    audio_.update(config_);
    save_config_and_scores();
    refresh_inspector_and_hint();
}

void MainWindow::set_midi_track_enabled(int track_id, bool enabled) {
    config_.set_track_enabled(track_id, enabled);
    sync_menu_states();
    audio_.update(config_);
    save_config_and_scores();
    refresh_inspector_and_hint();
}

void MainWindow::set_inspector_visible(bool visible) {
    if (!inspector_dlg_) return;
    if (act_inspector_) {
        act_inspector_->blockSignals(true);
        act_inspector_->setChecked(visible);
        act_inspector_->blockSignals(false);
    }
    if (visible) {
        refresh_inspector_and_hint();
        inspector_dlg_->show();
        inspector_dlg_->raise();
    } else {
        inspector_dlg_->hide();
    }
}

void MainWindow::set_hint_highlight(bool enabled) {
    show_hint_overlay_ = enabled;
    if (inspector_dlg_ && inspector_dlg_->hint_highlight_enabled() != enabled) {
        inspector_dlg_->set_hint_highlight_enabled(enabled);
    }
    refresh_inspector_and_hint();
}

void MainWindow::refresh_inspector_and_hint() {
    auto snap = common::build_inspector_snapshot(game_state_, audio_, config_, i18n_);
    if (board_widget_) {
        board_widget_->set_hint_overlay(
            show_hint_overlay_ && snap.has_valid_hint,
            snap.hint_c1, snap.hint_r1, snap.hint_c2, snap.hint_r2);
    }
    if (inspector_dlg_ && inspector_dlg_->isVisible()) {
        inspector_dlg_->refresh(game_state_, audio_, config_);
    }
}

void MainWindow::on_game_tick() {
    uint64_t now = now_ticks_ms();
    int prev_rem = game_state_.remaining_seconds();
    bool prev_anim = game_state_.animating_link();
    int prev_tiles = game_state_.board().tiles_left();

    game_state_.tick(now);
    audio_.update(config_);

    process_game_events(now);

    if (game_state_.remaining_seconds() != prev_rem ||
        game_state_.animating_link() != prev_anim ||
        game_state_.board().tiles_left() != prev_tiles) {
        renderer_.render(game_state_);
        board_widget_->sync_full(false);
        refresh_inspector_and_hint();
    } else if (inspector_dlg_ && inspector_dlg_->isVisible()) {
        inspector_dlg_->refresh(game_state_, audio_, config_);
    }
}

void MainWindow::process_game_events(uint64_t now_ms) {
    auto events = game_state_.consume_events();
    if (events.empty()) return;

    bool has_modal = false;
    for (const auto& ev : events) {
        if (ev.type != core::GameEventType::PlaySfx) {
            has_modal = true;
            break;
        }
    }

    // Flush the cleared board synchronously BEFORE opening any modal dialog so the
    // uncovered current stage background (Pic_Level(level_)) is visible behind the popup!
    renderer_.render(game_state_);
    board_widget_->sync_full(has_modal);

    for (const auto& ev : events) {
        switch (ev.type) {
            case core::GameEventType::PlaySfx:
                audio_.play_sfx(ev.sfx_id);
                break;

            case core::GameEventType::StageClearModal: {
                QString title = QString::fromStdString(i18n_.tr("msg.stage_clear.title"));
                QString body = QString::fromStdString(i18n_.format("msg.stage_clear.body", {
                    {"level", std::to_string(ev.next_level_display)},
                    {"bonus", std::to_string(ev.bonus)}
                }));
                QMessageBox::information(this, title, body);
                game_state_.acknowledge_modal(now_ticks_ms());
                sync_menu_states();
                renderer_.render(game_state_);
                board_widget_->sync_full(false);
                break;
            }

            case core::GameEventType::AllStagesClearModal: {
                int total_bonus = ev.bonus + ev.life_bonus;
                int total_sc = static_cast<int>(std::lround(ev.total_score));
                QString title = QString::fromStdString(i18n_.tr("msg.all_clear.title"));
                QString body = QString::fromStdString(i18n_.format("msg.all_clear.body", {
                    {"bonus", std::to_string(total_bonus)},
                    {"score", std::to_string(total_sc)}
                }));
                QMessageBox::information(this, title, body);
                check_and_show_high_score(ev.total_score);
                game_state_.acknowledge_modal(now_ticks_ms());
                sync_menu_states();
                renderer_.render(game_state_);
                board_widget_->sync_full(false);
                break;
            }

            case core::GameEventType::NoMovesGameOverModal: {
                QString title = QString::fromStdString(i18n_.tr("msg.no_moves.title"));
                QString body = QString::fromStdString(i18n_.tr("msg.no_moves.body"));
                QMessageBox::information(this, title, body);
                check_and_show_high_score(ev.total_score);
                game_state_.acknowledge_modal(now_ticks_ms());
                sync_menu_states();
                renderer_.render(game_state_);
                board_widget_->sync_full(false);
                break;
            }

            case core::GameEventType::TimeoutGameOverModal: {
                int total_sc = static_cast<int>(std::lround(ev.total_score));
                QString title = QString::fromStdString(i18n_.tr("msg.timeout.title"));
                QString body = QString::fromStdString(i18n_.format("msg.timeout.body", {
                    {"bonus", std::to_string(ev.bonus)},
                    {"score", std::to_string(total_sc)}
                }));
                QMessageBox::information(this, title, body);
                check_and_show_high_score(ev.total_score);
                game_state_.acknowledge_modal(now_ticks_ms());
                sync_menu_states();
                renderer_.render(game_state_);
                board_widget_->sync_full(false);
                break;
            }
        }
    }
    refresh_inspector_and_hint();
    (void)now_ms;
}

void MainWindow::check_and_show_high_score(float final_score) {
    if (!scores_.is_high_score(final_score)) {
        return;
    }
    std::string def_name = config_.player_name.empty() ? i18n_.tr("table.default_player") : config_.player_name;
    int rank = scores_.insert_score(def_name, final_score);
    if (rank >= 0) {
        TableDialog dlg(scores_, i18n_, rank, def_name, this);
        dlg.exec();
        config_.player_name = dlg.entered_name();
        save_config_and_scores();
    }
}

void MainWindow::on_show_records() {
    TableDialog dlg(scores_, i18n_, -1, config_.player_name, this);
    dlg.exec();
}

void MainWindow::on_show_explain() {
    ImageSplashDialog dlg(assets_.form_explain(), QString::fromStdString(i18n_.tr("dialog.explain.title")), this);
    dlg.exec();
}

void MainWindow::on_show_author() {
    ImageSplashDialog dlg(assets_.form_author(), QString::fromStdString(i18n_.tr("dialog.author.title")), this);
    dlg.exec();
}

void MainWindow::on_import_hst() {
    QString path = QFileDialog::getOpenFileName(
        this,
        QString::fromStdString(i18n_.tr("menu.settings.import_hst")),
        QString::fromStdString(work_dir_.string()),
        "Legacy High Score (d4s.hst *.hst *.HST);;All Files (*)");
    if (path.isEmpty()) return;

    if (scores_.load_legacy_hst(path.toStdString())) {
        save_config_and_scores();
        on_show_records();
    }
}

void MainWindow::on_export_hst() {
    QString path = QFileDialog::getSaveFileName(
        this,
        QString::fromStdString(i18n_.tr("menu.settings.export_hst")),
        QString::fromStdString((work_dir_ / "d4s.hst").string()),
        "Legacy High Score (d4s.hst *.hst *.HST);;All Files (*)");
    if (path.isEmpty()) return;

    scores_.save_legacy_hst(path.toStdString());
}

void MainWindow::on_select_soundfont() {
    QString path = QFileDialog::getOpenFileName(
        this,
        QString::fromStdString(i18n_.tr("menu.settings.select_soundfont")),
        QString::fromStdString(work_dir_.string()),
        "SoundFont (*.sf2 *.SF2 *.sf3 *.SF3);;All Files (*)");
    if (path.isEmpty()) return;

    std::string sf_path = path.toStdString();
    if (audio_.load_soundfont(sf_path)) {
        config_.soundfont_path = sf_path;
        set_midi_backend(core::MidiBackend::EmbeddedSoundFont);
    }
}

} // namespace llk2::ui::qt
