// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file themedtoolbutton.cpp
/// \brief Implements a theme-aware tool button.
///

#include <QEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QStyleOptionToolButton>
#include <QStylePainter>

#include "appcolors.h"
#include "appicons.h"
#include "themedtoolbutton.h"

///
/// \brief Constructs the themed tool button.
/// \param parent Parent widget.
///
ThemedToolButton::ThemedToolButton(QWidget *parent)
    : QToolButton(parent)
{
}

///
/// \brief Returns the minimum size hint, squared when in icon-only mode.
/// \return Minimum size hint.
///
QSize ThemedToolButton::minimumSizeHint() const
{
    return squareSize(QToolButton::minimumSizeHint());
}

///
/// \brief Returns the preferred size, squared when in icon-only mode.
/// \return Preferred size hint.
///
QSize ThemedToolButton::sizeHint() const
{
    return squareSize(QToolButton::sizeHint());
}

///
/// \brief Reports whether the button is forced square when it shows only an icon.
/// \return True when square icon-only mode is enabled.
///
bool ThemedToolButton::squareIconOnly() const
{
    return _squareIconOnly;
}

///
/// \brief Reports whether the button is painted as a link-style action.
/// \return True when link-style rendering is enabled.
///
bool ThemedToolButton::linkStyle() const
{
    return _linkStyle;
}

///
/// \brief Returns the themed icon resource name.
/// \return Icon resource name.
///
QString ThemedToolButton::iconName() const
{
    return _iconName;
}

///
/// \brief Enables or disables forcing a square shape for icon-only buttons.
/// \param enabled True to force a square shape.
///
void ThemedToolButton::setSquareIconOnly(bool enabled)
{
    if (_squareIconOnly == enabled) {
        return;
    }

    _squareIconOnly = enabled;
    updateGeometry();
}

///
/// \brief Enables or disables link-style rendering.
/// \param enabled True to paint as a link-style action.
///
void ThemedToolButton::setLinkStyle(bool enabled)
{
    if (_linkStyle == enabled) {
        return;
    }

    _linkStyle = enabled;
    setCursor(_linkStyle ? Qt::PointingHandCursor : Qt::ArrowCursor);
    updateGeometry();
    update();
}

///
/// \brief Sets the themed icon resource name.
/// \param name Icon resource name.
///
void ThemedToolButton::setIconName(const QString &name)
{
    setIcon(name);
}

///
/// \brief Sets the themed icon by resource name.
/// \param name Icon resource name.
///
void ThemedToolButton::setIcon(const QString &name)
{
    if (_iconName == name) {
        return;
    }

    _iconName = name;
    if (_iconName.isEmpty()) {
        QToolButton::setIcon({});
        return;
    }
    refreshIcon();
}

///
/// \brief Re-renders the icon when the palette changes.
/// \param event Change event being handled.
///
void ThemedToolButton::changeEvent(QEvent *event)
{
    QToolButton::changeEvent(event);

    if (!_iconName.isEmpty()
        && (event->type() == QEvent::PaletteChange
            || event->type() == QEvent::ApplicationPaletteChange)) {
        refreshIcon();
        update();
    }
}

///
/// \brief Paints link-style actions without a push-button frame.
///
void ThemedToolButton::paintEvent(QPaintEvent *event)
{
    if (!_linkStyle) {
        QToolButton::paintEvent(event);
        return;
    }

    Q_UNUSED(event)

    QStyleOptionToolButton option;
    initStyleOption(&option);

    const QColor foreground = linkColor();
    option.palette.setColor(QPalette::ButtonText, foreground);
    option.palette.setColor(QPalette::Text, foreground);
    option.palette.setColor(QPalette::WindowText, foreground);
    option.icon = tintedLinkIcon(foreground);
    option.state &= ~QStyle::State_Raised;
    option.state &= ~QStyle::State_Sunken;

    QStylePainter painter(this);
    if (isEnabled() && option.state.testFlag(QStyle::State_MouseOver)) {
        QColor fill = foreground;
        fill.setAlpha(AppIcons::isDarkTheme() ? 38 : 24);
        QColor border = foreground;
        border.setAlpha(AppIcons::isDarkTheme() ? 90 : 64);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(border);
        painter.setBrush(fill);
        painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 4, 4);
    }

    painter.drawControl(QStyle::CE_ToolButtonLabel, option);
}

///
/// \brief Reloads the current icon for the active theme.
///
void ThemedToolButton::refreshIcon()
{
    QToolButton::setIcon(AppIcons::themed(_iconName));
}

///
/// \brief Returns the current icon tinted with the link foreground colour.
/// \param color Colour to apply to opaque icon pixels.
/// \return Tinted icon for link-style painting.
///
QIcon ThemedToolButton::tintedLinkIcon(const QColor &color) const
{
    if (_iconName.isEmpty()) {
        return icon();
    }

    const QSize size = iconSize().isValid() ? iconSize() : QSize(16, 16);
    QPixmap source = AppIcons::themed(_iconName).pixmap(size);
    if (source.isNull()) {
        return icon();
    }

    QPixmap tinted(source.size());
    tinted.setDevicePixelRatio(source.devicePixelRatio());
    tinted.fill(Qt::transparent);

    QPainter painter(&tinted);
    painter.drawPixmap(0, 0, source);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(tinted.rect(), color);
    painter.end();

    return QIcon(tinted);
}

///
/// \brief Returns the foreground colour used by link-style rendering.
/// \return Enabled link colour or disabled text colour.
///
QColor ThemedToolButton::linkColor() const
{
    if (!isEnabled()) {
        return palette().color(QPalette::Disabled, QPalette::ButtonText);
    }
    return AppColors::header();
}

///
/// \brief Squares a size hint when icon-only mode is active and there is no text.
/// \param size Size to adjust.
/// \return Squared size, or the original when not applicable.
///
QSize ThemedToolButton::squareSize(const QSize &size) const
{
    if (!_squareIconOnly || !text().isEmpty()) {
        return size;
    }

    const int side = qMax(size.width(), size.height());
    return QSize(side, side);
}
