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
    ui->themeGroup->setVisible(theApp()->theme().isManualToggleSupported());

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
    connect(ui->lightThemeButton, &QAbstractButton::clicked,
            this, &SettingsDialog::markDirty);
    connect(ui->darkThemeButton, &QAbstractButton::clicked,
            this, &SettingsDialog::markDirty);
    connect(ui->systemThemeButton, &QAbstractButton::clicked,
            this, &SettingsDialog::markDirty);
    connect(ui->timestampModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::markDirty);
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
/// \brief Creates tabbed checkboxes for application and open62541 logging categories.
///
void SettingsDialog::setupLogCategories()
{
    populateLogCategoryLayout(ui->applicationLogGroupLayout,
                              AppSettings::availableApplicationLogCategories());
    QVector<AppSettings::LogCategory> open62541Categories =
        AppSettings::availableQtOpcUaLogCategories();
    const QVector<AppSettings::LogCategory> sdkCategories =
        AppSettings::availableOpen62541LogCategories();
    for (const AppSettings::LogCategory &category : sdkCategories)
        open62541Categories.append(category);
    populateLogCategoryLayout(ui->open62541LogGroupLayout, open62541Categories);
    for (int column = 0; column < 3; ++column)
        ui->applicationLogGroupLayout->setColumnStretch(column, 1);
    for (int column = 0; column < 3; ++column)
        ui->open62541LogGroupLayout->setColumnStretch(column, 1);
}

///
/// \brief Creates one logging-category checkbox and records it for loading and saving.
/// \param category Logging category to expose.
/// \return Checkbox bound to the category.
///
QCheckBox *SettingsDialog::createLogCategoryCheck(const AppSettings::LogCategory &category)
{
    auto *check = new QCheckBox(category.displayName, ui->logCategoriesGroup);
    check->setObjectName(QStringLiteral("logCategory_%1").arg(category.key));
    check->setToolTip(category.categoryName);
    check->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    connect(check, &QCheckBox::toggled, this, &SettingsDialog::markDirty);
    _logCategoryChecks.insert(category.key, check);
    return check;
}

///
/// \brief Adds logging-category checkboxes to a three-column grid.
/// \param layout Grid that receives the checkboxes.
/// \param categories Ordered category list.
///
void SettingsDialog::populateLogCategoryLayout(
    QGridLayout *layout, const QVector<AppSettings::LogCategory> &categories)
{
    constexpr int rowsPerColumn = 4;
    for (int index = 0; index < categories.size(); ++index) {
        QCheckBox *check = createLogCategoryCheck(categories.at(index));
        layout->addWidget(check, index % rowsPerColumn, index / rowsPerColumn);
    }
}

///
/// \brief Pre-selects the dialog controls from the stored preferences.
///
void SettingsDialog::loadSettings()
{
    AppSettings settings;
    setThemeSelection(settings.themeMode());
    setTimestampSelection(settings.timestampMode());

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

    theApp()->setTimestampMode(selectedTimestampMode());

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
/// \brief Updates the checked preview card from the theme index.
/// \param index Theme index.
///
void SettingsDialog::selectThemeCard(int index)
{
    ui->lightThemeButton->setChecked(index == 0);
    ui->darkThemeButton->setChecked(index == 1);
    ui->systemThemeButton->setChecked(index == 2);
}

///
/// \brief Selects the preview card for a stored theme mode.
/// \param mode Stored theme preference.
///
void SettingsDialog::setThemeSelection(AppSettings::ThemeMode mode)
{
    int index = 2;
    if (mode == AppSettings::ThemeMode::Light)
        index = 0;
    else if (mode == AppSettings::ThemeMode::Dark)
        index = 1;
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
/// \brief Selects the combo entry for a stored timestamp mode.
/// \param mode Stored timestamp preference.
///
void SettingsDialog::setTimestampSelection(AppSettings::TimestampMode mode)
{
    ui->timestampModeCombo->setCurrentIndex(
        mode == AppSettings::TimestampMode::Utc ? 1 : 0);
}

///
/// \brief Returns the timestamp mode selected in the combo box.
/// \return Selected timestamp preference.
///
AppSettings::TimestampMode SettingsDialog::selectedTimestampMode() const
{
    return ui->timestampModeCombo->currentIndex() == 1
        ? AppSettings::TimestampMode::Utc
        : AppSettings::TimestampMode::LocalTime;
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
