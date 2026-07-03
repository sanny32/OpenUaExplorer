// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file textviewdialog.h
/// \brief Declares a reusable dialog that shows read-only text with a copy action.
///

#pragma once

#include "dialogs/appbasedialog.h"

#include <QString>

namespace Ui {
class TextViewDialog;
}

///
/// \brief Displays arbitrary text in a read-only, monospaced view with a copy button.
///
/// Intended as a shared viewer for file contents such as PEM keys or certificates,
/// so callers do not have to build a text window each time.
///
class TextViewDialog : public AppBaseDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the dialog and wires its actions.
    /// \param parent Parent widget.
    ///
    explicit TextViewDialog(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the dialog and its generated UI.
    ///
    ~TextViewDialog() override;

    ///
    /// \brief Sets the text shown in the view.
    /// \param text Text to display.
    ///
    void setText(const QString &text);

    ///
    /// \brief Returns the text currently shown in the view.
    ///
    QString text() const;

    ///
    /// \brief Shows the given text modally in a text viewer.
    /// \param parent Parent widget owning the dialog.
    /// \param title Window title for the viewer.
    /// \param text Text to display.
    ///
    static void showText(QWidget *parent, const QString &title, const QString &text);

private slots:
    void copyText();

private:
    Ui::TextViewDialog *ui;
};
