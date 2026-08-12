// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include <QTest>
#include <QSignalSpy>
#include <QTimeZone>

#include <QOpcUaApplicationDescription>
#include <QOpcUaBinaryDataEncoding>
#include <QOpcUaDataValue>
#include <QOpcUaEndpointDescription>
#include <QOpcUaEnumDefinition>
#include <QOpcUaEnumField>
#include <QOpcUaExpandedNodeId>
#include <QOpcUaExtensionObject>
#include <QOpcUaGenericStructHandler>
#include <QOpcUaGenericStructValue>
#include <QOpcUaHistoryData>
#include <QOpcUaHistoryEvent>
#include <QOpcUaLocalizedText>
#include <QOpcUaMonitoringParameters>
#include <QOpcUaQualifiedName>
#include <QOpcUaReadResult>
#include <QOpcUaReferenceDescription>
#include <QOpcUaStructureDefinition>
#include <QOpcUaStructureField>
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
    void mapsApplicationDescriptions();
    void mapsReadResultsAndBrowseEnrichment();
    void mapsHistoryResults();
    void mapsEventFields();
    void resolvesSessionByApplicationAndRecency();
    void ignoresInvalidSessionDiagnostics();
    void detectsValuesWaitingForTypeDefinitions();
    void decodesStructuresWithAbstractEnumerationFields();
    void decodesStandardDiagnosticStructuresWithScalarAliases();
    void readsEnumerationDefinitionsFromTheTypeTree();
    void coordinatesIndependentAndSupersededRequests();
    void keepsKeyedRequestsIndependent();
    void invalidatesAllRequests();
    void boundsTimeouts();
    void backendSwitchClearsDiscoveryState();
    void serverLookupKeepsDiscoveredEndpoints();
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
    reference.setTypeDefinition(QOpcUaExpandedNodeId(QStringLiteral("ns=0;i=63")));
    reference.setNodeClass(QOpcUa::NodeClass::Variable);
    const QVector<OpcUaNodeInfo> nodes = QtOpcUaTypeMapper::nodeInfos({reference});
    QCOMPARE(nodes.size(), 1);
    QCOMPARE(nodes.first().nodeId, QStringLiteral("ns=2;s=Value"));
    QCOMPARE(nodes.first().browseName, QStringLiteral("Value"));
    QCOMPARE(nodes.first().typeDefinitionId, QStringLiteral("ns=0;i=63"));
}

