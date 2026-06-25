// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_connectiondata.cpp
/// \brief Unit tests for endpoint history, the endpoint model, certificate status, and validation.
///

#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QTimeZone>

#include "opcua/certificateinfo.h"
#include "opcua/connectionprofilevalidator.h"
#include "opcua/endpointhistorystore.h"
#include "models/endpointmodel.h"

namespace {
const QTimeZone kUtc = QTimeZone::UTC;
}

///
/// \brief Tests the pure connection-data helpers against a temporary settings store.
///
class TestConnectionData : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();
    void endpointHistoryIsDeduplicatedAndBounded();
    void endpointModelExposesSelectionRoles();
    void endpointModelSortsByRank();
    void certificateStatusCoversDateBoundaries();
    void corruptCertificateIsReportedAsInvalid();
    void profileValidationCoversCredentials();

private:
    QTemporaryDir _settingsDirectory;
};

void TestConnectionData::initTestCase()
{
    QVERIFY(_settingsDirectory.isValid());
    QCoreApplication::setOrganizationName(QStringLiteral("OpenUaExplorerTests"));
    QCoreApplication::setApplicationName(QStringLiteral("ConnectionData"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       _settingsDirectory.path());
}

void TestConnectionData::cleanup()
{
    QSettings settings;
    settings.clear();
}

void TestConnectionData::endpointHistoryIsDeduplicatedAndBounded()
{
    EndpointHistoryStore store;
    for (int index = 0; index < 12; ++index) {
        store.save(QStringLiteral("opc.tcp://server-%1:4840").arg(index));
    }
    store.save(QStringLiteral("opc.tcp://server-5:4840"));

    const QStringList history = store.history();
    QCOMPARE(history.size(), 10);
    QCOMPARE(history.first(), QStringLiteral("opc.tcp://server-5:4840"));
    QCOMPARE(history.count(QStringLiteral("opc.tcp://server-5:4840")), 1);
}

void TestConnectionData::endpointModelExposesSelectionRoles()
{
    EndpointInfo endpoint;
    endpoint.endpointUrl = QStringLiteral("opc.tcp://localhost:4840");
    endpoint.securityPolicy =
        QStringLiteral("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");
    endpoint.securityMode = QStringLiteral("Sign & Encrypt");
    endpoint.securityModeValue = 3;

    EndpointModel model;
    model.setEndpoints({endpoint});
    const QModelIndex index = model.index(0, 0);

    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(index, EndpointModel::PolicyRole).toString(),
             QStringLiteral("Basic256Sha256"));
    QCOMPARE(model.data(index, EndpointModel::ModeRole).toString(),
             endpoint.securityMode);
    QCOMPARE(model.data(index, EndpointModel::IconRole).toString(),
             QStringLiteral("lock"));
    QCOMPARE(model.endpointAt(0).endpointUrl, endpoint.endpointUrl);
}

void TestConnectionData::endpointModelSortsByRank()
{
    const auto makeEndpoint = [](const QString &policy, int modeValue) {
        EndpointInfo endpoint;
        endpoint.endpointUrl = QStringLiteral("opc.tcp://localhost:4840");
        endpoint.securityPolicy =
            QStringLiteral("http://opcfoundation.org/UA/SecurityPolicy#") + policy;
        endpoint.securityModeValue = modeValue;
        return endpoint;
    };

    // Supplied out of rank order: insecure, then weak-secure, then strong-secure.
    const QList<EndpointInfo> endpoints = {
        makeEndpoint(QStringLiteral("None"), 1),
        makeEndpoint(QStringLiteral("Basic256"), 3),
        makeEndpoint(QStringLiteral("Aes256_Sha256_RsaPss"), 3),
    };

    EndpointModel model;
    model.setEndpoints(endpoints);

    QCOMPARE(model.rowCount(), 3);

    // Recommended (strongest secure) first, then good, then None.
    QCOMPARE(model.data(model.index(0, 0), EndpointModel::PolicyRole).toString(),
             QStringLiteral("Aes256_Sha256_RsaPss"));
    QCOMPARE(model.data(model.index(0, 0), EndpointModel::RecommendedRole).toBool(),
             true);
    QCOMPARE(model.data(model.index(1, 0), EndpointModel::PolicyRole).toString(),
             QStringLiteral("Basic256"));
    QCOMPARE(model.data(model.index(1, 0), EndpointModel::StatusRole).toString(),
             QStringLiteral("Good"));
    QCOMPARE(model.data(model.index(2, 0), EndpointModel::PolicyRole).toString(),
             QStringLiteral("None"));
    QCOMPARE(model.data(model.index(2, 0), EndpointModel::StatusRole).toString(),
             QStringLiteral("Not secure"));
}

void TestConnectionData::certificateStatusCoversDateBoundaries()
{
    const QDateTime now = QDateTime::fromSecsSinceEpoch(1000, kUtc);
    QCOMPARE(CertificateInfo::statusForDates(
                 now.addSecs(1), now.addSecs(10), now),
             CertificateInfo::Status::NotYetValid);
    QCOMPARE(CertificateInfo::statusForDates(
                 now.addSecs(-10), now.addSecs(-1), now),
             CertificateInfo::Status::Expired);
    QCOMPARE(CertificateInfo::statusForDates(
                 now.addSecs(-10), now.addSecs(10), now),
             CertificateInfo::Status::Valid);
    QCOMPARE(CertificateInfo::statusForDates({}, {}, now),
             CertificateInfo::Status::Invalid);
}

void TestConnectionData::corruptCertificateIsReportedAsInvalid()
{
    const CertificateInfo info =
        CertificateInfo::fromDer(QByteArrayLiteral("not-a-certificate"));
    QVERIFY(!info.readable);
    QCOMPARE(info.status, CertificateInfo::Status::Invalid);
    QVERIFY(info.serialNumber.isEmpty());
    QVERIFY(!info.fingerprint.isEmpty());
}

void TestConnectionData::profileValidationCoversCredentials()
{
    ConnectionProfile profile;
    QCOMPARE(ConnectionProfileValidator::validate(profile),
             ConnectionProfileValidator::Error::MissingEndpoint);

    profile.endpointUrl = QStringLiteral("opc.tcp://localhost:4840");
    profile.authentication = ConnectionProfile::Authentication::Username;
    QCOMPARE(ConnectionProfileValidator::validate(profile),
             ConnectionProfileValidator::Error::MissingUsername);

    profile.username = QStringLiteral("operator");
    profile.securityMode = 3;
    QCOMPARE(ConnectionProfileValidator::validate(profile),
             ConnectionProfileValidator::Error::MissingClientCertificate);

    profile.clientCertificateFile = QStringLiteral("client.der");
    profile.privateKeyFile = QStringLiteral("client.pem");
    QCOMPARE(ConnectionProfileValidator::validate(profile),
             ConnectionProfileValidator::Error::None);
}

QTEST_GUILESS_MAIN(TestConnectionData)

#include "test_connectiondata.moc"
