// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file appbasedialog.cpp
/// \brief Implements the themed base dialog.
///

#include <QEvent>
#include <QShowEvent>
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#include <QGuiApplication>
#include <QStyleHints>
#endif

#include "appicons.h"
#include "appbasedialog.h"

///
/// \brief AppBaseDialog::AppBaseDialog
/// \param parent
///
AppBaseDialog::AppBaseDialog(QWidget *parent)
    : QDialog(parent)
{
    updateWindowIcon();

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
            this, [this](Qt::ColorScheme) { updateWindowIcon(); });
#endif
}

///
/// \brief AppBaseDialog::changeEvent
/// \param event
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
/// \brief AppBaseDialog::showEvent
/// \param event
///
void AppBaseDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    updateWindowIcon();
}

///
/// \brief AppBaseDialog::updateWindowIcon
///
void AppBaseDialog::updateWindowIcon()
{
    setWindowIcon(AppIcons::themed("app.ico"));
}
