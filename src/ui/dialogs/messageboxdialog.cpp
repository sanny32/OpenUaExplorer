// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file messageboxdialog.cpp
/// \brief Implements the themed message dialog.
///

#include <QAbstractButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

#include "messageboxdialog.h"

///
/// \brief Builds an empty message dialog.
/// \param parent Parent widget.
///
MessageBoxDialog::MessageBoxDialog(QWidget *parent)
    : AppBaseDialog(parent)
    , _iconLabel(new QLabel(this))
    , _textLabel(new QLabel(this))
    , _buttonBox(new DialogButtonBox(this))
{
    setWindowFlag(Qt::WindowStaysOnTopHint, true);

    _iconLabel->setAlignment(Qt::AlignTop);
    _textLabel->setWordWrap(true);
    _textLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    _textLabel->setMinimumWidth(fontMetrics().averageCharWidth() * 60);

    auto *content = new QHBoxLayout;
    content->setSpacing(16);
    content->addWidget(_iconLabel, 0, Qt::AlignTop);
    content->addWidget(_textLabel, 1);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(content);
    layout->addSpacing(8);
    layout->addWidget(_buttonBox);

    connect(_buttonBox, &QDialogButtonBox::clicked,
            this, &MessageBoxDialog::onButtonClicked);

    setIcon(NoIcon);
}

///
/// \brief Sets the message text.
/// \param text Message shown to the user.
///
void MessageBoxDialog::setText(const QString &text)
{
    _textLabel->setText(text);
}

///
/// \brief Sets the standard icon shown beside the message.
/// \param icon Icon to display.
///
void MessageBoxDialog::setIcon(Icon icon)
{
    QStyle::StandardPixmap pixmap = QStyle::SP_MessageBoxInformation;
    bool hasIcon = true;
    switch (icon) {
    case Information:
        pixmap = QStyle::SP_MessageBoxInformation;
        break;
    case Warning:
        pixmap = QStyle::SP_MessageBoxWarning;
        break;
    case Critical:
        pixmap = QStyle::SP_MessageBoxCritical;
        break;
    case Question:
        pixmap = QStyle::SP_MessageBoxQuestion;
        break;
    case NoIcon:
        hasIcon = false;
        break;
    }

    if (!hasIcon) {
        _iconLabel->clear();
        _iconLabel->hide();
        return;
    }

    int size = style()->pixelMetric(QStyle::PM_MessageBoxIconSize, nullptr, this);
    if (size <= 0)
        size = 32;
    _iconLabel->setPixmap(style()->standardIcon(pixmap, nullptr, this).pixmap(size, size));
    _iconLabel->show();
}

///
/// \brief Creates the requested standard buttons.
/// \param buttons Standard buttons to display.
///
void MessageBoxDialog::setStandardButtons(DialogButtonBox::StandardButtons buttons)
{
    _buttons = buttons;
    _buttonBox->setStandardButtons(buttons);
}

///
/// \brief Makes a standard button the default (activated by Enter).
/// \param button Standard button to focus as the default.
///
void MessageBoxDialog::setDefaultButton(DialogButtonBox::StandardButton button)
{
    if (QPushButton *push = _buttonBox->button(button)) {
        push->setDefault(true);
        push->setFocus();
    }
}

///
/// \brief Returns the standard button that closed the dialog.
/// \return Clicked standard button, or NoButton when dismissed without one.
///
DialogButtonBox::StandardButton MessageBoxDialog::clickedButton() const
{
    return _clicked;
}

///
/// \brief Records the clicked standard button and closes the dialog.
/// \param button Button that was clicked.
///
void MessageBoxDialog::onButtonClicked(QAbstractButton *button)
{
    _clicked = _buttonBox->standardButton(button);
    done(_clicked);
}

///
/// \brief Treats Escape or window close as the cancelling button when one exists.
///
void MessageBoxDialog::reject()
{
    if (_clicked == DialogButtonBox::NoButton) {
        for (const DialogButtonBox::StandardButton candidate :
             { DialogButtonBox::Cancel, DialogButtonBox::Close, DialogButtonBox::No,
               DialogButtonBox::Abort }) {
            if (_buttons & candidate) {
                _clicked = candidate;
                break;
            }
        }
    }
    AppBaseDialog::reject();
}

///
/// \brief Shows a themed warning message and returns the chosen button.
///
DialogButtonBox::StandardButton MessageBoxDialog::warning(
    QWidget *parent, const QString &title, const QString &text,
    DialogButtonBox::StandardButtons buttons,
    DialogButtonBox::StandardButton defaultButton)
{
    MessageBoxDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.setIcon(Warning);
    dialog.setText(text);
    dialog.setStandardButtons(buttons);
    if (defaultButton != DialogButtonBox::NoButton)
        dialog.setDefaultButton(defaultButton);
    dialog.exec();
    return dialog.clickedButton();
}

///
/// \brief Shows a themed question message and returns the chosen button.
///
DialogButtonBox::StandardButton MessageBoxDialog::question(
    QWidget *parent, const QString &title, const QString &text,
    DialogButtonBox::StandardButtons buttons,
    DialogButtonBox::StandardButton defaultButton)
{
    MessageBoxDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.setIcon(Question);
    dialog.setText(text);
    dialog.setStandardButtons(buttons);
    if (defaultButton != DialogButtonBox::NoButton)
        dialog.setDefaultButton(defaultButton);
    dialog.exec();
    return dialog.clickedButton();
}

///
/// \brief Shows a themed information message and returns the chosen button.
///
DialogButtonBox::StandardButton MessageBoxDialog::information(
    QWidget *parent, const QString &title, const QString &text,
    DialogButtonBox::StandardButtons buttons,
    DialogButtonBox::StandardButton defaultButton)
{
    MessageBoxDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.setIcon(Information);
    dialog.setText(text);
    dialog.setStandardButtons(buttons);
    if (defaultButton != DialogButtonBox::NoButton)
        dialog.setDefaultButton(defaultButton);
    dialog.exec();
    return dialog.clickedButton();
}

///
/// \brief Shows a themed critical message and returns the chosen button.
///
DialogButtonBox::StandardButton MessageBoxDialog::critical(
    QWidget *parent, const QString &title, const QString &text,
    DialogButtonBox::StandardButtons buttons,
    DialogButtonBox::StandardButton defaultButton)
{
    MessageBoxDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.setIcon(Critical);
    dialog.setText(text);
    dialog.setStandardButtons(buttons);
    if (defaultButton != DialogButtonBox::NoButton)
        dialog.setDefaultButton(defaultButton);
    dialog.exec();
    return dialog.clickedButton();
}
