#include "ui/qt/image_dialogs.hpp"

#include <QFont>
#include <QPainter>
#include <QRectF>

namespace llk2::ui::qt {

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

} // namespace llk2::ui::qt
