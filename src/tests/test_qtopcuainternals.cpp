// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include <QTest>
#include <QSignalSpy>
#include <QTimeZone>

#include <QOpcUaBinaryDataEncoding>
#include <QOpcUaDataValue>
#include <QOpcUaEndpointDescription>
#include <QOpcUaExpandedNodeId>
#include <QOpcUaExtensionObject>
#include <QOpcUaHistoryData>
#include <QOpcUaHistoryEvent>
#include <QOpcUaLocalizedText>
#include <QOpcUaMonitoringParameters>
#include <QOpcUaQualifiedName>
#include <QOpcUaReadResult>
#include <QOpcUaReferenceDescription>
#include <QOpcUaUserTokenPolicy>

#include "opcua/qtopcuaconnectionmanager.h"
#include "opcua/qtopcuarequestcoordinator.h"
#include "opcua/qtopcuaresultmapper.h"
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
    void mapsReadResultsAndBrowseEnrichment();
    void mapsHistoryResults();
    void mapsEventFields();
    void resolvesSessionByApplicationAndRecency();
    void ignoresInvalidSessionDiagnostics();
    void coordinatesIndependentAndSupersededRequests();
    void keepsKeyedRequestsIndependent();
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

    QOpcUaEndpointDescription duplicate = endpoint;
    duplicate.setServerCertificate(QByteArrayLiteral("alternate-certificate"));
    duplicate.setUserIdentityTokens({anonymous});
    const QList<EndpointInfo> endpoints = QtOpcUaTypeMapper::endpointInfos({duplicate, endpoint});
    QCOMPARE(endpoints.size(), 1);
    QCOMPARE(endpoints.first().index, 0);
    QCOMPARE(endpoints.first().endpointUrl, endpoint.endpointUrl());
    QVERIFY(endpoints.first().supportsAnonymous);
    QVERIFY(endpoints.first().supportsUsername);
    QVERIFY(!endpoints.first().supportsCertificate);

    QOpcUaEndpointDescription httpsEndpoint = endpoint;
    httpsEndpoint.setEndpointUrl(QStringLiteral("opc.https://localhost:53443"));
    const QVector<QOpcUaEndpointDescription> transportFiltered =
        QtOpcUaResultMapper::endpointsWithSupportedPolicy({endpoint, httpsEndpoint},
                                                          {QStringLiteral("policy")},
                                                          QStringLiteral("opc.tcp"));
    QCOMPARE(transportFiltered.size(), 1);
    QCOMPARE(transportFiltered.first().endpointUrl(), endpoint.endpointUrl());

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

/// \brief Verifies mapping of attribute reads used by Value reads and browse enrichment.
void TestQtOpcUaInternals::mapsReadResultsAndBrowseEnrichment()
{
    const QDateTime source = QDateTime::fromMSecsSinceEpoch(1000, kUtc);
    const QDateTime server = QDateTime::fromMSecsSinceEpoch(2000, kUtc);

    QOpcUaReadResult valueResult;
    valueResult.setNodeId(QStringLiteral("ns=2;s=Value"));
    valueResult.setAttribute(QOpcUa::NodeAttribute::Value);
    valueResult.setValue(42);
    valueResult.setStatusCode(QOpcUa::UaStatusCode::Good);
    valueResult.setSourceTimestamp(source);
    valueResult.setServerTimestamp(server);

    const QVector<OpcUaDataValue> values = QtOpcUaResultMapper::dataValues({valueResult});
    QCOMPARE(values.size(), 1);
    QCOMPARE(values.first().nodeId, valueResult.nodeId());
    QCOMPARE(values.first().value.toInt(), 42);
    QCOMPARE(values.first().status, QStringLiteral("Good"));
    QCOMPARE(values.first().sourceTimestamp, source);
    QCOMPARE(values.first().serverTimestamp, server);

    QVector<OpcUaNodeInfo> nodes;
    OpcUaNodeInfo object;
    object.nodeId = QStringLiteral("ns=2;s=Object");
    nodes.append(object);
    OpcUaNodeInfo variable;
    variable.nodeId = QStringLiteral("ns=2;s=Variable");
    nodes.append(variable);

    QOpcUaReadResult eventNotifier;
    eventNotifier.setNodeId(object.nodeId);
    eventNotifier.setAttribute(QOpcUa::NodeAttribute::EventNotifier);
    eventNotifier.setValue(OpcUa::SubscribeToEvents);
    eventNotifier.setStatusCode(QOpcUa::UaStatusCode::Good);
    QOpcUaReadResult historizing;
    historizing.setNodeId(variable.nodeId);
    historizing.setAttribute(QOpcUa::NodeAttribute::Historizing);
    historizing.setValue(true);
    historizing.setStatusCode(QOpcUa::UaStatusCode::Good);

    QtOpcUaResultMapper::applyBrowseAttributeResults(&nodes, {eventNotifier, historizing});
    QCOMPARE(nodes.at(0).eventNotifier, OpcUa::SubscribeToEvents);
    QVERIFY(nodes.at(1).historizing);
}

