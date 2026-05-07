// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file apptheme.cpp
/// \brief Implements the application theme manager.
///

#include <QStyle>
#include <QApplication>
#include <QGuiApplication>
#include <QPalette>
#include <QStyleHints>
#ifdef HAS_QTDBUS
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusVariant>
#endif

#include "apptheme.h"

static QPalette fusionPalette(bool darkAppearance)
{
    const QColor windowText = darkAppearance ? QColor(240, 240, 240) : Qt::black;
    const QColor backGround = darkAppearance ? QColor(50, 50, 50) : QColor(239, 239, 239);
    const QColor light = backGround.lighter(150);
    const QColor mid = backGround.darker(130);
    const QColor midLight = mid.lighter(110);
    const QColor base = darkAppearance ? backGround.darker(140) : Qt::white;
    const QColor disabledBase(backGround);
    const QColor dark = backGround.darker(150);
    const QColor darkDisabled = QColor(209, 209, 209).darker(110);
    const QColor text = darkAppearance ? windowText : Qt::black;
    const QColor highlight = QColor(48, 140, 198);
    const QColor highlightedText = darkAppearance ? windowText : Qt::white;
    const QColor disabledText = darkAppearance ? QColor(130, 130, 130) : QColor(190, 190, 190);
    const QColor button = backGround;
    const QColor shadow = dark.darker(135);
    const QColor disabledShadow = shadow.lighter(150);
    const QColor disabledHighlight(145, 145, 145);
    QColor placeholder = text;
    placeholder.setAlpha(128);

    QPalette p(windowText, backGround, light, dark, mid, text, base);
    p.setBrush(QPalette::Midlight,        midLight);
    p.setBrush(QPalette::Button,          button);
    p.setBrush(QPalette::Shadow,          shadow);
    p.setBrush(QPalette::HighlightedText, highlightedText);
    p.setBrush(QPalette::Disabled, QPalette::Text,       disabledText);
    p.setBrush(QPalette::Disabled, QPalette::WindowText, disabledText);
    p.setBrush(QPalette::Disabled, QPalette::ButtonText, disabledText);
    p.setBrush(QPalette::Disabled, QPalette::Base,       disabledBase);
    p.setBrush(QPalette::Disabled, QPalette::Dark,       darkDisabled);
    p.setBrush(QPalette::Disabled, QPalette::Shadow,     disabledShadow);
    p.setBrush(QPalette::Active,   QPalette::Highlight,  highlight);
    p.setBrush(QPalette::Inactive, QPalette::Highlight,  highlight);
    p.setBrush(QPalette::Disabled, QPalette::Highlight,  disabledHighlight);
    p.setBrush(QPalette::Active,   QPalette::Accent,     highlight);
    p.setBrush(QPalette::Inactive, QPalette::Accent,     highlight);
    p.setBrush(QPalette::Disabled, QPalette::Accent,     disabledHighlight);
    p.setBrush(QPalette::PlaceholderText, placeholder);
    if (darkAppearance)
        p.setBrush(QPalette::Link, highlight);
    return p;
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

    QDBusMessage msg = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.portal.Desktop"),
        QStringLiteral("/org/freedesktop/portal/desktop"),
        QStringLiteral("org.freedesktop.portal.Settings"),
        QStringLiteral("Read"));
    msg << QStringLiteral("org.freedesktop.appearance") << QStringLiteral("color-scheme");
    const QDBusMessage reply = QDBusConnection::sessionBus().call(msg);
    if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
        const QDBusVariant outer = reply.arguments().first().value<QDBusVariant>();
        const uint scheme = outer.variant().value<QDBusVariant>().variant().toUInt();
        _dark = (scheme == 1);
        _manualToggleSupported = true;
    }
#else
    const Qt::ColorScheme initial = QGuiApplication::styleHints()->colorScheme();
    _dark = (initial == Qt::ColorScheme::Dark);
    _manualToggleSupported = (initial != Qt::ColorScheme::Unknown);
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
            this, [this](Qt::ColorScheme scheme) {
                applyColorScheme(scheme == Qt::ColorScheme::Dark);
            });
#endif
}

///
/// \brief AppTheme::isDark
/// \return True if the current color scheme is dark.
///
bool AppTheme::isDark() const
{
    return _dark;
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
    applyColorScheme(!_dark);
}

///
/// \brief AppTheme::applyInitialScheme
///
void AppTheme::applyInitialScheme()
{
    if (_dark)
        applyColorScheme(true);
}

///
/// \brief AppTheme::applyColorScheme
/// \param dark True to apply the dark palette, false for light.
///
void AppTheme::applyColorScheme(bool dark)
{
    _dark = dark;
    if (QApplication::style()->name().compare(QLatin1String("fusion"), Qt::CaseInsensitive) == 0)
        QApplication::setPalette(fusionPalette(dark));
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
    if (group != QLatin1String("org.freedesktop.appearance")
        || key != QLatin1String("color-scheme")) {
        return;
    }

    // 1 = dark, 2 = light, 0 = no preference
    applyColorScheme(value.variant().toUInt() == 1);
}
#endif
