// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file messageboxdialog.h
/// \brief Declares a themed message dialog rendered with the application button box.
///

#pragma once

#include "appbasedialog.h"
#include "widgets/dialogbuttonbox.h"

class QAbstractButton;
class QLabel;

///
/// \brief Modal message dialog matching the application's dialog styling.
///
/// A themed replacement for QMessageBox: it renders a standard icon and message
/// alongside a DialogButtonBox, so confirmation prompts share the ColoredPushButton
/// look and window-icon theming of the rest of the application. The static helpers
/// mirror the QMessageBox convenience API and return the standard button clicked.
///
class MessageBoxDialog : public AppBaseDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Standard icon shown beside the message.
    ///
    enum Icon {
        NoIcon,
        Information,
        Warning,
        Critical,
        Question
    };

    ///
    /// \brief Builds an empty message dialog.
    /// \param parent Parent widget.
    ///
    explicit MessageBoxDialog(QWidget *parent = nullptr);

    ///
    /// \brief Sets the message text.
    /// \param text Message shown to the user.
    ///
    void setText(const QString &text);

    ///
    /// \brief Sets the standard icon shown beside the message.
    /// \param icon Icon to display.
    ///
    void setIcon(Icon icon);

    ///
    /// \brief Creates the requested standard buttons.
    /// \param buttons Standard buttons to display.
    ///
    void setStandardButtons(DialogButtonBox::StandardButtons buttons);

    ///
    /// \brief Makes a standard button the default (activated by Enter).
    /// \param button Standard button to focus as the default.
    ///
    void setDefaultButton(DialogButtonBox::StandardButton button);

    ///
    /// \brief Returns the standard button that closed the dialog.
    /// \return Clicked standard button, or NoButton when dismissed without one.
    ///
    DialogButtonBox::StandardButton clickedButton() const;

    ///
    /// \brief Shows a themed warning message and returns the chosen button.
    /// \param parent Parent widget.
    /// \param title Window title.
    /// \param text Message text.
    /// \param buttons Standard buttons to offer.
    /// \param defaultButton Button focused as the default, or NoButton for none.
    /// \return Clicked standard button.
    ///
    static DialogButtonBox::StandardButton warning(
        QWidget *parent, const QString &title, const QString &text,
        DialogButtonBox::StandardButtons buttons,
        DialogButtonBox::StandardButton defaultButton = DialogButtonBox::NoButton);

    ///
    /// \brief Shows a themed question message and returns the chosen button.
    /// \param parent Parent widget.
    /// \param title Window title.
    /// \param text Message text.
    /// \param buttons Standard buttons to offer.
    /// \param defaultButton Button focused as the default, or NoButton for none.
    /// \return Clicked standard button.
    ///
    static DialogButtonBox::StandardButton question(
        QWidget *parent, const QString &title, const QString &text,
        DialogButtonBox::StandardButtons buttons,
        DialogButtonBox::StandardButton defaultButton = DialogButtonBox::NoButton);

    ///
    /// \brief Shows a themed information message and returns the chosen button.
    /// \param parent Parent widget.
    /// \param title Window title.
    /// \param text Message text.
    /// \param buttons Standard buttons to offer.
    /// \param defaultButton Button focused as the default, or NoButton for none.
    /// \return Clicked standard button.
    ///
    static DialogButtonBox::StandardButton information(
        QWidget *parent, const QString &title, const QString &text,
        DialogButtonBox::StandardButtons buttons = DialogButtonBox::Ok,
        DialogButtonBox::StandardButton defaultButton = DialogButtonBox::NoButton);

    ///
    /// \brief Shows a themed critical message and returns the chosen button.
    /// \param parent Parent widget.
    /// \param title Window title.
    /// \param text Message text.
    /// \param buttons Standard buttons to offer.
    /// \param defaultButton Button focused as the default, or NoButton for none.
    /// \return Clicked standard button.
    ///
    static DialogButtonBox::StandardButton critical(
        QWidget *parent, const QString &title, const QString &text,
        DialogButtonBox::StandardButtons buttons = DialogButtonBox::Ok,
        DialogButtonBox::StandardButton defaultButton = DialogButtonBox::NoButton);

public slots:
    void reject() override;

private:
    void onButtonClicked(QAbstractButton *button);

    QLabel *_iconLabel;
    QLabel *_textLabel;
    DialogButtonBox *_buttonBox;
    DialogButtonBox::StandardButtons _buttons = DialogButtonBox::NoButton;
    DialogButtonBox::StandardButton _clicked = DialogButtonBox::NoButton;
};
