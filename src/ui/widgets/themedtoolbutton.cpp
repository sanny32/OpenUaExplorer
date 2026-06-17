// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file themedtoolbutton.cpp
/// \brief Implements a theme-aware tool button.
///

#include <QEvent>

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
    }
}

///
/// \brief Reloads the current icon for the active theme.
///
void ThemedToolButton::refreshIcon()
{
    QToolButton::setIcon(AppIcons::themed(_iconName));
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
