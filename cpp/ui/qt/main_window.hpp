#pragma once

#include "core/config.hpp"
#include "core/game_state.hpp"
#include "core/hst_score.hpp"
#include "core/i18n.hpp"
#include "media/asset_manager.hpp"
#include "media/audio_engine.hpp"
#include "media/board_renderer.hpp"

#include <QAction>
#include <QActionGroup>
#include <QCloseEvent>
#include <QImage>
#include <QMainWindow>
#include <QMenu>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QTimer>
#include <QWidget>
#include <array>
#include <filesystem>
#include <functional>

namespace llk2::ui::qt {

class InspectorDialog;

class BoardWidget : public QWidget {
public:
    using MouseMoveCallback = std::function<void(float lx, float ly)>;
    using MouseDownCallback = std::function<void(int button, float lx, float ly)>;

    BoardWidget(media::BoardRenderer& renderer, const core::Config& config, QWidget* parent = nullptr);

    void set_callbacks(MouseMoveCallback on_move, MouseDownCallback on_down);
    void set_hint_overlay(bool enabled, int c1, int r1, int c2, int r2);

    // Sync full 768x550 RGBA8888 canvas to native QImage::Format_RGB32 and schedule repaint
    void sync_full(bool immediate_repaint = false);

    // Fast partial sync for mouse hover transitions (updates only the 1-2 affected 46x56 tiles!)
    void sync_tile_hover_change(int prev_c, int prev_r, int new_c, int new_r);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    bool map_to_logical(const QPointF& pos, float& out_lx, float& out_ly) const;
    void copy_rgba_rect_to_rgb32(int rx, int ry, int rw, int rh);
    QRect map_logical_rect_to_widget(int rx, int ry, int rw, int rh) const;

    media::BoardRenderer& renderer_;
    const core::Config& config_;
    QImage rgb32_buf_;
    MouseMoveCallback on_move_;
    MouseDownCallback on_down_;

    bool hint_enabled_ = false;
    int hint_c1_ = -1;
    int hint_r1_ = -1;
    int hint_c2_ = -1;
    int hint_r2_ = -1;
};

class MainWindow : public QMainWindow {
public:
    MainWindow(media::AssetManager& assets,
               media::AudioEngine& audio,
               core::Config& config,
               core::I18n& i18n,
               core::HighScoreTable& scores,
               const std::filesystem::path& work_dir,
               QWidget* parent = nullptr);

    // Public helpers for automated smoke tests & direct programmatic control
    void trigger_new_game();
    void trigger_give_up();
    void set_language(const std::string& lang_code);
    void set_scale_mode(core::WindowScaleMode mode);
    void set_scale_filter(core::ScaleFilter filter);
    void set_midi_backend(core::MidiBackend backend);
    void set_midi_track_enabled(int track_id, bool enabled);
    void set_inspector_visible(bool visible);
    void set_hint_highlight(bool enabled);

    core::GameState& game_state() noexcept { return game_state_; }
    BoardWidget* board_widget() noexcept { return board_widget_; }

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void build_menus();
    void retranslate_ui();
    void sync_menu_states();
    void apply_window_scale();
    void save_config_and_scores();

    void on_game_tick();
    void process_game_events(uint64_t now_ms);
    void check_and_show_high_score(float final_score);

    void on_show_records();
    void on_show_explain();
    void on_show_author();
    void on_import_hst();
    void on_export_hst();
    void on_select_soundfont();
    void refresh_inspector_and_hint();

    static uint64_t now_ticks_ms();

    media::AssetManager& assets_;
    media::AudioEngine& audio_;
    core::Config& config_;
    core::I18n& i18n_;
    core::HighScoreTable& scores_;
    std::filesystem::path work_dir_;

    core::GameState game_state_;
    media::BoardRenderer renderer_;
    BoardWidget* board_widget_ = nullptr;
    InspectorDialog* inspector_dlg_ = nullptr;
    QTimer* tick_timer_ = nullptr;
    bool show_hint_overlay_ = false;

    // Top-level menus: 遊戲(&G) - 背景聲音(&B) - 設定(&S) - 關於(&A)
    QMenu* menu_game_ = nullptr;
    QMenu* menu_audio_ = nullptr;
    QMenu* menu_settings_ = nullptr;
    QMenu* menu_about_ = nullptr;

    // Game menu actions
    QAction* act_new_game_ = nullptr;
    QAction* act_give_up_ = nullptr;
    QAction* act_records_ = nullptr;
    QAction* act_exit_ = nullptr;

    // Audio menu actions
    QAction* act_music_ = nullptr;
    QAction* act_sound_ = nullptr;

    // Settings submenus & actions
    QMenu* submenu_lang_ = nullptr;
    QActionGroup* group_lang_ = nullptr;
    QAction* act_lang_tw_ = nullptr;
    QAction* act_lang_cn_ = nullptr;

    QMenu* submenu_midi_tracks_ = nullptr;
    std::array<QAction*, 3> act_midi_tracks_{};

    QMenu* submenu_midi_device_ = nullptr;
    QActionGroup* group_midi_device_ = nullptr;
    QAction* act_midi_ost_ = nullptr;
    QAction* act_midi_native_ = nullptr;
    QAction* act_midi_embedded_ = nullptr;
    QAction* act_select_sf2_ = nullptr;

    QMenu* submenu_scale_ = nullptr;
    QActionGroup* group_scale_mode_ = nullptr;
    QAction* act_scale_1x_ = nullptr;
    QAction* act_scale_1_5x_ = nullptr;
    QAction* act_scale_2x_ = nullptr;
    QAction* act_scale_3x_ = nullptr;
    QAction* act_scale_free_ = nullptr;

    QActionGroup* group_scale_filter_ = nullptr;
    QAction* act_filter_nearest_ = nullptr;
    QAction* act_filter_bilinear_ = nullptr;

    QAction* act_import_hst_ = nullptr;
    QAction* act_export_hst_ = nullptr;
    QAction* act_inspector_ = nullptr;

    // About menu actions
    QAction* act_explain_ = nullptr;
    QAction* act_author_ = nullptr;
};

} // namespace llk2::ui::qt
