// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_opcua_auth_integration.cpp
/// \brief Drives username and certificate authentication against a real (Python asyncua) server.
///
/// Each test function launches its own server instance (tools/opcua_test_server.py)
/// configured for exactly one authentication mode, so the accepted and the rejected
/// credential paths are exercised end-to-end. When Python or the asyncua package is
/// unavailable, or no OPC UA backend is installed, the tests skip themselves so they
/// stay CI-friendly.
///

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QSignalSpy>
#include <QTest>

#include "opcua/certificatetrustdecider.h"
#include "opcua/connectionprofile.h"
#include "opcua/opcuabackend.h"
#include "opcua/qtopcuabackend.h"
#include "opcua/opcuatypes.h"
#include "opcua/pkimanager.h"
#include "opcuatestserver.h"

namespace {

constexpr auto userName = "operator";
constexpr auto userPassword = "s3cret";

///
/// \brief Spins the event loop until the service reaches \a target or times out.
///
bool waitForState(const OpcUaBackend &service, OpcUaConnectionState target,
                  int timeoutMs)
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeoutMs) {
        if (service.state() == target)
            return true;
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        QTest::qWait(20);
    }
    return service.state() == target;
}

///
/// \brief Spins the event loop until a connection attempt settles, then reports success.
///
/// A rejected connection leaves the service in Disconnected, so waiting for Connected
/// alone would burn the whole timeout on the negative cases.
///
bool waitForConnectionResult(const OpcUaBackend &service, int timeoutMs)
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeoutMs) {
        if (service.state() == OpcUaConnectionState::Connected)
            return true;
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        QTest::qWait(20);
    }
    return false;
}

///
/// \brief Trust decider that accepts every server certificate presented to it.
///
class AcceptingTrustDecider : public CertificateTrustDecider
{
public:
    CertificateTrustDecision decide(const QByteArray &certificate, const QString &message) override
    {
        Q_UNUSED(message)
        lastCertificate = certificate;
        return CertificateTrustDecision::TrustPermanently;
    }

    QByteArray lastCertificate;
};

} // namespace

///
/// \brief End-to-end authentication tests against a live asyncua server.
///
class TestOpcUaAuthIntegration : public QObject
{
    Q_OBJECT

private slots:
    void usernameAuthenticationConnectsAndReads();
    void usernameAuthenticationRejectsWrongPassword();
    void certificateAuthenticationConnectsAndReads();
    void certificateAuthenticationRejectsUntrustedCertificate();

private:
    bool discoverEndpoint(OpcUaBackend &service, const QString &url,
                          EndpointInfo *endpoint, bool secured);
    bool generateClientCertificate(QString *certificateFile, QString *privateKeyFile);
};

///
/// \brief Discovers endpoints and selects the unsecured or the Basic256Sha256 one.
/// \param service Backend to discover with.
/// \param url Discovery URL.
/// \param endpoint Receives the selected endpoint.
/// \param secured Whether to select the SignAndEncrypt endpoint instead of the open one.
/// \return True when a matching endpoint was advertised.
///
bool TestOpcUaAuthIntegration::discoverEndpoint(OpcUaBackend &service, const QString &url,
                                                EndpointInfo *endpoint, bool secured)
{
    QSignalSpy discoverSpy(&service, &OpcUaBackend::endpointsDiscovered);
    service.discoverEndpoints(url, QStringLiteral("open62541"), 10000);
    if (!discoverSpy.wait(15000))
        return false;
    const QList<QVariant> arguments = discoverSpy.takeFirst();
    if (!arguments.at(1).toString().isEmpty())
        return false;

    const auto endpoints = arguments.at(0).value<QList<EndpointInfo>>();
    for (const EndpointInfo &candidate : endpoints) {
        const bool isSecured = candidate.securityPolicy.endsWith(QStringLiteral("#Basic256Sha256"))
            && candidate.securityModeValue == 3;
        const bool isOpen = candidate.securityPolicy.endsWith(QStringLiteral("#None"));
        if (secured ? isSecured : isOpen) {
            *endpoint = candidate;
            return true;
        }
    }
    return false;
}

