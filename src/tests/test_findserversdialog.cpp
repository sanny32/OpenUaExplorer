// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_findserversdialog.cpp
/// \brief UI tests for FindServersDialog: discovery, lazy endpoint loading, selection.
///

#include <QAbstractItemModel>
#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QTreeView>

#include "dialogs/findserversdialog.h"
#include "models/serverdiscoverymodel.h"
#include "opcua/opcuabackend.h"
#include "settingsstore.h"

///
/// \brief OPC UA backend double that records the discovery requests the dialog issues.
///
class FindServersFakeBackend : public OpcUaBackend
{
    Q_OBJECT

public:
    using OpcUaBackend::OpcUaBackend;

    bool isAvailable() const override { return true; }
    QStringList availableBackends() const override { return {QStringLiteral("fake")}; }
    OpcUaConnectionState state() const override { return OpcUaConnectionState::Disconnected; }
    QString lastError() const override { return {}; }
    void setCertificateTrustDecider(CertificateTrustDecider *) override {}

    void findServers(const QString &url, const QString &, int) override
    {
        findServersUrls.append(url);
    }
    void discoverEndpoints(const QString &url, const QString &, int) override
    {
        endpointUrls.append(url);
    }
    void connectToEndpoint(const ConnectionProfile &, const QString &,
                           const QString &) override {}
    void disconnectFromEndpoint() override {}
    void browse(const QString &) override {}
    void browseReferences(const QString &) override {}
    void readNode(const QString &) override {}
    void readValues(const QStringList &) override {}
    void writeValue(const QString &, const QVariant &, int) override {}

    QStringList findServersUrls;
    QStringList endpointUrls;
};

///
/// \brief Drives the dialog through discovery, expansion, and endpoint selection.
///
class TestFindServersDialog : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();
    void findButtonRequestsTheEnteredDiscoveryUrl();
    void emptyResultKeepsTheEmptyState();
    void findServersErrorIsShownInTheStatusLine();
    void expandingAServerRequestsItsEndpointsOnce();
    void concurrentExpansionsAreQueued();
    void serverWithoutOpcTcpUrlFailsWithoutARequest();
    void endpointSelectionEnablesTheAcceptButton();
    void serverIconIsPresentInBothThemes();

private:
    QTemporaryDir _settingsDirectory;
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
    endpoint.securityMode = QStringLiteral("Sign & Encrypt");
    endpoint.securityModeValue = modeValue;
    return endpoint;
}

///
/// \brief Runs a FindServers round trip and returns the dialog's server tree.
/// \param dialog Dialog under test.
/// \param backend Backend double driving the results.
/// \param servers Servers the backend reports.
/// \return The dialog's tree view.
///
QTreeView *discover(FindServersDialog &dialog, FindServersFakeBackend &backend,
                    const QList<ServerInfo> &servers)
{
    auto *button = dialog.findChild<QPushButton *>(QStringLiteral("findServersButton"));
    button->click();
    emit backend.serversDiscovered(servers, {});
    return dialog.findChild<QTreeView *>(QStringLiteral("serverTreeWidget"));
}

}

void TestFindServersDialog::initTestCase()
{
    QVERIFY(_settingsDirectory.isValid());
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("OpenUaExplorerTests"));
    QCoreApplication::setApplicationName(QStringLiteral("FindServersDialog"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, _settingsDirectory.path());
}

void TestFindServersDialog::cleanup()
{
    while (QGuiApplication::overrideCursor())
        QGuiApplication::restoreOverrideCursor();
    SettingsStore settings;
    settings.clear();
}

