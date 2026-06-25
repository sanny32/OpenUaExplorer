// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include <QTest>
#include <QSignalSpy>
#include <QTimeZone>

#include <QOpcUaBinaryDataEncoding>
#include <QOpcUaEndpointDescription>
#include <QOpcUaExpandedNodeId>
#include <QOpcUaExtensionObject>
#include <QOpcUaLocalizedText>
#include <QOpcUaQualifiedName>
#include <QOpcUaReferenceDescription>
#include <QOpcUaUserTokenPolicy>

#include "opcua/qtopcuaconnectionmanager.h"
#include "opcua/qtopcuarequestcoordinator.h"
#include "opcua/qtopcuatypemapper.h"

namespace {
const QTimeZone kUtc = QTimeZone::UTC;

/// \brief Encodes the SessionDiagnostics fields consumed by the mapper.
QOpcUaExtensionObject sessionDiagnostics(const QString &name, const QString &applicationUri,
                                         const QDateTime &connectionTime)
{
    QByteArray body;
    QOpcUaBinaryDataEncoding encoder(&body);
    encoder.encode<QString, QOpcUa::Types::NodeId>(QStringLiteral("ns=1;g={00000000-0000-0000-0000-000000000001}"));
    encoder.encode<QString>(name);
    encoder.encode<QString>(applicationUri);
    encoder.encode<QString>(QStringLiteral("product"));
    encoder.encode<QOpcUaLocalizedText>(QOpcUaLocalizedText(QStringLiteral("en"), QStringLiteral("client")));
    encoder.encode<quint32>(1);
    encoder.encode<QString>(QString());
    encoder.encode<QString>(QString());
    encoder.encodeArray<QString>({});
    encoder.encode<QString>(QStringLiteral("server"));
    encoder.encode<QString>(QStringLiteral("opc.tcp://localhost:4840"));
    encoder.encodeArray<QString>({});
    encoder.encode<double>(15000.0);
    encoder.encode<quint32>(0);
    encoder.encode<QDateTime>(connectionTime);

    QOpcUaExtensionObject object;
    object.setBinaryEncodedBody(body, QStringLiteral("ns=0;i=867"));
    return object;
}

} // namespace

class TestQtOpcUaInternals : public QObject
{
    Q_OBJECT

private slots:
    void mapsEndpointsAndReferences();
    void resolvesSessionByApplicationAndRecency();
    void ignoresInvalidSessionDiagnostics();
    void coordinatesIndependentAndSupersededRequests();
    void invalidatesAllRequests();
    void boundsTimeouts();
    void backendSwitchClearsDiscoveryState();
};

/// \brief Verifies structural mapping of discovery and browse results.
void TestQtOpcUaInternals::mapsEndpointsAndReferences()
{
    QOpcUaUserTokenPolicy anonymous;
    anonymous.setTokenType(QOpcUaUserTokenPolicy::Anonymous);
    QOpcUaUserTokenPolicy username;
    username.setTokenType(QOpcUaUserTokenPolicy::Username);
    QOpcUaEndpointDescription endpoint;
    endpoint.setEndpointUrl(QStringLiteral("opc.tcp://localhost:4840"));
    endpoint.setSecurityPolicy(QStringLiteral("policy"));
    endpoint.setSecurityMode(QOpcUaEndpointDescription::Sign);
    endpoint.setServerCertificate(QByteArrayLiteral("certificate"));
    endpoint.setUserIdentityTokens({anonymous, username});

    const QList<EndpointInfo> endpoints = QtOpcUaTypeMapper::endpointInfos({endpoint});
    QCOMPARE(endpoints.size(), 1);
    QCOMPARE(endpoints.first().index, 0);
    QCOMPARE(endpoints.first().endpointUrl, endpoint.endpointUrl());
    QVERIFY(endpoints.first().supportsAnonymous);
    QVERIFY(endpoints.first().supportsUsername);
    QVERIFY(!endpoints.first().supportsCertificate);

    QOpcUaReferenceDescription reference;
    reference.setTargetNodeId(QOpcUaExpandedNodeId(QStringLiteral("ns=2;s=Value")));
    reference.setBrowseName(QOpcUaQualifiedName(2, QStringLiteral("Value")));
    reference.setDisplayName(QOpcUaLocalizedText(QStringLiteral("en"), QStringLiteral("Value")));
    reference.setRefTypeId(QStringLiteral("ns=0;i=35"));
    reference.setNodeClass(QOpcUa::NodeClass::Variable);
    const QVector<OpcUaNodeInfo> nodes = QtOpcUaTypeMapper::nodeInfos({reference});
    QCOMPARE(nodes.size(), 1);
    QCOMPARE(nodes.first().nodeId, QStringLiteral("ns=2;s=Value"));
    QCOMPARE(nodes.first().browseName, QStringLiteral("Value"));
}