///
/// \brief Creates the client key pair the certificate tests authenticate with.
/// \param certificateFile Receives the generated DER certificate path.
/// \param privateKeyFile Receives the generated PEM private key path.
/// \return True when the key pair was created.
///
bool TestOpcUaAuthIntegration::generateClientCertificate(QString *certificateFile,
                                                         QString *privateKeyFile)
{
    PkiManager pki;
    QString error;
    const bool generated = pki.generateClientCertificate(PkiManager::clientCertificateCommonName(),
                                                         PkiManager::applicationUri(),
                                                         certificateFile, privateKeyFile, &error);
    if (!generated)
        qWarning("%s", qPrintable(error));
    return generated;
}

///
/// \brief Connects with the credentials the server accepts and reads the test variable.
///
void TestOpcUaAuthIntegration::usernameAuthenticationConnectsAndReads()
{
    QtOpcUaBackend service;
    if (!service.isAvailable())
        QSKIP("No OPC UA backend is available.");

    OpcUaTestServer server;
    QString skipReason;
    if (!server.start({QStringLiteral("--port"), QStringLiteral("48402"),
                       QStringLiteral("--user"), QLatin1String(userName),
                       QStringLiteral("--password"), QLatin1String(userPassword)},
                      &skipReason)) {
        QSKIP(qPrintable(skipReason));
    }

    EndpointInfo endpoint;
    QVERIFY2(discoverEndpoint(service, server.endpoint(), &endpoint, false),
             "No unsecured endpoint was advertised.");
    QVERIFY2(endpoint.supportsUsername, "The endpoint does not advertise the username token.");
    QVERIFY2(!endpoint.supportsAnonymous, "The endpoint still advertises the anonymous token.");

    ConnectionProfile profile;
    profile.endpointUrl = endpoint.endpointUrl;
    profile.securityPolicy = endpoint.securityPolicy;
    profile.securityMode = endpoint.securityModeValue;
    profile.authentication = ConnectionProfile::Authentication::Username;
    profile.username = QLatin1String(userName);
    service.connectToEndpoint(profile, QLatin1String(userPassword), QString());
    QVERIFY2(waitForState(service, OpcUaConnectionState::Connected, 15000),
             qPrintable(service.lastError()));

    QSignalSpy readSpy(&service, &OpcUaBackend::dataValuesReady);
    service.readValues({server.nodeId()});
    QVERIFY(readSpy.wait(15000));
    const auto values = readSpy.takeFirst().at(0).value<QVector<OpcUaDataValue>>();
    QCOMPARE(values.size(), 1);
    QCOMPARE(values.first().value.toDouble(), 42.0);

    service.disconnectFromEndpoint();
    QVERIFY(waitForState(service, OpcUaConnectionState::Disconnected, 10000));
}

///
/// \brief Verifies a wrong password never reaches the Connected state.
///
void TestOpcUaAuthIntegration::usernameAuthenticationRejectsWrongPassword()
{
    QtOpcUaBackend service;
    if (!service.isAvailable())
        QSKIP("No OPC UA backend is available.");

    OpcUaTestServer server;
    QString skipReason;
    if (!server.start({QStringLiteral("--port"), QStringLiteral("48403"),
                       QStringLiteral("--user"), QLatin1String(userName),
                       QStringLiteral("--password"), QLatin1String(userPassword)},
                      &skipReason)) {
        QSKIP(qPrintable(skipReason));
    }

    EndpointInfo endpoint;
    QVERIFY2(discoverEndpoint(service, server.endpoint(), &endpoint, false),
             "No unsecured endpoint was advertised.");

    ConnectionProfile profile;
    profile.endpointUrl = endpoint.endpointUrl;
    profile.securityPolicy = endpoint.securityPolicy;
    profile.securityMode = endpoint.securityModeValue;
    profile.authentication = ConnectionProfile::Authentication::Username;
    profile.username = QLatin1String(userName);
    service.connectToEndpoint(profile, QStringLiteral("wrong-password"), QString());
    QVERIFY2(!waitForConnectionResult(service, 10000),
             "The server accepted a wrong password.");
    QVERIFY2(service.lastError().contains(QStringLiteral("Access denied")),
             qPrintable(service.lastError()));
}