/// \brief Verifies mapping of FindServers results to transport-neutral server records.
void TestQtOpcUaInternals::mapsApplicationDescriptions()
{
    QOpcUaApplicationDescription server;
    server.setApplicationName(QOpcUaLocalizedText(QStringLiteral("en"),
                                                  QStringLiteral("Simulation Server")));
    server.setApplicationUri(QStringLiteral("urn:localhost:Simulation"));
    server.setProductUri(QStringLiteral("urn:vendor:Simulation"));
    server.setApplicationType(QOpcUaApplicationDescription::ClientAndServer);
    server.setGatewayServerUri(QStringLiteral("urn:localhost:Gateway"));
    server.setDiscoveryProfileUri(QStringLiteral("urn:profile"));
    server.setDiscoveryUrls({QStringLiteral("opc.tcp://localhost:53530/OPCUA/Simulation"),
                             QStringLiteral("opc.tcp://192.168.1.10:53530/OPCUA/Simulation")});

    QOpcUaApplicationDescription discovery;
    discovery.setApplicationName(QOpcUaLocalizedText(QStringLiteral("en"),
                                                    QStringLiteral("Local Discovery Server")));
    discovery.setApplicationUri(QStringLiteral("urn:localhost:LDS"));
    discovery.setApplicationType(QOpcUaApplicationDescription::DiscoveryServer);

    const QList<ServerInfo> servers = QtOpcUaTypeMapper::serverInfos({server, discovery});
    QCOMPARE(servers.size(), 2);
    QCOMPARE(servers.first().applicationName, QStringLiteral("Simulation Server"));
    QCOMPARE(servers.first().applicationUri, QStringLiteral("urn:localhost:Simulation"));
    QCOMPARE(servers.first().productUri, QStringLiteral("urn:vendor:Simulation"));
    QCOMPARE(servers.first().applicationType, OpcUaApplicationType::ClientAndServer);
    QCOMPARE(servers.first().gatewayServerUri, QStringLiteral("urn:localhost:Gateway"));
    QCOMPARE(servers.first().discoveryProfileUri, QStringLiteral("urn:profile"));
    QCOMPARE(servers.first().discoveryUrls.size(), 2);
    QCOMPARE(servers.first().discoveryUrls.first(),
             QStringLiteral("opc.tcp://localhost:53530/OPCUA/Simulation"));

    QCOMPARE(servers.last().applicationType, OpcUaApplicationType::DiscoveryServer);
    QVERIFY(servers.last().discoveryUrls.isEmpty());
    QVERIFY(QtOpcUaTypeMapper::serverInfos({}).isEmpty());
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
    QOpcUaReadResult dataType;
    dataType.setNodeId(variable.nodeId);
    dataType.setAttribute(QOpcUa::NodeAttribute::DataType);
    dataType.setValue(QStringLiteral("ns=0;i=11"));
    dataType.setStatusCode(QOpcUa::UaStatusCode::Good);
    QOpcUaReadResult valueRank;
    valueRank.setNodeId(variable.nodeId);
    valueRank.setAttribute(QOpcUa::NodeAttribute::ValueRank);
    valueRank.setValue(-1);
    valueRank.setStatusCode(QOpcUa::UaStatusCode::Good);

    QtOpcUaResultMapper::applyBrowseAttributeResults(
        &nodes, {eventNotifier, historizing, dataType, valueRank});
    QCOMPARE(nodes.at(0).eventNotifier, OpcUa::SubscribeToEvents);
    QVERIFY(nodes.at(1).historizing);
    QCOMPARE(nodes.at(1).dataTypeId, QStringLiteral("ns=0;i=11"));
    QCOMPARE(nodes.at(1).valueRank, -1);
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

/// \brief Recognises the values that are worth reading again once structures decode.
void TestQtOpcUaInternals::detectsValuesWaitingForTypeDefinitions()
{
    QOpcUaExtensionObject object;
    object.setBinaryEncodedBody(QByteArrayLiteral("body"), QStringLiteral("ns=2;i=3062"));

    QVERIFY(QtOpcUaTypeMapper::containsOpaqueStruct(QVariant::fromValue(object)));
    QVERIFY(QtOpcUaTypeMapper::containsOpaqueStruct(
        QVariant::fromValue(QList<QOpcUaExtensionObject>{object})));
    QVERIFY(QtOpcUaTypeMapper::containsOpaqueStruct(
        QVariant(QVariantList{42, QVariant::fromValue(object)})));

    QVERIFY(!QtOpcUaTypeMapper::containsOpaqueStruct(QVariant(42)));
    QVERIFY(!QtOpcUaTypeMapper::containsOpaqueStruct(QVariant(QVariantList{1, 2})));
    QVERIFY(!QtOpcUaTypeMapper::containsOpaqueStruct(
        QVariant::fromValue(QList<QOpcUaExtensionObject>{})));
}

/// \brief Decodes a structure whose field is declared as the abstract Enumeration type.
void TestQtOpcUaInternals::decodesStructuresWithAbstractEnumerationFields()
{
    QOpcUaStructureField mode;
    mode.setName(QStringLiteral("Mode"));
    mode.setDataType(QOpcUa::namespace0Id(QOpcUa::NodeIds::Namespace0::Enumeration));
    QOpcUaStructureField count;
    count.setName(QStringLiteral("Count"));
    count.setDataType(QOpcUa::namespace0Id(QOpcUa::NodeIds::Namespace0::Int32));

    QOpcUaStructureDefinition definition;
    definition.setDefaultEncodingId(QStringLiteral("ns=1;i=5001"));
    definition.setFields({mode, count});

    QOpcUaGenericStructHandler handler(nullptr);
    QVERIFY(handler.addCustomStructureDefinition(definition, QStringLiteral("ns=1;i=3001"),
                                                 QStringLiteral("SampleStruct")));
    // The server's type definitions declare Enumeration abstract, which is what stops Qt.
    QVERIFY(handler.addCustomEnumDefinition(
        QOpcUaEnumDefinition(),
        QOpcUa::namespace0Id(QOpcUa::NodeIds::Namespace0::Enumeration),
        QStringLiteral("Enumeration"), QOpcUa::IsAbstract::Abstract));

    QByteArray body;
    QOpcUaBinaryDataEncoding encoder(&body);
    QVERIFY(encoder.encode<qint32>(2));
    QVERIFY(encoder.encode<qint32>(7));
    QOpcUaExtensionObject object;
    object.setBinaryEncodedBody(body, definition.defaultEncodingId());

    QVERIFY(!handler.decode(object).has_value());

    QtOpcUaTypeMapper::allowAbstractEnumerationFields(&handler);
    const std::optional<QOpcUaGenericStructValue> decoded = handler.decode(object);
    QVERIFY(decoded.has_value());
    QCOMPARE(decoded->fields().value(QStringLiteral("Mode")).toInt(), 2);
    QCOMPARE(decoded->fields().value(QStringLiteral("Count")).toInt(), 7);
}

/// \brief Decodes standard diagnostics whose fields use scalar DataType aliases.
void TestQtOpcUaInternals::decodesStandardDiagnosticStructuresWithScalarAliases()
{
    using NodeId = QOpcUa::NodeIds::Namespace0;
    const QString buildInfoType = QOpcUa::namespace0Id(NodeId::BuildInfo);
    const QString serverStatusType = QOpcUa::namespace0Id(NodeId::ServerStatusDataType);
    const QString subscriptionType =
        QOpcUa::namespace0Id(NodeId::SubscriptionDiagnosticsDataType);

    QOpcUaStructureField buildDate;
    buildDate.setName(QStringLiteral("BuildDate"));
    buildDate.setDataType(QOpcUa::namespace0Id(NodeId::UtcTime));
    QOpcUaStructureDefinition buildInfoDefinition;
    buildInfoDefinition.setDefaultEncodingId(QStringLiteral("ns=0;i=340"));
    buildInfoDefinition.setFields({buildDate});

    QOpcUaStructureField startTime;
    startTime.setName(QStringLiteral("StartTime"));
    startTime.setDataType(QOpcUa::namespace0Id(NodeId::UtcTime));
    QOpcUaStructureField buildInfo;
    buildInfo.setName(QStringLiteral("BuildInfo"));
    buildInfo.setDataType(buildInfoType);
    QOpcUaStructureDefinition serverStatusDefinition;
    serverStatusDefinition.setDefaultEncodingId(QStringLiteral("ns=0;i=864"));
    serverStatusDefinition.setFields({startTime, buildInfo});

    QOpcUaStructureField publishingInterval;
    publishingInterval.setName(QStringLiteral("PublishingInterval"));
    publishingInterval.setDataType(QOpcUa::namespace0Id(NodeId::Duration));
    QOpcUaStructureDefinition subscriptionDefinition;
    subscriptionDefinition.setDefaultEncodingId(QStringLiteral("ns=0;i=876"));
    subscriptionDefinition.setFields({publishingInterval});

    QOpcUaGenericStructHandler handler(nullptr);
    QVERIFY(handler.addCustomStructureDefinition(
        buildInfoDefinition, buildInfoType, QStringLiteral("BuildInfo")));
    QVERIFY(handler.addCustomStructureDefinition(
        serverStatusDefinition, serverStatusType, QStringLiteral("ServerStatusDataType")));
    QVERIFY(handler.addCustomStructureDefinition(
        subscriptionDefinition, subscriptionType,
        QStringLiteral("SubscriptionDiagnosticsDataType")));

    const QDateTime start = QDateTime::fromMSecsSinceEpoch(1000, kUtc);
    const QDateTime build = QDateTime::fromMSecsSinceEpoch(2000, kUtc);
    QByteArray serverStatusBody;
    QOpcUaBinaryDataEncoding serverStatusEncoder(&serverStatusBody);
    QVERIFY(serverStatusEncoder.encode<QDateTime>(start));
    QVERIFY(serverStatusEncoder.encode<QDateTime>(build));
    QOpcUaExtensionObject serverStatus;
    serverStatus.setBinaryEncodedBody(serverStatusBody,
                                      serverStatusDefinition.defaultEncodingId());

    QByteArray subscriptionBody;
    QOpcUaBinaryDataEncoding subscriptionEncoder(&subscriptionBody);
    QVERIFY(subscriptionEncoder.encode<double>(1250.0));
    QOpcUaExtensionObject subscription;
    subscription.setBinaryEncodedBody(subscriptionBody,
                                      subscriptionDefinition.defaultEncodingId());

    QVERIFY(!handler.decode(serverStatus).has_value());
    QVERIFY(!handler.decode(subscription).has_value());

    QtOpcUaTypeMapper::allowStandardDiagnosticScalarAliases(&handler);

    const std::optional<QOpcUaGenericStructValue> decodedStatus = handler.decode(serverStatus);
    QVERIFY(decodedStatus.has_value());
    QCOMPARE(decodedStatus->fields().value(QStringLiteral("StartTime")).toDateTime(), start);
    const QOpcUaGenericStructValue decodedBuild = decodedStatus->fields()
        .value(QStringLiteral("BuildInfo")).value<QOpcUaGenericStructValue>();
    QCOMPARE(decodedBuild.fields().value(QStringLiteral("BuildDate")).toDateTime(), build);

    const std::optional<QOpcUaGenericStructValue> decodedSubscription =
        handler.decode(subscription);
    QVERIFY(decodedSubscription.has_value());
    QCOMPARE(decodedSubscription->fields()
                 .value(QStringLiteral("PublishingInterval")).toDouble(), 1250.0);
}

/// \brief Reads the named values of an enumeration DataType from the server's type definitions.
void TestQtOpcUaInternals::readsEnumerationDefinitionsFromTheTypeTree()
{
    QOpcUaEnumField disabled;
    disabled.setName(QStringLiteral("Disabled"));
    disabled.setValue(0);
    QOpcUaEnumField enabled;
    enabled.setName(QStringLiteral("Enabled"));
    enabled.setValue(1);

    QOpcUaEnumDefinition definition;
    definition.setFields({disabled, enabled});

    QOpcUaGenericStructHandler handler(nullptr);
    QVERIFY(handler.addCustomEnumDefinition(definition, QStringLiteral("ns=1;s=SensorState"),
                                            QStringLiteral("SensorState")));

    const OpcUaEnumEntries entries =
        QtOpcUaTypeMapper::enumEntries(QStringLiteral("ns=1;s=SensorState"), &handler);
    QCOMPARE(entries.size(), 2);
    QCOMPARE(entries.at(0).value, 0);
    QCOMPARE(entries.at(0).name, QStringLiteral("Disabled"));
    QCOMPARE(entries.at(1).value, 1);
    QCOMPARE(entries.at(1).name, QStringLiteral("Enabled"));

    // A structure, an unknown type, and a session without type definitions name nothing.
    QCOMPARE(QtOpcUaTypeMapper::enumEntries(QStringLiteral("ns=0;i=6"), &handler).size(), 0);
    QCOMPARE(QtOpcUaTypeMapper::enumEntries(QStringLiteral("ns=1;s=SensorState"), nullptr).size(), 0);
    QCOMPARE(QtOpcUaTypeMapper::enumEntries(QString(), &handler).size(), 0);
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

/// \brief Verifies a FindServers request does not invalidate the cached endpoint list.
void TestQtOpcUaInternals::serverLookupKeepsDiscoveredEndpoints()
{
    QtOpcUaConnectionManager manager;
    const QStringList backends = manager.availableBackends();
    if (backends.isEmpty())
        QSKIP("Qt OpcUa backend is not available.");

    QVERIFY(manager.prepareDiscovery(backends.constFirst()));
    QOpcUaEndpointDescription endpoint;
    endpoint.setEndpointUrl(QStringLiteral("opc.tcp://localhost:4840"));
    manager.finishDiscovery({endpoint});

    manager.setState(OpcUaConnectionState::Discovering);
    manager.finishServerLookup();
    QCOMPARE(manager.state(), OpcUaConnectionState::Disconnected);
    QCOMPARE(manager.endpointDescriptions().size(), 1);
    QCOMPARE(manager.endpointDescriptions().first().endpointUrl(), endpoint.endpointUrl());
}

QTEST_GUILESS_MAIN(TestQtOpcUaInternals)

#include "test_qtopcuainternals.moc"
