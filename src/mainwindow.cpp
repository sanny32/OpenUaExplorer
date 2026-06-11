// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file mainwindow.cpp
/// \brief Implements the main application window.
///

#include <memory>

#include <QAction>
#include <QDockWidget>
#include <QEvent>
#include <QList>
#include <QMessageBox>

#include "appicons.h"
#include "application.h"
#include "dialogs/certificatetrustdialog.h"
#include "dialogs/dialogabout.h"
#include "dialogs/connectiondialog.h"
#include "dialogs/writevaluedialog.h"
#include "itestdatapopulatable.h"
#include "loggingcategories.h"
#include "mainwindow.h"
#include "opcua/connectionprofilestore.h"
#include "opcua/opcuaclientservice.h"
#include "opcua/secretstore.h"
#include "ui_mainwindow.h"
#include "widgets/addressspacewidget.h"
#include "widgets/attributeswidget.h"
#include "widgets/dataaccesswidget.h"


///
/// \brief MainWindow::MainWindow
/// \param parent
///
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , _clientService(new OpcUaClientService(this))
    , _secretStore(new SecretStore(this))
    , _profileStore(new ConnectionProfileStore)
{
    ui->setupUi(this);
    ui->actionTheme->setVisible(theApp()->theme().isManualToggleSupported());

    setupMainMenu();
    ui->mainToolBar->setupFromDesignerActions();

    setupDockOptions();
    bindIcons();

    resetLayout();
    setupOpcUaClient();
    rebuildRecentConnections();
}

///
/// \brief MainWindow::~MainWindow
///
MainWindow::~MainWindow()
{
    delete _profileStore;
    delete ui;
}

///
/// \brief MainWindow::changeEvent
/// \param event
///
void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);

    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange) {
        setWindowIcon(AppIcons::themed("app.ico"));
    }
}

///
/// \brief MainWindow::on_actionNewConnection_triggered
///
void MainWindow::on_actionNewConnection_triggered()
{
    openConnectionDialog();
}

///
/// \brief MainWindow::on_actionConnect_triggered
///
void MainWindow::on_actionConnect_triggered()
{
    openConnectionDialog();
}

///
/// \brief MainWindow::on_actionDisconnect_triggered
///
void MainWindow::on_actionDisconnect_triggered()
{
    _clientService->disconnectFromEndpoint();
}

///
/// \brief MainWindow::on_actionBrowse_triggered
///
void MainWindow::on_actionBrowse_triggered()
{
    initializeAddressSpace();
}

///
/// \brief MainWindow::on_actionBrowseAddressSpace_triggered
///
void MainWindow::on_actionBrowseAddressSpace_triggered()
{
    initializeAddressSpace();
}

///
/// \brief MainWindow::on_actionRefresh_triggered
///
void MainWindow::on_actionRefresh_triggered()
{
    const OpcUaNodeInfo selected = ui->addressSpaceWidget->selectedNode();
    _clientService->browse(selected.nodeId.isEmpty()
        ? QStringLiteral("ns=0;i=84") : selected.nodeId);
}

///
/// \brief MainWindow::on_actionRead_triggered
///
void MainWindow::on_actionRead_triggered()
{
    const OpcUaNodeInfo selected = ui->addressSpaceWidget->selectedNode();
    if (!selected.nodeId.isEmpty())
        _clientService->readNode(selected.nodeId);
}

///
/// \brief MainWindow::on_actionReadSelected_triggered
///
void MainWindow::on_actionReadSelected_triggered()
{
    on_actionRead_triggered();
}

///
/// \brief MainWindow::on_actionWrite_triggered
///
void MainWindow::on_actionWrite_triggered()
{
    if (_selectedNodeDetails.nodeId.isEmpty())
        return;
    showWriteDialog(_selectedNodeDetails.nodeId, _selectedNodeDetails.value,
                    _selectedNodeDetails.valueType, _selectedNodeDetails.dataTypeId,
                    (_selectedNodeDetails.userAccessLevel & 0x02) != 0);
}

///
/// \brief MainWindow::on_actionWriteValue_triggered
///
void MainWindow::on_actionWriteValue_triggered()
{
    on_actionWrite_triggered();
}

///
/// \brief MainWindow::on_actionAddToDataAccess_triggered
///
void MainWindow::on_actionAddToDataAccess_triggered()
{
    if ((_selectedNodeDetails.nodeClass & 2) != 0)
        ui->dataAccessWidget->addNode(_selectedNodeDetails);
}

///
/// \brief MainWindow::on_actionExit_triggered
///
void MainWindow::on_actionExit_triggered()
{
    close();
}

///
/// \brief MainWindow::on_actionTheme_triggered
///
void MainWindow::on_actionTheme_triggered()
{
    toggleTheme();
}

