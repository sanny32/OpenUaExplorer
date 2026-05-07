// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

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

signals:
    void colorSchemeChanged();

private:
    void applyColorScheme(bool dark);

    bool _dark = false;

#ifdef HAS_QTDBUS
    Q_SLOT void onPortalSettingChanged(const QString &group, const QString &key,
                                       const QDBusVariant &value);
#endif
};
