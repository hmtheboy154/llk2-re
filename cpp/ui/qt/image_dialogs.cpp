#include "ui/qt/image_dialogs.hpp"

#include <QCheckBox>
#include <QFont>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QRadioButton>
#include <QRectF>
#include <QVBoxLayout>
#include <array>
#include <cstdio>

namespace llk2::ui::qt {

namespace {

std::string html_escape(std::string_view sv) {
    std::string out;
    out.reserve(sv.size() + 16);
    for (char c : sv) {
        switch (c) {
            case '&':  out.append("&amp;"); break;
            case '<':  out.append("&lt;"); break;
            case '>':  out.append("&gt;"); break;
            case '"':  out.append("&quot;"); break;
            case '\n': out.append("<br/>"); break;
            case '\r': break;
            default:   out.push_back(c); break;
        }
    }
    return out;
}

std::string format_rgb(const common::AboutColor& c) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "rgb(%u,%u,%u)", c.r, c.g, c.b);
    return buf;
}

QString build_paragraph_html(const common::AboutParagraph& para) {
    const std::string text_rgb = format_rgb(para.text_color);
    const std::string link_rgb = format_rgb(para.link_color);
    std::string html = "<div align=\"center\" style=\"line-height: 138%; color: " + text_rgb + ";\">";
    for (const auto& span : para.spans) {
        if (span.url.empty()) {
            html.append(html_escape(span.text));
        } else {
            html.append("<a href=\"");
            html.append(html_escape(span.url));
            html.append("\" style=\"color: ");
            html.append(link_rgb);
            html.append("; text-decoration: underline;\">");
            html.append(html_escape(span.text));
            html.append("</a>");
        }
    }
    html.append("</div>");
    return QString::fromStdString(html);
}

} // namespace

QImage qimage_from_rgba(const media::RgbaSurface& surf) {
    if (!surf.valid()) {
        return QImage();
    }
    QImage img(surf.pixels.data(), surf.width, surf.height, surf.width * 4, QImage::Format_RGBA8888);
    return img.copy(); // Deep-copy so QImage owns its pixel buffer safely
}

ImageSplashDialog::ImageSplashDialog(const media::RgbaSurface& surf,
                                     const QString& title,
                                     QWidget* parent,
                                     common::ExplainOverlayContent overlay)
    : QDialog(parent, Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::MSWindowsFixedSizeDialogHint),
      image_(qimage_from_rgba(surf)),
      overlay_(std::move(overlay)) {
    setWindowTitle(title);
    setModal(true);
    int w = image_.isNull() ? 400 : image_.width();
    int h = image_.isNull() ? 300 : image_.height();
    setFixedSize(w, h);
}

void ImageSplashDialog::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    if (!image_.isNull()) {
        painter.drawImage(0, 0, image_);
    } else {
        painter.fillRect(rect(), Qt::black);
    }

    if (!overlay_.enabled || image_.isNull()) {
        return;
    }

    const double sx = static_cast<double>(width()) / static_cast<double>(common::ExplainOverlayLayout::kImageW);
    const double sy = static_cast<double>(height()) / static_cast<double>(common::ExplainOverlayLayout::kImageH);
    auto map_rect = [&](const common::ExplainRect& r) {
        return QRectF(r.x * sx, r.y * sy, r.w * sx, r.h * sy);
    };

    // Sample the image's own background color at (10,10) so the masks blend 100% seamlessly
    QColor mask_bg = (image_.width() > 10 && image_.height() > 10) ? image_.pixelColor(10, 10) : QColor(2, 2, 2);

    // Mask out original Chinese text while keeping all tile diagrams and the red frame intact
    painter.fillRect(map_rect(common::ExplainOverlayLayout::kMaskTurn0), mask_bg);
    painter.fillRect(map_rect(common::ExplainOverlayLayout::kMaskTurn1), mask_bg);
    painter.fillRect(map_rect(common::ExplainOverlayLayout::kMaskTurn2Top), mask_bg);
    painter.fillRect(map_rect(common::ExplainOverlayLayout::kMaskTurn2Bot), mask_bg);
    painter.fillRect(map_rect(common::ExplainOverlayLayout::kMaskRulesBox), mask_bg);

    // 4 diagram labels in crisp white
    QFont label_font = painter.font();
    label_font.setPixelSize(std::max(10, static_cast<int>(13.5 * sy)));
    label_font.setBold(true);
    painter.setFont(label_font);
    painter.setPen(QColor(245, 245, 245));

    painter.drawText(map_rect(common::ExplainOverlayLayout::kMaskTurn0),
                     Qt::AlignCenter | Qt::TextWordWrap,
                     QString::fromStdString(overlay_.turn_0));
    painter.drawText(map_rect(common::ExplainOverlayLayout::kMaskTurn1),
                     Qt::AlignCenter | Qt::TextWordWrap,
                     QString::fromStdString(overlay_.turn_1));
    painter.drawText(map_rect(common::ExplainOverlayLayout::kMaskTurn2Top),
                     Qt::AlignCenter | Qt::TextWordWrap,
                     QString::fromStdString(overlay_.turn_2));
    painter.drawText(map_rect(common::ExplainOverlayLayout::kMaskTurn2Bot),
                     Qt::AlignCenter | Qt::TextWordWrap,
                     QString::fromStdString(overlay_.turn_2));

    // Rules body inside the iconic red border in original VB5 bright green (2, 254, 2)
    QFont body_font = painter.font();
    body_font.setPixelSize(std::max(10, static_cast<int>(12.5 * sy)));
    body_font.setBold(false);
    painter.setFont(body_font);
    painter.setPen(QColor(2, 254, 2));
    painter.drawText(map_rect(common::ExplainOverlayLayout::kTextRulesBody),
                     Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                     QString::fromStdString(overlay_.body));

    // Controls legend at bottom of red box in original VB5 bright yellow (254, 253, 5)
    QFont ctrl_font = painter.font();
    ctrl_font.setPixelSize(std::max(10, static_cast<int>(12.5 * sy)));
    ctrl_font.setBold(true);
    painter.setFont(ctrl_font);
    painter.setPen(QColor(254, 253, 5));
    painter.drawText(map_rect(common::ExplainOverlayLayout::kTextControls),
                     Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                     QString::fromStdString(overlay_.controls));
}

