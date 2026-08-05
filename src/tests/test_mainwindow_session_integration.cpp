// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_mainwindow_session_integration.cpp
/// \brief Drives the real window through opening a session that authenticates with a password.
///
/// The window builds its own controller, coordinators and dialogs, so this is the only place
/// where the whole restore path is exercised as the user sees it: the session file asks for a
/// password, the credentials dialog collects it, and the workspace comes back on a real
/// (Python asyncua) server. When Python or the asyncua package is unavailable, or no OPC UA
/// backend is installed, the test skips itself so it stays CI-friendly.
///

#include <QApplication>
#include <QDialog>
#include <QElapsedTimer>
#include <QLineEdit>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QUuid>

#include "application.h"
#include "appsettings.h"
#include "mainwindow.h"
#include "opcua/opcuabackend.h"
#include "opcua/opcuatypes.h"
#include "opcua/qtopcuabackend.h"
#include "opcua/secretstore.h"
#include "opcuatestserver.h"
#include "session/sessionstore.h"
#include "settingsstore.h"

namespace {

constexpr auto userName = "operator";
constexpr auto userPassword = "s3cret";

///
/// \brief Answers the next credentials dialog with a password, as the user would.
/// \param password Password to type into the dialog.
/// \param typed Set to true once the dialog was answered.
/// \param attemptsLeft Event-loop turns still spent waiting for the dialog to appear.
///
void answerCredentialsDialog(const QString &password, bool *typed, int attemptsLeft = 600)
{
    QTimer::singleShot(50, qApp, [password, typed, attemptsLeft]() {
        auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!dialog) {
            if (attemptsLeft > 0)
                answerCredentialsDialog(password, typed, attemptsLeft - 1);
            return;
        }
        auto *passwordEdit = dialog->findChild<QLineEdit *>(QStringLiteral("passwordEdit"));
        if (!passwordEdit) {
            if (attemptsLeft > 0)
                answerCredentialsDialog(password, typed, attemptsLeft - 1);
            return;
        }
        passwordEdit->setText(password);
        *typed = true;
        dialog->accept();
    });
}

///
/// \brief Spins the event loop until the predicate holds or the timeout expires.
/// \param predicate Condition to wait for.
/// \param timeoutMs Time budget.
/// \return True when the predicate held before the timeout.
///
template <typename Predicate>
bool waitFor(Predicate predicate, int timeoutMs)
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeoutMs) {
        if (predicate())
            return true;
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        QTest::qWait(20);
    }
    return predicate();
}

} // namespace

///
/// \brief Tests the session restore path of the assembled window against a real server.
///
class TestMainWindowSessionIntegration : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void openedSessionAsksForThePasswordAndConnects();
    void typedCredentialsLeaveTheSessionModified();
    void refusedPasswordIsRetypedAndMarksTheSessionModified();
    void sessionRestoredAtStartupIsMarkedModifiedAfterTyping();

private:
    bool writeUsernameSession(const QString &path, const QString &discoveryUrl,
                              const QString &profileId);

    QTemporaryDir _settingsDirectory;
};