///
/// \brief MainWindow::on_actionAbout_triggered
///
void MainWindow::on_actionAbout_triggered()
{
    DialogAbout dialog(this);
    dialog.exec();
}

///
/// \brief MainWindow::on_actionViewAddressSpace_toggled
///
void MainWindow::on_actionViewAddressSpace_toggled(bool checked)
{
    ui->addressSpaceDock->setVisible(checked);
}

///
/// \brief MainWindow::on_addressSpaceDock_visibilityChanged
///
void MainWindow::on_addressSpaceDock_visibilityChanged(bool visible)
{
    if (!ui) return;
    ui->actionViewAddressSpace->setChecked(visible);
}

///
/// \brief MainWindow::on_actionViewActivity_toggled
///
void MainWindow::on_actionViewActivity_toggled(bool checked)
{
    ui->logDock->setVisible(checked);
}

///
/// \brief MainWindow::on_logDock_visibilityChanged
///
void MainWindow::on_logDock_visibilityChanged(bool visible)
{
    ui->actionViewActivity->setChecked(visible);
}

///
/// \brief MainWindow::on_actionViewDataAccess_triggered
///
void MainWindow::on_actionViewDataAccess_triggered()
{
    ui->dataAccessWidget->setCurrentPage(DataAccessWidget::DataAccessPage);
}

///
/// \brief MainWindow::on_actionViewSubscriptions_triggered
///
void MainWindow::on_actionViewSubscriptions_triggered()
{
    ui->dataAccessWidget->setCurrentPage(DataAccessWidget::SubscriptionsPage);
}

///
/// \brief MainWindow::on_actionViewEvents_triggered
///
void MainWindow::on_actionViewEvents_triggered()
{
    ui->dataAccessWidget->setCurrentPage(DataAccessWidget::EventsPage);
}

///
/// \brief MainWindow::on_actionViewHistory_triggered
///
void MainWindow::on_actionViewHistory_triggered()
{
    ui->dataAccessWidget->setCurrentPage(DataAccessWidget::HistoryPage);
}

///
/// \brief MainWindow::on_actionResetLayout_triggered
///
void MainWindow::on_actionResetLayout_triggered()
{
    resetLayout();
}

///
/// \brief MainWindow::openConnectionDialog
///
void MainWindow::openConnectionDialog()
{
    ConnectionDialog dialog(this);
    dialog.setClientService(_clientService);
    if (dialog.exec() != QDialog::Accepted)
        return;
    _activeProfile = dialog.profile();
    if (_activeProfile.saveProfile)
        saveProfile(_activeProfile, dialog.password(), dialog.privateKeyPassword());
    _clientService->connectToEndpoint(_activeProfile, dialog.password(),
                                      dialog.privateKeyPassword());
}

///
/// \brief MainWindow::setupMainMenu
///
void MainWindow::setupMainMenu()
{
    ui->actionViewAddressSpace->setChecked(!ui->addressSpaceDock->isHidden());
    ui->actionViewActivity->setChecked(!ui->logDock->isHidden());
}

///
/// \brief MainWindow::setupDockOptions
///
void MainWindow::setupDockOptions()
{
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
}

///
/// \brief MainWindow::resetLayout
///
void MainWindow::resetLayout()
{
    ui->addressSpaceDock->show();
    ui->attributesDock->show();
    ui->logDock->show();

    addDockWidget(Qt::LeftDockWidgetArea, ui->addressSpaceDock);
    addDockWidget(Qt::RightDockWidgetArea, ui->attributesDock);
    addDockWidget(Qt::BottomDockWidgetArea, ui->logDock);

    ui->centralSplitter->setSizes({360, 310});
    resizeDocks({ui->addressSpaceDock, ui->attributesDock}, {300, 390}, Qt::Horizontal);
    resizeDocks({ui->logDock}, {245}, Qt::Vertical);
}

///
/// \brief MainWindow::toggleTheme
///
void MainWindow::toggleTheme()
{
    theApp()->theme().toggle();
}

///
/// \brief MainWindow::populateWithTestData
///
void MainWindow::populateWithTestData()
{
    const QList<ITestDataPopulatable *> targets = {
        ui->addressSpaceWidget,
        ui->attributesWidget,
        ui->dataAccessWidget,
        ui->logWidget,
    };
    for (ITestDataPopulatable *t : targets)
        t->populateWithTestData();
}

