// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_serverdiscoverymodel.cpp
/// \brief Unit tests for the lazy server/endpoint discovery tree model.
///

#include <QSignalSpy>
#include <QTest>

#include "models/serverdiscoverymodel.h"

///
/// \brief Exercises the tree shape, the lazy load states, and the ranking of endpoints.
///
class TestServerDiscoveryModel : public QObject
{
    Q_OBJECT

private slots:
    void serversFormTheTopLevelRows();
    void unloadedServerClaimsChildrenWithoutListingThem();
    void loadingAndFailureShowOneUnselectablePlaceholder();
    void loadedEndpointsAreRankedStrongestFirst();
    void loadedServerWithoutEndpointsShowsPlaceholder();
    void discoveryUrlPrefersOpcTcp();
    void endpointRowsCarryPolicyModeAndTransport();
};

namespace {

///
/// \brief Builds a server description for the tests.
/// \param name Application name.
/// \param discoveryUrls Advertised discovery URLs.
/// \return Server description.
///
ServerInfo makeServer(const QString &name, const QStringList &discoveryUrls)
{
    ServerInfo server;
    server.applicationName = name;
    server.applicationUri = QStringLiteral("urn:%1").arg(name);
    server.discoveryUrls = discoveryUrls;
    return server;
}

///
/// \brief Builds an endpoint description for the tests.
/// \param policy Short security policy name.
/// \param modeValue Message security mode value.
/// \return Endpoint description.
///
EndpointInfo makeEndpoint(const QString &policy, int modeValue)
{
    EndpointInfo endpoint;
    endpoint.endpointUrl = QStringLiteral("opc.tcp://host:4840/Server");
    endpoint.securityPolicy =
        QStringLiteral("http://opcfoundation.org/UA/SecurityPolicy#%1").arg(policy);
    endpoint.securityMode = modeValue == 3 ? QStringLiteral("Sign & Encrypt")
                                           : QStringLiteral("Sign");
    endpoint.securityModeValue = modeValue;
    return endpoint;
}

}

void TestServerDiscoveryModel::serversFormTheTopLevelRows()
{
    ServerDiscoveryModel model;
    model.setServers({makeServer(QStringLiteral("Simulation"),
                                 {QStringLiteral("opc.tcp://host:53530/OPCUA")}),
                      makeServer(QStringLiteral("Historian"),
                                 {QStringLiteral("opc.tcp://host:4841")})});

    QCOMPARE(model.serverCount(), 2);
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.columnCount(), int(ServerDiscoveryModel::ColumnCount));

    const QModelIndex first = model.index(0, ServerDiscoveryModel::PrimaryColumn);
    QVERIFY(first.isValid());
    QVERIFY(!first.parent().isValid());
    QCOMPARE(first.data(Qt::DisplayRole).toString(), QStringLiteral("Simulation"));
    QCOMPARE(first.data(ServerDiscoveryModel::SubtitleRole).toString(),
             QStringLiteral("opc.tcp://host:53530/OPCUA"));
    QVERIFY(first.data(ServerDiscoveryModel::IsServerRole).toBool());
    QCOMPARE(first.data(ServerDiscoveryModel::IconRole).toString(), QStringLiteral("server"));
    QCOMPARE(model.serverRowOf(first), 0);
    QVERIFY(!model.isEndpoint(first));
}

void TestServerDiscoveryModel::unloadedServerClaimsChildrenWithoutListingThem()
{
    ServerDiscoveryModel model;
    model.setServers({makeServer(QStringLiteral("Simulation"),
                                 {QStringLiteral("opc.tcp://host:4840")})});

    const QModelIndex server = model.index(0, ServerDiscoveryModel::PrimaryColumn);
    QCOMPARE(model.serverState(0), ServerDiscoveryModel::LoadState::NotLoaded);
    QVERIFY(model.hasChildren(server));
    QCOMPARE(model.rowCount(server), 0);
}

void TestServerDiscoveryModel::loadingAndFailureShowOneUnselectablePlaceholder()
{
    ServerDiscoveryModel model;
    model.setServers({makeServer(QStringLiteral("Simulation"),
                                 {QStringLiteral("opc.tcp://host:4840")})});
    const QModelIndex server = model.index(0, ServerDiscoveryModel::PrimaryColumn);

    QSignalSpy inserted(&model, &QAbstractItemModel::rowsInserted);
    model.setServerState(0, ServerDiscoveryModel::LoadState::Loading);
    QCOMPARE(model.rowCount(server), 1);
    QCOMPARE(inserted.count(), 1);

    QModelIndex placeholder = model.index(0, ServerDiscoveryModel::PrimaryColumn, server);
    QVERIFY(placeholder.data(ServerDiscoveryModel::IsPlaceholderRole).toBool());
    QVERIFY(!placeholder.data(Qt::DisplayRole).toString().isEmpty());
    QVERIFY(!model.flags(placeholder).testFlag(Qt::ItemIsSelectable));
    QVERIFY(!model.isEndpoint(placeholder));

    model.setServerState(0, ServerDiscoveryModel::LoadState::Failed,
                         QStringLiteral("Discovery timed out."));
    QCOMPARE(model.serverState(0), ServerDiscoveryModel::LoadState::Failed);
    QCOMPARE(model.rowCount(server), 1);
    placeholder = model.index(0, ServerDiscoveryModel::PrimaryColumn, server);
    QCOMPARE(placeholder.data(Qt::DisplayRole).toString(),
             QStringLiteral("Discovery timed out."));
}

