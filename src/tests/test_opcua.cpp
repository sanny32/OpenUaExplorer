// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_opcua.cpp
/// \brief Tests OPC UA profiles, PKI paths and lazy address-space behavior.
///

#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "opcua/connectionprofilestore.h"
#include "opcua/opcuaclientservice.h"
#include "opcua/pkimanager.h"
#include "widgets/addressspacemodel.h"

///
/// \brief Unit tests for transport-neutral OPC UA components.
///
class TestOpcUa : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void profileRoundTripDoesNotStoreSecrets();
    void pkiPathsUseExpectedLayout();
    void lazyModelRequestsAndAppliesBrowse();
    void open62541BackendIsAvailable();
    void integrationEndpointDiscovery();

private:
    QTemporaryDir _settingsDirectory;
};

///
/// \brief TestOpcUa::initTestCase
///
void TestOpcUa::initTestCase()
{
    QVERIFY(_settingsDirectory.isValid());
    QCoreApplication::setOrganizationName(QStringLiteral("OpenUaExplorerTests"));
    QCoreApplication::setApplicationName(QStringLiteral("OpenUaExplorerTests"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       _settingsDirectory.path());
}

///
/// \brief TestOpcUa::profileRoundTripDoesNotStoreSecrets
///
void TestOpcUa::profileRoundTripDoesNotStoreSecrets()
{
    ConnectionProfile profile;
    profile.id = QStringLiteral("profile-one");
    profile.name = QStringLiteral("Local server");
    profile.endpointUrl = QStringLiteral("opc.tcp://localhost:4840");
    profile.securityPolicy = QStringLiteral("http://opcfoundation.org/UA/SecurityPolicy#None");
    profile.username = QStringLiteral("operator");
    profile.authentication = ConnectionProfile::Authentication::Username;
    profile.saveProfile = true;

    ConnectionProfileStore store;
    QVERIFY(store.save(profile));
    const QList<ConnectionProfile> profiles = store.profiles();
    QCOMPARE(profiles.size(), 1);
    QCOMPARE(profiles.first().endpointUrl, profile.endpointUrl);
    QCOMPARE(profiles.first().username, profile.username);

    QSettings settings;
    for (const QString &key : settings.allKeys()) {
        QVERIFY2(!key.contains(QStringLiteral("password"), Qt::CaseInsensitive),
                 qPrintable(key));
    }
}

///
/// \brief TestOpcUa::pkiPathsUseExpectedLayout
///
void TestOpcUa::pkiPathsUseExpectedLayout()
{
    const PkiManager::Paths paths = PkiManager().paths();
    QVERIFY(paths.ownCertificates.endsWith(QStringLiteral("/own/certs")));
    QVERIFY(paths.ownPrivate.endsWith(QStringLiteral("/own/private")));
    QVERIFY(paths.trustedCertificates.endsWith(QStringLiteral("/trusted/certs")));
    QVERIFY(paths.rejectedCertificates.endsWith(QStringLiteral("/rejected/certs")));
    QVERIFY(paths.issuerCertificates.endsWith(QStringLiteral("/issuers/certs")));
}

///
/// \brief TestOpcUa::lazyModelRequestsAndAppliesBrowse
///
void TestOpcUa::lazyModelRequestsAndAppliesBrowse()
{
    AddressSpaceModel model;
    OpcUaNodeInfo root;
    root.nodeId = QStringLiteral("ns=0;i=84");
    root.displayName = QStringLiteral("Root");
    root.nodeClass = 1;
    root.hasChildren = true;
    model.setRootNode(root);

    const QModelIndex rootIndex = model.index(0, 0);
    QVERIFY(rootIndex.isValid());
    QVERIFY(model.canFetchMore(rootIndex));

    QSignalSpy browseSpy(&model, &AddressSpaceModel::browseRequested);
    model.fetchMore(rootIndex);
    QCOMPARE(browseSpy.size(), 1);
    QCOMPARE(browseSpy.first().first().toString(), root.nodeId);

    OpcUaNodeInfo child;
    child.nodeId = QStringLiteral("ns=0;i=85");
    child.displayName = QStringLiteral("Objects");
    child.nodeClass = 1;
    model.setChildren(root.nodeId, {child});
    QCOMPARE(model.rowCount(rootIndex), 1);
    QCOMPARE(model.data(model.index(0, 0, rootIndex)).toString(), child.displayName);
}

///
/// \brief TestOpcUa::open62541BackendIsAvailable
///
void TestOpcUa::open62541BackendIsAvailable()
{
#ifdef OUAEXP_HAS_OPCUA
    const OpcUaClientService service;
    QVERIFY2(service.availableBackends().contains(QStringLiteral("open62541")),
             qPrintable(service.availableBackends().join(QStringLiteral(", "))));
#else
    QSKIP("Qt OpcUa was not available in this Qt kit.");
#endif
}

///
/// \brief TestOpcUa::integrationEndpointDiscovery
///
void TestOpcUa::integrationEndpointDiscovery()
{
    const QString endpoint = qEnvironmentVariable("OUAEXP_TEST_ENDPOINT");
    if (endpoint.isEmpty())
        QSKIP("OUAEXP_TEST_ENDPOINT is not configured.");

    OpcUaClientService service;
    if (!service.isAvailable())
        QSKIP("Qt OpcUa backend is not available.");

    QSignalSpy endpointSpy(&service, &OpcUaClientService::endpointsDiscovered);
    service.discoverEndpoints(endpoint);
    QVERIFY(endpointSpy.wait(15000));
    const QList<QVariant> arguments = endpointSpy.takeFirst();
    QVERIFY2(arguments.at(1).toString().isEmpty(),
             qPrintable(arguments.at(1).toString()));
}

QTEST_MAIN(TestOpcUa)

#include "test_opcua.moc"
