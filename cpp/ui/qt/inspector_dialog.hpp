#pragma once

#include "core/config.hpp"
#include "core/game_state.hpp"
#include "core/i18n.hpp"
#include "media/audio_engine.hpp"

#include <QCheckBox>
#include <QCloseEvent>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <functional>

namespace llk2::ui::qt {

class InspectorDialog : public QDialog {
public:
    InspectorDialog(const core::I18n& i18n, QWidget* parent = nullptr);

    void set_callbacks(std::function<void(bool)> on_hint_toggled,
                       std::function<void()> on_closed);

    void retranslate();
    void refresh(const core::GameState& gs,
                 const media::AudioEngine& audio,
                 const core::Config& cfg);

    bool hint_highlight_enabled() const noexcept;
    void set_hint_highlight_enabled(bool enabled);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    const core::I18n& i18n_;
    std::function<void(bool)> on_hint_toggled_;
    std::function<void()> on_closed_;

    QLabel* key_mode_ = nullptr;
    QLabel* val_mode_ = nullptr;
    QLabel* key_stage_ = nullptr;
    QLabel* val_stage_ = nullptr;
    QLabel* key_tiles_ = nullptr;
    QLabel* val_tiles_ = nullptr;
    QLabel* val_lives_score_ = nullptr;
    QLabel* key_time_ = nullptr;
    QLabel* val_time_ = nullptr;

    QLabel* key_backend_ = nullptr;
    QLabel* val_backend_ = nullptr;
    QLabel* key_sf2_ = nullptr;
    QLabel* val_sf2_ = nullptr;
    QLabel* key_track_ = nullptr;
    QLabel* val_track_ = nullptr;

    QLabel* key_hint_ = nullptr;
    QLabel* val_hint_ = nullptr;
    QCheckBox* chk_hint_ = nullptr;
    QPushButton* btn_close_ = nullptr;
};

} // namespace llk2::ui::qt