void TestServerDiscoveryModel::loadedEndpointsAreRankedStrongestFirst()
{
    ServerDiscoveryModel model;
    model.setServers({makeServer(QStringLiteral("Simulation"),
                                 {QStringLiteral("opc.tcp://host:4840")})});
    const QModelIndex server = model.index(0, ServerDiscoveryModel::PrimaryColumn);

    model.setServerEndpoints(0, {makeEndpoint(QStringLiteral("None"), 1),
                                 makeEndpoint(QStringLiteral("Basic256Sha256"), 3),
                                 makeEndpoint(QStringLiteral("Aes256_Sha256_RsaPss"), 3)});

    QCOMPARE(model.serverState(0), ServerDiscoveryModel::LoadState::Loaded);
    QCOMPARE(model.rowCount(server), 3);

    const QModelIndex first = model.index(0, ServerDiscoveryModel::PrimaryColumn, server);
    QVERIFY(model.isEndpoint(first));
    QCOMPARE(first.parent(), server);
    QCOMPARE(model.serverRowOf(first), 0);
    QCOMPARE(first.data(ServerDiscoveryModel::PolicyRole).toString(),
             QStringLiteral("Aes256_Sha256_RsaPss"));
    QCOMPARE(model.index(2, ServerDiscoveryModel::PrimaryColumn, server)
                 .data(ServerDiscoveryModel::PolicyRole)
                 .toString(),
             QStringLiteral("None"));
    QCOMPARE(model.endpointAt(first).securityModeValue, 3);
    QVERIFY(model.flags(first).testFlag(Qt::ItemIsSelectable));
}

void TestServerDiscoveryModel::loadedServerWithoutEndpointsShowsPlaceholder()
{
    ServerDiscoveryModel model;
    model.setServers({makeServer(QStringLiteral("Simulation"),
                                 {QStringLiteral("opc.tcp://host:4840")})});
    const QModelIndex server = model.index(0, ServerDiscoveryModel::PrimaryColumn);

    model.setServerEndpoints(0, {});
    QCOMPARE(model.rowCount(server), 1);

    const QModelIndex placeholder = model.index(0, ServerDiscoveryModel::PrimaryColumn, server);
    QVERIFY(placeholder.data(ServerDiscoveryModel::IsPlaceholderRole).toBool());
    QVERIFY(!model.isEndpoint(placeholder));
}

void TestServerDiscoveryModel::discoveryUrlPrefersOpcTcp()
{
    ServerDiscoveryModel model;
    model.setServers({makeServer(QStringLiteral("Http"),
                                 {QStringLiteral("https://host/discovery"),
                                  QStringLiteral("opc.tcp://host:4840")}),
                      makeServer(QStringLiteral("Unreachable"),
                                 {QStringLiteral("https://host/discovery")}),
                      makeServer(QStringLiteral("Silent"), {})});

    QCOMPARE(model.discoveryUrl(0), QStringLiteral("opc.tcp://host:4840"));
    QVERIFY(model.discoveryUrl(1).isEmpty());
    QVERIFY(model.discoveryUrl(2).isEmpty());
    QVERIFY(model.discoveryUrl(9).isEmpty());
}

void TestServerDiscoveryModel::endpointRowsCarryPolicyModeAndTransport()
{
    ServerDiscoveryModel model;
    model.setServers({makeServer(QStringLiteral("Simulation"),
                                 {QStringLiteral("opc.tcp://host:4840")})});
    const QModelIndex server = model.index(0, ServerDiscoveryModel::PrimaryColumn);
    model.setServerEndpoints(0, {makeEndpoint(QStringLiteral("Basic256Sha256"), 3)});

    const QModelIndex primary = model.index(0, ServerDiscoveryModel::PrimaryColumn, server);
    const QModelIndex transport = model.index(0, ServerDiscoveryModel::TransportColumn, server);

    QCOMPARE(primary.data(Qt::DisplayRole).toString(),
             QStringLiteral("Basic256Sha256 — Sign & Encrypt"));
    QCOMPARE(primary.data(ServerDiscoveryModel::ModeRole).toString(),
             QStringLiteral("Sign & Encrypt"));
    QCOMPARE(primary.data(ServerDiscoveryModel::IconRole).toString(), QStringLiteral("lock"));
    QCOMPARE(transport.data(Qt::DisplayRole).toString(), QStringLiteral("opc.tcp"));
}

QTEST_MAIN(TestServerDiscoveryModel)
#include "test_serverdiscoverymodel.moc"
