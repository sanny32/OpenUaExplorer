// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file apptheme.h
/// \brief Declares the application theme manager.
///

#pragma once

#include <QObject>
#ifdef HAS_QTDBUS
#include <QDBusVariant>
#endif

class AppTheme : public QObject
{
    Q_OBJECT

public:
    explicit AppTheme(QObject *parent = nullptr);

    bool isDark() const;
    bool isManualToggleSupported() const;
    void toggle();
    void applyInitialScheme();

signals:
    void colorSchemeChanged();

private:
    void applyColorScheme(bool dark);

    bool _dark = false;
    bool _manualToggleSupported = false;

#ifdef HAS_QTDBUS
    Q_SLOT void onPortalSettingChanged(const QString &group, const QString &key,
                                       const QDBusVariant &value);
#endif
};
