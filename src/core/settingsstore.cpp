// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file settingsstore.cpp
/// \brief Implements the QSettings binding to the product-wide settings branch.
///

#include "settingsstore.h"

#ifdef Q_OS_WIN

namespace {
///
/// \brief Resolves the storage location for the current default format.
/// \return Registry branch of the product, or the redirected INI file path.
///
/// The registry branch is opened explicitly because the organization-only
/// QSettings constructor puts keys into an "OrganizationDefaults" subkey on
/// Windows. When tests switch the default format to INI, the location the
/// organization-only constructor would use (which honours QSettings::setPath()
/// redirection) is resolved instead.
///
QString storagePath()
{
    if (QSettings::defaultFormat() == QSettings::NativeFormat)
        return QStringLiteral("HKEY_CURRENT_USER\\Software\\") + QStringLiteral(APP_PRODUCT_NAME);
    return QSettings(QSettings::UserScope, QStringLiteral(APP_PRODUCT_NAME)).fileName();
}
}

///
/// \brief Constructs settings rooted at the product branch.
///
/// Opens the HKEY_CURRENT_USER\Software\Open UaExplorer registry branch
/// directly so keys are not nested in any subkey, or the redirected INI file
/// when tests have changed the default format.
///
SettingsStore::SettingsStore()
    : QSettings(storagePath(), QSettings::defaultFormat())
{
}

#else

///
/// \brief Constructs settings rooted at the product branch.
///
/// Passes the product name as the organization with no application name so
/// everything lands in one location per product: ~/.config/Open UaExplorer.conf
/// on Linux and the product plist on macOS. This constructor keeps the default
/// format, which lets tests redirect storage to a temporary directory via
/// QSettings::setDefaultFormat() and QSettings::setPath().
///
SettingsStore::SettingsStore()
    : QSettings(QSettings::UserScope, QStringLiteral(APP_PRODUCT_NAME))
{
}

#endif
