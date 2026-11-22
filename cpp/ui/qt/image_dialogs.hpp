#pragma once

#include "media/asset_manager.hpp"
#include "ui/common/ui_helpers.hpp"

#include <QDialog>
#include <QImage>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QString>

namespace llk2::ui::qt {

QImage qimage_from_rgba(const media::RgbaSurface& surf);

// Splash dialog that displays an exact-size image (with optional localized rules text overlay)
// and closes when clicked anywhere (mirrors Form_Explain and Form_Author in VB5 slots 53 & 55).
class ImageSplashDialog : public QDialog {
public:
    ImageSplashDialog(const media::RgbaSurface& surf,
                      const QString& title,
                      QWidget* parent = nullptr,
                      common::ExplainOverlayContent overlay = {});

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    QImage image_;
    common::ExplainOverlayContent overlay_;
};

} // namespace llk2::ui::qt
