#pragma once

#include "core/app_paths.hpp"
#include "core/config.hpp"
#include "core/hst_score.hpp"
#include "core/i18n.hpp"
#include "launcher/asset_mod_builder.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>

#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace llk2::launcher {

class LauncherWindow : public QMainWindow {
public:
    LauncherWindow(core::AppPaths paths,
                   core::Config& config,
                   core::I18n& i18n,
                   core::HighScoreTable& scores,
                   QWidget* parent = nullptr);
    ~LauncherWindow() override;

    void set_language(const std::string& lang_code);
    void select_tab(int index);
    void refresh_all();

    // Accessors for automated smoke testing
    int tab_count() const;
    std::filesystem::path selected_pack_dir() const;
    std::filesystem::path selected_frontend_binary() const;
    const core::AppPaths& paths() const noexcept { return paths_; }

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    struct FrontendInfo {
        std::string id;           // "llk2-qt", "llk2-imgui", "llk2-bui"
        std::string display_name; // "Qt 6 (llk2-qt)", etc.
        std::filesystem::path binary_path;
    };

    struct PackEntry {
        std::string label;
        std::filesystem::path dir;
        bool is_default = false;
        bool is_custom = false;
    };

    QString qtr(const std::string& key) const;
    QString qtr_mnem_safe(const std::string& key) const;

    void build_ui();
    QWidget* build_play_tab();
    QWidget* build_assets_tab();
    QWidget* build_settings_tab();
    QWidget* build_scores_tab();
    QWidget* build_diagnostics_tab();

    void retranslate_ui();
    void refresh_frontends();
    void refresh_packs();
    void refresh_play_preview();
    void refresh_settings_from_config();
    void refresh_scores_table();
    void refresh_diagnostics();
    void append_log(const std::string& line);

    // Tab 1 actions
    void on_browse_custom_pack();
    void on_launch_game();

    // Tab 2 actions
    void on_exe_path_changed(const std::filesystem::path& path);
    void on_browse_exe();
    void on_extract_exe();
    void on_import_ost();
    void on_new_mod_pack();
    void on_clone_mod_pack();
    void on_delete_mod_pack();
    void on_check_pack_integrity();
    void on_replace_tile();
    void on_replace_level_bg();
    void on_replace_title_img();
    void on_replace_audio();

    // Tab 3 actions
    void on_save_settings(bool show_status = true);
    void on_reset_settings();
    void on_browse_soundfont();
    void on_clear_soundfont();

    // Tab 4 actions
    void on_save_scores();
    void on_import_hst();
    void on_export_hst();
    void on_backup_scores_json();
    void on_restore_scores_json();
    void on_clear_scores();

    // Tab 5 actions
    void open_folder(const std::filesystem::path& dir, bool create_if_missing);

    std::filesystem::path current_mod_target_dir() const;
    std::string current_mod_target_name() const;
    std::string find_python_executable() const;

    core::AppPaths paths_;
    core::Config& config_;
    core::I18n& i18n_;
    core::HighScoreTable& scores_;

    std::vector<FrontendInfo> frontends_;
    std::vector<PackEntry> packs_;
    std::filesystem::path custom_pack_dir_;
    std::vector<std::string> lang_codes_;
    bool syncing_ui_ = false;

    QTabWidget* tabs_ = nullptr;

    // ── Tab 1: Play ──
    QLabel* lbl_frontend_ = nullptr;
    QComboBox* cmb_frontend_ = nullptr;
    QLabel* lbl_pack_ = nullptr;
    QComboBox* cmb_pack_ = nullptr;
    QPushButton* btn_browse_pack_ = nullptr;
    QPushButton* btn_refresh_play_ = nullptr;
    QGroupBox* grp_preview_title_ = nullptr;
    QLabel* lbl_title_preview_ = nullptr;
    QLabel* lbl_pack_status_ = nullptr;
    QGroupBox* grp_preview_tiles_ = nullptr;
    QLabel* lbl_tiles_preview_ = nullptr;
    QCheckBox* chk_close_on_launch_ = nullptr;
    QPushButton* btn_launch_ = nullptr;

    // ── Tab 2: Assets & Mods ──
    QGroupBox* grp_extract_ = nullptr;
    QLineEdit* edit_exe_path_ = nullptr;
    QPushButton* btn_browse_exe_ = nullptr;
    QLabel* lbl_hash_status_ = nullptr;
    QCheckBox* chk_skip_hash_ = nullptr;
    QPushButton* btn_extract_ = nullptr;
    QPushButton* btn_import_ost_ = nullptr;
    QProcess* extract_proc_ = nullptr;

