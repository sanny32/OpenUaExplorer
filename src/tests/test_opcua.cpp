// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_opcua.cpp
/// \brief Tests OPC UA profiles, PKI paths and lazy address-space behavior.
///

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QSysInfo>
#include <QTemporaryDir>
#include <QTest>

#include <QOpcUaConnectionSettings>
#include <QOpcUaPkiConfiguration>
#include <QSslCertificate>
#include <QSslCertificateExtension>

#include "opcua/certificateinfo.h"
#include "opcua/connectionprofilestore.h"
#include "opcua/opcuabackend.h"
#include "opcua/qtopcuabackend.h"
#include "opcua/pkimanager.h"
#include "opcua/standardnodeid.h"
#include "models/addressspacemodel.h"
#include "models/attributesmodel.h"

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
    void applicationNameIdentifiesProductAndHost();
    void generatedCertificateProvidesApplicationIdentity();
    void generatedCertificateIsAnEndEntityCertificate();
    void connectionSettingsCarryTheSessionName();
    void lazyModelRequestsAndAppliesBrowse();
    void attributesModelExposesStructuredValues();
    void open62541BackendIsAvailable();
    void encryptedPrivateKeyPasswordIsRejected();
    void findServersRejectsNonOpcTcpUrl();

private:
    QTemporaryDir _settingsDirectory;
};

