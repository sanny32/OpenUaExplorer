// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file apptheme.cpp
/// \brief Implements the application theme manager.
///

#include <QApplication>
#include <QGuiApplication>
#include <QStyleHints>

#include "apptheme.h"

#ifdef HAS_QTDBUS
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusVariant>
#endif

#include "appstyle.h"
#include "fusionstyle.h"

namespace {

///
/// \brief Reports whether Qt's native color-scheme API is usable on this build.
/// \return True on Qt 6 and newer, where the style-hints color scheme exists.
///
bool qtThemeApiAvailable()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return true;
#else
    return false;
#endif
}

///
/// \brief Forces the light Fusion palette, unconditionally on Windows.
///
void applyLightPalette()
{
#ifdef Q_OS_WIN
    QApplication::setPalette(FusionStyle::palette(false));
#else
    if (AppStyle::isFusionStyle())
        QApplication::setPalette(FusionStyle::palette(false));
#endif
}

///
/// \brief Applies the Fusion palette matching the scheme, when Fusion is active.
/// \param scheme Color scheme that selects the light or dark palette.
///
void applyFusionPalette(AppTheme::ColorScheme scheme)
{
    if (AppStyle::isFusionStyle())
        QApplication::setPalette(FusionStyle::palette(scheme == AppTheme::ColorScheme::Dark));
}

///
/// \brief Reports whether any color-scheme source (Qt API or D-Bus portal) exists.
/// \return True when the scheme can be detected at runtime.
///
bool colorSchemeAvailable()
{
    if (qtThemeApiAvailable())
        return true;

#ifdef HAS_QTDBUS
    return true;
#else
    return false;
#endif
}

#ifdef HAS_QTDBUS
///
/// \brief Maps a freedesktop portal color-scheme code to the application enum.
/// \param value Portal value: 1 = dark, 2 = light, anything else = unknown.
/// \return Corresponding color scheme.
///
AppTheme::ColorScheme colorSchemeFromDBus(uint value)
{
    switch (value) {
    case 1: return AppTheme::ColorScheme::Dark;
    case 2: return AppTheme::ColorScheme::Light;
    default: return AppTheme::ColorScheme::Unknown;
    }
}
#endif
}

///
/// \brief Constructs the theme manager and wires up system color-scheme detection.
/// \param parent Owning QObject.
///
AppTheme::AppTheme(QObject *parent)
    : QObject(parent)
{
    setupSystemColorScheme();
}

///
/// \brief Selects the color-scheme source: D-Bus portal, Qt style hints, or light fallback.
///
void AppTheme::setupSystemColorScheme()
{
    if (setupPortalColorScheme())
        return;

    if (!qtThemeApiAvailable()) {
        _scheme = ColorScheme::Light;
        return;
    }

    readStyleHintsColorScheme();
}

///
/// \brief Subscribes to freedesktop portal SettingChanged notifications.
/// \return True when D-Bus support is compiled in and the subscription was made.
///
bool AppTheme::setupPortalColorScheme()
{
#ifdef HAS_QTDBUS
    QDBusConnection::sessionBus().connect(
        QString(),
        QStringLiteral("/org/freedesktop/portal/desktop"),
        QStringLiteral("org.freedesktop.portal.Settings"),
        QStringLiteral("SettingChanged"),
        this,
        SLOT(onPortalSettingChanged(QString, QString, QDBusVariant)));
    return true;
#else
    return false;
#endif
}

///
/// \brief Reads the initial scheme from Qt style hints and tracks later changes.
///
void AppTheme::readStyleHintsColorScheme()
{
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
}

///
/// \brief Reports whether the dark scheme is active.
/// \return True if the current color scheme is dark.
///
bool AppTheme::isDark() const
{
    return _scheme == ColorScheme::Dark;
}

///
/// \brief Reports whether the user may switch schemes manually.
/// \return True if the user can manually toggle the color scheme.
///
bool AppTheme::isManualToggleSupported() const
{
    return _manualToggleSupported;
}

///
/// \brief Switches between light and dark, marking the scheme as manually overridden.
///
void AppTheme::toggle()
{
    if (!_manualToggleSupported)
        return;

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
/// \brief Applies the startup color scheme from the portal, Qt API, or light fallback.
///
void AppTheme::applyInitialScheme()
{
    if (applyPortalColorScheme())
        return;

    if (!qtThemeApiAvailable()) {
        applyColorScheme(ColorScheme::Light);
        return;
    }

    applyColorScheme(_scheme);
}

///
/// \brief Queries the portal (and impl backends) for the current scheme and applies it.
/// \return True when a scheme was read and applied.
///
bool AppTheme::applyPortalColorScheme()
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
            return true;
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
            return true;
        }
    }
#endif

    return false;
}

///
/// \brief Stores the scheme, applies it natively, and emits colorSchemeChanged().
/// \param scheme The color scheme to apply.
///
void AppTheme::applyColorScheme(ColorScheme scheme)
{
    if (!colorSchemeAvailable())
        scheme = ColorScheme::Light;

    _scheme = scheme;
    applyNativeColorScheme(_scheme);

    emit colorSchemeChanged();
}

///
/// \brief Pushes the scheme to the platform: Qt style hints on Windows, Fusion palette elsewhere.
/// \param scheme Color scheme to realise natively.
///
void AppTheme::applyNativeColorScheme(ColorScheme scheme)
{
    if (!qtThemeApiAvailable()) {
#ifdef HAS_QTDBUS
        applyFusionPalette(scheme);
#else
        applyLightPalette();
#endif
        return;
    }

#ifdef Q_OS_WIN
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    QGuiApplication::styleHints()->setColorScheme(
        scheme == ColorScheme::Dark ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light);
#endif
#else
    applyFusionPalette(scheme);
#endif
}

#ifdef HAS_QTDBUS
///
/// \brief Applies a portal color-scheme change unless the user has overridden it.
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
