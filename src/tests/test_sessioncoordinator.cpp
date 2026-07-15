// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_sessioncoordinator.cpp
/// \brief Tests session restoration cursor handling.
///

#include <QGuiApplication>
#include <QMenu>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QWidget>

#include "application.h"
#include "opcua/connectioncontroller.h"
#include "opcua/connectionprofilestore.h"
#include "opcua/opcuabackend.h"
#include "opcua/recentconnectionstore.h"
#include "session/sessionstore.h"
#include "sessioncoordinator.h"
#include "settingsstore.h"

class SessionFakeBackend : public OpcUaBackend
{
    Q_OBJECT

public:
    using OpcUaBackend::OpcUaBackend;

    bool isAvailable() const override { return true; }
    QStringList availableBackends() const override { return {QStringLiteral("fake")}; }
    OpcUaConnectionState state() const override { return currentState; }
    QString lastError() const override { return {}; }
    void setCertificateTrustDecider(CertificateTrustDecider *) override {}
    void discoverEndpoints(const QString &, const QString &, int) override
    {
        setState(OpcUaConnectionState::Discovering);
    }
    void connectToEndpoint(const ConnectionProfile &, const QString &,
                           const QString &) override {}
    void disconnectFromEndpoint() override {}
    void browse(const QString &) override {}
    void browseReferences(const QString &) override {}
    void readNode(const QString &) override {}
    void readValues(const QStringList &) override {}
    void writeValue(const QString &, const QVariant &, int) override {}

    void setState(OpcUaConnectionState state)
    {
        currentState = state;
        emit stateChanged(state);
    }

    OpcUaConnectionState currentState = OpcUaConnectionState::Disconnected;
};

class TestSessionCoordinator : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();
    void openSessionUsesWaitCursorUntilConnectionFails();

private:
    QTemporaryDir _settingsDirectory;
};

void TestSessionCoordinator::initTestCase()
{
    QVERIFY(_settingsDirectory.isValid());
    QCoreApplication::setOrganizationName(QStringLiteral("OpenUaExplorerSessionCoordinatorTests"));
    QCoreApplication::setApplicationName(QStringLiteral("OpenUaExplorerSessionCoordinatorTests"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       _settingsDirectory.path());
}

void TestSessionCoordinator::cleanup()
{
    while (QGuiApplication::overrideCursor())
        QGuiApplication::restoreOverrideCursor();
    SettingsStore settings;
    settings.clear();
}

void TestSessionCoordinator::openSessionUsesWaitCursorUntilConnectionFails()
{
    QTemporaryDir sessionsDirectory;
    QVERIFY(sessionsDirectory.isValid());
    const QString path = sessionsDirectory.filePath(QStringLiteral("waiting.ouas"));

    SessionData session;
    session.profile.id = QStringLiteral("waiting-profile");
    session.profile.name = QStringLiteral("Waiting");
    session.profile.endpointUrl = QStringLiteral("opc.tcp://waiting.invalid:4840");
    session.profile.authentication = ConnectionProfile::Authentication::Anonymous;
    QVERIFY(SessionStore::save(path, session));

    SessionFakeBackend backend;
    SecretStore secrets;
    ConnectionProfileStore profiles;
    RecentConnectionStore recents;
    ConnectionController controller(&backend, &secrets, &profiles, &recents);
    QWidget window;
    QMenu recentSessionsMenu(&window);

    SessionCoordinatorContext context;
    context.window = &window;
    context.recentSessionsMenu = &recentSessionsMenu;
    context.connectionController = &controller;
    context.backend = &backend;
    SessionCoordinator coordinator(context);

    QVERIFY(!QGuiApplication::overrideCursor());
    coordinator.openSessionFromFile(path);

    QVERIFY(QGuiApplication::overrideCursor());
    QCOMPARE(QGuiApplication::overrideCursor()->shape(), Qt::WaitCursor);

    backend.setState(OpcUaConnectionState::Disconnected);
    QVERIFY(!QGuiApplication::overrideCursor());
}

int main(int argc, char *argv[])
{
    Application app(argc, argv);
    TestSessionCoordinator test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_sessioncoordinator.moc"