///
/// \brief Connects over a secured channel with an X509 user token the server trusts.
///
void TestOpcUaAuthIntegration::certificateAuthenticationConnectsAndReads()
{
    QtOpcUaBackend service;
    if (!service.isAvailable())
        QSKIP("No OPC UA backend is available.");

    QString certificateFile;
    QString privateKeyFile;
    QVERIFY(generateClientCertificate(&certificateFile, &privateKeyFile));

    OpcUaTestServer server;
    QString skipReason;
    if (!server.start({QStringLiteral("--port"), QStringLiteral("48404"),
                       QStringLiteral("--certificate-auth"),
                       QStringLiteral("--client-certificate"), certificateFile},
                      &skipReason)) {
        QSKIP(qPrintable(skipReason));
    }

    AcceptingTrustDecider decider;
    service.setCertificateTrustDecider(&decider);

    EndpointInfo endpoint;
    QVERIFY2(discoverEndpoint(service, server.endpoint(), &endpoint, true),
             "No Basic256Sha256 SignAndEncrypt endpoint was advertised.");
    QVERIFY2(endpoint.supportsCertificate, "The endpoint does not advertise the X509 token.");
    QVERIFY2(!endpoint.supportsAnonymous, "The endpoint still advertises the anonymous token.");

    ConnectionProfile profile;
    profile.endpointUrl = endpoint.endpointUrl;
    profile.securityPolicy = endpoint.securityPolicy;
    profile.securityMode = endpoint.securityModeValue;
    profile.authentication = ConnectionProfile::Authentication::Certificate;
    profile.clientCertificateFile = certificateFile;
    profile.privateKeyFile = privateKeyFile;
    service.connectToEndpoint(profile, QString(), QString());
    QVERIFY2(waitForState(service, OpcUaConnectionState::Connected, 20000),
             qPrintable(service.lastError()));
    QVERIFY2(!decider.lastCertificate.isEmpty(),
             "The server certificate was never presented for a trust decision.");

    QSignalSpy readSpy(&service, &OpcUaBackend::dataValuesReady);
    service.readValues({server.nodeId()});
    QVERIFY(readSpy.wait(15000));
    const auto values = readSpy.takeFirst().at(0).value<QVector<OpcUaDataValue>>();
    QCOMPARE(values.size(), 1);
    QCOMPARE(values.first().value.toDouble(), 42.0);

    service.disconnectFromEndpoint();
    QVERIFY(waitForState(service, OpcUaConnectionState::Disconnected, 10000));
}

///
/// \brief Verifies a client certificate outside the server allow-list never connects.
///
void TestOpcUaAuthIntegration::certificateAuthenticationRejectsUntrustedCertificate()
{
    QtOpcUaBackend service;
    if (!service.isAvailable())
        QSKIP("No OPC UA backend is available.");

    QString certificateFile;
    QString privateKeyFile;
    QVERIFY(generateClientCertificate(&certificateFile, &privateKeyFile));

    // The server secures itself but trusts no client certificate at all.
    OpcUaTestServer server;
    QString skipReason;
    if (!server.start({QStringLiteral("--port"), QStringLiteral("48405"),
                       QStringLiteral("--certificate-auth")},
                      &skipReason)) {
        QSKIP(qPrintable(skipReason));
    }

    AcceptingTrustDecider decider;
    service.setCertificateTrustDecider(&decider);

    EndpointInfo endpoint;
    QVERIFY2(discoverEndpoint(service, server.endpoint(), &endpoint, true),
             "No Basic256Sha256 SignAndEncrypt endpoint was advertised.");

    ConnectionProfile profile;
    profile.endpointUrl = endpoint.endpointUrl;
    profile.securityPolicy = endpoint.securityPolicy;
    profile.securityMode = endpoint.securityModeValue;
    profile.authentication = ConnectionProfile::Authentication::Certificate;
    profile.clientCertificateFile = certificateFile;
    profile.privateKeyFile = privateKeyFile;
    service.connectToEndpoint(profile, QString(), QString());
    QVERIFY2(!waitForConnectionResult(service, 15000),
             "The server accepted an untrusted client certificate.");

    // The client trusts the server certificate through the decider above, so the
    // connection must fail on the user token rather than on channel validation.
    QVERIFY2(service.lastError().contains(QStringLiteral("Access denied")),
             qPrintable(service.lastError()));
}

QTEST_GUILESS_MAIN(TestOpcUaAuthIntegration)

#include "test_opcua_auth_integration.moc"