    QGroupBox* grp_mods_ = nullptr;
    QLabel* lbl_mod_select_ = nullptr;
    QComboBox* cmb_mod_target_ = nullptr;
    QPushButton* btn_new_mod_ = nullptr;
    QPushButton* btn_clone_mod_ = nullptr;
    QPushButton* btn_delete_mod_ = nullptr;
    QPushButton* btn_check_pack_ = nullptr;
    QLabel* lbl_mod_tile_ = nullptr;
    QSpinBox* spin_tile_id_ = nullptr;
    QPushButton* btn_replace_tile_ = nullptr;
    QLabel* lbl_mod_level_ = nullptr;
    QComboBox* cmb_level_idx_ = nullptr;
    QPushButton* btn_replace_level_ = nullptr;
    QPushButton* btn_replace_title_ = nullptr;
    QLabel* lbl_mod_audio_ = nullptr;
    QComboBox* cmb_audio_slot_ = nullptr;
    QPushButton* btn_replace_audio_ = nullptr;
    QLabel* lbl_log_ = nullptr;
    QPlainTextEdit* txt_log_ = nullptr;

    // ── Tab 3: Settings ──
    QGroupBox* grp_display_ = nullptr;
    QLabel* lbl_set_lang_ = nullptr;
    QComboBox* cmb_set_lang_ = nullptr;
    QLabel* lbl_set_scale_ = nullptr;
    QComboBox* cmb_set_scale_ = nullptr;
    QLabel* lbl_set_filter_ = nullptr;
    QComboBox* cmb_set_filter_ = nullptr;
    QCheckBox* chk_set_fullscreen_ = nullptr;
    QCheckBox* chk_set_rumble_ = nullptr;
    QLabel* lbl_set_player_ = nullptr;
    QLineEdit* edit_set_player_ = nullptr;

    QGroupBox* grp_audio_ = nullptr;
    QCheckBox* chk_set_music_ = nullptr;
    QCheckBox* chk_set_sound_ = nullptr;
    QLabel* lbl_set_backend_ = nullptr;
    QComboBox* cmb_set_backend_ = nullptr;
    QLabel* lbl_set_sf2_ = nullptr;
    QLineEdit* edit_set_sf2_ = nullptr;
    QPushButton* btn_browse_sf2_ = nullptr;
    QPushButton* btn_clear_sf2_ = nullptr;
    QLabel* lbl_set_tracks_ = nullptr;
    std::array<QCheckBox*, 3> chk_set_tracks_{};
    QPushButton* btn_save_settings_ = nullptr;
    QPushButton* btn_reset_settings_ = nullptr;

    // ── Tab 4: High Scores ──
    QLabel* lbl_scores_hint_ = nullptr;
    QTableWidget* tbl_scores_ = nullptr;
    QPushButton* btn_save_scores_ = nullptr;
    QPushButton* btn_import_hst_ = nullptr;
    QPushButton* btn_export_hst_ = nullptr;
    QPushButton* btn_backup_json_ = nullptr;
    QPushButton* btn_restore_json_ = nullptr;
    QPushButton* btn_clear_scores_ = nullptr;

    // ── Tab 5: Diagnostics ──
    QGroupBox* grp_storage_ = nullptr;
    QLabel* lbl_diag_mode_k_ = nullptr;
    QLabel* lbl_diag_mode_v_ = nullptr;
    QLabel* lbl_diag_data_k_ = nullptr;
    QLabel* lbl_diag_data_v_ = nullptr;
    QLabel* lbl_diag_exe_k_ = nullptr;
    QLabel* lbl_diag_exe_v_ = nullptr;
    QGroupBox* grp_folders_ = nullptr;
    QPushButton* btn_open_data_ = nullptr;
    QPushButton* btn_open_assets_ = nullptr;
    QPushButton* btn_open_mods_ = nullptr;
    QPushButton* btn_open_ost_ = nullptr;
    QPushButton* btn_open_locales_ = nullptr;
    QGroupBox* grp_health_ = nullptr;
    QPlainTextEdit* txt_health_ = nullptr;
    QPushButton* btn_refresh_diag_ = nullptr;
};

} // namespace llk2::launcher
