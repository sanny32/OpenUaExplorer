// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file settingsdialog.cpp
/// \brief Implements the application settings dialog.
///

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

#include "application.h"
#include "appsettings.h"
#include "apptheme.h"
#include "dialogs/settingsdialog.h"

///
/// \brief Builds the settings dialog and loads the current preferences.
/// \param parent Parent widget.
///
SettingsDialog::SettingsDialog(QWidget *parent)
    : AppBaseDialog(parent)
{
    setupUi();
    loadSettings();
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
/// \brief Creates the dialog widgets and wires the dialog buttons.
///
void SettingsDialog::setupUi()
{
    setWindowTitle(tr("Settings"));

    auto *layout = new QVBoxLayout(this);

    auto *appearanceGroup = new QGroupBox(tr("Appearance"), this);
    auto *appearanceLayout = new QVBoxLayout(appearanceGroup);
    _systemThemeButton = new QRadioButton(tr("Follow system theme"), appearanceGroup);
    _lightThemeButton = new QRadioButton(tr("Light"), appearanceGroup);
    _darkThemeButton = new QRadioButton(tr("Dark"), appearanceGroup);
    appearanceLayout->addWidget(_systemThemeButton);
    appearanceLayout->addWidget(_lightThemeButton);
    appearanceLayout->addWidget(_darkThemeButton);
    appearanceGroup->setVisible(theApp()->theme().isManualToggleSupported());
    layout->addWidget(appearanceGroup);

    auto *layoutGroup = new QGroupBox(tr("Window Layout"), this);
    auto *layoutGroupLayout = new QVBoxLayout(layoutGroup);
    _restoreLayoutCheck = new QCheckBox(tr("Restore window layout on startup"), layoutGroup);
    auto *resetButton = new QPushButton(tr("Restore Default Layout"), layoutGroup);
    layoutGroupLayout->addWidget(_restoreLayoutCheck);
    layoutGroupLayout->addWidget(resetButton);
    layout->addWidget(layoutGroup);

    auto *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, [this] {
        applyChanges();
        accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(resetButton, &QPushButton::clicked, this, [this] {
        _layoutResetRequested = true;
        applyChanges();
        AppSettings().clearLayout();
        accept();
    });
}

///
/// \brief Pre-selects the dialog controls from the stored preferences.
///
void SettingsDialog::loadSettings()
{
    AppSettings settings;
    switch (settings.themeMode()) {
    case AppSettings::ThemeMode::Light:
        _lightThemeButton->setChecked(true);
        break;
    case AppSettings::ThemeMode::Dark:
        _darkThemeButton->setChecked(true);
        break;
    case AppSettings::ThemeMode::System:
        _systemThemeButton->setChecked(true);
        break;
    }
    _restoreLayoutCheck->setChecked(settings.restoreLayoutOnStartup());
}

///
/// \brief Persists the chosen preferences and applies the theme immediately.
///
void SettingsDialog::applyChanges()
{
    AppSettings settings;
    settings.setRestoreLayoutOnStartup(_restoreLayoutCheck->isChecked());

    if (theApp()->theme().isManualToggleSupported()) {
        AppSettings::ThemeMode mode = AppSettings::ThemeMode::System;
        if (_lightThemeButton->isChecked())
            mode = AppSettings::ThemeMode::Light;
        else if (_darkThemeButton->isChecked())
            mode = AppSettings::ThemeMode::Dark;
        theApp()->theme().setColorSchemePreference(mode);
    }
}