void TestFindServersDialog::findButtonRequestsTheEnteredDiscoveryUrl()
{
    FindServersFakeBackend backend;
    FindServersDialog dialog;
    dialog.setBackend(&backend);

    auto *url = dialog.findChild<QComboBox *>(QStringLiteral("discoveryUrlComboBox"));
    auto *button = dialog.findChild<QPushButton *>(QStringLiteral("findServersButton"));
    auto *status = dialog.findChild<QLabel *>(QStringLiteral("statusLabel"));
    QVERIFY(url);
    QVERIFY(button);
    QVERIFY(status);
    QCOMPARE(url->currentText(), QStringLiteral("opc.tcp://localhost:4840"));

    url->setEditText(QStringLiteral("opc.tcp://lds:4840"));
    button->click();

    QCOMPARE(backend.findServersUrls, QStringList{QStringLiteral("opc.tcp://lds:4840")});
    QVERIFY(!button->isEnabled());
    QVERIFY(QGuiApplication::overrideCursor());

    emit backend.serversDiscovered({makeServer(QStringLiteral("Simulation"),
                                               {QStringLiteral("opc.tcp://host:4840")})},
                                   {});

    QVERIFY(button->isEnabled());
    QVERIFY(!QGuiApplication::overrideCursor());
    auto *tree = dialog.findChild<QTreeView *>(QStringLiteral("serverTreeWidget"));
    QVERIFY(tree);
    QCOMPARE(tree->model()->rowCount(), 1);
    QVERIFY(status->text().contains(QStringLiteral("1")));
}

void TestFindServersDialog::emptyResultKeepsTheEmptyState()
{
    FindServersFakeBackend backend;
    FindServersDialog dialog;
    dialog.setBackend(&backend);

    auto *pages = dialog.findChild<QStackedWidget *>();
    QVERIFY(pages);
    QCOMPARE(pages->currentIndex(), 1);

    QTreeView *tree = discover(dialog, backend, {});
    QCOMPARE(tree->model()->rowCount(), 0);
    QCOMPARE(pages->currentIndex(), 1);

    discover(dialog, backend,
             {makeServer(QStringLiteral("Simulation"), {QStringLiteral("opc.tcp://host:4840")})});
    QCOMPARE(pages->currentIndex(), 0);
}

void TestFindServersDialog::findServersErrorIsShownInTheStatusLine()
{
    FindServersFakeBackend backend;
    FindServersDialog dialog;
    dialog.setBackend(&backend);

    auto *button = dialog.findChild<QPushButton *>(QStringLiteral("findServersButton"));
    auto *status = dialog.findChild<QLabel *>(QStringLiteral("statusLabel"));
    button->click();
    emit backend.serversDiscovered({}, QStringLiteral("Finding servers requires an idle client."));

    QCOMPARE(status->text(), QStringLiteral("Finding servers requires an idle client."));
    QVERIFY(button->isEnabled());
    QVERIFY(!QGuiApplication::overrideCursor());
}

void TestFindServersDialog::expandingAServerRequestsItsEndpointsOnce()
{
    FindServersFakeBackend backend;
    FindServersDialog dialog;
    dialog.setBackend(&backend);

    QTreeView *tree = discover(dialog, backend,
                               {makeServer(QStringLiteral("Simulation"),
                                           {QStringLiteral("opc.tcp://host:53530/OPCUA")})});
    auto *model = qobject_cast<ServerDiscoveryModel *>(tree->model());
    QVERIFY(model);

    const QModelIndex server = model->index(0, ServerDiscoveryModel::PrimaryColumn);
    tree->expand(server);

    QCOMPARE(backend.endpointUrls, QStringList{QStringLiteral("opc.tcp://host:53530/OPCUA")});
    QCOMPARE(model->serverState(0), ServerDiscoveryModel::LoadState::Loading);

    emit backend.endpointsDiscovered({makeEndpoint(QStringLiteral("Basic256Sha256"), 3)}, {});
    QCOMPARE(model->serverState(0), ServerDiscoveryModel::LoadState::Loaded);
    QCOMPARE(model->rowCount(server), 1);

    tree->collapse(server);
    tree->expand(server);
    QCOMPARE(backend.endpointUrls.size(), 1);
}

