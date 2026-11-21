#include "ui/qt/image_dialogs.hpp"

#include <QPainter>

namespace llk2::ui::qt {

QImage qimage_from_rgba(const media::RgbaSurface& surf) {
    if (!surf.valid()) {
        return QImage();
    }
    QImage img(surf.pixels.data(), surf.width, surf.height, surf.width * 4, QImage::Format_RGBA8888);
    return img.copy(); // Deep-copy so QImage owns its pixel buffer safely
}

ImageSplashDialog::ImageSplashDialog(const media::RgbaSurface& surf, const QString& title, QWidget* parent)
    : QDialog(parent, Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::MSWindowsFixedSizeDialogHint),
      image_(qimage_from_rgba(surf)) {
    setWindowTitle(title);
    setModal(true);
    int w = image_.isNull() ? 400 : image_.width();
    int h = image_.isNull() ? 300 : image_.height();
    setFixedSize(w, h);
}

void ImageSplashDialog::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    if (!image_.isNull()) {
        painter.drawImage(0, 0, image_);
    } else {
        painter.fillRect(rect(), Qt::black);
    }
}

void ImageSplashDialog::mousePressEvent(QMouseEvent* /*event*/) {
    // VB5 Form_Author_MouseDown (slot 53) & Form_Explain_MouseDown (slot 55): Unload Me on click
    accept();
}

} // namespace llk2::ui::qt
