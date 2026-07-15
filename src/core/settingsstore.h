// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file settingsstore.h
/// \brief Declares the QSettings binding to the product-wide settings branch.
///

#pragma once

#include <QSettings>

///
/// \brief QSettings bound to the single product-wide settings branch.
///
/// Every persisted key lives directly under one "Open UaExplorer" branch
/// (HKEY_CURRENT_USER\Software\Open UaExplorer on Windows) instead of the
/// organization/application pair the default QSettings constructor nests
/// keys in. All code that persists state must create this class rather
/// than a plain QSettings.
///
class SettingsStore : public QSettings
{
public:
    ///
    /// \brief Constructs settings rooted at the product branch.
    ///
    SettingsStore();
};
