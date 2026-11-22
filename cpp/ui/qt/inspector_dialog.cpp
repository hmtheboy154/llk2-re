#include "ui/qt/inspector_dialog.hpp"
#include "ui/common/ui_helpers.hpp"

#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace llk2::ui::qt {

InspectorDialog::InspectorDialog(const core::I18n& i18n, QWidget* parent)
    : QDialog(parent, Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint),
      i18n_(i18n) {
    setModal(false);
    setMinimumWidth(330);

    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(14, 12, 14, 12);
    main_layout->setSpacing(8);

    auto make_sep = [this]() {
        auto* s = new QFrame(this);
        s->setFrameShape(QFrame::HLine);
        s->setFrameShadow(QFrame::Sunken);
        return s;
    };

    auto* game_form = new QFormLayout();
    game_form->setHorizontalSpacing(14);
    game_form->setVerticalSpacing(5);
    game_form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    key_mode_ = new QLabel(this);
    val_mode_ = new QLabel(this);
    key_stage_ = new QLabel(this);
    val_stage_ = new QLabel(this);
    key_tiles_ = new QLabel(this);
    val_tiles_ = new QLabel(this);
    key_time_ = new QLabel(this);
    val_time_ = new QLabel(this);

    game_form->addRow(key_mode_, val_mode_);
    game_form->addRow(key_stage_, val_stage_);
    game_form->addRow(key_tiles_, val_tiles_);
    game_form->addRow(key_time_, val_time_);
    main_layout->addLayout(game_form);

    val_lives_score_ = new QLabel(this);
    main_layout->addWidget(val_lives_score_);

    main_layout->addWidget(make_sep());

    auto* audio_form = new QFormLayout();
    audio_form->setHorizontalSpacing(14);
    audio_form->setVerticalSpacing(5);
    audio_form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    key_backend_ = new QLabel(this);
    val_backend_ = new QLabel(this);
    key_sf2_ = new QLabel(this);
    val_sf2_ = new QLabel(this);
    key_track_ = new QLabel(this);
    val_track_ = new QLabel(this);

    audio_form->addRow(key_backend_, val_backend_);
    audio_form->addRow(key_sf2_, val_sf2_);
    audio_form->addRow(key_track_, val_track_);
    main_layout->addLayout(audio_form);

    main_layout->addWidget(make_sep());

    auto* hint_form = new QFormLayout();
    hint_form->setHorizontalSpacing(14);
    hint_form->setVerticalSpacing(5);
    key_hint_ = new QLabel(this);
    val_hint_ = new QLabel(this);
    hint_form->addRow(key_hint_, val_hint_);
    main_layout->addLayout(hint_form);

    chk_hint_ = new QCheckBox(this);
    connect(chk_hint_, &QCheckBox::toggled, this, [this](bool checked) {
        if (on_hint_toggled_) on_hint_toggled_(checked);
    });
    main_layout->addWidget(chk_hint_);

    auto* btn_row = new QHBoxLayout();
    btn_row->addStretch(1);
    btn_close_ = new QPushButton(this);
    btn_close_->setMinimumWidth(90);
    connect(btn_close_, &QPushButton::clicked, this, &QDialog::close);
    btn_row->addWidget(btn_close_);
    btn_row->addStretch(1);
    main_layout->addLayout(btn_row);

    retranslate();
}

void InspectorDialog::set_callbacks(std::function<void(bool)> on_hint_toggled,
                                    std::function<void()> on_closed) {
    on_hint_toggled_ = std::move(on_hint_toggled);
    on_closed_ = std::move(on_closed);
}

void InspectorDialog::retranslate() {
    auto qtr = [this](std::string_view key) {
        return QString::fromStdString(i18n_.tr(key));
    };

    setWindowTitle(qtr("inspector.title"));
    key_mode_->setText(qtr("inspector.mode") + ":");
    key_stage_->setText(qtr("inspector.stage") + ":");
    key_tiles_->setText(qtr("inspector.tiles_left") + ":");
    key_time_->setText(qtr("inspector.time_left") + ":");
    key_backend_->setText(qtr("inspector.audio_backend") + ":");
    key_sf2_->setText(qtr("inspector.soundfont") + ":");
    key_track_->setText(qtr("inspector.track") + ":");
    key_hint_->setText(qtr("inspector.next_hint") + ":");
    chk_hint_->setText(qtr("inspector.highlight_hint"));
    btn_close_->setText(qtr("dialog.btn.close"));
}

void InspectorDialog::refresh(const core::GameState& gs,
                              const media::AudioEngine& audio,
                              const core::Config& cfg) {
    auto snap = common::build_inspector_snapshot(gs, audio, cfg, i18n_);
    val_mode_->setText(QString::fromStdString(snap.mode));
    val_stage_->setText(QString::fromStdString(snap.stage));
    val_tiles_->setText(QString::fromStdString(snap.tiles_left));
    val_lives_score_->setText(QString::fromStdString(snap.lives_score));
    val_time_->setText(QString::fromStdString(snap.time_left));
    val_backend_->setText(QString::fromStdString(snap.audio_backend));
    val_sf2_->setText(QString::fromStdString(snap.soundfont));
    val_track_->setText(QString::fromStdString(snap.track));
    val_hint_->setText(QString::fromStdString(snap.next_hint));
}

bool InspectorDialog::hint_highlight_enabled() const noexcept {
    return chk_hint_ && chk_hint_->isChecked();
}

void InspectorDialog::set_hint_highlight_enabled(bool enabled) {
    if (chk_hint_) {
        chk_hint_->setChecked(enabled);
    }
}

void InspectorDialog::closeEvent(QCloseEvent* event) {
    QDialog::closeEvent(event);
    if (on_closed_) {
        on_closed_();
    }
}

} // namespace llk2::ui::qt
