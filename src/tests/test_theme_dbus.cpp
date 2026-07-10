// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_theme_dbus.cpp
/// \brief Tests that AppTheme reacts to freedesktop portal color-scheme signals over D-Bus.
///

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusVariant>
#include <QSignalSpy>
#include <QTest>

#include "application.h"

///
/// \brief Verifies portal SettingChanged handling; skips when no session bus is available.
///
class TestThemeDBus : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void portalSettingChangedSwitchesTheme();
    void unrelatedPortalSettingIsIgnored();

private:
    bool sendPortalSettingChanged(uint value,
                                  const QString &group =
                                      QStringLiteral("org.freedesktop.appearance"),
                                  const QString &key = QStringLiteral("color-scheme"));
};

void TestThemeDBus::initTestCase()
{
    if (!QDBusConnection::sessionBus().isConnected())
        QSKIP("No D-Bus session bus available.");
}

///
/// \brief Emits a portal SettingChanged signal on the session bus.
/// \param value Color-scheme value to broadcast.
/// \param group Settings namespace.
/// \param key Setting key.
/// \return True when the signal was queued for delivery.
///
bool TestThemeDBus::sendPortalSettingChanged(uint value, const QString &group,
                                             const QString &key)
{
    QDBusMessage message = QDBusMessage::createSignal(
        QStringLiteral("/org/freedesktop/portal/desktop"),
        QStringLiteral("org.freedesktop.portal.Settings"),
        QStringLiteral("SettingChanged"));
    message.setArguments({
        group,
        key,
        QVariant::fromValue(QDBusVariant(QVariant(value)))
    });
    return QDBusConnection::sessionBus().send(message);
}

void TestThemeDBus::portalSettingChangedSwitchesTheme()
{
    AppTheme &theme = theApp()->theme();
    QSignalSpy spy(&theme, &AppTheme::colorSchemeChanged);
    QVERIFY(spy.isValid());

    const int beforeDark = spy.size();
    QVERIFY(sendPortalSettingChanged(1));
    QTRY_VERIFY(spy.size() > beforeDark);
    QVERIFY(theme.isDark());

    const int beforeLight = spy.size();
    QVERIFY(sendPortalSettingChanged(2));
    QTRY_VERIFY(spy.size() > beforeLight);
    QVERIFY(!theme.isDark());
}

void TestThemeDBus::unrelatedPortalSettingIsIgnored()
{
    AppTheme &theme = theApp()->theme();
    QSignalSpy spy(&theme, &AppTheme::colorSchemeChanged);
    QVERIFY(spy.isValid());

    const bool wasDark = theme.isDark();
    QVERIFY(sendPortalSettingChanged(1, QStringLiteral("org.freedesktop.interface")));
    QTest::qWait(100);

    QCOMPARE(spy.size(), 0);
    QCOMPARE(theme.isDark(), wasDark);
}

///
/// \brief Runs the suite under a real Application so AppTheme's D-Bus wiring is live.
/// \param argc Argument count.
/// \param argv Argument vector.
/// \return Test exit code.
///
int main(int argc, char *argv[])
{
    Application app(argc, argv);
    TestThemeDBus test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_theme_dbus.moc"