///
/// \brief MainWindow::setupOpcUaClient
///
void MainWindow::setupOpcUaClient()
{
    connect(_clientService, &OpcUaClientService::stateChanged,
            this, &MainWindow::updateClientUi);
    connect(_clientService, &OpcUaClientService::errorOccurred,
            this, [](const QString &message) {
        qCWarning(lcClient) << message;
    });
    connect(_clientService, &OpcUaClientService::browseFinished,
            ui->addressSpaceWidget, &AddressSpaceWidget::setBrowseChildren);
    connect(ui->addressSpaceWidget, &AddressSpaceWidget::browseRequested,
            _clientService, &OpcUaClientService::browse);
    connect(ui->addressSpaceWidget, &AddressSpaceWidget::refreshRequested,
            this, [this](const QString &nodeId) {
        _clientService->browse(nodeId.isEmpty() ? QStringLiteral("ns=0;i=84") : nodeId);
    });
    connect(ui->addressSpaceWidget, &AddressSpaceWidget::nodeSelected,
            this, [this](const OpcUaNodeInfo &node) {
        _clientService->readNode(node.nodeId);
    });
    connect(_clientService, &OpcUaClientService::nodeDetailsReady,
            this, [this](const OpcUaNodeDetails &details, const QString &error) {
        if (!error.isEmpty())
            return;
        _selectedNodeDetails = details;
        ui->addressSpaceWidget->setNodeDetails(details);
        ui->attributesWidget->setNodeDetails(details);
        const bool variable = (details.nodeClass & 2) != 0;
        ui->actionRead->setEnabled(variable);
        ui->actionReadSelected->setEnabled(variable);
        ui->actionWrite->setEnabled(variable && (details.userAccessLevel & 0x02));
        ui->actionWriteValue->setEnabled(variable && (details.userAccessLevel & 0x02));
        ui->actionAddToDataAccess->setEnabled(variable);
    });
    connect(ui->dataAccessWidget, &DataAccessWidget::readRequested,
            _clientService, &OpcUaClientService::readValues);
    connect(ui->dataAccessWidget, &DataAccessWidget::addSelectedNodeRequested,
            this, &MainWindow::on_actionAddToDataAccess_triggered);
    connect(ui->dataAccessWidget, &DataAccessWidget::writeRequested,
            this, &MainWindow::showWriteDialog);
    connect(_clientService, &OpcUaClientService::dataValuesReady,
            this, [this](const QVector<OpcUaDataValue> &values, const QString &error) {
        if (error.isEmpty())
            ui->dataAccessWidget->updateValues(values);
    });
    connect(_clientService, &OpcUaClientService::writeFinished,
            this, [this](const QString &nodeId, bool success, const QString &error) {
        if (success) {
            _clientService->readNode(nodeId);
            _clientService->readValues({nodeId});
        } else {
            QMessageBox::warning(this, tr("Write Failed"), error);
        }
    });
    connect(_clientService, &OpcUaClientService::certificateValidationRequired,
            this, [this](const QByteArray &certificate, const QString &message, int *decision) {
        CertificateTrustDialog dialog(this);
        dialog.setCertificate(certificate, message);
        dialog.exec();
        *decision = static_cast<int>(dialog.decision());
    }, Qt::DirectConnection);
    connect(_secretStore, &SecretStore::readFinished,
            this, [this](const QString &, SecretStore::Secret secret,
                         const QString &value, const QString &error) {
        if (!error.isEmpty())
            qCWarning(lcClient) << error;
        if (secret == SecretStore::Secret::Password)
            _pendingPassword = value;
        else
            _pendingPrivateKeyPassword = value;
        if (--_pendingSecretReads == 0) {
            _activeProfile = _pendingProfile;
            _clientService->discoverEndpoints(_pendingProfile.endpointUrl,
                                              _pendingProfile.backend);
            auto connection = std::make_shared<QMetaObject::Connection>();
            *connection = connect(_clientService, &OpcUaClientService::endpointsDiscovered,
                                  this, [this, connection](const QList<EndpointInfo> &,
                                                          const QString &error) {
                disconnect(*connection);
                if (error.isEmpty())
                    _clientService->connectToEndpoint(_pendingProfile, _pendingPassword,
                                                      _pendingPrivateKeyPassword);
            });
        }
    });
    updateClientUi(_clientService->state());
}

///
/// \brief MainWindow::updateClientUi
/// \param state Current OPC UA client state.
///
void MainWindow::updateClientUi(OpcUaConnectionState state)
{
    const bool connected = state == OpcUaConnectionState::Connected;
    const bool idle = state == OpcUaConnectionState::Disconnected
        || state == OpcUaConnectionState::Unavailable;
    ui->actionConnect->setEnabled(idle);
    ui->actionNewConnection->setEnabled(idle);
    ui->actionDisconnect->setEnabled(connected);
    ui->actionBrowse->setEnabled(connected);
    ui->actionBrowseAddressSpace->setEnabled(connected);
    ui->actionRefresh->setEnabled(connected);
    ui->statusbar->setConnectionState(state, _activeProfile.endpointUrl,
                                      _activeProfile.securityPolicy);
    if (connected) {
        initializeAddressSpace();
    } else if (state == OpcUaConnectionState::Disconnected
               || state == OpcUaConnectionState::Unavailable) {
        ui->addressSpaceWidget->clear();
        ui->attributesWidget->clear();
        _selectedNodeDetails = {};
    }
}

