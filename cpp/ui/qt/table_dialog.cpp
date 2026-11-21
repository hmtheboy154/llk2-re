#include "ui/qt/table_dialog.hpp"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <cmath>

namespace llk2::ui::qt {

TableDialog::TableDialog(core::HighScoreTable& table,
                         const core::I18n& i18n,
                         int editable_rank,
                         const std::string& initial_name,
                         QWidget* parent)
    : QDialog(parent, Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::MSWindowsFixedSizeDialogHint),
      table_(table),
      i18n_(i18n),
      editable_rank_(editable_rank) {
    setWindowTitle(QString::fromStdString(i18n_.tr("dialog.table.title")));
    setModal(true);
    setMinimumWidth(310);

    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(16, 14, 16, 14);
    main_layout->setSpacing(10);

    auto* grid = new QGridLayout();
    grid->setHorizontalSpacing(18);
    grid->setVerticalSpacing(6);

    auto* hdr_rank = new QLabel(QString::fromStdString(i18n_.tr("table.col.rank")), this);
    auto* hdr_name = new QLabel(QString::fromStdString(i18n_.tr("table.col.name")), this);
    auto* hdr_score = new QLabel(QString::fromStdString(i18n_.tr("table.col.score")), this);

    QFont hdr_font = hdr_rank->font();
    hdr_font.setBold(true);
    hdr_rank->setFont(hdr_font);
    hdr_name->setFont(hdr_font);
    hdr_score->setFont(hdr_font);

    hdr_rank->setAlignment(Qt::AlignCenter);
    hdr_name->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    hdr_score->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    grid->addWidget(hdr_rank, 0, 0);
    grid->addWidget(hdr_name, 0, 1);
    grid->addWidget(hdr_score, 0, 2);

    auto* sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    grid->addWidget(sep, 1, 0, 1, 3);

    for (int i = 0; i < core::HighScoreTable::kCount; ++i) {
        rank_labels_[i] = new QLabel(QString::number(i + 1), this);
        name_labels_[i] = new QLabel(this);
        score_labels_[i] = new QLabel(this);

        rank_labels_[i]->setAlignment(Qt::AlignCenter);
        name_labels_[i]->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        score_labels_[i]->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        if (i == editable_rank_) {
            const QString active_style = "color: #D02020; font-weight: bold;";
            rank_labels_[i]->setStyleSheet(active_style);
            name_labels_[i]->setStyleSheet(active_style);
            score_labels_[i]->setStyleSheet(active_style);
        }

        grid->addWidget(rank_labels_[i], i + 2, 0);
        grid->addWidget(name_labels_[i], i + 2, 1);
        grid->addWidget(score_labels_[i], i + 2, 2);
    }
    grid->setColumnStretch(1, 1);
    main_layout->addLayout(grid);

    if (editable_rank_ >= 0 && editable_rank_ < core::HighScoreTable::kCount) {
        input_frame_ = new QFrame(this);
        auto* input_layout = new QHBoxLayout(input_frame_);
        input_layout->setContentsMargins(0, 6, 0, 4);

        auto* prompt_lbl = new QLabel(QString::fromStdString(i18n_.tr("table.prompt.name")) + ":", input_frame_);
        name_edit_ = new QLineEdit(QString::fromStdString(initial_name), input_frame_);
        name_edit_->setMaxLength(24);
        name_edit_->selectAll();

        input_layout->addWidget(prompt_lbl);
        input_layout->addWidget(name_edit_, 1);
        main_layout->addWidget(input_frame_);

        connect(name_edit_, &QLineEdit::textChanged, this, &TableDialog::on_name_edited);
        table_.update_rank_name(editable_rank_, initial_name);
    }

    auto* btn_layout = new QHBoxLayout();
    btn_layout->addStretch(1);
    ok_btn_ = new QPushButton(QString::fromStdString(i18n_.tr("table.btn.ok")), this);
    ok_btn_->setDefault(true);
    ok_btn_->setMinimumWidth(90);
    btn_layout->addWidget(ok_btn_);
    btn_layout->addStretch(1);
    main_layout->addLayout(btn_layout);

    connect(ok_btn_, &QPushButton::clicked, this, &QDialog::accept);

    refresh_rows();

    if (name_edit_) {
        name_edit_->setFocus();
    }
}

void TableDialog::refresh_rows() {
    for (int i = 0; i < core::HighScoreTable::kCount; ++i) {
        const auto& e = table_.entry(i);
        name_labels_[i]->setText(QString::fromStdString(e.name));
        int iscore = static_cast<int>(std::lround(e.score));
        score_labels_[i]->setText(QString::number(iscore));
    }
}

void TableDialog::on_name_edited(const QString& text) {
    if (editable_rank_ < 0 || editable_rank_ >= core::HighScoreTable::kCount) {
        return;
    }
    std::string utf8 = text.trimmed().toStdString();
    if (utf8.empty()) {
        utf8 = i18n_.tr("table.default_player");
    }
    table_.update_rank_name(editable_rank_, utf8);
    name_labels_[editable_rank_]->setText(QString::fromStdString(table_.entry(editable_rank_).name));
}

std::string TableDialog::entered_name() const {
    if (name_edit_) {
        std::string s = name_edit_->text().trimmed().toStdString();
        if (!s.empty()) return s;
    }
    return i18n_.tr("table.default_player");
}

} // namespace llk2::ui::qt
