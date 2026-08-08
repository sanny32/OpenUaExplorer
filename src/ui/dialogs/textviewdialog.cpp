// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file textviewdialog.cpp
/// \brief Implements the reusable read-only text viewer dialog.
///

#include <QApplication>
#include <QClipboard>
#include <QFont>
#include <QFontDatabase>
#include <QTextOption>

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

    // The fixed-pitch system font carries its own point size, which is larger than the
    // one the rest of the application uses, so only the family is taken from it.
    QFont monospace = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    const QFont base = ui->textEdit->font();
    if (base.pointSizeF() > 0.0)
        monospace.setPointSizeF(base.pointSizeF());
    else
        monospace.setPixelSize(base.pixelSize());
    ui->textEdit->setFont(monospace);

    // A ByteString or a NodeId is one unbroken token, so word boundaries alone would
    // leave it running off the edge.
    ui->textEdit->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

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
