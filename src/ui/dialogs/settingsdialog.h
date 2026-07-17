// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file settingsdialog.h
/// \brief Declares the application settings dialog.
///

#pragma once

#include <QHash>
#include <QString>

#include "appsettings.h"
#include "dialogs/appbasedialog.h"

class QCheckBox;
class QEvent;
class QGridLayout;

namespace Ui {
class SettingsDialog;
}

///
/// \brief Dialog for editing application preferences (theme, window layout and logging).
///
class SettingsDialog : public AppBaseDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the settings dialog and loads the current preferences.
    /// \param parent Parent widget.
    ///
    explicit SettingsDialog(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the dialog and its generated UI.
    ///
    ~SettingsDialog() override;

    ///
    /// \brief Reports whether the user asked to restore the default window layout.
    /// \return True when the default layout should be re-applied.
    ///
    bool layoutResetRequested() const;

private:
    void setupLogCategories();
    QCheckBox *createLogCategoryCheck(const AppSettings::LogCategory &category);
    void populateLogCategoryLayout(QGridLayout *layout,
                                   const QVector<AppSettings::LogCategory> &categories);
    void loadSettings();
    void applyChanges();
    void acceptChanges();
    void requestLayoutReset();
    void selectThemeCard(int index);
    void setThemeSelection(AppSettings::ThemeMode mode);
    AppSettings::ThemeMode selectedThemeMode() const;
    void setTimestampSelection(AppSettings::TimestampMode mode);
    AppSettings::TimestampMode selectedTimestampMode() const;
    void populateLanguageCombo();
    void setLanguageSelection(AppSettings::Language language);
    AppSettings::Language selectedLanguage() const;
    void markDirty();
    void setDirty(bool dirty);

protected:
    void changeEvent(QEvent *event) override;

private:
    Ui::SettingsDialog *ui;
    bool _layoutResetRequested = false;
    QHash<QString, QCheckBox *> _logCategoryChecks;
};
