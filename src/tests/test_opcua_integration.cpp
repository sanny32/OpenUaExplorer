// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_opcua_integration.cpp
/// \brief Drives OpcUaClientService against a real (Python asyncua) OPC UA server.
///
/// The server is launched as a child process (tools/opcua_test_server.py). When
/// Python or the asyncua package is unavailable, or no OPC UA backend is
/// installed, the whole case skips itself so it stays CI-friendly.
///

#include <QCoreApplication>
#include <QDateTime>
#include <QElapsedTimer>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

#include <QtOpcUa/qopcuatype.h>

#include "opcua/connectionprofile.h"
#include "opcua/opcuaclientservice.h"
#include "opcua/opcuatypes.h"

namespace {

///
/// \brief Spins the event loop until the service reaches \a target or times out.
///
bool waitForState(const OpcUaClientService &service, OpcUaConnectionState target,
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

} // namespace

///
/// \brief End-to-end client tests against a live asyncua server.
///
class TestOpcUaIntegration : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void discoverConnectBrowseReadWrite();

private:
    QProcess _server;
    QString _endpoint;
    QString _nodeId;
    QString _counterNodeId;
    QString _methodNodeId;
    QString _objectsNodeId;
};

///
/// \brief Launches the Python OPC UA server and waits for it to advertise itself.
///
void TestOpcUaIntegration::initTestCase()
{
    // Prefer the interpreter the coverage/test runner pinned for us (guaranteed
    // to have asyncua installed); otherwise fall back to whatever is on PATH.
    QString python = qEnvironmentVariable("OUAEXP_TEST_PYTHON");
    if (python.isEmpty() || !QFile::exists(python)) {
        python = QStandardPaths::findExecutable(QStringLiteral("python"));
        if (python.isEmpty())
            python = QStandardPaths::findExecutable(QStringLiteral("python3"));
    }
    if (python.isEmpty())
        QSKIP("Python interpreter not found; skipping OPC UA integration test.");

    const QString script = QStringLiteral(OUAEXP_TEST_SERVER_SCRIPT);
    if (!QFile::exists(script))
        QSKIP(qPrintable(QStringLiteral("Test server script missing: %1").arg(script)));

    // The Python server needs none of Qt's libraries, but the test runs with Qt
    // (and the bundled OpenSSL) prepended to the loader path. Inheriting that
    // breaks asyncua's native dependencies (cryptography loads the wrong
    // libcrypto), so launch the server with a clean loader path.
    QProcessEnvironment serverEnv = QProcessEnvironment::systemEnvironment();
    serverEnv.remove(QStringLiteral("LD_LIBRARY_PATH"));
    serverEnv.remove(QStringLiteral("DYLD_LIBRARY_PATH"));
    _server.setProcessEnvironment(serverEnv);

    _server.setProcessChannelMode(QProcess::MergedChannels);
    _server.start(python, {script, QStringLiteral("--port"), QStringLiteral("48401")});
    if (!_server.waitForStarted(5000))
        QSKIP("Could not start the Python OPC UA server.");

    QByteArray output;
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 25000 && !output.contains("READY")) {
        if (_server.waitForReadyRead(1000))
            output += _server.readAll();
        if (_server.state() == QProcess::NotRunning) {
            output += _server.readAll();
            break;
        }
    }

    for (const QByteArray &line : output.split('\n')) {
        const QString text = QString::fromUtf8(line).trimmed();
        if (text.startsWith(QStringLiteral("ENDPOINT ")))
            _endpoint = text.mid(9).trimmed();
        else if (text.startsWith(QStringLiteral("NODE ")))
            _nodeId = text.mid(5).trimmed();
        else if (text.startsWith(QStringLiteral("COUNTER ")))
            _counterNodeId = text.mid(8).trimmed();
        else if (text.startsWith(QStringLiteral("METHOD ")))
            _methodNodeId = text.mid(7).trimmed();
        else if (text.startsWith(QStringLiteral("OBJECTS ")))
            _objectsNodeId = text.mid(8).trimmed();
    }

    if (_endpoint.isEmpty() || _nodeId.isEmpty()) {
        QSKIP(qPrintable(QStringLiteral(
            "OPC UA test server did not become ready (asyncua missing?). "
            "Interpreter: %1\nOutput:\n%2")
            .arg(python, QString::fromUtf8(output))));
    }
}

///
/// \brief Stops the server process.
///
void TestOpcUaIntegration::cleanupTestCase()
{
    if (_server.state() != QProcess::NotRunning) {
        _server.kill();
        _server.waitForFinished(3000);
    }
}

