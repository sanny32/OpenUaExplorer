// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_connectiondata.cpp
/// \brief Unit tests for endpoint history, the endpoint model, certificate status, and validation.
///

#include <QColor>
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
    void removedEndpointDoesNotReturnAsLastUsed();
    void endpointModelExposesSelectionRoles();
    void endpointModelSortsByRank();
    void endpointModelDataRolesHeaderAndClear();
    void endpointModelRanksPolicyStrengths();
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

void TestConnectionData::removedEndpointDoesNotReturnAsLastUsed()
{
    EndpointHistoryStore store;
    store.save(QStringLiteral("opc.tcp://first:4840"));
    store.save(QStringLiteral("opc.tcp://second:4840"));

    store.remove(QStringLiteral("opc.tcp://second:4840"));

    QCOMPARE(store.history(), QStringList{QStringLiteral("opc.tcp://first:4840")});

    store.remove(QStringLiteral("opc.tcp://first:4840"));
    QVERIFY(store.history().isEmpty());
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

void TestConnectionData::endpointModelDataRolesHeaderAndClear()
{
    const auto make = [](const QString &policy, int modeValue, const QString &modeText) {
        EndpointInfo endpoint;
        endpoint.endpointUrl = QStringLiteral("opc.tcp://host/%1").arg(policy);
        endpoint.securityPolicy =
            QStringLiteral("http://opcfoundation.org/UA/SecurityPolicy#") + policy;
        endpoint.securityModeValue = modeValue;
        endpoint.securityMode = modeText;
        return endpoint;
    };

    EndpointModel model;
    model.setEndpoints({
        make(QStringLiteral("Aes256_Sha256_RsaPss"), 3, QStringLiteral("Sign & Encrypt")),
        make(QStringLiteral("Basic256"), 2, QStringLiteral("Sign")),
        make(QStringLiteral("None"), 1, QStringLiteral("None")),
    });
    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(model.columnCount(), int(EndpointModel::ColumnCount));

    // Qt::DisplayRole per column of the recommended (row 0) endpoint.
    QCOMPARE(model.data(model.index(0, EndpointModel::PolicyColumn), Qt::DisplayRole).toString(),
             QStringLiteral("Aes256_Sha256_RsaPss"));
    QCOMPARE(model.data(model.index(0, EndpointModel::ModeColumn), Qt::DisplayRole).toString(),
             QStringLiteral("Sign & Encrypt"));
    QCOMPARE(model.data(model.index(0, EndpointModel::StatusColumn), Qt::DisplayRole).toString(),
             QStringLiteral("Recommended"));
    QCOMPARE(model.data(model.index(1, EndpointModel::StatusColumn), Qt::DisplayRole).toString(),
             QStringLiteral("Good"));
    QCOMPARE(model.data(model.index(2, EndpointModel::StatusColumn), Qt::DisplayRole).toString(),
             QStringLiteral("Not secure"));

    // Icons follow the security mode value.
    QCOMPARE(model.data(model.index(0, 0), EndpointModel::IconRole).toString(),
             QStringLiteral("lock"));         // mode 3
    QCOMPARE(model.data(model.index(1, 0), EndpointModel::IconRole).toString(),
             QStringLiteral("shield-check")); // mode 2
    QCOMPARE(model.data(model.index(2, 0), EndpointModel::IconRole).toString(),
             QStringLiteral("unlock"));       // mode 1

    // Status colours: recommended (green), secure (amber), insecure (red).
    QCOMPARE(model.data(model.index(0, 0), EndpointModel::StatusColorRole).value<QColor>(),
             QColor(0x2e, 0x9e, 0x44));
    QCOMPARE(model.data(model.index(1, 0), EndpointModel::StatusColorRole).value<QColor>(),
             QColor(0xc0, 0x7d, 0x00));
    QCOMPARE(model.data(model.index(2, 0), EndpointModel::StatusColorRole).value<QColor>(),
             QColor(0xd1, 0x34, 0x38));

    // EndpointRole carries the whole endpoint.
    const auto carried =
        model.data(model.index(0, 0), EndpointModel::EndpointRole).value<EndpointInfo>();
    QCOMPARE(carried.securityPolicy,
             QStringLiteral("http://opcfoundation.org/UA/SecurityPolicy#Aes256_Sha256_RsaPss"));

    // Out-of-range / invalid indices return an invalid variant.
    QVERIFY(!model.data(QModelIndex(), EndpointModel::PolicyRole).isValid());
    QVERIFY(!model.data(model.index(0, 0), Qt::CheckStateRole).isValid());

    // Header titles and role names.
    QCOMPARE(model.headerData(EndpointModel::PolicyColumn, Qt::Horizontal).toString(),
             QStringLiteral("Security Policy"));
    QCOMPARE(model.headerData(EndpointModel::ModeColumn, Qt::Horizontal).toString(),
             QStringLiteral("Security Mode"));
    // Non-horizontal or non-display headers defer to the base implementation.
    QVERIFY(!model.headerData(EndpointModel::PolicyColumn, Qt::Horizontal,
                              Qt::DecorationRole).isValid());
    QVERIFY(model.headerData(0, Qt::Vertical).isValid());
    const QHash<int, QByteArray> roles = model.roleNames();
    QCOMPARE(roles.value(EndpointModel::PolicyRole), QByteArrayLiteral("policy"));
    QCOMPARE(roles.value(EndpointModel::EndpointRole), QByteArrayLiteral("endpoint"));

    // endpoints() exposes the ranked list; clear() empties the model.
    QCOMPARE(model.endpoints().size(), 3);
    model.clear();
    QCOMPARE(model.rowCount(), 0);
    QVERIFY(model.endpoints().isEmpty());
    QVERIFY(model.endpointAt(5).endpointUrl.isEmpty()); // out-of-range row
}

void TestConnectionData::endpointModelRanksPolicyStrengths()
{
    const auto make = [](const QString &policy) {
        EndpointInfo endpoint;
        endpoint.securityPolicy =
            QStringLiteral("http://opcfoundation.org/UA/SecurityPolicy#") + policy;
        endpoint.securityModeValue = 3; // secure, so policy strength decides the rank
        return endpoint;
    };

    EndpointModel model;
    model.setEndpoints({
        make(QStringLiteral("Basic128Rsa15")),        // strength 2
        make(QStringLiteral("Aes128_Sha256_RsaOaep")), // strength 5
        make(QStringLiteral("FuturePolicy")),          // unknown -> strength 1
    });

    QCOMPARE(model.data(model.index(0, 0), EndpointModel::PolicyRole).toString(),
             QStringLiteral("Aes128_Sha256_RsaOaep"));
    QCOMPARE(model.data(model.index(1, 0), EndpointModel::PolicyRole).toString(),
             QStringLiteral("Basic128Rsa15"));
    QCOMPARE(model.data(model.index(2, 0), EndpointModel::PolicyRole).toString(),
             QStringLiteral("FuturePolicy"));
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