///
/// \brief MainWindow::initializeAddressSpace
///
void MainWindow::initializeAddressSpace()
{
    OpcUaNodeInfo root;
    root.nodeId = QStringLiteral("ns=0;i=84");
    root.browseName = tr("Root");
    root.displayName = tr("Root");
    root.nodeClass = 1;
    root.hasChildren = true;
    ui->addressSpaceWidget->setRootNode(root);
}

///
/// \brief MainWindow::showWriteDialog
/// \param nodeId Node to write.
/// \param value Current value.
/// \param valueType OPC UA value type.
/// \param dataTypeId DataType NodeId.
/// \param writable Whether the user may write.
///
void MainWindow::showWriteDialog(const QString &nodeId, const QVariant &value,
                                 int valueType, const QString &dataTypeId,
                                 bool writable)
{
    WriteValueDialog dialog(this);
    dialog.setValue(value, valueType, dataTypeId, writable);
    if (dialog.exec() == QDialog::Accepted)
        _clientService->writeValue(nodeId, dialog.value(), dialog.valueType());
}

///
/// \brief MainWindow::saveProfile
/// \param profile Profile metadata.
/// \param password Username password.
/// \param privateKeyPassword Private key password.
///
void MainWindow::saveProfile(const ConnectionProfile &profile,
                             const QString &password,
                             const QString &privateKeyPassword)
{
    _profileStore->save(profile);
    if (!password.isEmpty())
        _secretStore->write(profile.id, SecretStore::Secret::Password, password);
    if (!privateKeyPassword.isEmpty()) {
        _secretStore->write(profile.id, SecretStore::Secret::PrivateKeyPassword,
                            privateKeyPassword);
    }
    rebuildRecentConnections();
}

///
/// \brief MainWindow::rebuildRecentConnections
///
void MainWindow::rebuildRecentConnections()
{
    ui->menuRecentConnections->clear();
    const QList<ConnectionProfile> profiles = _profileStore->profiles();
    if (profiles.isEmpty()) {
        ui->menuRecentConnections->addAction(tr("No Recent Connections"))->setEnabled(false);
        return;
    }
    for (const ConnectionProfile &profile : profiles) {
        ui->menuRecentConnections->addAction(
            profile.name.isEmpty() ? profile.endpointUrl : profile.name,
            this, [this, profile]() { connectProfile(profile); });
    }
}

///
/// \brief MainWindow::connectProfile
/// \param profile Saved connection profile.
///
void MainWindow::connectProfile(const ConnectionProfile &profile)
{
    _pendingProfile = profile;
    _pendingPassword.clear();
    _pendingPrivateKeyPassword.clear();
    _pendingSecretReads = 0;
    if (profile.authentication == ConnectionProfile::Authentication::Username) {
        ++_pendingSecretReads;
        _secretStore->read(profile.id, SecretStore::Secret::Password);
    }
    if (!profile.privateKeyFile.isEmpty()) {
        ++_pendingSecretReads;
        _secretStore->read(profile.id, SecretStore::Secret::PrivateKeyPassword);
    }
    if (_pendingSecretReads == 0) {
        _activeProfile = profile;
        _clientService->discoverEndpoints(profile.endpointUrl, profile.backend);
        auto connection = std::make_shared<QMetaObject::Connection>();
        *connection = connect(_clientService, &OpcUaClientService::endpointsDiscovered,
                              this, [this, connection](const QList<EndpointInfo> &,
                                                      const QString &error) {
            disconnect(*connection);
            if (error.isEmpty())
                _clientService->connectToEndpoint(_pendingProfile);
        });
    }
}

///
/// \brief MainWindow::bindIcons
///
void MainWindow::bindIcons()
{
    setWindowIcon(AppIcons::themed("app.ico"));
    AppIcons::bindIcon(ui->actionConnect,     "connect.svg");
    AppIcons::bindIcon(ui->actionDisconnect,  "disconnect.svg");
    AppIcons::bindIcon(ui->actionBrowse,      "browse.svg");
    AppIcons::bindIcon(ui->actionRefresh,     "refresh.svg");
    AppIcons::bindIcon(ui->actionRead,        "read.svg");
    AppIcons::bindIcon(ui->actionWrite,       "write.svg");
    AppIcons::bindIcon(ui->actionSubscribe,   "subscribe.svg");
    AppIcons::bindIcon(ui->actionUnsubscribe, "unsubscribe.svg");
    AppIcons::bindIcon(ui->actionSettings,    "settings.svg");
    AppIcons::bindIcon(ui->actionTheme,       "theme.svg");
}
