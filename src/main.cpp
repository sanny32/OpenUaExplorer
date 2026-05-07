// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file main.cpp
/// \brief Defines the application entry point.
///

#include <QApplication>
#include <QStyleHints>
#ifdef HAS_QTDBUS
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusVariant>
#endif

#include "appstyle.h"
#include "mainwindow.h"

#ifdef HAS_QTDBUS
static void applyPortalColorScheme()
{
    QDBusMessage msg = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.portal.Desktop"),
        QStringLiteral("/org/freedesktop/portal/desktop"),
        QStringLiteral("org.freedesktop.portal.Settings"),
        QStringLiteral("Read"));
    msg << QStringLiteral("org.freedesktop.appearance") << QStringLiteral("color-scheme");

    const QDBusMessage reply = QDBusConnection::sessionBus().call(msg);
    if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().isEmpty())
        return;

    // reply is variant<variant<uint>>
    const QDBusVariant outer = reply.arguments().first().value<QDBusVariant>();
    const uint scheme = outer.variant().value<QDBusVariant>().variant().toUInt();
    if (scheme == 1)
        QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Dark);
    else if (scheme == 2)
        QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
}
#endif

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName(APP_NAME);
    a.setApplicationVersion(APP_VERSION);
    a.setStyle(new AppStyle(a.style()));

#ifdef HAS_QTDBUS
    applyPortalColorScheme();
#endif

    MainWindow window;
    window.show();

    return a.exec();
}
