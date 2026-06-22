// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file settingsdialog.cpp
/// \brief Implements the application settings dialog.
///

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLoggingCategory>
#include <QPushButton>
#include <QVBoxLayout>
#include <QVector>

#include "application.h"
#include "appsettings.h"
#include "apptheme.h"
#include "dialogs/settingsdialog.h"
#include "ui_settingsdialog.h"

///
/// \brief Builds the settings dialog and loads the current preferences.
/// \param parent Parent widget.
///
SettingsDialog::SettingsDialog(QWidget *parent)
    : AppBaseDialog(parent)
    , ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    ui->appearanceGroup->setVisible(theApp()->theme().isManualToggleSupported());

    setupLogCategories();
    loadSettings();

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, [this] {
        applyChanges();
        accept();
    });
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(ui->resetButton, &QPushButton::clicked, this, [this] {
        _layoutResetRequested = true;
        applyChanges();
        AppSettings().clearLayout();
        accept();
    });
}

///
/// \brief Destroys the dialog and its generated UI.
///
SettingsDialog::~SettingsDialog()
{
    delete ui;
}

///
/// \brief Reports whether the user asked to restore the default window layout.
/// \return True when the default layout should be re-applied.
///
bool SettingsDialog::layoutResetRequested() const
{
    return _layoutResetRequested;
}

///
/// \brief Creates a checkbox for every configurable open62541 logging category.
///
void SettingsDialog::setupLogCategories()
{
    const QVector<AppSettings::LogCategory> categories = AppSettings::availableLogCategories();
    for (const AppSettings::LogCategory &category : categories) {
        auto *check = new QCheckBox(category.displayName, ui->logGroup);
        check->setToolTip(category.categoryName);
        ui->logGroupLayout->addWidget(check);
        _logCategoryChecks.insert(category.key, check);
    }
}

///
/// \brief Pre-selects the dialog controls from the stored preferences.
///
void SettingsDialog::loadSettings()
{
    AppSettings settings;
    switch (settings.themeMode()) {
    case AppSettings::ThemeMode::Light:
        ui->lightThemeButton->setChecked(true);
        break;
    case AppSettings::ThemeMode::Dark:
        ui->darkThemeButton->setChecked(true);
        break;
    case AppSettings::ThemeMode::System:
        ui->systemThemeButton->setChecked(true);
        break;
    }
    ui->restoreLayoutCheck->setChecked(settings.restoreLayoutOnStartup());

    const QHash<QString, bool> states = settings.logCategoryStates();
    for (auto it = _logCategoryChecks.cbegin(); it != _logCategoryChecks.cend(); ++it)
        it.value()->setChecked(states.value(it.key(), true));
}

///
/// \brief Persists the chosen preferences and applies the theme immediately.
///
void SettingsDialog::applyChanges()
{
    AppSettings settings;
    settings.setRestoreLayoutOnStartup(ui->restoreLayoutCheck->isChecked());

    QHash<QString, bool> states;
    for (auto it = _logCategoryChecks.cbegin(); it != _logCategoryChecks.cend(); ++it)
        states.insert(it.key(), it.value()->isChecked());
    settings.setLogCategoryStates(states);
    QLoggingCategory::setFilterRules(settings.logFilterRules());

    if (theApp()->theme().isManualToggleSupported()) {
        AppSettings::ThemeMode mode = AppSettings::ThemeMode::System;
        if (ui->lightThemeButton->isChecked())
            mode = AppSettings::ThemeMode::Light;
        else if (ui->darkThemeButton->isChecked())
            mode = AppSettings::ThemeMode::Dark;
        theApp()->theme().setColorSchemePreference(mode);
    }
}
