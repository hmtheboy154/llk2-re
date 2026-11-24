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

// Modern About dialog: original Form_Author_Picture on the left, neat divider margin,
// and localized colored credits with clickable external links on the right.
class AboutDialog : public QDialog {
public:
    AboutDialog(const media::RgbaSurface& author_surf,
                const core::I18n& i18n,
                QWidget* parent = nullptr);
};

// Endless Mode configuration dialog:
// Option 1: All Levels (Stages 1-7) with optional stage shuffle
// Option 2: Specific Level(s) Loop (1..7 checkboxes)
// Plus optional No Time Limit (Relaxed) checkbox.
class EndlessDialog : public QDialog {
public:
    EndlessDialog(const core::I18n& i18n,
                  const core::EndlessConfig& initial,
                  QWidget* parent = nullptr);

    core::EndlessConfig selected_config() const noexcept { return result_; }

private:
    core::EndlessConfig result_;
};

} // namespace llk2::ui::qt
