// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dialogbuttonbox.h
/// \brief Declares a dialog button box that renders its buttons as ColoredPushButton.
///

#pragma once

#include <QDialogButtonBox>
#include <QHash>

#include "coloredpushbutton.h"

class QAbstractButton;
class QPushButton;

///
/// \brief Dialog button box whose standard buttons are ColoredPushButton instances.
///
/// Drop-in replacement for QDialogButtonBox: the standard buttons keep their roles,
/// translated text and accepted()/rejected() wiring, but render through
/// ColoredPushButton. The Apply button is painted with the application accent colour
/// by default, and any button can be given an explicit colour set.
///
class DialogButtonBox : public QDialogButtonBox
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs a horizontal button box.
    /// \param parent Parent widget.
    ///
    explicit DialogButtonBox(QWidget *parent = nullptr);

    ///
    /// \brief Constructs a button box with the given orientation.
    /// \param orientation Layout orientation.
    /// \param parent Parent widget.
    ///
    explicit DialogButtonBox(Qt::Orientation orientation, QWidget *parent = nullptr);

    ///
    /// \brief Creates the requested standard buttons as ColoredPushButton instances.
    /// \param buttons Standard buttons to display.
    ///
    void setStandardButtons(StandardButtons buttons);

    ///
    /// \brief Returns the button for a standard button role.
    /// \param which Standard button to look up.
    /// \return Matching button, or nullptr when not present.
    ///
    QPushButton *button(StandardButton which) const;

    ///
    /// \brief Returns the standard button role backing a button instance.
    /// \param button Button created by setStandardButtons().
    /// \return Matching standard button, or NoButton when unknown.
    ///
    /// Reimplemented because the base class cannot map buttons that were added as
    /// custom ColoredPushButton instances back to their standard role.
    ///
    StandardButton standardButton(QAbstractButton *button) const;

    ///
    /// \brief Paints a standard button with an explicit colour set.
    /// \param which Standard button to colour.
    /// \param colors Light-theme colour set (the dark variant is derived).
    ///
    void setButtonColors(StandardButton which, const ColoredPushButton::Colors &colors);

    ///
    /// \brief Restores a standard button's default palette appearance.
    /// \param which Standard button to reset.
    ///
    void clearButtonColors(StandardButton which);

private:
    ColoredPushButton *coloredButton(StandardButton which) const;

    QHash<int, ColoredPushButton *> _standardButtons;
};
