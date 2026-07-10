// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file appbasedialog.cpp
/// \brief Implements the themed base dialog.
///

#include <QEvent>
#include <QShowEvent>
#include <QGuiApplication>
#include <QStyleHints>

#include "appicons.h"
#include "appbasedialog.h"

///
/// \brief Constructs the dialog and keeps its window icon in sync with the colour scheme.
/// \param parent Parent widget.
///
AppBaseDialog::AppBaseDialog(QWidget *parent)
    : QDialog(parent)
{
    updateWindowIcon();

    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
            this, [this](Qt::ColorScheme) { updateWindowIcon(); });
}

///
/// \brief Refreshes the window icon when the palette changes.
/// \param event Change event being handled.
///
void AppBaseDialog::changeEvent(QEvent *event)
{
    QDialog::changeEvent(event);

    if (event->type() == QEvent::PaletteChange
        || event->type() == QEvent::ApplicationPaletteChange) {
        updateWindowIcon();
    }
}

///
/// \brief Refreshes the window icon as the dialog is shown.
/// \param event Show event being handled.
///
void AppBaseDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    updateWindowIcon();
}

///
/// \brief Sets the window icon to the themed application icon.
///
void AppBaseDialog::updateWindowIcon()
{
    setWindowIcon(AppIcons::themed("app.ico"));
}
