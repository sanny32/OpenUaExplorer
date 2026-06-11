// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file apptheme.cpp
/// \brief Implements the application theme manager.
///

#include <QApplication>
#include <QGuiApplication>
#include <QStyleHints>

#ifdef HAS_QTDBUS
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusVariant>
#endif

#include "appstyle.h"
#include "apptheme.h"
#include "fusionstyle.h"

namespace {

///
/// \brief colorSchemeFromDBus
/// \param value
/// \return
///
AppTheme::ColorScheme colorSchemeFromDBus(uint value)
{
    switch (value) {
    case 1: return AppTheme::ColorScheme::Dark;
    case 2: return AppTheme::ColorScheme::Light;
    default: return AppTheme::ColorScheme::Unknown;
    }
}
}

///
/// \brief AppTheme::AppTheme
/// \param parent
///
AppTheme::AppTheme(QObject *parent)
    : QObject(parent)
{
#ifdef HAS_QTDBUS
    QDBusConnection::sessionBus().connect(
        QString(),
        QStringLiteral("/org/freedesktop/portal/desktop"),
        QStringLiteral("org.freedesktop.portal.Settings"),
        QStringLiteral("SettingChanged"),
        this,
        SLOT(onPortalSettingChanged(QString, QString, QDBusVariant)));
#else
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    const Qt::ColorScheme applicationScheme = QGuiApplication::styleHints()->colorScheme();
    _scheme = applicationScheme == Qt::ColorScheme::Dark
        ? ColorScheme::Dark
        : applicationScheme == Qt::ColorScheme::Light
            ? ColorScheme::Light
            : ColorScheme::Unknown;
    _manualToggleSupported = (_scheme != ColorScheme::Unknown);

    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
            this, [this](Qt::ColorScheme scheme) {
        applyColorScheme(scheme == Qt::ColorScheme::Dark
            ? ColorScheme::Dark : ColorScheme::Light);
    });
#else
    _scheme = qApp->palette().color(QPalette::Window).lightness() < 128
        ? ColorScheme::Dark : ColorScheme::Light;
    _manualToggleSupported = true;
#endif
#endif
}

///
/// \brief AppTheme::isDark
/// \return True if the current color scheme is dark.
///
bool AppTheme::isDark() const
{
    return _scheme == ColorScheme::Dark;
}

///
/// \brief AppTheme::isManualToggleSupported
/// \return True if the user can manually toggle the color scheme.
///
bool AppTheme::isManualToggleSupported() const
{
    return _manualToggleSupported;
}

///
/// \brief AppTheme::toggle
///
void AppTheme::toggle()
{
    _manualSchemeOverriden = true;
    switch (_scheme) {
    case ColorScheme::Dark:
        applyColorScheme(ColorScheme::Light);
        break;
    case ColorScheme::Light:
        applyColorScheme(ColorScheme::Dark);
        break;
    default:
        break;
    }
}

///
/// \brief AppTheme::applyInitialScheme
///
void AppTheme::applyInitialScheme()
{
#ifdef HAS_QTDBUS
    QDBusMessage msg = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.portal.Desktop"),
        QStringLiteral("/org/freedesktop/portal/desktop"),
        QStringLiteral("org.freedesktop.portal.Settings"),
        QStringLiteral("Read"));
    msg << QStringLiteral("org.freedesktop.appearance") << QStringLiteral("color-scheme");
    const QDBusMessage reply = QDBusConnection::sessionBus().call(msg, QDBus::Block, 500);
    const QList<QVariant> arguments = reply.arguments();
    if (reply.type() == QDBusMessage::ReplyMessage && !arguments.isEmpty()) {
        QVariant v = arguments.first().value<QDBusVariant>().variant();
        if (v.canConvert<QDBusVariant>())
            v = v.value<QDBusVariant>().variant();
        const ColorScheme scheme = colorSchemeFromDBus(v.toUInt());
        if (scheme != ColorScheme::Unknown) {
            _manualToggleSupported = true;
            applyColorScheme(scheme);
            return;
        }
    }

    // portal returned 0 (no preference) — try impl backends directly
    const QStringList implBackends = {
        QStringLiteral("org.freedesktop.impl.portal.desktop.kde"),
        QStringLiteral("org.freedesktop.impl.portal.desktop.gnome"),
        QStringLiteral("org.freedesktop.impl.portal.desktop.gtk"),
    };
    const auto iface = QDBusConnection::sessionBus().interface();
    for (const QString &service : implBackends) {
        if (!iface || !iface->isServiceRegistered(service))
            continue;
        QDBusMessage msgImpl = QDBusMessage::createMethodCall(
            service,
            QStringLiteral("/org/freedesktop/portal/desktop"),
            QStringLiteral("org.freedesktop.impl.portal.Settings"),
            QStringLiteral("Read"));
        msgImpl << QStringLiteral("org.freedesktop.appearance") << QStringLiteral("color-scheme");
        const QDBusMessage replyImpl = QDBusConnection::sessionBus().call(msgImpl, QDBus::Block, 500);
        const QList<QVariant> argumentsImpl = replyImpl.arguments();
        if (replyImpl.type() == QDBusMessage::ReplyMessage && !argumentsImpl.isEmpty()) {
            QVariant v = argumentsImpl.first().value<QDBusVariant>().variant();
            if (v.canConvert<QDBusVariant>())
                v = v.value<QDBusVariant>().variant();
            const ColorScheme scheme = colorSchemeFromDBus(v.toUInt());
            _manualToggleSupported = true;
            applyColorScheme(scheme);
            return;
        }
    }
#endif
    applyColorScheme(_scheme);
}

///
/// \brief AppTheme::applyColorScheme
/// \param scheme The color scheme to apply.
///
void AppTheme::applyColorScheme(ColorScheme scheme)
{
    _scheme = scheme;

#ifdef Q_OS_WIN
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    QGuiApplication::styleHints()->setColorScheme(
        scheme == ColorScheme::Dark ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light);
#else
    QApplication::setPalette(FusionStyle::palette(_scheme == ColorScheme::Dark));
#endif
#else
    if (AppStyle::isFusionStyle())
        QApplication::setPalette(FusionStyle::palette(_scheme == ColorScheme::Dark));
#endif

    emit colorSchemeChanged();
}

#ifdef HAS_QTDBUS
///
/// \brief AppTheme::onPortalSettingChanged
/// \param group  D-Bus settings namespace.
/// \param key    Setting key name.
/// \param value  New setting value.
///
void AppTheme::onPortalSettingChanged(const QString &group, const QString &key,
                                      const QDBusVariant &value)
{
    if (_manualSchemeOverriden ||
        group != QLatin1String("org.freedesktop.appearance") ||
        key != QLatin1String("color-scheme"))
    {
        return;
    }

    const ColorScheme scheme = colorSchemeFromDBus(value.variant().toUInt());
    if (scheme != ColorScheme::Unknown)
        applyColorScheme(scheme);
}
#endif
