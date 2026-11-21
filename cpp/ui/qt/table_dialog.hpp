#pragma once

#include "core/hst_score.hpp"
#include "core/i18n.hpp"

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>
#include <array>
#include <string>

namespace llk2::ui::qt {

class TableDialog : public QDialog {
public:
    // If editable_rank is 0..9, shows the name input frame and live-updates that rank's name.
    // If editable_rank is -1, opens in view-only mode (best records viewer).
    TableDialog(core::HighScoreTable& table,
                const core::I18n& i18n,
                int editable_rank = -1,
                const std::string& initial_name = "Player",
                QWidget* parent = nullptr);

    std::string entered_name() const;

private:
    void refresh_rows();
    void on_name_edited(const QString& text);

    core::HighScoreTable& table_;
    const core::I18n& i18n_;
    int editable_rank_ = -1;

    std::array<QLabel*, core::HighScoreTable::kCount> rank_labels_{};
    std::array<QLabel*, core::HighScoreTable::kCount> name_labels_{};
    std::array<QLabel*, core::HighScoreTable::kCount> score_labels_{};

    QWidget* input_frame_ = nullptr;
    QLineEdit* name_edit_ = nullptr;
    QPushButton* ok_btn_ = nullptr;
};

} // namespace llk2::ui::qt