/// \brief Prefers an application match and otherwise the newest session.
void TestQtOpcUaInternals::resolvesSessionByApplicationAndRecency()
{
    const QDateTime older = QDateTime::fromMSecsSinceEpoch(1000, kUtc);
    const QDateTime newer = QDateTime::fromMSecsSinceEpoch(2000, kUtc);
    const QList<QOpcUaExtensionObject> sessions = {
        sessionDiagnostics(QStringLiteral("matched"), QStringLiteral("urn:ours"), older),
        sessionDiagnostics(QStringLiteral("latest"), QStringLiteral("urn:other"), newer)
    };
    QCOMPARE(QtOpcUaTypeMapper::ownSessionName(QVariant::fromValue(sessions), QStringLiteral("urn:ours")),
             QStringLiteral("matched"));
    QCOMPARE(QtOpcUaTypeMapper::ownSessionName(QVariant::fromValue(sessions), QStringLiteral("urn:missing")),
             QStringLiteral("latest"));
}

/// \brief Ignores malformed SessionDiagnostics extension objects.
void TestQtOpcUaInternals::ignoresInvalidSessionDiagnostics()
{
    QOpcUaExtensionObject invalid;
    invalid.setBinaryEncodedBody(QByteArrayLiteral("invalid"), QStringLiteral("ns=0;i=867"));
    QCOMPARE(QtOpcUaTypeMapper::ownSessionName(QVariant::fromValue(QList<QOpcUaExtensionObject>{invalid}),
                                               QStringLiteral("urn:ours")),
             QString());
}

/// \brief Keeps operation categories independent and settles tokens once.
void TestQtOpcUaInternals::coordinatesIndependentAndSupersededRequests()
{
    QtOpcUaRequestCoordinator coordinator;
    const auto first = coordinator.begin(QtOpcUaRequestCoordinator::Operation::Browse);
    const auto read = coordinator.begin(QtOpcUaRequestCoordinator::Operation::NodeRead);
    const auto second = coordinator.begin(QtOpcUaRequestCoordinator::Operation::Browse);
    QVERIFY(!coordinator.isCurrent(first));
    QVERIFY(coordinator.isCurrent(read));
    QVERIFY(coordinator.settle(second));
    QVERIFY(!coordinator.settle(second));
}

/// \brief Invalidates every active token when a connection is replaced.
void TestQtOpcUaInternals::invalidatesAllRequests()
{
    QtOpcUaRequestCoordinator coordinator;
    const auto browse = coordinator.begin(QtOpcUaRequestCoordinator::Operation::Browse);
    const auto write = coordinator.begin(QtOpcUaRequestCoordinator::Operation::Write);
    coordinator.cancelAll();
    QVERIFY(!coordinator.isCurrent(browse));
    QVERIFY(!coordinator.isCurrent(write));
}

/// \brief Enforces the one-second minimum request timeout.
void TestQtOpcUaInternals::boundsTimeouts()
{
    QCOMPARE(QtOpcUaRequestCoordinator::boundedTimeout(1), 1000);
    QCOMPARE(QtOpcUaRequestCoordinator::boundedTimeout(2500), 2500);
}

/// \brief Clears cached endpoints when switching to another backend fails.
void TestQtOpcUaInternals::backendSwitchClearsDiscoveryState()
{
    QtOpcUaConnectionManager manager;
    const QStringList backends = manager.availableBackends();
    if (backends.isEmpty())
        QSKIP("Qt OpcUa backend is not available.");

    QVERIFY(manager.prepareDiscovery(backends.constFirst()));
    QOpcUaEndpointDescription endpoint;
    endpoint.setEndpointUrl(QStringLiteral("opc.tcp://localhost:4840"));
    manager.finishDiscovery({endpoint});
    QCOMPARE(manager.endpointDescriptions().size(), 1);

    QSignalSpy invalidatedSpy(&manager, &QtOpcUaConnectionManager::clientInvalidated);
    QVERIFY(!manager.prepareDiscovery(QStringLiteral("missing-backend")));
    QCOMPARE(invalidatedSpy.size(), 1);
    QVERIFY(manager.endpointDescriptions().isEmpty());
}

QTEST_GUILESS_MAIN(TestQtOpcUaInternals)

#include "test_qtopcuainternals.moc"
