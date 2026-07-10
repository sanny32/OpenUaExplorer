// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file fixedgap.h
/// \brief Declares a fixed-width spacer widget.
///

#pragma once

#include <QWidget>

///
/// \brief Fixed-width spacer widget.
///
class FixedGap : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs a spacer of fixed width.
    /// \param width Fixed width in pixels.
    /// \param parent Parent widget.
    ///
    explicit FixedGap(int width, QWidget *parent = nullptr);
};
