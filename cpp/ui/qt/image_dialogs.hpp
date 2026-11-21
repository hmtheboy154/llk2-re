#pragma once

#include "media/asset_manager.hpp"

#include <QDialog>
#include <QImage>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QString>

namespace llk2::ui::qt {

QImage qimage_from_rgba(const media::RgbaSurface& surf);

// Splash dialog that displays an exact-size image and closes when clicked anywhere
// (mirrors Form_Explain and Form_Author in VB5 slots 53 & 55).
class ImageSplashDialog : public QDialog {
public:
    ImageSplashDialog(const media::RgbaSurface& surf, const QString& title, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    QImage image_;
};

} // namespace llk2::ui::qt