/// \brief Verifies mapping of raw history data samples.
void TestQtOpcUaInternals::mapsHistoryResults()
{
    const QDateTime source = QDateTime::fromMSecsSinceEpoch(1000, kUtc);
    const QDateTime server = QDateTime::fromMSecsSinceEpoch(2000, kUtc);

    QOpcUaDataValue sample;
    sample.setValue(12.5);
    sample.setStatusCode(QOpcUa::UaStatusCode::Good);
    sample.setSourceTimestamp(source);
    sample.setServerTimestamp(server);
    QOpcUaHistoryData history(QStringLiteral("ns=2;s=Temperature"));
    history.addValue(sample);

    const QVector<OpcUaHistoryValue> values = QtOpcUaResultMapper::historyValues(history);
    QCOMPARE(values.size(), 1);
    QCOMPARE(values.first().nodeId, history.nodeId());
    QCOMPARE(values.first().value.toDouble(), 12.5);
    QCOMPARE(values.first().status, QStringLiteral("Good"));
    QCOMPARE(values.first().sourceTimestamp, source);
    QCOMPARE(values.first().serverTimestamp, server);
}

/// \brief Verifies the shared event filter and field-to-event mapping.
void TestQtOpcUaInternals::mapsEventFields()
{
    const QOpcUaMonitoringParameters::EventFilter filter =
        QtOpcUaResultMapper::baseEventFilter();
    QCOMPARE(filter.selectClauses().size(), 5);

    const QDateTime time = QDateTime::fromMSecsSinceEpoch(3000, kUtc);
    const QVariantList fields = {
        time,
        500u,
        QStringLiteral("Server"),
        QVariant::fromValue(QOpcUaLocalizedText(QStringLiteral("en"), QStringLiteral("Started"))),
        QStringLiteral("ns=0;i=2041")
    };

    const OpcUaEvent event = QtOpcUaResultMapper::eventFromFields(
        QStringLiteral("ns=0;i=2253"), fields);
    QCOMPARE(event.sourceNodeId, QStringLiteral("ns=0;i=2253"));
    QCOMPARE(event.time, time);
    QCOMPARE(event.severity, quint16(500));
    QCOMPARE(event.sourceName, QStringLiteral("Server"));
    QCOMPARE(event.message, QStringLiteral("Started"));
    QCOMPARE(event.eventType, QStringLiteral("ns=0;i=2041"));
    QCOMPARE(event.fields.size(), fields.size());

    QOpcUaHistoryEvent history(QStringLiteral("ns=0;i=2253"));
    history.addEvent(fields);
    const QVector<OpcUaEvent> events = QtOpcUaResultMapper::historyEvents(history);
    QCOMPARE(events.size(), 1);
    QCOMPARE(events.first().message, QStringLiteral("Started"));
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

/// \brief Keeps keyed requests independent across keys but superseding within one key.
void TestQtOpcUaInternals::keepsKeyedRequestsIndependent()
{
    using Op = QtOpcUaRequestCoordinator::Operation;
    QtOpcUaRequestCoordinator coordinator;
    const auto nodeA = coordinator.begin(Op::HistoryRead, QStringLiteral("A"));
    const auto nodeB = coordinator.begin(Op::HistoryRead, QStringLiteral("B"));
    QVERIFY(coordinator.isCurrent(nodeA));
    QVERIFY(coordinator.isCurrent(nodeB));

    const auto nodeAAgain = coordinator.begin(Op::HistoryRead, QStringLiteral("A"));
    QVERIFY(!coordinator.isCurrent(nodeA));
    QVERIFY(coordinator.isCurrent(nodeB));
    QVERIFY(coordinator.isCurrent(nodeAAgain));

    QVERIFY(coordinator.settle(nodeB));
    QVERIFY(!coordinator.settle(nodeB));
    QVERIFY(coordinator.settle(nodeAAgain));

    const auto beforeReset = coordinator.begin(Op::HistoryRead, QStringLiteral("C"));
    coordinator.cancelAll();
    QVERIFY(!coordinator.isCurrent(beforeReset));
    QVERIFY(coordinator.isCurrent(coordinator.begin(Op::HistoryRead, QStringLiteral("C"))));
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