void TestMainWindowSessionIntegration::initTestCase()
{
    QVERIFY(_settingsDirectory.isValid());
    QCoreApplication::setOrganizationName(
        QStringLiteral("OpenUaExplorerSessionIntegrationTests"));
    QCoreApplication::setApplicationName(
        QStringLiteral("OpenUaExplorerSessionIntegrationTests"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, _settingsDirectory.path());
}

void TestMainWindowSessionIntegration::cleanup()
{
    SettingsStore settings;
    settings.clear();
}

///
/// \brief Writes a session file bound to the server's unsecured username endpoint.
///
/// The endpoint is taken from discovery so the stored policy, mode and URL are exactly the
/// ones the connection manager matches against later.
///
/// \param path Session file to write.
/// \param discoveryUrl Server discovery URL.
/// \param profileId Identifier the session profile carries.
/// \return True when an unsecured username endpoint was found and the file was written.
///
bool TestMainWindowSessionIntegration::writeUsernameSession(const QString &path,
                                                            const QString &discoveryUrl,
                                                            const QString &profileId)
{
    QtOpcUaBackend discovery;
    QSignalSpy discoverSpy(&discovery, &OpcUaBackend::endpointsDiscovered);
    discovery.discoverEndpoints(discoveryUrl, QStringLiteral("open62541"), 10000);
    if (!discoverSpy.wait(15000))
        return false;

    const QList<QVariant> arguments = discoverSpy.takeFirst();
    if (!arguments.at(1).toString().isEmpty())
        return false;

    const auto endpoints = arguments.at(0).value<QList<EndpointInfo>>();
    for (const EndpointInfo &candidate : endpoints) {
        if (!candidate.securityPolicy.endsWith(QStringLiteral("#None"))
            || !candidate.supportsUsername) {
            continue;
        }

        SessionData session;
        session.profile.id = profileId;
        session.profile.name = candidate.endpointUrl;
        session.profile.backend = QStringLiteral("open62541");
        session.profile.endpointUrl = candidate.endpointUrl;
        session.profile.securityPolicy = candidate.securityPolicy;
        session.profile.securityMode = candidate.securityModeValue;
        session.profile.authentication = ConnectionProfile::Authentication::Username;
        session.profile.username = QLatin1String(userName);
        return SessionStore::save(path, session);
    }
    return false;
}

///
/// \brief Opening a session whose profile needs a password prompts for it and connects.
///
void TestMainWindowSessionIntegration::openedSessionAsksForThePasswordAndConnects()
{
    QtOpcUaBackend probe;
    if (!probe.isAvailable())
        QSKIP("No OPC UA backend is available.");

    OpcUaTestServer server;
    QString skipReason;
    if (!server.start({QStringLiteral("--port"), QStringLiteral("48404"),
                       QStringLiteral("--user"), QLatin1String(userName),
                       QStringLiteral("--password"), QLatin1String(userPassword)},
                      &skipReason)) {
        QSKIP(qPrintable(skipReason));
    }

    QTemporaryDir sessionsDirectory;
    QVERIFY(sessionsDirectory.isValid());
    const QString path = sessionsDirectory.filePath(QStringLiteral("username.ouas"));
    QVERIFY2(writeUsernameSession(path, server.endpoint(), QStringLiteral("integration-open")),
             "No unsecured username endpoint was advertised.");

    MainWindow window;
    auto *backend = window.findChild<OpcUaBackend *>();
    QVERIFY(backend);

    bool typed = false;
    answerCredentialsDialog(QLatin1String(userPassword), &typed);
    window.openSessionFile(path);

    QVERIFY2(waitFor([&typed]() { return typed; }, 20000),
             "The credentials dialog never appeared.");
    QVERIFY2(waitFor([backend]() {
        return backend->state() == OpcUaConnectionState::Connected;
    }, 30000), "The session did not connect with the entered password.");
}

///
/// \brief A session restored with a hand-typed password stays marked as changed.
///
void TestMainWindowSessionIntegration::typedCredentialsLeaveTheSessionModified()
{
    QtOpcUaBackend probe;
    if (!probe.isAvailable())
        QSKIP("No OPC UA backend is available.");

    OpcUaTestServer server;
    QString skipReason;
    if (!server.start({QStringLiteral("--port"), QStringLiteral("48405"),
                       QStringLiteral("--user"), QLatin1String(userName),
                       QStringLiteral("--password"), QLatin1String(userPassword)},
                      &skipReason)) {
        QSKIP(qPrintable(skipReason));
    }

    QTemporaryDir sessionsDirectory;
    QVERIFY(sessionsDirectory.isValid());
    const QString path = sessionsDirectory.filePath(QStringLiteral("modified.ouas"));
    QVERIFY2(writeUsernameSession(path, server.endpoint(),
                                  QStringLiteral("integration-modified")),
             "No unsecured username endpoint was advertised.");

    MainWindow window;
    auto *backend = window.findChild<OpcUaBackend *>();
    QVERIFY(backend);

    bool typed = false;
    answerCredentialsDialog(QLatin1String(userPassword), &typed);
    window.openSessionFile(path);

    QVERIFY2(waitFor([backend]() {
        return backend->state() == OpcUaConnectionState::Connected;
    }, 30000), "The session did not connect with the entered password.");
    QVERIFY(typed);

    // The window re-evaluates the marker on a timer, so give that turn a chance to run.
    QVERIFY2(waitFor([&window]() { return window.isWindowModified(); }, 5000),
             "The session was not marked as changed after the password was typed in.");
}

///
/// \brief A stored password the server refuses is asked for again, and the session is changed.
///
/// This is the path a password change leaves behind: the credential store still holds the old
/// secret, the server turns it down, and the session only comes back after it is retyped.
///
void TestMainWindowSessionIntegration::refusedPasswordIsRetypedAndMarksTheSessionModified()
{
    QtOpcUaBackend probe;
    if (!probe.isAvailable())
        QSKIP("No OPC UA backend is available.");

    OpcUaTestServer server;
    QString skipReason;
    if (!server.start({QStringLiteral("--port"), QStringLiteral("48406"),
                       QStringLiteral("--user"), QLatin1String(userName),
                       QStringLiteral("--password"), QLatin1String(userPassword)},
                      &skipReason)) {
        QSKIP(qPrintable(skipReason));
    }

    QTemporaryDir sessionsDirectory;
    QVERIFY(sessionsDirectory.isValid());
    const QString path = sessionsDirectory.filePath(QStringLiteral("refused.ouas"));
    // Unique per run so the credential store of the machine keeps no test leftovers around.
    const QString profileId = QStringLiteral("ouaexp-integration-")
        + QUuid::createUuid().toString(QUuid::WithoutBraces);
    QVERIFY2(writeUsernameSession(path, server.endpoint(), profileId),
             "No unsecured username endpoint was advertised.");

    SecretStore secrets;
    QSignalSpy writeSpy(&secrets, &SecretStore::writeFinished);
    secrets.write(profileId, SecretStore::Secret::Password, QStringLiteral("outdated-secret"));
    QVERIFY(writeSpy.wait(5000));
    if (!writeSpy.takeFirst().at(2).toString().isEmpty())
        QSKIP("No usable keychain backend.");

    MainWindow window;
    auto *backend = window.findChild<OpcUaBackend *>();
    QVERIFY(backend);

    bool typed = false;
    answerCredentialsDialog(QLatin1String(userPassword), &typed);
    window.openSessionFile(path);

    const bool connected = waitFor([backend]() {
        return backend->state() == OpcUaConnectionState::Connected;
    }, 40000);
    const bool modified = connected
        && waitFor([&window]() { return window.isWindowModified(); }, 5000);

    QSignalSpy removeSpy(&secrets, &SecretStore::writeFinished);
    secrets.remove(profileId, SecretStore::Secret::Password);
    removeSpy.wait(5000);

    QVERIFY2(typed, "The refused password was not asked for again.");
    QVERIFY2(connected, "The session did not connect with the retyped password.");
    QVERIFY2(modified, "The session was not marked as changed after the password was retyped.");
}

///
/// \brief The session restored at startup is marked changed once its password is typed in.
///
/// Startup restoration reaches the same profile through a different door than File > Open,
/// so it gets its own walk-through.
///
void TestMainWindowSessionIntegration::sessionRestoredAtStartupIsMarkedModifiedAfterTyping()
{
    QtOpcUaBackend probe;
    if (!probe.isAvailable())
        QSKIP("No OPC UA backend is available.");

    OpcUaTestServer server;
    QString skipReason;
    if (!server.start({QStringLiteral("--port"), QStringLiteral("48407"),
                       QStringLiteral("--user"), QLatin1String(userName),
                       QStringLiteral("--password"), QLatin1String(userPassword)},
                      &skipReason)) {
        QSKIP(qPrintable(skipReason));
    }

    QTemporaryDir sessionsDirectory;
    QVERIFY(sessionsDirectory.isValid());
    const QString path = sessionsDirectory.filePath(QStringLiteral("startup.ouas"));
    QVERIFY2(writeUsernameSession(path, server.endpoint(), QStringLiteral("integration-startup")),
             "No unsecured username endpoint was advertised.");

    AppSettings settings;
    settings.setRestoreLastSessionOnStartup(true);
    settings.setLastSavedSessionPath(path);

    MainWindow window;
    auto *backend = window.findChild<OpcUaBackend *>();
    QVERIFY(backend);

    bool typed = false;
    answerCredentialsDialog(QLatin1String(userPassword), &typed);

    QVERIFY2(waitFor([backend]() {
        return backend->state() == OpcUaConnectionState::Connected;
    }, 40000), "The staged session did not connect with the entered password.");
    QVERIFY(typed);
    QVERIFY2(waitFor([&window]() { return window.isWindowModified(); }, 5000),
             "The restored session was not marked as changed after the password was typed in.");
}

int main(int argc, char *argv[])
{
    Application app(argc, argv);
    TestMainWindowSessionIntegration test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_mainwindow_session_integration.moc"
