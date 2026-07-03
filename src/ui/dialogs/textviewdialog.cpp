// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file textviewdialog.cpp
/// \brief Implements the reusable read-only text viewer dialog.
///

#include <QApplication>
#include <QClipboard>
#include <QFontDatabase>

#include "appcolors.h"
#include "textviewdialog.h"
#include "ui_textviewdialog.h"

///
/// \brief Builds the dialog and wires its actions.
/// \param parent Parent widget.
///
TextViewDialog::TextViewDialog(QWidget *parent)
    : AppBaseDialog(parent)
    , ui(new Ui::TextViewDialog)
{
    ui->setupUi(this);

    ui->textEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    ui->closeButton->setColors(
        { AppColors::accent(), AppColors::accentHover(), AppColors::accentPressed() });

    connect(ui->copyButton, &QPushButton::clicked, this, &TextViewDialog::copyText);
    connect(ui->closeButton, &QPushButton::clicked, this, &QDialog::accept);
}

///
/// \brief Destroys the dialog and its generated UI.
///
TextViewDialog::~TextViewDialog()
{
    delete ui;
}

///
/// \brief Sets the text shown in the view.
/// \param text Text to display.
///
void TextViewDialog::setText(const QString &text)
{
    ui->textEdit->setPlainText(text);
}

///
/// \brief Returns the text currently shown in the view.
///
QString TextViewDialog::text() const
{
    return ui->textEdit->toPlainText();
}

///
/// \brief Shows the given text modally in a text viewer.
/// \param parent Parent widget owning the dialog.
/// \param title Window title for the viewer.
/// \param text Text to display.
///
void TextViewDialog::showText(QWidget *parent, const QString &title, const QString &text)
{
    TextViewDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.setText(text);
    dialog.exec();
}

///
/// \brief Copies the displayed text to the clipboard.
///
void TextViewDialog::copyText()
{
    QApplication::clipboard()->setText(ui->textEdit->toPlainText());
}
