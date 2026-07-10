// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file spinneraction.h
/// \brief Declares an animated busy-spinner action for embedding in a QLineEdit.
///

#pragma once

#include <QAction>
#include <QPixmap>
#include <QTimer>

class QWidget;

///
/// \brief Action that renders a rotating arc while a background task runs.
///
/// Intended for QLineEdit::addAction(), which accepts only icons, so each
/// animation step re-renders the arc into the action's icon. The arc is painted
/// from the owning widget's palette rather than a themed resource, which keeps
/// it correct in both colour schemes without observing the theme object.
///
class SpinnerAction : public QAction
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs a hidden spinner that paints itself from a widget's palette.
    /// \param owner Widget supplying the palette and device pixel ratio.
    ///
    explicit SpinnerAction(QWidget *owner);

    ///
    /// \brief Shows the spinner and starts the rotation.
    ///
    void start();

    ///
    /// \brief Stops the rotation and hides the spinner.
    ///
    void stop();

    ///
    /// \brief Reports whether the spinner is currently animating.
    /// \return True while the rotation timer runs.
    ///
    bool isSpinning() const;

    ///
    /// \brief Returns the spinner edge length, matching the line edit's built-in icon buttons.
    /// \return Icon size in logical pixels.
    ///
    int spinnerSize() const;

private:
    void advance();
    QPixmap renderArc(int angle) const;

    QWidget *_owner;
    QTimer _timer;
    int _angle = 0;
};