///
/// \brief Discovers, connects, browses, reads, writes and disconnects.
///
void TestOpcUaIntegration::discoverConnectBrowseReadWrite()
{
    OpcUaClientService service;
    if (!service.isAvailable())
        QSKIP("No OPC UA backend is available.");

    // 1. Discover endpoints.
    QSignalSpy discoverSpy(&service, &OpcUaClientService::endpointsDiscovered);
    service.discoverEndpoints(_endpoint);
    QVERIFY(discoverSpy.wait(15000));
    const QList<QVariant> discoverArgs = discoverSpy.takeFirst();
    QVERIFY2(discoverArgs.at(1).toString().isEmpty(),
             qPrintable(discoverArgs.at(1).toString()));
    const auto endpoints = discoverArgs.at(0).value<QList<EndpointInfo>>();
    QVERIFY(!endpoints.isEmpty());

    // Pick the unsecured, anonymous endpoint.
    EndpointInfo chosen;
    bool found = false;
    for (const EndpointInfo &endpoint : endpoints) {
        if (endpoint.securityPolicy.endsWith(QStringLiteral("#None"))
            && endpoint.supportsAnonymous) {
            chosen = endpoint;
            found = true;
            break;
        }
    }
    QVERIFY2(found, "No unsecured anonymous endpoint was advertised.");

    // 2. Connect.
    ConnectionProfile profile;
    profile.endpointUrl = chosen.endpointUrl;
    profile.securityPolicy = chosen.securityPolicy;
    profile.securityMode = chosen.securityModeValue;
    profile.authentication = ConnectionProfile::Authentication::Anonymous;
    service.connectToEndpoint(profile);
    QVERIFY2(waitForState(service, OpcUaConnectionState::Connected, 15000),
             qPrintable(service.lastError()));

    // 3. Browse the Objects folder and find our variable.
    QSignalSpy browseSpy(&service, &OpcUaClientService::browseFinished);
    service.browse(QStringLiteral("ns=0;i=85"));
    QVERIFY(browseSpy.wait(15000));
    const QList<QVariant> browseArgs = browseSpy.takeFirst();
    QVERIFY2(browseArgs.at(2).toString().isEmpty(),
             qPrintable(browseArgs.at(2).toString()));
    const auto children = browseArgs.at(1).value<QVector<OpcUaNodeInfo>>();
    bool hasNode = false;
    for (const OpcUaNodeInfo &child : children)
        hasNode |= child.nodeId == _nodeId;
    QVERIFY2(hasNode, "The test variable was not found while browsing Objects.");

    QSignalSpy referencesSpy(&service, &OpcUaClientService::referencesBrowseFinished);
    service.browseReferences(_nodeId);
    QVERIFY(referencesSpy.wait(15000));
    const QList<QVariant> referencesArgs = referencesSpy.takeFirst();
    QVERIFY2(referencesArgs.at(2).toString().isEmpty(),
             qPrintable(referencesArgs.at(2).toString()));
    QVERIFY(!referencesArgs.at(1).value<QVector<OpcUaNodeInfo>>().isEmpty());

    // 3b. Read the full attribute set (drives the readNode formatting path).
    QSignalSpy detailsSpy(&service, &OpcUaClientService::nodeDetailsReady);
    service.readNode(_nodeId);
    QVERIFY(detailsSpy.wait(15000));
    const QList<QVariant> detailsArgs = detailsSpy.takeFirst();
    QVERIFY2(detailsArgs.at(1).toString().isEmpty(),
             qPrintable(detailsArgs.at(1).toString()));
    const auto details = detailsArgs.at(0).value<OpcUaNodeDetails>();
    QCOMPARE(details.nodeId, _nodeId);
    QVERIFY(!details.attributes.isEmpty());

    // 4. Read the initial value (42).
    QSignalSpy readSpy(&service, &OpcUaClientService::dataValuesReady);
    service.readValues({_nodeId});
    QVERIFY(readSpy.wait(15000));
    const auto initial = readSpy.takeFirst().at(0).value<QVector<OpcUaDataValue>>();
    QCOMPARE(initial.size(), 1);
    QCOMPARE(initial.first().value.toDouble(), 42.0);

    // 5. Write a new value (99).
    QSignalSpy writeSpy(&service, &OpcUaClientService::writeFinished);
    service.writeValue(_nodeId, 99.0, static_cast<int>(QOpcUa::Types::Double));
    QVERIFY(writeSpy.wait(15000));
    const QList<QVariant> writeArgs = writeSpy.takeFirst();
    QVERIFY2(writeArgs.at(1).toBool(), qPrintable(writeArgs.at(2).toString()));

    // 6. Read it back (99).
    QSignalSpy readBackSpy(&service, &OpcUaClientService::dataValuesReady);
    service.readValues({_nodeId});
    QVERIFY(readBackSpy.wait(15000));
    const auto updated = readBackSpy.takeFirst().at(0).value<QVector<OpcUaDataValue>>();
    QCOMPARE(updated.size(), 1);
    QCOMPARE(updated.first().value.toDouble(), 99.0);

    // 7. Read the server namespace table.
    QSignalSpy namespacesSpy(&service, &OpcUaClientService::namespacesReady);
    service.requestNamespaces();
    QVERIFY(namespacesSpy.wait(15000));
    const QList<QVariant> namespacesArgs = namespacesSpy.takeFirst();
    QVERIFY2(namespacesArgs.at(1).toString().isEmpty(),
             qPrintable(namespacesArgs.at(1).toString()));
    QVERIFY(!namespacesArgs.at(0).toStringList().isEmpty());

    // 8. Crawl the address space for per-namespace node counts.
    QSignalSpy statsSpy(&service, &OpcUaClientService::namespaceStatisticsReady);
    service.requestNamespaceStatistics();
    QVERIFY(statsSpy.wait(30000));
    QVERIFY2(statsSpy.takeFirst().at(1).toString().isEmpty(),
             "namespace statistics reported an error");

    // 9. Monitor the changing counter, receive a data change, then stop.
    QVERIFY(!_counterNodeId.isEmpty());
    QSignalSpy monitorSpy(&service, &OpcUaClientService::monitoringFinished);
    QSignalSpy counterDataSpy(&service, &OpcUaClientService::dataValuesReady);
    service.subscribe(_counterNodeId, 200.0);
    QVERIFY(monitorSpy.wait(15000));
    const QList<QVariant> monitorArgs = monitorSpy.takeFirst();
    QCOMPARE(monitorArgs.at(0).toString(), _counterNodeId);
    QVERIFY(monitorArgs.at(1).toBool());
    QVERIFY2(monitorArgs.at(2).toBool(), qPrintable(monitorArgs.at(3).toString()));
    QVERIFY(counterDataSpy.wait(15000)); // the counter ticks ~5x/s

    QSignalSpy unsubscribeSpy(&service, &OpcUaClientService::monitoringFinished);
    service.unsubscribe(_counterNodeId);
    QVERIFY(unsubscribeSpy.wait(15000));
    QVERIFY(!unsubscribeSpy.takeFirst().at(1).toBool());

    // 10. Read a method's argument metadata and call it (6 * 7 = 42).
    QVERIFY(!_methodNodeId.isEmpty() && !_objectsNodeId.isEmpty());
    QSignalSpy methodInfoSpy(&service, &OpcUaClientService::methodInfoReady);
    service.readMethodInfo(_methodNodeId);
    QVERIFY(methodInfoSpy.wait(15000));
    QVERIFY2(methodInfoSpy.takeFirst().at(3).toString().isEmpty(),
             "reading method info reported an error");

    QSignalSpy methodCallSpy(&service, &OpcUaClientService::methodCallFinished);
    service.callMethod(_objectsNodeId, _methodNodeId, {6.0, 7.0},
                       {static_cast<int>(QOpcUa::Types::Double),
                        static_cast<int>(QOpcUa::Types::Double)});
    QVERIFY(methodCallSpy.wait(15000));
    const QList<QVariant> methodCallArgs = methodCallSpy.takeFirst();
    QVERIFY2(methodCallArgs.at(2).toBool(), qPrintable(methodCallArgs.at(3).toString()));
    QCOMPARE(methodCallArgs.at(1).toDouble(), 42.0);

    // 11. Drive the raw-history read path (the server keeps no history, so the
    //     result may be empty; this exercises the client request/response code).
    QSignalSpy historySpy(&service, &OpcUaClientService::historyDataReady);
    service.readHistoryRaw(_counterNodeId, QDateTime::currentDateTimeUtc().addSecs(-60),
                           QDateTime::currentDateTimeUtc(), 100);
    QVERIFY(historySpy.wait(15000));

    // 12. Disconnect.
    service.disconnectFromEndpoint();
    QVERIFY(waitForState(service, OpcUaConnectionState::Disconnected, 10000));
}

QTEST_GUILESS_MAIN(TestOpcUaIntegration)

#include "test_opcua_integration.moc"