void TestFindServersDialog::concurrentExpansionsAreQueued()
{
    FindServersFakeBackend backend;
    FindServersDialog dialog;
    dialog.setBackend(&backend);

    QTreeView *tree = discover(dialog, backend,
                               {makeServer(QStringLiteral("First"),
                                           {QStringLiteral("opc.tcp://first:4840")}),
                                makeServer(QStringLiteral("Second"),
                                           {QStringLiteral("opc.tcp://second:4840")})});
    auto *model = qobject_cast<ServerDiscoveryModel *>(tree->model());

    tree->expand(model->index(0, ServerDiscoveryModel::PrimaryColumn));
    tree->expand(model->index(1, ServerDiscoveryModel::PrimaryColumn));

    QCOMPARE(backend.endpointUrls, QStringList{QStringLiteral("opc.tcp://first:4840")});
    QCOMPARE(model->serverState(1), ServerDiscoveryModel::LoadState::Loading);

    emit backend.endpointsDiscovered({makeEndpoint(QStringLiteral("Basic256Sha256"), 3)}, {});
    QCOMPARE(backend.endpointUrls,
             (QStringList{QStringLiteral("opc.tcp://first:4840"),
                          QStringLiteral("opc.tcp://second:4840")}));

    emit backend.endpointsDiscovered({}, QStringLiteral("Endpoint discovery timed out."));
    QCOMPARE(model->serverState(0), ServerDiscoveryModel::LoadState::Loaded);
    QCOMPARE(model->serverState(1), ServerDiscoveryModel::LoadState::Failed);
}

void TestFindServersDialog::serverWithoutOpcTcpUrlFailsWithoutARequest()
{
    FindServersFakeBackend backend;
    FindServersDialog dialog;
    dialog.setBackend(&backend);

    QTreeView *tree = discover(dialog, backend,
                               {makeServer(QStringLiteral("Http"),
                                           {QStringLiteral("https://host/discovery")})});
    auto *model = qobject_cast<ServerDiscoveryModel *>(tree->model());

    tree->expand(model->index(0, ServerDiscoveryModel::PrimaryColumn));

    QVERIFY(backend.endpointUrls.isEmpty());
    QCOMPARE(model->serverState(0), ServerDiscoveryModel::LoadState::Failed);
}

void TestFindServersDialog::endpointSelectionEnablesTheAcceptButton()
{
    FindServersFakeBackend backend;
    FindServersDialog dialog;
    dialog.setBackend(&backend);

    auto *accept = dialog.findChild<QPushButton *>(QStringLiteral("useEndpointButton"));
    QVERIFY(accept);
    QVERIFY(!accept->isEnabled());

    QTreeView *tree = discover(dialog, backend,
                               {makeServer(QStringLiteral("Simulation"),
                                           {QStringLiteral("opc.tcp://host:4840")})});
    auto *model = qobject_cast<ServerDiscoveryModel *>(tree->model());
    const QModelIndex server = model->index(0, ServerDiscoveryModel::PrimaryColumn);

    tree->setCurrentIndex(server);
    QVERIFY(!accept->isEnabled());

    tree->expand(server);
    emit backend.endpointsDiscovered({makeEndpoint(QStringLiteral("None"), 1),
                                      makeEndpoint(QStringLiteral("Aes256_Sha256_RsaPss"), 3)},
                                     {});

    tree->setCurrentIndex(model->index(0, ServerDiscoveryModel::PrimaryColumn, server));
    QVERIFY(accept->isEnabled());

    accept->click();
    QCOMPARE(dialog.result(), int(QDialog::Accepted));
    QCOMPARE(dialog.selectedEndpoint().securityModeValue, 3);
    QVERIFY(dialog.selectedEndpoint().securityPolicy.endsWith(
        QStringLiteral("Aes256_Sha256_RsaPss")));
}

///
/// \brief Guards the server row artwork, which only the tree delegate resolves at paint time.
///
void TestFindServersDialog::serverIconIsPresentInBothThemes()
{
    for (const QString &theme : {QStringLiteral("light"), QStringLiteral("dark")}) {
        const QIcon icon(QStringLiteral(":/icons/%1/server.svg").arg(theme));
        QVERIFY2(!icon.isNull(), qPrintable(theme));
        QVERIFY(!icon.pixmap(20, 20).isNull());
    }
}

QTEST_MAIN(TestFindServersDialog)
#include "test_findserversdialog.moc"
