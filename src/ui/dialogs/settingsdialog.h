// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file settingsdialog.h
/// \brief Declares the application settings dialog.
///

#pragma once

#include "dialogs/appbasedialog.h"

class QCheckBox;
class QRadioButton;

///
/// \brief Dialog for editing application preferences (theme and window layout).
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
    /// \brief Reports whether the user asked to restore the default window layout.
    /// \return True when the default layout should be re-applied.
    ///
    bool layoutResetRequested() const;

private:
    void setupUi();
    void loadSettings();
    void applyChanges();

    bool          _layoutResetRequested = false;
    QRadioButton *_systemThemeButton = nullptr;
    QRadioButton *_lightThemeButton = nullptr;
    QRadioButton *_darkThemeButton = nullptr;
    QCheckBox    *_restoreLayoutCheck = nullptr;
};
