// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file mainwindow.cpp
/// \brief Implements the main application window.
///

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
#include "loggingcategories.h"
#include "mainwindow.h"
#include "opcua/connectioncontroller.h"
#include "opcua/opcuaclientservice.h"
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
    , _connectionController(new ConnectionController(this))
    , _clientService(_connectionController->clientService())
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
    browseNodeOrRoot(ui->addressSpaceWidget->selectedNode().nodeId);
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
                    OpcUa::isWritable(_selectedNodeDetails.userAccessLevel));
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
    if (OpcUa::isVariable(_selectedNodeDetails.nodeClass))
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
    const ConnectionProfile profile = dialog.profile();
    if (profile.saveProfile) {
        _connectionController->saveProfile(
            profile, dialog.password(), dialog.privateKeyPassword());
    }
    _connectionController->connectNewProfile(
        profile, dialog.password(), dialog.privateKeyPassword());
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
/// \brief MainWindow::setupOpcUaClient
///
void MainWindow::setupOpcUaClient()
{
    connect(_clientService, &OpcUaClientService::stateChanged,
            this, &MainWindow::updateClientUi);
    connect(_clientService, &OpcUaClientService::errorOccurred,
            this, &MainWindow::onClientError);
    connect(_clientService, &OpcUaClientService::browseFinished,
            ui->addressSpaceWidget, &AddressSpaceWidget::setBrowseChildren);
    connect(ui->addressSpaceWidget, &AddressSpaceWidget::browseRequested,
            _clientService, &OpcUaClientService::browse);
    connect(ui->addressSpaceWidget, &AddressSpaceWidget::refreshRequested,
            this, &MainWindow::browseNodeOrRoot);
    connect(ui->addressSpaceWidget, &AddressSpaceWidget::nodeSelected,
            this, &MainWindow::onNodeSelected);
    connect(_clientService, &OpcUaClientService::nodeDetailsReady,
            this, &MainWindow::onNodeDetailsReady);
    connect(ui->dataAccessWidget, &DataAccessWidget::readRequested,
            _clientService, &OpcUaClientService::readValues);
    connect(ui->dataAccessWidget, &DataAccessWidget::addSelectedNodeRequested,
            this, &MainWindow::on_actionAddToDataAccess_triggered);
    connect(ui->dataAccessWidget, &DataAccessWidget::writeRequested,
            this, &MainWindow::showWriteDialog);
    connect(_clientService, &OpcUaClientService::dataValuesReady,
            this, &MainWindow::onDataValuesReady);
    connect(_clientService, &OpcUaClientService::writeFinished,
            this, &MainWindow::onWriteFinished);
    connect(_connectionController, &ConnectionController::profilesChanged,
            this, &MainWindow::rebuildRecentConnections);
    connect(_connectionController, &ConnectionController::errorOccurred,
            this, &MainWindow::onClientError);
    _connectionController->setCertificateTrustDecider(this);
    updateClientUi(_clientService->state());
}

///
/// \brief MainWindow::onClientError
/// \param message Error reported by the OPC UA client service.
///
void MainWindow::onClientError(const QString &message)
{
    qCWarning(lcClient) << message;
}

///
/// \brief MainWindow::browseNodeOrRoot
/// \param nodeId Node to browse, or empty to browse the Objects folder.
///
void MainWindow::browseNodeOrRoot(const QString &nodeId)
{
    _clientService->browse(nodeId.isEmpty()
        ? QString::fromLatin1(OpcUa::kObjectsFolderId) : nodeId);
}

///
/// \brief MainWindow::onNodeSelected
/// \param node Node selected in the address space.
///
void MainWindow::onNodeSelected(const OpcUaNodeInfo &node)
{
    _clientService->readNode(node.nodeId);
}

///
/// \brief MainWindow::onNodeDetailsReady
/// \param details Read node details.
/// \param error Read error, if any.
///
void MainWindow::onNodeDetailsReady(const OpcUaNodeDetails &details, const QString &error)
{
    if (!error.isEmpty())
        return;
    _selectedNodeDetails = details;
    ui->addressSpaceWidget->setNodeDetails(details);
    ui->attributesWidget->setNodeDetails(details);
    const bool variable = OpcUa::isVariable(details.nodeClass);
    const bool writable = variable && OpcUa::isWritable(details.userAccessLevel);
    ui->actionRead->setEnabled(variable);
    ui->actionReadSelected->setEnabled(variable);
    ui->actionWrite->setEnabled(writable);
    ui->actionWriteValue->setEnabled(writable);
    ui->actionAddToDataAccess->setEnabled(variable);
}

///
/// \brief MainWindow::onDataValuesReady
/// \param values Latest data access values.
/// \param error Read error, if any.
///
void MainWindow::onDataValuesReady(const QVector<OpcUaDataValue> &values, const QString &error)
{
    if (error.isEmpty())
        ui->dataAccessWidget->updateValues(values);
}

///
/// \brief MainWindow::onWriteFinished
/// \param nodeId Written node.
/// \param success Whether the write succeeded.
/// \param error Write error, if any.
///
void MainWindow::onWriteFinished(const QString &nodeId, bool success, const QString &error)
{
    if (success) {
        _clientService->readNode(nodeId);
        _clientService->readValues({nodeId});
    } else {
        QMessageBox::warning(this, tr("Write Failed"), error);
    }
}

///
/// \brief Shows the certificate prompt and returns the selected trust policy.
/// \param certificate Server certificate awaiting a trust decision.
/// \param message Validation message to display.
/// \return Selected certificate trust policy.
///
CertificateTrustDecision MainWindow::decide(const QByteArray &certificate,
                                            const QString &message)
{
    CertificateTrustDialog dialog(this);
    dialog.setCertificate(certificate, message);
    dialog.exec();
    switch (dialog.decision()) {
    case CertificateTrustDialog::TrustOnce:
        return CertificateTrustDecision::TrustOnce;
    case CertificateTrustDialog::TrustPermanently:
        return CertificateTrustDecision::TrustPermanently;
    case CertificateTrustDialog::Reject:
        return CertificateTrustDecision::Reject;
    }
    return CertificateTrustDecision::Reject;
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
    const ConnectionProfile &profile = _connectionController->activeProfile();
    ui->statusbar->setConnectionState(state, profile.endpointUrl,
                                      profile.securityPolicy);
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
    root.nodeId = QString::fromLatin1(OpcUa::kObjectsFolderId);
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
/// \brief MainWindow::rebuildRecentConnections
///
void MainWindow::rebuildRecentConnections()
{
    ui->menuRecentConnections->clear();
    const QList<ConnectionProfile> profiles = _connectionController->profiles();
    if (profiles.isEmpty()) {
        ui->menuRecentConnections->addAction(tr("No Recent Connections"))->setEnabled(false);
        return;
    }
    for (const ConnectionProfile &profile : profiles) {
        ui->menuRecentConnections->addAction(
            profile.name.isEmpty() ? profile.endpointUrl : profile.name,
            this, [this, profile]() {
                _connectionController->connectSavedProfile(profile);
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
