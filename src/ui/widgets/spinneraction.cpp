// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file spinneraction.cpp
/// \brief Implements the animated busy-spinner action.
///

#include <QPainter>
#include <QStyle>
#include <QWidget>

#include "spinneraction.h"

namespace {

/// \brief Size used when no owner widget supplies a style.
constexpr int FallbackSpinnerSize = 16;

/// \brief Milliseconds between animation steps.
constexpr int FrameIntervalMs = 60;

/// \brief Degrees advanced per animation step.
constexpr int DegreesPerFrame = 30;

/// \brief Sweep of the drawn arc in degrees, leaving a gap like the Lucide loader glyph.
constexpr int ArcSweepDegrees = 270;

} // namespace

///
/// \brief Constructs a hidden spinner that paints itself from a widget's palette.
/// \param owner Widget supplying the palette and device pixel ratio.
///
SpinnerAction::SpinnerAction(QWidget *owner)
    : QAction(owner)
    , _owner(owner)
{
    setVisible(false);
    _timer.setInterval(FrameIntervalMs);
    connect(&_timer, &QTimer::timeout, this, &SpinnerAction::advance);
}

///
/// \brief Shows the spinner and starts the rotation.
///
void SpinnerAction::start()
{
    if (_timer.isActive())
        return;
    _angle = 0;
    setIcon(QIcon(renderArc(_angle)));
    setVisible(true);
    _timer.start();
}

///
/// \brief Stops the rotation and hides the spinner.
///
void SpinnerAction::stop()
{
    _timer.stop();
    setVisible(false);
    setIcon(QIcon());
}

///
/// \brief Reports whether the spinner is currently animating.
/// \return True while the rotation timer runs.
///
bool SpinnerAction::isSpinning() const
{
    return _timer.isActive();
}

///
/// \brief Advances the rotation by one frame and repaints the icon.
///
void SpinnerAction::advance()
{
    _angle = (_angle + DegreesPerFrame) % 360;
    setIcon(QIcon(renderArc(_angle)));
}

///
/// \brief Renders the spinner arc rotated by an angle.
/// \param angle Rotation in degrees, measured clockwise from twelve o'clock.
/// \return Pixmap scaled for the owner's device pixel ratio.
///
QPixmap SpinnerAction::renderArc(int angle) const
{
    const int size = spinnerSize();
    const qreal ratio = _owner ? _owner->devicePixelRatioF() : qreal(1);
    QPixmap pixmap(QSize(size, size) * ratio);
    pixmap.setDevicePixelRatio(ratio);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QColor color = _owner ? _owner->palette().color(QPalette::WindowText) : QColor(Qt::black);
    color.setAlphaF(0.7);

    QPen pen(color);
    pen.setWidthF(1.6);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);

    const QRectF bounds = QRectF(0, 0, size, size).adjusted(2, 2, -2, -2);
    painter.drawArc(bounds, -angle * 16, -ArcSweepDegrees * 16);
    return pixmap;
}

///
/// \brief Returns the spinner edge length, matching the line edit's built-in icon buttons.
/// \return Icon size in logical pixels.
///
int SpinnerAction::spinnerSize() const
{
    if (!_owner)
        return FallbackSpinnerSize;
    return _owner->style()->pixelMetric(QStyle::PM_SmallIconSize, nullptr, _owner);
}
