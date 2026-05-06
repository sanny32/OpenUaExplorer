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
/// \brief ThemedToolButton::ThemedToolButton
/// \param parent
///
ThemedToolButton::ThemedToolButton(QWidget *parent)
    : QToolButton(parent)
{
}

///
/// \brief ThemedToolButton::minimumSizeHint
/// \return
///
QSize ThemedToolButton::minimumSizeHint() const
{
    return squareSize(QToolButton::minimumSizeHint());
}

///
/// \brief ThemedToolButton::sizeHint
/// \return
///
QSize ThemedToolButton::sizeHint() const
{
    return squareSize(QToolButton::sizeHint());
}

///
/// \brief ThemedToolButton::squareIconOnly
/// \return
///
bool ThemedToolButton::squareIconOnly() const
{
    return _squareIconOnly;
}

///
/// \brief ThemedToolButton::setSquareIconOnly
/// \param enabled
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
/// \brief ThemedToolButton::setIcon
/// \param name
///
void ThemedToolButton::setIcon(const QString &name)
{
    _iconName = name;
    refreshIcon();
}

///
/// \brief ThemedToolButton::changeEvent
/// \param event
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
/// \brief ThemedToolButton::refreshIcon
///
void ThemedToolButton::refreshIcon()
{
    QToolButton::setIcon(AppIcons::themed(_iconName));
}

///
/// \brief ThemedToolButton::squareSize
/// \param size
/// \return
///
QSize ThemedToolButton::squareSize(const QSize &size) const
{
    if (!_squareIconOnly || !text().isEmpty()) {
        return size;
    }

    const int side = qMax(size.width(), size.height());
    return QSize(side, side);
}
