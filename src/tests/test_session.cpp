// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_session.cpp
/// \brief Tests the working-session file store.
///

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include "session/sessionstore.h"

///
/// \brief Unit tests for SessionStore serialization.
///
class TestSession : public QObject
{
    Q_OBJECT

private slots:
    void roundTripPreservesWorkspace();
    void fileOmitsSecrets();
    void loadRejectsMissingConnection();
};

///
/// \brief Builds a representative session for the tests.
/// \return Populated session data.
///
static SessionData sampleSession()
{
    SessionData data;
    data.profile.id = QStringLiteral("profile-1");
    data.profile.name = QStringLiteral("Demo");
    data.profile.endpointUrl = QStringLiteral("opc.tcp://demo:4840");
    data.profile.securityPolicy = QStringLiteral("Basic256Sha256");
    data.profile.securityMode = 3;
    data.profile.authentication = ConnectionProfile::Authentication::Username;
    data.profile.username = QStringLiteral("operator");
    data.profile.requestTimeoutMs = 12345;

    SubscriptionItem fast;
    fast.name = QStringLiteral("Fast");
    fast.publishingInterval = 250.0;
    data.subscriptions.append(fast);

    data.dataAccessNodes.append({QStringLiteral("ns=2;s=Temp"), QStringLiteral("Fast")});
    data.dataAccessNodes.append({QStringLiteral("ns=2;s=Idle"), QString()});
    data.trendNodes = {QStringLiteral("ns=2;s=Temp")};
    return data;
}

///
/// \brief Saving then loading reproduces every serialized field.
///
void TestSession::roundTripPreservesWorkspace()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("s.ouas"));

    const SessionData original = sampleSession();
    QVERIFY(SessionStore::save(path, original));

    SessionData loaded;
    QVERIFY(SessionStore::load(path, loaded));

    QCOMPARE(loaded.profile.endpointUrl, original.profile.endpointUrl);
    QCOMPARE(loaded.profile.securityPolicy, original.profile.securityPolicy);
    QCOMPARE(loaded.profile.securityMode, original.profile.securityMode);
    QCOMPARE(int(loaded.profile.authentication), int(original.profile.authentication));
    QCOMPARE(loaded.profile.username, original.profile.username);
    QCOMPARE(loaded.profile.requestTimeoutMs, original.profile.requestTimeoutMs);

    QCOMPARE(loaded.subscriptions.size(), 1);
    QCOMPARE(loaded.subscriptions.first().name, QStringLiteral("Fast"));
    QCOMPARE(loaded.subscriptions.first().publishingInterval, 250.0);

    QCOMPARE(loaded.dataAccessNodes.size(), 2);
    QCOMPARE(loaded.dataAccessNodes.at(0).nodeId, QStringLiteral("ns=2;s=Temp"));
    QCOMPARE(loaded.dataAccessNodes.at(0).subscriptionName, QStringLiteral("Fast"));
    QCOMPARE(loaded.dataAccessNodes.at(1).subscriptionName, QString());

    QCOMPARE(loaded.trendNodes, original.trendNodes);
}

///
/// \brief The session file never contains password material.
///
void TestSession::fileOmitsSecrets()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("s.ouas"));

    SessionData data = sampleSession();
    data.profile.username = QStringLiteral("operator");
    QVERIFY(SessionStore::save(path, data));

    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString contents = QString::fromUtf8(file.readAll());
    QVERIFY(!contents.contains(QStringLiteral("password"), Qt::CaseInsensitive));
    QVERIFY(!contents.contains(QStringLiteral("privateKeyPassword")));
}

///
/// \brief Loading a file without a connection endpoint fails with an error.
///
void TestSession::loadRejectsMissingConnection()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("bad.ouas"));

    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write("{\"schemaVersion\":1}");
    file.close();

    SessionData loaded;
    QString error;
    QVERIFY(!SessionStore::load(path, loaded, &error));
    QVERIFY(!error.isEmpty());
}

QTEST_MAIN(TestSession)
#include "test_session.moc"
