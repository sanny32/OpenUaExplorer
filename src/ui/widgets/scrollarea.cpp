// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file scrollarea.cpp
/// \brief Implements a scroll area that does not paint its own background.
///

#include "scrollarea.h"

///
/// \brief Constructs the scroll area.
/// \param parent Parent widget.
///
ScrollArea::ScrollArea(QWidget *parent)
    : QScrollArea(parent)
{
    setFrameShape(QFrame::NoFrame);
    setAutoFillBackground(false);
    if (viewport()) {
        viewport()->setAutoFillBackground(false);
    }
}

///
/// \brief Sets the scrolled widget and keeps it transparent.
/// \param widget Widget to scroll.
///
void ScrollArea::setWidget(QWidget *widget)
{
    QScrollArea::setWidget(widget);
    if (widget) {
        widget->setAutoFillBackground(false);
    }
}

///
/// \brief ScrollArea::setupViewport
/// \param viewport
///
void ScrollArea::setupViewport(QWidget *viewport)
{
    QScrollArea::setupViewport(viewport);
    if (viewport) {
        viewport->setAutoFillBackground(false);
    }
}
