// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file themediconlabel.cpp
/// \brief Implements a theme-aware icon label.
///

#include <QEvent>

#include "appicons.h"
#include "themediconlabel.h"

///
/// \brief ThemedIconLabel::ThemedIconLabel
/// \param parent
///
ThemedIconLabel::ThemedIconLabel(QWidget *parent)
    : QLabel(parent)
{
}

///
/// \brief ThemedIconLabel::setIcon
/// \param name
/// \param size
///
void ThemedIconLabel::setIcon(const QString &name, QSize size)
{
    _iconName = name;
    _size     = size;
    refreshIcon();
}

///
/// \brief ThemedIconLabel::changeEvent
/// \param event
///
void ThemedIconLabel::changeEvent(QEvent *event)
{
    QLabel::changeEvent(event);

    if (!_iconName.isEmpty()
        && (event->type() == QEvent::PaletteChange
            || event->type() == QEvent::ApplicationPaletteChange)) {
        refreshIcon();
    }
}

///
/// \brief ThemedIconLabel::refreshIcon
///
void ThemedIconLabel::refreshIcon()
{
    const QIcon icon = AppIcons::themed(_iconName);
    const QSize size = _size.isValid() ? _size : icon.availableSizes().value(0);
    setPixmap(icon.pixmap(size));
}
