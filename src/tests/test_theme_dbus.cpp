// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusVariant>
#include <QSignalSpy>
#include <QTest>

#include "application.h"

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

bool TestThemeDBus::sendPortalSettingChanged(uint value, const QString &group,
                                             const QString &key)
{
    QDBusMessage message = QDBusMessage::createSignal(
        QStringLiteral("/org/freedesktop/portal/desktop"),
        QStringLiteral("org.freedesktop.portal.Settings"),
        QStringLiteral("SettingChanged"));
    message << group << key << QDBusVariant(QVariant(value));
    return QDBusConnection::sessionBus().send(message);
}

void TestThemeDBus::portalSettingChangedSwitchesTheme()
{
    AppTheme &theme = theApp()->theme();
    QSignalSpy spy(&theme, &AppTheme::colorSchemeChanged);
    QVERIFY(spy.isValid());

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QVERIFY(!theme.isDark());
    QVERIFY(sendPortalSettingChanged(1));
    QTest::qWait(100);
    QCOMPARE(spy.size(), 0);
    QVERIFY(!theme.isDark());
#else
    const int beforeDark = spy.size();
    QVERIFY(sendPortalSettingChanged(1));
    QTRY_VERIFY(spy.size() > beforeDark);
    QVERIFY(theme.isDark());

    const int beforeLight = spy.size();
    QVERIFY(sendPortalSettingChanged(2));
    QTRY_VERIFY(spy.size() > beforeLight);
    QVERIFY(!theme.isDark());
#endif
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

int main(int argc, char *argv[])
{
    Application app(argc, argv);
    TestThemeDBus test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_theme_dbus.moc"
