#include "launcher/launcher_window.hpp"

#include "media/asset_manager.hpp"
#include "media/audio_engine.hpp"
#include "ui/common/ui_helpers.hpp"

#include <QApplication>
#include <QDesktopServices>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QMimeData>
#include <QPixmap>
#include <QStandardPaths>
#include <QStatusBar>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <system_error>

namespace llk2::launcher {

namespace {

const char* kAudioSlotRelPaths[] = {
    "sfx/201.wav",
    "sfx/202.wav",
    "sfx/203.wav",
    "sfx/204.wav",
    "sfx/205.wav",
    "sfx/206.wav",
    "midi/101.mid",
    "midi/102.mid",
    "midi/103.mid",
};

const char* kAudioSlotNames[] = {
    "SFX 201 (Cancel / Right-click)",
    "SFX 202 (Select Tile)",
    "SFX 203 (Match Success)",
    "SFX 204 (Invalid / Timeout)",
    "SFX 205 (Stage Clear)",
    "SFX 206 (Deadlock Shuffle)",
    "MIDI 101 (Track 1: Aquarelle)",
    "MIDI 102 (Track 2: Sora to Kimi no Aida ni)",
    "MIDI 103 (Track 3: Making Love Out Of Nothing At All)",
};

} // namespace

LauncherWindow::LauncherWindow(core::AppPaths paths,
                               core::Config& config,
                               core::I18n& i18n,
                               core::HighScoreTable& scores,
                               QWidget* parent)
    : QMainWindow(parent),
      paths_(std::move(paths)),
      config_(config),
      i18n_(i18n),
      scores_(scores) {
    setAcceptDrops(true);
    setMinimumSize(760, 560);
    resize(860, 630);

    build_ui();
    retranslate_ui();
    refresh_all();
}

LauncherWindow::~LauncherWindow() {
    if (extract_proc_ && extract_proc_->state() != QProcess::NotRunning) {
        extract_proc_->kill();
        extract_proc_->waitForFinished(1000);
    }
}

QString LauncherWindow::qtr(const std::string& key) const {
    return QString::fromStdString(i18n_.tr(key));
}

QString LauncherWindow::qtr_mnem_safe(const std::string& key) const {
    return QString::fromStdString(i18n_.tr(key)).replace('&', QStringLiteral("&&"));
}

void LauncherWindow::build_ui() {
    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(10, 10, 10, 6);
    root->setSpacing(8);

    tabs_ = new QTabWidget(central);
    tabs_->addTab(build_play_tab(), "");
    tabs_->addTab(build_assets_tab(), "");
    tabs_->addTab(build_settings_tab(), "");
    tabs_->addTab(build_scores_tab(), "");
    tabs_->addTab(build_diagnostics_tab(), "");
    root->addWidget(tabs_, 1);

    setCentralWidget(central);
    statusBar()->showMessage("");
}

QWidget* LauncherWindow::build_play_tab() {
    auto* page = new QWidget(this);
    auto* lay = new QVBoxLayout(page);
    lay->setContentsMargins(12, 12, 12, 12);
    lay->setSpacing(10);

    auto* top_grid = new QGridLayout();
    top_grid->setHorizontalSpacing(10);
    top_grid->setVerticalSpacing(8);

    lbl_frontend_ = new QLabel(page);
    cmb_frontend_ = new QComboBox(page);
    cmb_frontend_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    lbl_pack_ = new QLabel(page);
    cmb_pack_ = new QComboBox(page);
    cmb_pack_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    btn_browse_pack_ = new QPushButton(page);
    btn_refresh_play_ = new QPushButton(page);

    top_grid->addWidget(lbl_frontend_, 0, 0);
    top_grid->addWidget(cmb_frontend_, 0, 1, 1, 2);
    top_grid->addWidget(btn_refresh_play_, 0, 3);
    top_grid->addWidget(lbl_pack_, 1, 0);
    top_grid->addWidget(cmb_pack_, 1, 1, 1, 2);
    top_grid->addWidget(btn_browse_pack_, 1, 3);
    lay->addLayout(top_grid);

    // Side-by-side live previews: Title Screen + Composited 42-Tile Sprite Sheet
    auto* prev_row = new QHBoxLayout();
    prev_row->setSpacing(12);

    grp_preview_title_ = new QGroupBox(page);
    auto* title_lay = new QVBoxLayout(grp_preview_title_);
    lbl_title_preview_ = new QLabel(grp_preview_title_);
    lbl_title_preview_->setAlignment(Qt::AlignCenter);
    lbl_title_preview_->setMinimumSize(320, 240);
    lbl_title_preview_->setStyleSheet("background-color: #0e1015; color: #c8ceda; border: 1px solid #383c48;");
    lbl_title_preview_->setWordWrap(true);
    title_lay->addWidget(lbl_title_preview_, 1);

    lbl_pack_status_ = new QLabel(grp_preview_title_);
    lbl_pack_status_->setAlignment(Qt::AlignCenter);
    lbl_pack_status_->setWordWrap(true);
    title_lay->addWidget(lbl_pack_status_);

    grp_preview_tiles_ = new QGroupBox(page);
    auto* tiles_lay = new QVBoxLayout(grp_preview_tiles_);
    lbl_tiles_preview_ = new QLabel(grp_preview_tiles_);
    lbl_tiles_preview_->setAlignment(Qt::AlignCenter);
    lbl_tiles_preview_->setMinimumSize(320, 260);
    lbl_tiles_preview_->setStyleSheet("background-color: #12141a; color: #c8ceda; border: 1px solid #383c48;");
    lbl_tiles_preview_->setWordWrap(true);
    tiles_lay->addWidget(lbl_tiles_preview_, 1);

    prev_row->addWidget(grp_preview_title_, 1);
    prev_row->addWidget(grp_preview_tiles_, 1);
    lay->addLayout(prev_row, 1);

    // Bottom launch bar
    auto* bottom_row = new QHBoxLayout();
    chk_close_on_launch_ = new QCheckBox(page);
    bottom_row->addWidget(chk_close_on_launch_);
    bottom_row->addStretch(1);

    btn_launch_ = new QPushButton(page);
    btn_launch_->setMinimumSize(170, 38);
    QFont bf = btn_launch_->font();
    bf.setBold(true);
    bf.setPointSize(bf.pointSize() + 1);
    btn_launch_->setFont(bf);
    bottom_row->addWidget(btn_launch_);
    lay->addLayout(bottom_row);

    connect(cmb_pack_, &QComboBox::currentIndexChanged, this, [this](int) {
        if (!syncing_ui_) refresh_play_preview();
    });
    connect(btn_browse_pack_, &QPushButton::clicked, this, &LauncherWindow::on_browse_custom_pack);
    connect(btn_refresh_play_, &QPushButton::clicked, this, &LauncherWindow::refresh_all);
    connect(btn_launch_, &QPushButton::clicked, this, &LauncherWindow::on_launch_game);

    return page;
}

QWidget* LauncherWindow::build_assets_tab() {
    auto* page = new QWidget(this);
    auto* lay = new QVBoxLayout(page);
    lay->setContentsMargins(12, 12, 12, 12);
    lay->setSpacing(10);

    // 1. Local EXE / ZIP Extractor
    grp_extract_ = new QGroupBox(page);
    auto* ext_lay = new QVBoxLayout(grp_extract_);
    ext_lay->setSpacing(6);

    auto* exe_row = new QHBoxLayout();
    edit_exe_path_ = new QLineEdit(grp_extract_);
    btn_browse_exe_ = new QPushButton(grp_extract_);
    exe_row->addWidget(edit_exe_path_, 1);
    exe_row->addWidget(btn_browse_exe_);
    ext_lay->addLayout(exe_row);

    lbl_hash_status_ = new QLabel(grp_extract_);
    lbl_hash_status_->setWordWrap(true);
    lbl_hash_status_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    ext_lay->addWidget(lbl_hash_status_);

    auto* ext_act_row = new QHBoxLayout();
    chk_skip_hash_ = new QCheckBox(grp_extract_);
    btn_extract_ = new QPushButton(grp_extract_);
    btn_import_ost_ = new QPushButton(grp_extract_);
    ext_act_row->addWidget(chk_skip_hash_);
    ext_act_row->addStretch(1);
    ext_act_row->addWidget(btn_import_ost_);
    ext_act_row->addWidget(btn_extract_);
    ext_lay->addLayout(ext_act_row);
    lay->addWidget(grp_extract_);

    // 2. Mod Pack Studio
    grp_mods_ = new QGroupBox(page);
    auto* mod_lay = new QVBoxLayout(grp_mods_);
    mod_lay->setSpacing(8);

    auto* mod_bar = new QHBoxLayout();
    lbl_mod_select_ = new QLabel(grp_mods_);
    cmb_mod_target_ = new QComboBox(grp_mods_);
    cmb_mod_target_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    btn_new_mod_ = new QPushButton(grp_mods_);
    btn_clone_mod_ = new QPushButton(grp_mods_);
    btn_delete_mod_ = new QPushButton(grp_mods_);
    btn_check_pack_ = new QPushButton(grp_mods_);
    mod_bar->addWidget(lbl_mod_select_);
    mod_bar->addWidget(cmb_mod_target_, 1);
    mod_bar->addWidget(btn_new_mod_);
    mod_bar->addWidget(btn_clone_mod_);
    mod_bar->addWidget(btn_delete_mod_);
    mod_bar->addWidget(btn_check_pack_);
    mod_lay->addLayout(mod_bar);

    auto* edit_grid = new QGridLayout();
    edit_grid->setHorizontalSpacing(8);
    edit_grid->setVerticalSpacing(6);

    lbl_mod_tile_ = new QLabel(grp_mods_);
    spin_tile_id_ = new QSpinBox(grp_mods_);
    spin_tile_id_->setRange(0, 41);
    btn_replace_tile_ = new QPushButton(grp_mods_);

    lbl_mod_level_ = new QLabel(grp_mods_);
    cmb_level_idx_ = new QComboBox(grp_mods_);
    btn_replace_level_ = new QPushButton(grp_mods_);
    btn_replace_title_ = new QPushButton(grp_mods_);

    lbl_mod_audio_ = new QLabel(grp_mods_);
    cmb_audio_slot_ = new QComboBox(grp_mods_);
    for (const char* name : kAudioSlotNames) {
        cmb_audio_slot_->addItem(QString::fromUtf8(name));
    }
    btn_replace_audio_ = new QPushButton(grp_mods_);

    edit_grid->addWidget(lbl_mod_tile_, 0, 0);
    edit_grid->addWidget(spin_tile_id_, 0, 1);
    edit_grid->addWidget(btn_replace_tile_, 0, 2);
    edit_grid->addWidget(btn_replace_title_, 0, 3);

    edit_grid->addWidget(lbl_mod_level_, 1, 0);
    edit_grid->addWidget(cmb_level_idx_, 1, 1, 1, 2);
    edit_grid->addWidget(btn_replace_level_, 1, 3);

    edit_grid->addWidget(lbl_mod_audio_, 2, 0);
    edit_grid->addWidget(cmb_audio_slot_, 2, 1, 1, 2);
    edit_grid->addWidget(btn_replace_audio_, 2, 3);
    mod_lay->addLayout(edit_grid);

    lay->addWidget(grp_mods_);

    // 3. Activity Log
    lbl_log_ = new QLabel(page);
    txt_log_ = new QPlainTextEdit(page);
    txt_log_->setReadOnly(true);
    txt_log_->setMaximumBlockCount(400);
    QFont mono("monospace");
    mono.setStyleHint(QFont::Monospace);
    txt_log_->setFont(mono);
    lay->addWidget(lbl_log_);
    lay->addWidget(txt_log_, 1);

    connect(edit_exe_path_, &QLineEdit::editingFinished, this, [this]() {
        on_exe_path_changed(edit_exe_path_->text().trimmed().toStdString());
    });
    connect(btn_browse_exe_, &QPushButton::clicked, this, &LauncherWindow::on_browse_exe);
    connect(btn_extract_, &QPushButton::clicked, this, &LauncherWindow::on_extract_exe);
    connect(btn_import_ost_, &QPushButton::clicked, this, &LauncherWindow::on_import_ost);
    connect(btn_new_mod_, &QPushButton::clicked, this, &LauncherWindow::on_new_mod_pack);
    connect(btn_clone_mod_, &QPushButton::clicked, this, &LauncherWindow::on_clone_mod_pack);
    connect(btn_delete_mod_, &QPushButton::clicked, this, &LauncherWindow::on_delete_mod_pack);
    connect(btn_check_pack_, &QPushButton::clicked, this, &LauncherWindow::on_check_pack_integrity);
    connect(btn_replace_tile_, &QPushButton::clicked, this, &LauncherWindow::on_replace_tile);
    connect(btn_replace_level_, &QPushButton::clicked, this, &LauncherWindow::on_replace_level_bg);
    connect(btn_replace_title_, &QPushButton::clicked, this, &LauncherWindow::on_replace_title_img);
    connect(btn_replace_audio_, &QPushButton::clicked, this, &LauncherWindow::on_replace_audio);

    return page;
}

QWidget* LauncherWindow::build_settings_tab() {
    auto* page = new QWidget(this);
    auto* lay = new QVBoxLayout(page);
    lay->setContentsMargins(12, 12, 12, 12);
    lay->setSpacing(10);

    // Display & Interface
    grp_display_ = new QGroupBox(page);
    auto* disp_form = new QFormLayout(grp_display_);
    disp_form->setSpacing(8);

    lbl_set_lang_ = new QLabel(grp_display_);
    cmb_set_lang_ = new QComboBox(grp_display_);
    disp_form->addRow(lbl_set_lang_, cmb_set_lang_);

    lbl_set_scale_ = new QLabel(grp_display_);
    cmb_set_scale_ = new QComboBox(grp_display_);
    disp_form->addRow(lbl_set_scale_, cmb_set_scale_);

    lbl_set_filter_ = new QLabel(grp_display_);
    cmb_set_filter_ = new QComboBox(grp_display_);
    disp_form->addRow(lbl_set_filter_, cmb_set_filter_);

    chk_set_fullscreen_ = new QCheckBox(grp_display_);
    disp_form->addRow("", chk_set_fullscreen_);

    lbl_set_player_ = new QLabel(grp_display_);
    edit_set_player_ = new QLineEdit(grp_display_);
    disp_form->addRow(lbl_set_player_, edit_set_player_);
    lay->addWidget(grp_display_);

    // Audio & Music
    grp_audio_ = new QGroupBox(page);
    auto* aud_form = new QFormLayout(grp_audio_);
    aud_form->setSpacing(8);

    auto* toggle_row = new QHBoxLayout();
    chk_set_music_ = new QCheckBox(grp_audio_);
    chk_set_sound_ = new QCheckBox(grp_audio_);
    toggle_row->addWidget(chk_set_music_);
    toggle_row->addSpacing(18);
    toggle_row->addWidget(chk_set_sound_);
    toggle_row->addStretch(1);
    aud_form->addRow("", toggle_row);

    lbl_set_backend_ = new QLabel(grp_audio_);
    cmb_set_backend_ = new QComboBox(grp_audio_);
    aud_form->addRow(lbl_set_backend_, cmb_set_backend_);

    lbl_set_sf2_ = new QLabel(grp_audio_);
    auto* sf2_row = new QHBoxLayout();
    edit_set_sf2_ = new QLineEdit(grp_audio_);
    edit_set_sf2_->setReadOnly(true);
    btn_browse_sf2_ = new QPushButton(grp_audio_);
    btn_clear_sf2_ = new QPushButton(grp_audio_);
    sf2_row->addWidget(edit_set_sf2_, 1);
    sf2_row->addWidget(btn_browse_sf2_);
    sf2_row->addWidget(btn_clear_sf2_);
    aud_form->addRow(lbl_set_sf2_, sf2_row);

    lbl_set_tracks_ = new QLabel(grp_audio_);
    auto* trk_box = new QVBoxLayout();
    trk_box->setSpacing(4);
    for (int i = 0; i < 3; ++i) {
        chk_set_tracks_[i] = new QCheckBox(grp_audio_);
        trk_box->addWidget(chk_set_tracks_[i]);
    }
    aud_form->addRow(lbl_set_tracks_, trk_box);
    lay->addWidget(grp_audio_);

    lay->addStretch(1);

    auto* btn_row = new QHBoxLayout();
    btn_reset_settings_ = new QPushButton(page);
    btn_save_settings_ = new QPushButton(page);
    btn_save_settings_->setMinimumWidth(130);
    btn_row->addWidget(btn_reset_settings_);
    btn_row->addStretch(1);
    btn_row->addWidget(btn_save_settings_);
    lay->addLayout(btn_row);

    connect(cmb_set_lang_, &QComboBox::currentIndexChanged, this, [this](int idx) {
        if (syncing_ui_ || idx < 0 || idx >= static_cast<int>(lang_codes_.size())) return;
        set_language(lang_codes_[static_cast<size_t>(idx)]);
        on_save_settings(true);
    });
    connect(btn_browse_sf2_, &QPushButton::clicked, this, &LauncherWindow::on_browse_soundfont);
    connect(btn_clear_sf2_, &QPushButton::clicked, this, &LauncherWindow::on_clear_soundfont);
    connect(btn_save_settings_, &QPushButton::clicked, this, [this]() { on_save_settings(true); });
    connect(btn_reset_settings_, &QPushButton::clicked, this, &LauncherWindow::on_reset_settings);

    return page;
}

QWidget* LauncherWindow::build_scores_tab() {
    auto* page = new QWidget(this);
    auto* lay = new QVBoxLayout(page);
    lay->setContentsMargins(12, 12, 12, 12);
    lay->setSpacing(10);

    lbl_scores_hint_ = new QLabel(page);
    lbl_scores_hint_->setWordWrap(true);
    lay->addWidget(lbl_scores_hint_);

    tbl_scores_ = new QTableWidget(core::HighScoreTable::kCount, 3, page);
    tbl_scores_->verticalHeader()->setVisible(false);
    tbl_scores_->setAlternatingRowColors(true);
    tbl_scores_->setSelectionBehavior(QAbstractItemView::SelectRows);
    tbl_scores_->setSelectionMode(QAbstractItemView::SingleSelection);
    tbl_scores_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tbl_scores_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    tbl_scores_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    lay->addWidget(tbl_scores_, 1);

    auto* row1 = new QHBoxLayout();
    btn_import_hst_ = new QPushButton(page);
    btn_export_hst_ = new QPushButton(page);
    btn_backup_json_ = new QPushButton(page);
    btn_restore_json_ = new QPushButton(page);
    row1->addWidget(btn_import_hst_);
    row1->addWidget(btn_export_hst_);
    row1->addWidget(btn_backup_json_);
    row1->addWidget(btn_restore_json_);
    row1->addStretch(1);
    lay->addLayout(row1);

    auto* row2 = new QHBoxLayout();
    btn_clear_scores_ = new QPushButton(page);
    btn_save_scores_ = new QPushButton(page);
    btn_save_scores_->setMinimumWidth(130);
    row2->addWidget(btn_clear_scores_);
    row2->addStretch(1);
    row2->addWidget(btn_save_scores_);
    lay->addLayout(row2);

    connect(btn_save_scores_, &QPushButton::clicked, this, &LauncherWindow::on_save_scores);
    connect(btn_import_hst_, &QPushButton::clicked, this, &LauncherWindow::on_import_hst);
    connect(btn_export_hst_, &QPushButton::clicked, this, &LauncherWindow::on_export_hst);
    connect(btn_backup_json_, &QPushButton::clicked, this, &LauncherWindow::on_backup_scores_json);
    connect(btn_restore_json_, &QPushButton::clicked, this, &LauncherWindow::on_restore_scores_json);
    connect(btn_clear_scores_, &QPushButton::clicked, this, &LauncherWindow::on_clear_scores);

    return page;
}

QWidget* LauncherWindow::build_diagnostics_tab() {
    auto* page = new QWidget(this);
    auto* lay = new QVBoxLayout(page);
    lay->setContentsMargins(12, 12, 12, 12);
    lay->setSpacing(10);

    grp_storage_ = new QGroupBox(page);
    auto* st_form = new QFormLayout(grp_storage_);
    st_form->setSpacing(6);

    lbl_diag_mode_k_ = new QLabel(grp_storage_);
    lbl_diag_mode_v_ = new QLabel(grp_storage_);
    lbl_diag_mode_v_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    st_form->addRow(lbl_diag_mode_k_, lbl_diag_mode_v_);

    lbl_diag_data_k_ = new QLabel(grp_storage_);
    lbl_diag_data_v_ = new QLabel(grp_storage_);
    lbl_diag_data_v_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    lbl_diag_data_v_->setWordWrap(true);
    st_form->addRow(lbl_diag_data_k_, lbl_diag_data_v_);

    lbl_diag_exe_k_ = new QLabel(grp_storage_);
    lbl_diag_exe_v_ = new QLabel(grp_storage_);
    lbl_diag_exe_v_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    lbl_diag_exe_v_->setWordWrap(true);
    st_form->addRow(lbl_diag_exe_k_, lbl_diag_exe_v_);
    lay->addWidget(grp_storage_);

    grp_folders_ = new QGroupBox(page);
    auto* fld_row = new QHBoxLayout(grp_folders_);
    btn_open_data_ = new QPushButton(grp_folders_);
    btn_open_assets_ = new QPushButton(grp_folders_);
    btn_open_mods_ = new QPushButton(grp_folders_);
    btn_open_ost_ = new QPushButton(grp_folders_);
    btn_open_locales_ = new QPushButton(grp_folders_);
    fld_row->addWidget(btn_open_data_);
    fld_row->addWidget(btn_open_assets_);
    fld_row->addWidget(btn_open_mods_);
    fld_row->addWidget(btn_open_ost_);
    fld_row->addWidget(btn_open_locales_);
    lay->addWidget(grp_folders_);

    grp_health_ = new QGroupBox(page);
    auto* hl_lay = new QVBoxLayout(grp_health_);
    txt_health_ = new QPlainTextEdit(grp_health_);
    txt_health_->setReadOnly(true);
    QFont mono("monospace");
    mono.setStyleHint(QFont::Monospace);
    txt_health_->setFont(mono);
    hl_lay->addWidget(txt_health_, 1);

    auto* hl_btn_row = new QHBoxLayout();
    hl_btn_row->addStretch(1);
    btn_refresh_diag_ = new QPushButton(grp_health_);
    hl_btn_row->addWidget(btn_refresh_diag_);
    hl_lay->addLayout(hl_btn_row);
    lay->addWidget(grp_health_, 1);

    connect(btn_open_data_, &QPushButton::clicked, this, [this]() { open_folder(paths_.data_dir, true); });
    connect(btn_open_assets_, &QPushButton::clicked, this, [this]() { open_folder(paths_.assets_dir(), true); });
    connect(btn_open_mods_, &QPushButton::clicked, this, [this]() { open_folder(paths_.mods_dir(), true); });
    connect(btn_open_ost_, &QPushButton::clicked, this, [this]() { open_folder(paths_.assets_dir() / "ost", true); });
    connect(btn_open_locales_, &QPushButton::clicked, this, [this]() { open_folder(paths_.locales_dir(), true); });
    connect(btn_refresh_diag_, &QPushButton::clicked, this, &LauncherWindow::refresh_all);

    return page;
}

void LauncherWindow::set_language(const std::string& lang_code) {
    if (i18n_.set_language(lang_code)) {
        config_.language = i18n_.language();
        retranslate_ui();
        refresh_frontends();
        refresh_packs();
        refresh_settings_from_config();
        refresh_scores_table();
        refresh_diagnostics();
    }
}

void LauncherWindow::select_tab(int index) {
    if (tabs_ && index >= 0 && index < tabs_->count()) {
        tabs_->setCurrentIndex(index);
    }
}

int LauncherWindow::tab_count() const {
    return tabs_ ? tabs_->count() : 0;
}

void LauncherWindow::retranslate_ui() {
    syncing_ui_ = true;

    setWindowTitle(qtr("launcher.title"));

    tabs_->setTabText(0, qtr_mnem_safe("launcher.tab.play"));
    tabs_->setTabText(1, qtr_mnem_safe("launcher.tab.assets"));
    tabs_->setTabText(2, qtr_mnem_safe("launcher.tab.settings"));
    tabs_->setTabText(3, qtr_mnem_safe("launcher.tab.scores"));
    tabs_->setTabText(4, qtr_mnem_safe("launcher.tab.diagnostics"));

    // Tab 1: Play
    lbl_frontend_->setText(qtr("launcher.play.frontend") + ":");
    lbl_pack_->setText(qtr("launcher.play.pack") + ":");
    btn_browse_pack_->setText(qtr_mnem_safe("launcher.play.pack_browse"));
    btn_refresh_play_->setText(qtr_mnem_safe("launcher.play.refresh"));
    grp_preview_title_->setTitle(qtr_mnem_safe("launcher.play.preview_title"));
    grp_preview_tiles_->setTitle(qtr_mnem_safe("launcher.play.preview_tiles"));
    chk_close_on_launch_->setText(qtr_mnem_safe("launcher.play.close_on_launch"));
    btn_launch_->setText(qtr_mnem_safe("launcher.play.btn_launch"));

    // Tab 2: Assets & Mods
    grp_extract_->setTitle(qtr_mnem_safe("launcher.assets.extract_group"));
    edit_exe_path_->setPlaceholderText(qtr("launcher.assets.exe_placeholder"));
    btn_browse_exe_->setText(qtr_mnem_safe("launcher.assets.btn_browse_exe"));
    if (edit_exe_path_->text().trimmed().isEmpty()) {
        lbl_hash_status_->setText(qtr("launcher.assets.hash_idle"));
    }
    chk_skip_hash_->setText(qtr_mnem_safe("launcher.assets.skip_hash"));
    btn_extract_->setText(qtr_mnem_safe("launcher.assets.btn_extract"));
    btn_import_ost_->setText(qtr_mnem_safe("launcher.assets.btn_import_ost"));

    grp_mods_->setTitle(qtr_mnem_safe("launcher.assets.mod_group"));
    lbl_mod_select_->setText(qtr("launcher.assets.mod_select") + ":");
    btn_new_mod_->setText(qtr_mnem_safe("launcher.assets.btn_new_mod"));
    btn_clone_mod_->setText(qtr_mnem_safe("launcher.assets.btn_clone_mod"));
    btn_delete_mod_->setText(qtr_mnem_safe("launcher.assets.btn_delete_mod"));
    btn_check_pack_->setText(qtr_mnem_safe("launcher.assets.btn_check_pack"));
    lbl_mod_tile_->setText(qtr("launcher.assets.mod_tile_label") + ":");
    btn_replace_tile_->setText(qtr_mnem_safe("launcher.assets.btn_replace_tile"));
    lbl_mod_level_->setText(qtr("launcher.assets.mod_level_label") + ":");
    btn_replace_level_->setText(qtr_mnem_safe("launcher.assets.btn_replace_level"));
    btn_replace_title_->setText(qtr_mnem_safe("launcher.assets.btn_replace_title"));
    lbl_mod_audio_->setText(qtr("launcher.assets.mod_audio_label") + ":");
    btn_replace_audio_->setText(qtr_mnem_safe("launcher.assets.btn_replace_audio"));
    lbl_log_->setText(qtr("launcher.assets.log_label") + ":");

    int prev_lvl = std::max(0, cmb_level_idx_->currentIndex());
    cmb_level_idx_->clear();
    for (int i = 0; i < 7; ++i) {
        cmb_level_idx_->addItem(QString::fromStdString(ui::common::format_endless_stage_label(i, i18n_)));
    }
    cmb_level_idx_->setCurrentIndex(std::clamp(prev_lvl, 0, 6));

    // Tab 3: Settings
    grp_display_->setTitle(qtr_mnem_safe("launcher.settings.display_group"));
    lbl_set_lang_->setText(qtr("launcher.settings.language") + ":");
    lbl_set_scale_->setText(qtr("launcher.settings.scale_mode") + ":");
    lbl_set_filter_->setText(qtr("launcher.settings.scale_filter") + ":");
    chk_set_fullscreen_->setText(qtr_mnem_safe("launcher.settings.fullscreen"));
    lbl_set_player_->setText(qtr("launcher.settings.player_name") + ":");

    grp_audio_->setTitle(qtr_mnem_safe("launcher.settings.audio_group"));
    chk_set_music_->setText(qtr_mnem_safe("launcher.settings.music_on"));
    chk_set_sound_->setText(qtr_mnem_safe("launcher.settings.sound_on"));
    lbl_set_backend_->setText(qtr("launcher.settings.midi_backend") + ":");
    lbl_set_sf2_->setText(qtr("launcher.settings.soundfont") + ":");
    edit_set_sf2_->setPlaceholderText(qtr("launcher.settings.soundfont_default"));
    btn_browse_sf2_->setText(qtr_mnem_safe("launcher.settings.btn_browse_sf2"));
    btn_clear_sf2_->setText(qtr_mnem_safe("launcher.settings.btn_clear_sf2"));
    lbl_set_tracks_->setText(qtr("launcher.settings.tracks") + ":");
    chk_set_tracks_[0]->setText(qtr_mnem_safe("menu.settings.track_101"));
    chk_set_tracks_[1]->setText(qtr_mnem_safe("menu.settings.track_102"));
    chk_set_tracks_[2]->setText(qtr_mnem_safe("menu.settings.track_103"));
    btn_save_settings_->setText(qtr_mnem_safe("launcher.settings.btn_save"));
    btn_reset_settings_->setText(qtr_mnem_safe("launcher.settings.btn_reset"));

    // Repopulate settings combos with localized labels
    lang_codes_ = ui::common::ordered_languages(i18n_);
    cmb_set_lang_->clear();
    for (const auto& code : lang_codes_) {
        cmb_set_lang_->addItem(QString::fromStdString(ui::common::language_display_name(i18n_, code)));
    }

    cmb_set_scale_->clear();
    cmb_set_scale_->addItem(qtr("menu.settings.scale_1x"));
    cmb_set_scale_->addItem(qtr("menu.settings.scale_1_5x"));
    cmb_set_scale_->addItem(qtr("menu.settings.scale_2x"));
    cmb_set_scale_->addItem(qtr("menu.settings.scale_3x"));
    cmb_set_scale_->addItem(qtr("menu.settings.scale_free"));

    cmb_set_filter_->clear();
    cmb_set_filter_->addItem(qtr("menu.settings.filter_nearest"));
    cmb_set_filter_->addItem(qtr("menu.settings.filter_bilinear"));

    cmb_set_backend_->clear();
    cmb_set_backend_->addItem(qtr("menu.settings.midi_embedded"), static_cast<int>(core::MidiBackend::EmbeddedSoundFont));
    cmb_set_backend_->addItem(qtr("menu.settings.midi_native"), static_cast<int>(core::MidiBackend::NativeOS));
    cmb_set_backend_->addItem(qtr("menu.settings.midi_ost"), static_cast<int>(core::MidiBackend::OstAudio));

    // Tab 4: High Scores
    lbl_scores_hint_->setText(qtr("launcher.scores.hint"));
    tbl_scores_->setHorizontalHeaderLabels({
        qtr("table.col.rank"),
        qtr("table.col.name"),
        qtr("table.col.score")
    });
    btn_save_scores_->setText(qtr("launcher.scores.btn_save"));
    btn_import_hst_->setText(qtr("launcher.scores.btn_import_hst"));
    btn_export_hst_->setText(qtr("launcher.scores.btn_export_hst"));
    btn_backup_json_->setText(qtr("launcher.scores.btn_backup_json"));
    btn_restore_json_->setText(qtr("launcher.scores.btn_restore_json"));
    btn_clear_scores_->setText(qtr("launcher.scores.btn_clear"));

    // Tab 5: Diagnostics
    grp_storage_->setTitle(qtr_mnem_safe("launcher.diag.storage_group"));
    lbl_diag_mode_k_->setText(qtr("launcher.diag.mode") + ":");
    lbl_diag_data_k_->setText(qtr("launcher.diag.data_dir") + ":");
    lbl_diag_exe_k_->setText(qtr("launcher.diag.exe_dir") + ":");
    grp_folders_->setTitle(qtr_mnem_safe("launcher.diag.folders_group"));
    btn_open_data_->setText(qtr_mnem_safe("launcher.diag.open_data"));
    btn_open_assets_->setText(qtr_mnem_safe("launcher.diag.open_assets"));
    btn_open_mods_->setText(qtr_mnem_safe("launcher.diag.open_mods"));
    btn_open_ost_->setText(qtr_mnem_safe("launcher.diag.open_ost"));
    btn_open_locales_->setText(qtr_mnem_safe("launcher.diag.open_locales"));
    grp_health_->setTitle(qtr_mnem_safe("launcher.diag.health_group"));
    btn_refresh_diag_->setText(qtr_mnem_safe("launcher.diag.btn_refresh"));

    syncing_ui_ = false;
}

void LauncherWindow::refresh_all() {
    refresh_frontends();
    refresh_packs();
    refresh_play_preview();
    refresh_settings_from_config();
    refresh_scores_table();
    refresh_diagnostics();
}

void LauncherWindow::refresh_frontends() {
    syncing_ui_ = true;
    std::string prev_id;
    int prev_idx = cmb_frontend_->currentIndex();
    if (prev_idx >= 0 && prev_idx < static_cast<int>(frontends_.size())) {
        prev_id = frontends_[static_cast<size_t>(prev_idx)].id;
    }

    frontends_.clear();
    frontends_.push_back({"llk2-qt", "Qt 6 (llk2-qt)", paths_.find_binary("llk2-qt")});
    frontends_.push_back({"llk2-imgui", "Dear ImGui (llk2-imgui)", paths_.find_binary("llk2-imgui")});
    frontends_.push_back({"llk2-bui", "Blender UI (llk2-bui)", paths_.find_binary("llk2-bui")});

    cmb_frontend_->clear();
    int restore_idx = -1;
    int first_avail = -1;
    for (int i = 0; i < static_cast<int>(frontends_.size()); ++i) {
        const auto& fe = frontends_[static_cast<size_t>(i)];
        if (!fe.binary_path.empty()) {
            cmb_frontend_->addItem(QString::fromStdString(fe.display_name));
            if (first_avail < 0) first_avail = i;
        } else {
            std::string lbl = i18n_.format("launcher.play.frontend_missing", {{"name", fe.display_name}});
            cmb_frontend_->addItem(QString::fromStdString(lbl));
        }
        if (fe.id == prev_id) restore_idx = i;
    }
    if (restore_idx >= 0) cmb_frontend_->setCurrentIndex(restore_idx);
    else if (first_avail >= 0) cmb_frontend_->setCurrentIndex(first_avail);

    syncing_ui_ = false;
}

void LauncherWindow::refresh_packs() {
    syncing_ui_ = true;

    std::filesystem::path prev_play_dir = selected_pack_dir();
    std::filesystem::path prev_mod_dir = current_mod_target_dir();

    packs_.clear();
    packs_.push_back({ui::common::strip_mnemonic(i18n_.tr("launcher.play.pack_default")),
                      paths_.assets_dir(), true, false});

    auto mods = list_mod_packs(paths_.mods_dir());
    for (const auto& name : mods) {
        packs_.push_back({"mods/" + name, paths_.mods_dir() / name, false, false});
    }

    if (!custom_pack_dir_.empty()) {
        packs_.push_back({custom_pack_dir_.string(), custom_pack_dir_, false, true});
    }

    cmb_pack_->clear();
    cmb_mod_target_->clear();
    int play_idx = 0;
    int mod_idx = 0;
    for (int i = 0; i < static_cast<int>(packs_.size()); ++i) {
        cmb_pack_->addItem(QString::fromStdString(packs_[static_cast<size_t>(i)].label));
        if (!packs_[static_cast<size_t>(i)].is_custom) {
            cmb_mod_target_->addItem(QString::fromStdString(packs_[static_cast<size_t>(i)].label),
                                     QString::fromStdString(packs_[static_cast<size_t>(i)].dir.string()));
            if (!prev_mod_dir.empty() && packs_[static_cast<size_t>(i)].dir == prev_mod_dir) {
                mod_idx = cmb_mod_target_->count() - 1;
            }
        }
        if (!prev_play_dir.empty() && packs_[static_cast<size_t>(i)].dir == prev_play_dir) {
            play_idx = i;
        }
    }

    if (cmb_pack_->count() > 0) cmb_pack_->setCurrentIndex(play_idx);
    if (cmb_mod_target_->count() > 0) cmb_mod_target_->setCurrentIndex(mod_idx);

    syncing_ui_ = false;
}

std::filesystem::path LauncherWindow::selected_pack_dir() const {
    int idx = cmb_pack_ ? cmb_pack_->currentIndex() : -1;
    if (idx >= 0 && idx < static_cast<int>(packs_.size())) {
        return packs_[static_cast<size_t>(idx)].dir;
    }
    return paths_.assets_dir();
}

std::filesystem::path LauncherWindow::selected_frontend_binary() const {
    int idx = cmb_frontend_ ? cmb_frontend_->currentIndex() : -1;
    if (idx >= 0 && idx < static_cast<int>(frontends_.size())) {
        return frontends_[static_cast<size_t>(idx)].binary_path;
    }
    return {};
}

void LauncherWindow::refresh_play_preview() {
    auto pack_dir = selected_pack_dir();
    auto report = check_pack_integrity(pack_dir);

    media::AssetManager preview_assets;
    std::string err;
    bool loaded = preview_assets.load_from_dir(pack_dir, &err);

    if (loaded && preview_assets.pic_title().valid()) {
        QImage title_img = surface_to_qimage(preview_assets.pic_title());
        QPixmap pix = QPixmap::fromImage(title_img).scaled(340, 255, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        lbl_title_preview_->setPixmap(pix);
    } else {
        lbl_title_preview_->setPixmap(QPixmap());
        lbl_title_preview_->setText(qtr("launcher.play.no_assets"));
    }

    if (loaded && preview_assets.pic_mjdata().valid()) {
        QImage tiles_img = render_composited_tile_preview(preview_assets, 0);
        if (!tiles_img.isNull()) {
            QPixmap pix = QPixmap::fromImage(tiles_img).scaled(360, 280, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            lbl_tiles_preview_->setPixmap(pix);
        } else {
            lbl_tiles_preview_->setPixmap(QPixmap());
            lbl_tiles_preview_->setText(qtr("launcher.play.no_assets"));
        }
    } else {
        lbl_tiles_preview_->setPixmap(QPixmap());
        lbl_tiles_preview_->setText(qtr("launcher.play.no_assets"));
    }

    if (report.valid) {
        std::string msg = i18n_.format("launcher.assets.integrity_ok", {
            {"count", std::to_string(report.required_total)},
            {"ost", std::to_string(report.ost_count)}
        });
        lbl_pack_status_->setStyleSheet("color: #2e8b57; font-weight: 600;");
        lbl_pack_status_->setText(QString::fromStdString(msg));
        statusBar()->showMessage(QString::fromStdString(
            i18n_.format("launcher.play.status_ready", {{"pack", cmb_pack_->currentText().toStdString()}})));
    } else {
        std::string msg = i18n_.format("launcher.assets.integrity_fail", {
            {"missing", std::to_string(report.missing.size())}
        });
        lbl_pack_status_->setStyleSheet("color: #c93b3b; font-weight: 600;");
        lbl_pack_status_->setText(QString::fromStdString(msg));
    }

    bool has_fe = !selected_frontend_binary().empty();
    btn_launch_->setEnabled(has_fe);
}

void LauncherWindow::on_browse_custom_pack() {
    QString start = QString::fromStdString(paths_.data_dir.string());
    QString picked = QFileDialog::getExistingDirectory(this, qtr("launcher.play.pack_custom"), start);
    if (picked.isEmpty()) return;
    custom_pack_dir_ = picked.toStdString();
    refresh_packs();
    cmb_pack_->setCurrentIndex(cmb_pack_->count() - 1);
    refresh_play_preview();
}

void LauncherWindow::on_launch_game() {
    // Save any pending settings changes first so the game launches with them
    on_save_settings(false);

    auto bin = selected_frontend_binary();
    if (bin.empty()) return;

    auto pack_dir = selected_pack_dir();
    auto report = check_pack_integrity(pack_dir);
    if (!report.valid) {
        QMessageBox::warning(this, qtr("launcher.title"), qtr("launcher.play.assets_missing_warn"));
        return;
    }

    QStringList args;
    args << "--data-dir" << QString::fromStdString(paths_.data_dir.string());
    args << "--assets" << QString::fromStdString(pack_dir.string());

    bool ok = QProcess::startDetached(QString::fromStdString(bin.string()),
                                      args,
                                      QString::fromStdString(paths_.data_dir.string()));
    if (!ok) {
        QMessageBox::critical(this, qtr("launcher.title"),
                              QString::fromStdString(i18n_.format("launcher.play.launch_failed", {
                                  {"exe", bin.string()}
                              })));
        return;
    }

    statusBar()->showMessage(QString::fromStdString(i18n_.format("launcher.play.status_launched", {
        {"frontend", cmb_frontend_->currentText().toStdString()},
        {"pack", cmb_pack_->currentText().toStdString()}
    })));

    if (chk_close_on_launch_->isChecked()) {
        close();
    }
}

void LauncherWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event && event->mimeData() && event->mimeData()->hasUrls()) {
        const auto urls = event->mimeData()->urls();
        if (!urls.isEmpty() && urls.first().isLocalFile()) {
            event->acceptProposedAction();
            return;
        }
    }
    QMainWindow::dragEnterEvent(event);
}

void LauncherWindow::dropEvent(QDropEvent* event) {
    if (event && event->mimeData() && event->mimeData()->hasUrls()) {
        const auto urls = event->mimeData()->urls();
        if (!urls.isEmpty() && urls.first().isLocalFile()) {
            std::filesystem::path dropped = urls.first().toLocalFile().toStdString();
            tabs_->setCurrentIndex(1); // Switch to Assets & Mods tab
            edit_exe_path_->setText(QString::fromStdString(dropped.string()));
            on_exe_path_changed(dropped);
            event->acceptProposedAction();
            return;
        }
    }
    QMainWindow::dropEvent(event);
}

void LauncherWindow::on_exe_path_changed(const std::filesystem::path& path) {
    if (path.empty()) {
        lbl_hash_status_->setStyleSheet("");
        lbl_hash_status_->setText(qtr("launcher.assets.hash_idle"));
        return;
    }
    auto info = inspect_source_archive(path);
    if (!info.ok) {
        lbl_hash_status_->setStyleSheet("color: #c93b3b;");
        lbl_hash_status_->setText(QString::fromStdString("File not readable: " + path.string()));
        return;
    }
    if (info.is_zip) {
        lbl_hash_status_->setStyleSheet("color: #2c6fbb;");
        lbl_hash_status_->setText(qtr("launcher.assets.hash_zip"));
    } else if (info.is_known) {
        lbl_hash_status_->setStyleSheet("color: #2e8b57; font-weight: 600;");
        lbl_hash_status_->setText(QString::fromStdString(i18n_.format("launcher.assets.hash_known", {
            {"version", info.version_label},
            {"hash", info.sha256_hex.substr(0, 16) + "..."}
        })));
    } else {
        lbl_hash_status_->setStyleSheet("color: #b86e00;");
        lbl_hash_status_->setText(QString::fromStdString(i18n_.format("launcher.assets.hash_unknown", {
            {"hash", info.sha256_hex.substr(0, 16) + "..."}
        })));
    }
}

void LauncherWindow::on_browse_exe() {
    QString picked = QFileDialog::getOpenFileName(
        this,
        qtr("launcher.assets.btn_browse_exe"),
        QString::fromStdString(paths_.data_dir.string()),
        "Original Game EXE / ZIP (*.exe *.EXE *.zip *.ZIP);;All Files (*)");
    if (picked.isEmpty()) return;
    edit_exe_path_->setText(picked);
    on_exe_path_changed(picked.toStdString());
}

std::string LauncherWindow::find_python_executable() const {
    for (const char* cand : {"python3", "python", "py"}) {
        QString found = QStandardPaths::findExecutable(QString::fromLatin1(cand));
        if (!found.isEmpty()) {
            return found.toStdString();
        }
    }
    return "python3";
}

void LauncherWindow::on_extract_exe() {
    std::filesystem::path src = edit_exe_path_->text().trimmed().toStdString();
    if (src.empty() || !std::filesystem::is_regular_file(src)) {
        append_log("[error] Please select a valid D4s.exe, lianliankan2.ExE, or .zip archive first.");
        return;
    }

    auto script = paths_.find_resource(std::filesystem::path("tools") / "extract_assets.py");
    if (script.empty()) {
        append_log("[error] Could not locate tools/extract_assets.py in resource search paths.");
        return;
    }

    if (extract_proc_ && extract_proc_->state() != QProcess::NotRunning) {
        return;
    }

    paths_.ensure_data_dir();
    auto out_assets = paths_.assets_dir();

    extract_proc_ = new QProcess(this);
    extract_proc_->setProcessChannelMode(QProcess::MergedChannels);

    QStringList args;
    args << QString::fromStdString(script.string())
         << QString::fromStdString(src.string())
         << "-o" << QString::fromStdString(out_assets.string());
    if (chk_skip_hash_->isChecked()) {
        args << "--skip-hash";
    }

    btn_extract_->setEnabled(false);
    append_log("[extract] Running: " + find_python_executable() + " " + script.filename().string() +
               " \"" + src.filename().string() + "\" -o \"" + out_assets.string() + "\"");

    connect(extract_proc_, &QProcess::readyReadStandardOutput, this, [this]() {
        if (!extract_proc_) return;
        QString chunk = QString::fromUtf8(extract_proc_->readAllStandardOutput());
        for (const QString& line : chunk.split('\n', Qt::SkipEmptyParts)) {
            append_log(line.trimmed().toStdString());
        }
    });

    connect(extract_proc_, &QProcess::finished, this, [this](int exit_code, QProcess::ExitStatus status) {
        btn_extract_->setEnabled(true);
        if (status == QProcess::NormalExit && exit_code == 0) {
            append_log("[extract] Extraction complete!");
            refresh_all();
        } else {
            append_log("[error] Extraction failed (exit code " + std::to_string(exit_code) + ").");
        }
        if (extract_proc_) {
            extract_proc_->deleteLater();
            extract_proc_ = nullptr;
        }
    });

    extract_proc_->start(QString::fromStdString(find_python_executable()), args);
}

void LauncherWindow::on_import_ost() {
    auto target_dir = current_mod_target_dir();
    QString picked_dir = QFileDialog::getExistingDirectory(
        this,
        qtr("launcher.assets.btn_import_ost"),
        QString::fromStdString(paths_.data_dir.string()));
    if (picked_dir.isEmpty()) return;

    std::string err;
    int copied = import_ost_from_directory(target_dir, picked_dir.toStdString(), &err);
    if (copied > 0) {
        append_log("[ost] Imported " + std::to_string(copied) + " OST track(s) into " +
                   (target_dir / "ost").string());
        refresh_play_preview();
        refresh_diagnostics();
    } else {
        append_log("[ost] " + err);
    }
}

std::filesystem::path LauncherWindow::current_mod_target_dir() const {
    if (!cmb_mod_target_ || cmb_mod_target_->currentIndex() < 0) {
        return paths_.assets_dir();
    }
    return cmb_mod_target_->currentData().toString().toStdString();
}

std::string LauncherWindow::current_mod_target_name() const {
    if (!cmb_mod_target_ || cmb_mod_target_->currentIndex() < 0) {
        return "assets";
    }
    return cmb_mod_target_->currentText().toStdString();
}

void LauncherWindow::on_new_mod_pack() {
    bool ok = false;
    QString raw = QInputDialog::getText(this, qtr("launcher.assets.btn_new_mod"),
                                        qtr("launcher.assets.prompt_mod_name"),
                                        QLineEdit::Normal, "my_mod", &ok);
    if (!ok) return;
    std::string name = sanitize_mod_name(raw.toStdString());
    if (name.empty()) return;

    auto dst = paths_.mods_dir() / name;
    std::string err;
    if (!clone_asset_pack(paths_.assets_dir(), dst, &err)) {
        append_log("[mod] Failed to create mod pack: " + err);
        QMessageBox::warning(this, qtr("launcher.title"), QString::fromStdString(err));
        return;
    }
    append_log("[mod] Created new mod pack: mods/" + name);
    refresh_packs();
    for (int i = 0; i < cmb_mod_target_->count(); ++i) {
        if (cmb_mod_target_->itemData(i).toString().toStdString() == dst.string()) {
            cmb_mod_target_->setCurrentIndex(i);
            break;
        }
    }
    refresh_diagnostics();
}

void LauncherWindow::on_clone_mod_pack() {
    auto src = current_mod_target_dir();
    bool ok = false;
    QString raw = QInputDialog::getText(this, qtr("launcher.assets.btn_clone_mod"),
                                        qtr("launcher.assets.prompt_mod_name"),
                                        QLineEdit::Normal, "mod_copy", &ok);
    if (!ok) return;
    std::string name = sanitize_mod_name(raw.toStdString());
    if (name.empty()) return;

    auto dst = paths_.mods_dir() / name;
    std::string err;
    if (!clone_asset_pack(src, dst, &err)) {
        append_log("[mod] Duplicate failed: " + err);
        QMessageBox::warning(this, qtr("launcher.title"), QString::fromStdString(err));
        return;
    }
    append_log("[mod] Duplicated '" + current_mod_target_name() + "' -> mods/" + name);
    refresh_packs();
    refresh_diagnostics();
}

void LauncherWindow::on_delete_mod_pack() {
    auto target = current_mod_target_dir();
    if (target == paths_.assets_dir()) {
        QMessageBox::information(this, qtr("launcher.title"), qtr("launcher.assets.cannot_delete_default"));
        return;
    }
    std::string name = target.filename().string();
    auto reply = QMessageBox::question(
        this, qtr("launcher.assets.btn_delete_mod"),
        QString::fromStdString(i18n_.format("launcher.assets.confirm_delete_mod", {{"name", name}})));
    if (reply != QMessageBox::Yes) return;

    std::error_code ec;
    std::filesystem::remove_all(target, ec);
    if (ec) {
        append_log("[mod] Delete failed: " + ec.message());
        return;
    }
    append_log("[mod] Deleted mod pack: mods/" + name);
    refresh_all();
}

void LauncherWindow::on_check_pack_integrity() {
    auto target = current_mod_target_dir();
    auto rep = check_pack_integrity(target);
    if (rep.valid) {
        std::string msg = i18n_.format("launcher.assets.integrity_ok", {
            {"count", std::to_string(rep.required_total)},
            {"ost", std::to_string(rep.ost_count)}
        });
        append_log("[check] " + current_mod_target_name() + ": " + msg);
    } else {
        std::string msg = i18n_.format("launcher.assets.integrity_fail", {
            {"missing", std::to_string(rep.missing.size())}
        });
        append_log("[check] " + current_mod_target_name() + ": " + msg);
        for (const auto& m : rep.missing) {
            append_log("   - " + m);
        }
    }
}

void LauncherWindow::on_replace_tile() {
    auto target = current_mod_target_dir();
    int tid = spin_tile_id_->value();
    QString picked = QFileDialog::getOpenFileName(
        this, qtr("launcher.assets.btn_replace_tile"),
        QString::fromStdString(paths_.data_dir.string()),
        "Images (*.png *.bmp *.jpg *.jpeg *.gif *.webp);;All Files (*)");
    if (picked.isEmpty()) return;

    std::string err;
    if (replace_tile_sprite(target, tid, picked.toStdString(), &err)) {
        append_log("[mod] Replaced Tile #" + std::to_string(tid) + " in " + current_mod_target_name());
        refresh_play_preview();
    } else {
        append_log("[mod] Tile replace failed: " + err);
        QMessageBox::warning(this, qtr("launcher.title"), QString::fromStdString(err));
    }
}

void LauncherWindow::on_replace_level_bg() {
    auto target = current_mod_target_dir();
    int lvl = std::clamp(cmb_level_idx_->currentIndex(), 0, 6);
    QString picked = QFileDialog::getOpenFileName(
        this, qtr("launcher.assets.btn_replace_level"),
        QString::fromStdString(paths_.data_dir.string()),
        "Images (*.png *.bmp *.jpg *.jpeg *.gif *.webp);;All Files (*)");
    if (picked.isEmpty()) return;

    std::string err;
    if (replace_level_background(target, lvl, picked.toStdString(), &err)) {
        append_log("[mod] Replaced Stage " + std::to_string(lvl + 1) + " background (600x450) in " +
                   current_mod_target_name());
    } else {
        append_log("[mod] Background replace failed: " + err);
        QMessageBox::warning(this, qtr("launcher.title"), QString::fromStdString(err));
    }
}

void LauncherWindow::on_replace_title_img() {
    auto target = current_mod_target_dir();
    QString picked = QFileDialog::getOpenFileName(
        this, qtr("launcher.assets.btn_replace_title"),
        QString::fromStdString(paths_.data_dir.string()),
        "Images (*.png *.bmp *.jpg *.jpeg *.gif *.webp);;All Files (*)");
    if (picked.isEmpty()) return;

    std::string err;
    if (replace_title_image(target, picked.toStdString(), &err)) {
        append_log("[mod] Replaced Title screen (500x375) in " + current_mod_target_name());
        refresh_play_preview();
    } else {
        append_log("[mod] Title replace failed: " + err);
        QMessageBox::warning(this, qtr("launcher.title"), QString::fromStdString(err));
    }
}

void LauncherWindow::on_replace_audio() {
    auto target = current_mod_target_dir();
    int slot = std::clamp(cmb_audio_slot_->currentIndex(), 0, 8);
    const char* rel = kAudioSlotRelPaths[slot];
    bool is_midi = (slot >= 6);
    QString filter = is_midi ? "MIDI Files (*.mid *.MID *.midi);;All Files (*)"
                             : "WAV Audio (*.wav *.WAV);;All Files (*)";
    QString picked = QFileDialog::getOpenFileName(
        this, qtr("launcher.assets.btn_replace_audio"),
        QString::fromStdString(paths_.data_dir.string()), filter);
    if (picked.isEmpty()) return;

    std::string err;
    if (replace_audio_asset(target, rel, picked.toStdString(), &err)) {
        append_log("[mod] Replaced " + std::string(rel) + " in " + current_mod_target_name());
    } else {
        append_log("[mod] Audio replace failed: " + err);
        QMessageBox::warning(this, qtr("launcher.title"), QString::fromStdString(err));
    }
}

void LauncherWindow::refresh_settings_from_config() {
    syncing_ui_ = true;

    for (int i = 0; i < static_cast<int>(lang_codes_.size()); ++i) {
        if (lang_codes_[static_cast<size_t>(i)] == config_.language) {
            cmb_set_lang_->setCurrentIndex(i);
            break;
        }
    }
    cmb_set_scale_->setCurrentIndex(std::clamp(static_cast<int>(config_.scale_mode), 0, 4));
    cmb_set_filter_->setCurrentIndex(config_.scale_filter == core::ScaleFilter::Bilinear ? 1 : 0);
    chk_set_fullscreen_->setChecked(config_.borderless_fullscreen);
    edit_set_player_->setText(QString::fromStdString(config_.player_name));

    chk_set_music_->setChecked(config_.music_on);
    chk_set_sound_->setChecked(config_.sound_on);
    for (int i = 0; i < cmb_set_backend_->count(); ++i) {
        if (cmb_set_backend_->itemData(i).toInt() == static_cast<int>(config_.midi_backend)) {
            cmb_set_backend_->setCurrentIndex(i);
            break;
        }
    }
    edit_set_sf2_->setText(QString::fromStdString(config_.soundfont_path));
    for (int i = 0; i < 3; ++i) {
        chk_set_tracks_[i]->setChecked(config_.midi_tracks_enabled[static_cast<size_t>(i)]);
    }

    syncing_ui_ = false;
}

void LauncherWindow::on_save_settings(bool show_status) {
    int lang_idx = cmb_set_lang_->currentIndex();
    if (lang_idx >= 0 && lang_idx < static_cast<int>(lang_codes_.size())) {
        config_.language = lang_codes_[static_cast<size_t>(lang_idx)];
    }
    config_.scale_mode = static_cast<core::WindowScaleMode>(std::clamp(cmb_set_scale_->currentIndex(), 0, 4));
    config_.scale_filter = (cmb_set_filter_->currentIndex() == 1) ? core::ScaleFilter::Bilinear
                                                                 : core::ScaleFilter::Nearest;
    config_.borderless_fullscreen = chk_set_fullscreen_->isChecked();
    std::string pname = edit_set_player_->text().trimmed().toStdString();
    config_.player_name = pname.empty() ? "Player" : pname;

    config_.music_on = chk_set_music_->isChecked();
    config_.sound_on = chk_set_sound_->isChecked();
    config_.midi_backend = static_cast<core::MidiBackend>(cmb_set_backend_->currentData().toInt());
    config_.soundfont_path = edit_set_sf2_->text().trimmed().toStdString();
    for (int i = 0; i < 3; ++i) {
        config_.midi_tracks_enabled[static_cast<size_t>(i)] = chk_set_tracks_[i]->isChecked();
    }

    paths_.ensure_data_dir();
    config_.save(paths_.config_file());
    if (show_status) {
        statusBar()->showMessage(qtr("launcher.settings.saved_status"), 4000);
    }
    refresh_diagnostics();
}

void LauncherWindow::on_reset_settings() {
    std::string keep_lang = config_.language;
    config_ = core::Config{};
    config_.language = keep_lang;
    refresh_settings_from_config();
    on_save_settings(true);
}

void LauncherWindow::on_browse_soundfont() {
    QString picked = QFileDialog::getOpenFileName(
        this, qtr("menu.settings.select_soundfont"),
        QString::fromStdString(paths_.data_dir.string()),
        "SoundFont (*.sf2 *.SF2 *.sf3 *.SF3);;All Files (*)");
    if (picked.isEmpty()) return;
    edit_set_sf2_->setText(picked);
    on_save_settings(true);
}

void LauncherWindow::on_clear_soundfont() {
    edit_set_sf2_->clear();
    on_save_settings(true);
}

void LauncherWindow::refresh_scores_table() {
    syncing_ui_ = true;
    for (int i = 0; i < core::HighScoreTable::kCount; ++i) {
        const auto& e = scores_.entry(i);

        auto* rank_item = new QTableWidgetItem(QString::number(i + 1));
        rank_item->setFlags(rank_item->flags() & ~Qt::ItemIsEditable);
        rank_item->setTextAlignment(Qt::AlignCenter);

        auto* name_item = new QTableWidgetItem(QString::fromStdString(e.name));
        name_item->setFlags(name_item->flags() | Qt::ItemIsEditable);

        int sc = static_cast<int>(std::lround(e.score));
        auto* score_item = new QTableWidgetItem(sc > 0 ? QString::number(sc) : "0");
        score_item->setFlags(score_item->flags() & ~Qt::ItemIsEditable);
        score_item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        tbl_scores_->setItem(i, 0, rank_item);
        tbl_scores_->setItem(i, 1, name_item);
        tbl_scores_->setItem(i, 2, score_item);
    }
    syncing_ui_ = false;
}

void LauncherWindow::on_save_scores() {
    for (int i = 0; i < core::HighScoreTable::kCount; ++i) {
        if (auto* item = tbl_scores_->item(i, 1)) {
            scores_.update_rank_name(i, item->text().trimmed().toStdString());
        }
    }
    paths_.ensure_data_dir();
    scores_.save_json(paths_.scores_file());
    statusBar()->showMessage(qtr("launcher.scores.saved_status"), 4000);
}

void LauncherWindow::on_import_hst() {
    QString start_dir = QString::fromStdString(paths_.data_dir.string());
#if defined(_WIN32)
    if (std::filesystem::exists("C:/Windows/D4s.hst")) {
        start_dir = "C:/Windows";
    }
#endif
    QString picked = QFileDialog::getOpenFileName(
        this, qtr("launcher.scores.btn_import_hst"), start_dir,
        "Legacy High Scores (d4s.hst *.hst *.HST);;All Files (*)");
    if (picked.isEmpty()) return;
    if (scores_.load_legacy_hst(picked.toStdString())) {
        paths_.ensure_data_dir();
        scores_.save_json(paths_.scores_file());
        refresh_scores_table();
        statusBar()->showMessage(qtr("launcher.scores.saved_status"), 4000);
    }
}

void LauncherWindow::on_export_hst() {
    on_save_scores();
    QString picked = QFileDialog::getSaveFileName(
        this, qtr("launcher.scores.btn_export_hst"),
        QString::fromStdString((paths_.data_dir / "d4s.hst").string()),
        "Legacy High Scores (*.hst);;All Files (*)");
    if (picked.isEmpty()) return;
    scores_.save_legacy_hst(picked.toStdString());
}

void LauncherWindow::on_backup_scores_json() {
    on_save_scores();
    QString picked = QFileDialog::getSaveFileName(
        this, qtr("launcher.scores.btn_backup_json"),
        QString::fromStdString((paths_.data_dir / "scores_backup.json").string()),
        "JSON (*.json);;All Files (*)");
    if (picked.isEmpty()) return;
    scores_.save_json(picked.toStdString());
}

void LauncherWindow::on_restore_scores_json() {
    QString picked = QFileDialog::getOpenFileName(
        this, qtr("launcher.scores.btn_restore_json"),
        QString::fromStdString(paths_.data_dir.string()),
        "JSON (*.json);;All Files (*)");
    if (picked.isEmpty()) return;
    if (scores_.load_json(picked.toStdString())) {
        paths_.ensure_data_dir();
        scores_.save_json(paths_.scores_file());
        refresh_scores_table();
        statusBar()->showMessage(qtr("launcher.scores.saved_status"), 4000);
    }
}

void LauncherWindow::on_clear_scores() {
    auto reply = QMessageBox::question(this, qtr("launcher.scores.btn_clear"),
                                       qtr("launcher.scores.confirm_clear"));
    if (reply != QMessageBox::Yes) return;
    scores_.reset_empty();
    paths_.ensure_data_dir();
    scores_.save_json(paths_.scores_file());
    refresh_scores_table();
    statusBar()->showMessage(qtr("launcher.scores.saved_status"), 4000);
}

void LauncherWindow::refresh_diagnostics() {
    QString mode_str;
    switch (paths_.mode) {
        case core::StorageMode::UserDirectory: mode_str = qtr("launcher.diag.mode_user"); break;
        case core::StorageMode::Portable: mode_str = qtr("launcher.diag.mode_portable"); break;
        case core::StorageMode::EnvOverride: mode_str = qtr("launcher.diag.mode_env"); break;
        case core::StorageMode::CustomOverride: mode_str = qtr("launcher.diag.mode_custom"); break;
    }
    lbl_diag_mode_v_->setText(mode_str);
    lbl_diag_data_v_->setText(QString::fromStdString(paths_.data_dir.string()));
    lbl_diag_exe_v_->setText(QString::fromStdString(paths_.exe_dir.string()));

    std::string report;
    report += "=== Game Frontends ===\n";
    for (const auto& fe : frontends_) {
        report += "  " + fe.id + ": " +
                  (fe.binary_path.empty() ? "NOT FOUND" : fe.binary_path.string()) + "\n";
    }

    report += "\n=== Assets & Mods ===\n";
    auto def_rep = check_pack_integrity(paths_.assets_dir());
    report += "  Default assets/: " + std::to_string(def_rep.present_count) + "/" +
              std::to_string(def_rep.required_total) + " required files (" +
              (def_rep.valid ? "OK" : "INCOMPLETE") + "), OST: " +
              std::to_string(def_rep.ost_count) + "/3\n";
    auto mods = list_mod_packs(paths_.mods_dir());
    report += "  Mod packs (mods/): " + std::to_string(mods.size()) + "\n";

    auto script = paths_.find_resource(std::filesystem::path("tools") / "extract_assets.py");
    report += "  Extractor script: " + (script.empty() ? "NOT FOUND" : script.string()) + "\n";

    report += "\n=== Audio ===\n";
    auto sf2 = media::AudioEngine::discover_soundfont_path(config_.soundfont_path);
    report += "  SoundFont (.sf2): " + (sf2.empty() ? "Built-in minimal fallback" : sf2.string()) + "\n";
    report += "  Music Backend:    " + ui::common::localized_backend_name(config_.midi_backend, i18n_) + "\n";

    report += "\n=== Config & Scores ===\n";
    report += "  config.json: " + paths_.config_file().string() +
              (std::filesystem::exists(paths_.config_file()) ? " (exists)\n" : " (defaults)\n");
    report += "  scores.json: " + paths_.scores_file().string() +
              (std::filesystem::exists(paths_.scores_file()) ? " (exists)\n" : " (empty)\n");

    txt_health_->setPlainText(QString::fromStdString(report));
}

void LauncherWindow::open_folder(const std::filesystem::path& dir, bool create_if_missing) {
    if (create_if_missing) {
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdString(dir.string())));
}

void LauncherWindow::append_log(const std::string& line) {
    if (txt_log_) {
        txt_log_->appendPlainText(QString::fromStdString(line));
    }
}

} // namespace llk2::launcher
