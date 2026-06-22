// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file settingsdialog.cpp
/// \brief Implements the application settings dialog.
///

#include <QAbstractButton>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLoggingCategory>
#include <QPushButton>
#include <QSizePolicy>
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

    setupThemeControls();
    setupLogCategories();
    loadSettings();
    setDirty(false);

    connect(ui->buttonBox, &QDialogButtonBox::accepted,
            this, &SettingsDialog::acceptChanges);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(ui->buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            this, &SettingsDialog::applyChanges);
    connect(ui->resetButton, &QPushButton::clicked,
            this, &SettingsDialog::requestLayoutReset);
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
/// \brief Wires the theme combo box and preview cards as one selection control.
///
void SettingsDialog::setupThemeControls()
{
    connect(ui->lightThemeButton, &QAbstractButton::clicked,
            this, &SettingsDialog::syncThemeComboFromCard);
    connect(ui->darkThemeButton, &QAbstractButton::clicked,
            this, &SettingsDialog::syncThemeComboFromCard);
    connect(ui->systemThemeButton, &QAbstractButton::clicked,
            this, &SettingsDialog::syncThemeComboFromCard);
    connect(ui->themeCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::selectThemeCard);
    connect(ui->themeCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::markDirty);
}

///
/// \brief Creates a checkbox for every configurable open62541 logging category.
///
void SettingsDialog::setupLogCategories()
{
    const QVector<AppSettings::LogCategory> categories = AppSettings::availableLogCategories();
    constexpr int rowsPerColumn = 4;
    for (int index = 0; index < categories.size(); ++index) {
        const AppSettings::LogCategory &category = categories.at(index);
        auto *check = new QCheckBox(category.displayName, ui->logCategoriesGroup);
        check->setObjectName(QStringLiteral("logCategory_%1").arg(category.key));
        check->setToolTip(category.categoryName);
        check->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        connect(check, &QCheckBox::toggled, this, &SettingsDialog::markDirty);
        ui->logGroupLayout->addWidget(check, index % rowsPerColumn,
                                      index / rowsPerColumn);
        _logCategoryChecks.insert(category.key, check);
    }
    for (int column = 0; column < 3; ++column)
        ui->logGroupLayout->setColumnStretch(column, 1);
}

///
/// \brief Pre-selects the dialog controls from the stored preferences.
///
void SettingsDialog::loadSettings()
{
    AppSettings settings;
    setThemeSelection(settings.themeMode());

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

    QHash<QString, bool> states;
    for (auto it = _logCategoryChecks.cbegin(); it != _logCategoryChecks.cend(); ++it)
        states.insert(it.key(), it.value()->isChecked());
    settings.setLogCategoryStates(states);
    QLoggingCategory::setFilterRules(settings.logFilterRules());

    if (_layoutResetRequested)
        settings.clearLayout();

    if (theApp()->theme().isManualToggleSupported())
        theApp()->theme().setColorSchemePreference(selectedThemeMode());

    setDirty(false);
}

///
/// \brief Applies pending changes and closes the dialog successfully.
///
void SettingsDialog::acceptChanges()
{
    applyChanges();
    accept();
}

///
/// \brief Marks the main-window layout for restoration when changes are accepted.
///
void SettingsDialog::requestLayoutReset()
{
    _layoutResetRequested = true;
    markDirty();
}

///
/// \brief Updates the combo box after a theme preview card is clicked.
///
void SettingsDialog::syncThemeComboFromCard()
{
    if (ui->lightThemeButton->isChecked())
        ui->themeCombo->setCurrentIndex(0);
    else if (ui->darkThemeButton->isChecked())
        ui->themeCombo->setCurrentIndex(1);
    else
        ui->themeCombo->setCurrentIndex(2);
}

///
/// \brief Updates the checked preview card from the combo-box index.
/// \param index Theme combo-box index.
///
void SettingsDialog::selectThemeCard(int index)
{
    ui->lightThemeButton->setChecked(index == 0);
    ui->darkThemeButton->setChecked(index == 1);
    ui->systemThemeButton->setChecked(index == 2);
}

///
/// \brief Selects a stored theme mode in both theme controls.
/// \param mode Stored theme preference.
///
void SettingsDialog::setThemeSelection(AppSettings::ThemeMode mode)
{
    int index = 2;
    if (mode == AppSettings::ThemeMode::Light)
        index = 0;
    else if (mode == AppSettings::ThemeMode::Dark)
        index = 1;
    ui->themeCombo->setCurrentIndex(index);
    selectThemeCard(index);
}

///
/// \brief Returns the mode selected by the synchronized theme controls.
/// \return Selected theme preference.
///
AppSettings::ThemeMode SettingsDialog::selectedThemeMode() const
{
    if (ui->lightThemeButton->isChecked())
        return AppSettings::ThemeMode::Light;
    if (ui->darkThemeButton->isChecked())
        return AppSettings::ThemeMode::Dark;
    return AppSettings::ThemeMode::System;
}

///
/// \brief Enables Apply after a preference or layout action changes.
///
void SettingsDialog::markDirty()
{
    setDirty(true);
}

///
/// \brief Updates whether pending changes can be applied.
/// \param dirty Whether the dialog differs from the last applied state.
///
void SettingsDialog::setDirty(bool dirty)
{
    ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(dirty);
}
