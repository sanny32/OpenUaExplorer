// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file scrollarea.h
/// \brief Declares a scroll area that does not paint its own background.
///

#pragma once

#include <QScrollArea>

///
/// \brief Scroll area whose viewport and contents stay transparent so the
/// surrounding background shows through.
///
class ScrollArea : public QScrollArea
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the scroll area.
    /// \param parent Parent widget.
    ///
    explicit ScrollArea(QWidget *parent = nullptr);

    ///
    /// \brief Sets the scrolled widget and keeps it transparent.
    /// \param widget Widget to scroll.
    ///
    void setWidget(QWidget *widget);

protected:
    void setupViewport(QWidget *viewport) override;
};