///
/// \brief TestOpcUa::initTestCase
///
void TestOpcUa::initTestCase()
{
    QVERIFY(_settingsDirectory.isValid());
    QStandardPaths::setTestModeEnabled(true);
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
/// \brief TestOpcUa::applicationNameIdentifiesProductAndHost
///
void TestOpcUa::applicationNameIdentifiesProductAndHost()
{
    QString productName = QString::fromUtf8(APP_PRODUCT_NAME);
    productName.remove(QLatin1Char(' '));
    QString host = QSysInfo::machineHostName().trimmed();
    if (host.isEmpty())
        host = QStringLiteral("localhost");
    host.remove(QLatin1Char(' '));

    QCOMPARE(PkiManager::applicationName(), QStringLiteral("%1@%2").arg(productName, host));
    QCOMPARE(PkiManager::productUri(), QStringLiteral("%1:%1").arg(productName));
}

///
/// \brief TestOpcUa::generatedCertificateProvidesApplicationIdentity
///
void TestOpcUa::generatedCertificateProvidesApplicationIdentity()
{
    PkiManager pki;
    QString certificateFile;
    QString privateKeyFile;
    QString error;
    const QString commonName = PkiManager::clientCertificateCommonName();
    QVERIFY(!commonName.contains(QLatin1Char(' ')));
    QVERIFY(commonName.toUtf8().size() <= 64);
    QVERIFY2(pki.generateClientCertificate(
                 commonName,
                 PkiManager::applicationUri(),
                 &certificateFile,
                 &privateKeyFile,
                 &error),
             qPrintable(error));

    QFile certificate(certificateFile);
    QVERIFY(certificate.open(QIODevice::ReadOnly));
    const QByteArray certificateData = certificate.readAll();
    certificate.close();
    const QList<QSslCertificate> certificates =
        QSslCertificate::fromData(certificateData, QSsl::Der);
    QCOMPARE(certificates.size(), 1);
    const QSslCertificate generated = certificates.constFirst();
    QCOMPARE(generated.subjectInfo(QSslCertificate::CommonName), QStringList({commonName}));
    QCOMPARE(generated.issuerInfo(QSslCertificate::CommonName), QStringList({commonName}));

    // Not QSslCertificate::isSelfSigned(): it demands the keyCertSign key usage of an issuer,
    // which an end-entity application instance certificate does not carry.
    const CertificateInfo info = CertificateInfo::fromDer(certificateData);
    QVERIFY(info.selfSigned);
    QCOMPARE(info.serialNumber, QString::fromLatin1(generated.serialNumber()).toUpper());
    const QString executableBaseName =
        QFileInfo(QCoreApplication::applicationFilePath()).completeBaseName();
    QCOMPARE(QFileInfo(certificateFile).completeBaseName(), executableBaseName);
    QCOMPARE(QFileInfo(privateKeyFile).completeBaseName(), executableBaseName);

    QOpcUaPkiConfiguration configuration;
    configuration.setClientCertificateFile(certificateFile);
    configuration.setPrivateKeyFile(privateKeyFile);
    QCOMPARE(configuration.applicationIdentity().applicationUri(),
             PkiManager::applicationUri());
    QVERIFY(configuration.applicationIdentity().isValid());

    QString existingCertificateFile;
    QString existingPrivateKeyFile;
    QVERIFY(pki.existingClientCertificate(
        &existingCertificateFile, &existingPrivateKeyFile));
    QCOMPARE(existingCertificateFile, certificateFile);
    QCOMPARE(existingPrivateKeyFile, privateKeyFile);

    QVERIFY(QFile::remove(certificateFile));
    QVERIFY(QFile::remove(privateKeyFile));
}

///
/// \brief TestOpcUa::generatedCertificateIsAnEndEntityCertificate
///
/// OPC UA Part 6 requires an application instance certificate to be an end entity: strict
/// servers reject a client that offers a CA certificate as its identity.
///
void TestOpcUa::generatedCertificateIsAnEndEntityCertificate()
{
    PkiManager pki;
    QString certificateFile;
    QString privateKeyFile;
    QString error;
    QVERIFY2(pki.generateClientCertificate(
                 PkiManager::clientCertificateCommonName(),
                 PkiManager::applicationUri(),
                 &certificateFile,
                 &privateKeyFile,
                 &error),
             qPrintable(error));

    QFile certificate(certificateFile);
    QVERIFY(certificate.open(QIODevice::ReadOnly));
    const QList<QSslCertificate> certificates =
        QSslCertificate::fromData(certificate.readAll(), QSsl::Der);
    certificate.close();
    QCOMPARE(certificates.size(), 1);

    bool basicConstraintsSeen = false;
    const auto extensions = certificates.constFirst().extensions();
    for (const QSslCertificateExtension &extension : extensions) {
        if (extension.name() != QLatin1String("basicConstraints"))
            continue;
        basicConstraintsSeen = true;
        const QVariantMap value = extension.value().toMap();
        QVERIFY(value.contains(QStringLiteral("ca")));
        QVERIFY2(!value.value(QStringLiteral("ca")).toBool(),
                 "The generated client certificate is a CA certificate.");
    }
    QVERIFY2(basicConstraintsSeen, "The generated client certificate has no basicConstraints.");

    QVERIFY(QFile::remove(certificateFile));
    QVERIFY(QFile::remove(privateKeyFile));
}

///
/// \brief TestOpcUa::connectionSettingsCarryTheSessionName
///
/// Qt OPC UA has no session name of its own; the backend patch adds one, and without it
/// open62541 sends a null SessionName that strict servers fault the CreateSession on.
///
void TestOpcUa::connectionSettingsCarryTheSessionName()
{
    QOpcUaConnectionSettings settings;
    QVERIFY(settings.sessionName().isEmpty());

    settings.setSessionName(QStringLiteral("OuaExp Session"));
    QCOMPARE(settings.sessionName(), QStringLiteral("OuaExp Session"));

    QOpcUaConnectionSettings copy = settings;
    QCOMPARE(copy, settings);
    copy.setSessionName(QStringLiteral("Other Session"));
    QVERIFY(copy != settings);
}

///
/// \brief TestOpcUa::lazyModelRequestsAndAppliesBrowse
///
void TestOpcUa::lazyModelRequestsAndAppliesBrowse()
{
    AddressSpaceModel model;
    OpcUaNodeInfo root;
    root.nodeId = QString::fromLatin1(StandardNodeId::ObjectsFolder);
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
/// \brief TestOpcUa::attributesModelExposesStructuredValues
///
void TestOpcUa::attributesModelExposesStructuredValues()
{
    OpcUaNodeAttribute nodeId;
    nodeId.name = QStringLiteral("Node Id");
    nodeId.displayValue = QStringLiteral("ns=0;i=2269");

    OpcUaNodeAttribute namespaceIndex;
    namespaceIndex.name = QStringLiteral("NamespaceIndex");
    namespaceIndex.displayValue = QStringLiteral("0");
    nodeId.children.append(namespaceIndex);

    OpcUaNodeAttribute identifier;
    identifier.name = QStringLiteral("Identifier");
    identifier.displayValue = QStringLiteral("2269");
    nodeId.children.append(identifier);

    AttributesModel model;
    model.setAttributes({nodeId});

    QCOMPARE(model.rowCount(), 1);
    const QModelIndex nodeIdIndex = model.index(0, AttributesModel::ColAttribute);
    QCOMPARE(model.data(nodeIdIndex).toString(), nodeId.name);
    QCOMPARE(model.rowCount(nodeIdIndex), 2);

    const QModelIndex identifierIndex =
        model.index(1, AttributesModel::ColValue, nodeIdIndex);
    QCOMPARE(model.data(identifierIndex).toString(), identifier.displayValue);
    QCOMPARE(model.parent(identifierIndex), nodeIdIndex);
}

///
/// \brief TestOpcUa::open62541BackendIsAvailable
///
void TestOpcUa::open62541BackendIsAvailable()
{
    const QtOpcUaBackend service;
    QVERIFY2(service.availableBackends().contains(QStringLiteral("open62541")),
             qPrintable(service.availableBackends().join(QStringLiteral(", "))));
}

void TestOpcUa::encryptedPrivateKeyPasswordIsRejected()
{
    QtOpcUaBackend service;
    const QStringList backends = service.availableBackends();
    if (backends.isEmpty())
        QSKIP("Qt OpcUa backend is not available.");

    ConnectionProfile profile;
    profile.backend = backends.constFirst();
    QSignalSpy errorSpy(&service, &OpcUaBackend::errorOccurred);
    service.connectToEndpoint(profile, {}, QStringLiteral("encrypted-key-password"));

    QCOMPARE(errorSpy.size(), 1);
    QVERIFY(service.lastError().contains(
        QStringLiteral("Encrypted private keys"), Qt::CaseInsensitive));
}

///
/// \brief TestOpcUa::findServersRejectsNonOpcTcpUrl
///
void TestOpcUa::findServersRejectsNonOpcTcpUrl()
{
    QtOpcUaBackend service;
    const QStringList backends = service.availableBackends();
    if (backends.isEmpty())
        QSKIP("Qt OpcUa backend is not available.");

    QSignalSpy serversSpy(&service, &OpcUaBackend::serversDiscovered);
    service.findServers(QStringLiteral("http://localhost:4840"), backends.constFirst(), 5000);

    QCOMPARE(serversSpy.size(), 1);
    const QList<QVariant> arguments = serversSpy.takeFirst();
    QVERIFY(arguments.at(0).value<QList<ServerInfo>>().isEmpty());
    QVERIFY(!arguments.at(1).toString().isEmpty());
    QCOMPARE(service.state(), OpcUaConnectionState::Disconnected);
}

QTEST_MAIN(TestOpcUa)

#include "test_opcua.moc"
