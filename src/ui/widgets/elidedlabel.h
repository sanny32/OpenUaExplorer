// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file elidedlabel.h
/// \brief Declares a label that elides its text to the width it is given.
///

#pragma once

#include <QLabel>

///
/// \brief Label that asks for the width of its full text but never demands it.
///
/// The label reports a minimum width of zero, so it shrinks with the window instead of
/// forcing it wider, and paints its text elided to whatever width the layout grants.
///
class ElidedLabel : public QLabel
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs an empty elided label.
    /// \param parent Parent widget.
    ///
    explicit ElidedLabel(QWidget *parent = nullptr);

    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
};