void ImageSplashDialog::mousePressEvent(QMouseEvent* /*event*/) {
    // VB5 Form_Author_MouseDown (slot 53) & Form_Explain_MouseDown (slot 55): Unload Me on click
    accept();
}

AboutDialog::AboutDialog(const media::RgbaSurface& author_surf,
                         const core::I18n& i18n,
                         QWidget* parent)
    : QDialog(parent, Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::MSWindowsFixedSizeDialogHint) {
    setWindowTitle(QString::fromStdString(i18n.tr("dialog.author.title")));
    setModal(true);
    setFixedSize(common::AboutLayout::kTotalW, common::AboutLayout::kTotalH);
    setStyleSheet("QDialog { background-color: #000000; }");

    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Left: original Form_Author_Picture.jpg (370x310)
    auto* img_lbl = new QLabel(this);
    img_lbl->setFixedSize(common::AboutLayout::kImageW, common::AboutLayout::kImageH);
    img_lbl->setStyleSheet("QLabel { background-color: #000000; }");
    QImage img = qimage_from_rgba(author_surf);
    if (!img.isNull()) {
        img_lbl->setPixmap(QPixmap::fromImage(img).scaled(
            common::AboutLayout::kImageW, common::AboutLayout::kImageH,
            Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    }
    root->addWidget(img_lbl);

    // Middle neat margin + subtle vertical divider
    root->addSpacing(common::AboutLayout::kDividerX - common::AboutLayout::kImageW);

    auto* div_wrap = new QVBoxLayout();
    div_wrap->setContentsMargins(0, common::AboutLayout::kDividerTop,
                                 0, common::AboutLayout::kTotalH - common::AboutLayout::kDividerBottom);
    div_wrap->setSpacing(0);
    auto* divider = new QFrame(this);
    divider->setFixedWidth(1);
    divider->setStyleSheet("QFrame { background-color: #3A3A3A; border: none; }");
    div_wrap->addWidget(divider);
    root->addLayout(div_wrap);

    root->addSpacing(common::AboutLayout::kTextLeft - common::AboutLayout::kDividerX - 1);

    // Right: localized colored credits with clickable links, vertically centered
    auto* right_col = new QVBoxLayout();
    right_col->setContentsMargins(0, 18, common::AboutLayout::kTextRightPad, 18);
    right_col->setSpacing(15);
    right_col->addStretch(1);

    const auto content = common::build_about_content(i18n);
    QFont credit_font = font();
    credit_font.setPixelSize(13);

    for (const auto& para : content.paragraphs) {
        auto* lbl = new QLabel(this);
        lbl->setFont(credit_font);
        lbl->setStyleSheet("QLabel { background: transparent; }");
        lbl->setTextFormat(Qt::RichText);
        lbl->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        lbl->setWordWrap(true);
        lbl->setOpenExternalLinks(true);
        lbl->setTextInteractionFlags(Qt::TextBrowserInteraction);
        lbl->setText(build_paragraph_html(para));
        connect(lbl, &QLabel::linkHovered, lbl, [lbl](const QString& link) {
            lbl->setToolTip(link);
        });
        right_col->addWidget(lbl);
    }

    right_col->addStretch(1);
    root->addLayout(right_col, 1);
}

EndlessDialog::EndlessDialog(const core::I18n& i18n,
                             const core::EndlessConfig& initial,
                             QWidget* parent)
    : QDialog(parent, Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint),
      result_(initial) {
    auto qtr = [&](std::string_view key) { return QString::fromStdString(i18n.tr(key)); };

    setWindowTitle(qtr("dialog.endless.title"));
    setModal(true);
    setMinimumWidth(390);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 14, 16, 14);
    root->setSpacing(10);

    auto* note_lbl = new QLabel(qtr("dialog.endless.note"), this);
    note_lbl->setWordWrap(true);
    QFont note_font = note_lbl->font();
    note_font.setItalic(true);
    note_lbl->setFont(note_font);
    root->addWidget(note_lbl);

    auto make_sep = [this]() {
        auto* s = new QFrame(this);
        s->setFrameShape(QFrame::HLine);
        s->setFrameShadow(QFrame::Sunken);
        return s;
    };
    root->addWidget(make_sep());

    auto* rb_all = new QRadioButton(qtr("dialog.endless.all_levels"), this);
    auto* chk_shuffle = new QCheckBox(qtr("dialog.endless.shuffle"), this);
    chk_shuffle->setChecked(initial.shuffle);

    auto* rb_specific = new QRadioButton(qtr("dialog.endless.specific_levels"), this);

    root->addWidget(rb_all);
    auto* shuffle_row = new QHBoxLayout();
    shuffle_row->setContentsMargins(24, 0, 0, 4);
    shuffle_row->addWidget(chk_shuffle);
    root->addLayout(shuffle_row);

    root->addWidget(rb_specific);

    auto* lvl_wrap = new QWidget(this);
    auto* lvl_grid = new QGridLayout(lvl_wrap);
    lvl_grid->setContentsMargins(24, 2, 0, 4);
    lvl_grid->setHorizontalSpacing(14);
    lvl_grid->setVerticalSpacing(6);

    std::array<QCheckBox*, 7> chk_levels{};
    for (int i = 0; i < 7; ++i) {
        chk_levels[i] = new QCheckBox(QString::fromStdString(common::format_endless_stage_label(i, i18n)), lvl_wrap);
        chk_levels[i]->setChecked((initial.level_mask & (1u << i)) != 0);
        lvl_grid->addWidget(chk_levels[i], i % 4, i / 4);
    }
    root->addWidget(lvl_wrap);

    root->addWidget(make_sep());

    auto* chk_no_time = new QCheckBox(qtr("dialog.endless.no_time_limit"), this);
    chk_no_time->setChecked(initial.no_time_limit);
    root->addWidget(chk_no_time);

    auto* chk_unlim_shuffle = new QCheckBox(qtr("dialog.endless.unlimited_shuffle"), this);
    chk_unlim_shuffle->setChecked(initial.unlimited_shuffle);
    root->addWidget(chk_unlim_shuffle);

    auto* btn_row = new QHBoxLayout();
    btn_row->addStretch(1);
    auto* btn_start = new QPushButton(qtr("dialog.endless.btn.start"), this);
    btn_start->setDefault(true);
    btn_start->setMinimumWidth(92);
    auto* btn_cancel = new QPushButton(qtr("dialog.endless.btn.cancel"), this);
    btn_cancel->setMinimumWidth(92);
    btn_row->addWidget(btn_start);
    btn_row->addWidget(btn_cancel);
    root->addLayout(btn_row);

    auto sync_state = [rb_all, rb_specific, chk_shuffle, lvl_wrap, chk_levels, btn_start]() {
        bool is_specific = rb_specific->isChecked();
        lvl_wrap->setEnabled(is_specific);
        int selected_cnt = 0;
        for (int i = 0; i < 7; ++i) {
            if (chk_levels[i]->isChecked()) ++selected_cnt;
        }
        chk_shuffle->setEnabled(!is_specific || selected_cnt > 1);
        btn_start->setEnabled(!is_specific || selected_cnt > 0);
        (void)rb_all;
    };

    if (initial.type == core::EndlessModeType::SpecificLevels) {
        rb_specific->setChecked(true);
    } else {
        rb_all->setChecked(true);
    }
    sync_state();

    connect(rb_all, &QRadioButton::toggled, this, [sync_state](bool) { sync_state(); });
    connect(rb_specific, &QRadioButton::toggled, this, [sync_state](bool) { sync_state(); });
    for (int i = 0; i < 7; ++i) {
        connect(chk_levels[i], &QCheckBox::toggled, this, [sync_state](bool) { sync_state(); });
    }

    connect(btn_cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(btn_start, &QPushButton::clicked, this, [this, rb_specific, chk_shuffle, chk_no_time, chk_unlim_shuffle, chk_levels]() {
        result_.type = rb_specific->isChecked() ? core::EndlessModeType::SpecificLevels
                                                : core::EndlessModeType::AllLevels;
        uint8_t mask = 0;
        for (int i = 0; i < 7; ++i) {
            if (chk_levels[i]->isChecked()) {
                mask |= static_cast<uint8_t>(1u << i);
            }
        }
        result_.level_mask = (mask != 0) ? mask : 0x01;
        result_.shuffle = chk_shuffle->isChecked();
        result_.no_time_limit = chk_no_time->isChecked();
        result_.unlimited_shuffle = chk_unlim_shuffle->isChecked();
        accept();
    });
}

} // namespace llk2::ui::qt
