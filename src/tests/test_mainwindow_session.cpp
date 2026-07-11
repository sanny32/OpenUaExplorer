// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include <QAction>
#include <QFile>
#include <QMenu>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

#include "application.h"
#include "mainwindow.h"
#include "session/recentsessionstore.h"

class TestMainWindowSession : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void titleStartsAsUntitled();
    void recentSessionsMenuShowsExistingFiles();

private:
    QTemporaryDir _settingsDirectory;
};

void TestMainWindowSession::initTestCase()
{
    QVERIFY(_settingsDirectory.isValid());
    QCoreApplication::setOrganizationName(QStringLiteral("OpenUaExplorerSessionTests"));
    QCoreApplication::setApplicationName(QStringLiteral("OpenUaExplorerSessionTests"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       _settingsDirectory.path());
}

void TestMainWindowSession::cleanup()
{
    QSettings settings;
    settings.clear();
}

void TestMainWindowSession::titleStartsAsUntitled()
{
    MainWindow window;

    QVERIFY(window.windowTitle().contains(QStringLiteral("Open UaExplorer")));
    QVERIFY(window.windowTitle().contains(QStringLiteral("Untitled")));
}

void TestMainWindowSession::recentSessionsMenuShowsExistingFiles()
{
    QTemporaryDir sessionsDir;
    QVERIFY(sessionsDir.isValid());

    const QString existingPath = sessionsDir.filePath(QStringLiteral("demo.ouas"));
    QFile existingFile(existingPath);
    QVERIFY(existingFile.open(QIODevice::WriteOnly | QIODevice::Text));
    existingFile.write("{}");
    existingFile.close();

    RecentSessionStore store;
    store.record(sessionsDir.filePath(QStringLiteral("missing.ouas")));
    store.record(existingPath);

    MainWindow window;
    auto *menu = window.findChild<QMenu *>(QStringLiteral("menuRecentSessions"));
    QVERIFY(menu);

    const QList<QAction *> actions = menu->actions();
    QCOMPARE(actions.size(), 1);
    QCOMPARE(actions.first()->text(), QStringLiteral("demo"));
    QCOMPARE(actions.first()->data().toString(), existingPath);
}

int main(int argc, char *argv[])
{
    Application app(argc, argv);
    TestMainWindowSession test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_mainwindow_session.moc"
