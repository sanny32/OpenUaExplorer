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
/// \brief Constructs an empty themed icon label.
/// \param parent Parent widget.
///
ThemedIconLabel::ThemedIconLabel(QWidget *parent)
    : QLabel(parent)
{
}

///
/// \brief Sets the themed icon to display.
/// \param name Icon resource name.
/// \param size Desired pixmap size; invalid uses the icon's first available size.
///
void ThemedIconLabel::setIcon(const QString &name, QSize size)
{
    _iconName = name;
    _size     = size;
    refreshIcon();
}

///
/// \brief Re-renders the icon when the palette changes.
/// \param event Change event being handled.
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
/// \brief Reloads the current icon for the active theme and updates the pixmap.
///
void ThemedIconLabel::refreshIcon()
{
    const QIcon icon = AppIcons::themed(_iconName);
    const QSize size = _size.isValid() ? _size : icon.availableSizes().value(0);
    setPixmap(icon.pixmap(size));
}
