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
#include <QMenu>
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
#include "opcua/standardnodeid.h"
#include "ui_mainwindow.h"
#include "widgets/addressspacewidget.h"
#include "widgets/attributeswidget.h"
#include "widgets/dataaccesswidget.h"


///
/// \brief Builds the window, wires up menus, docks, icons, and the OPC UA client.
/// \param parent Parent widget.
///
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , _connectionController(new ConnectionController(this))
    , _clientService(_connectionController->clientService())
{
    ui->setupUi(this);
    ui->nodeDetailsDock->setWidget(ui->addressSpaceWidget->takeNodeDetailsPanel());

    const bool manualThemeSupported = theApp()->theme().isManualToggleSupported();
    ui->actionTheme->setVisible(manualThemeSupported);
    ui->menuTheme->menuAction()->setVisible(manualThemeSupported);

    setupMainMenu();
    ui->mainToolBar->setupFromDesignerActions();

    setupDockOptions();
    bindIcons();

    resetLayout();
    setupOpcUaClient();
    rebuildRecentConnections();
}

///
/// \brief Destroys the window and its generated UI.
///
MainWindow::~MainWindow()
{
    delete ui;
}

///
/// \brief Re-applies the themed window icon when the palette changes.
/// \param event Change event being handled.
///
void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);

    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange) {
        setWindowIcon(AppIcons::themed("app.ico"));
    }
}

///
/// \brief Opens the connection dialog to create a new connection.
///
void MainWindow::on_actionNewConnection_triggered()
{
    openConnectionDialog();
}

///
/// \brief Opens the connection dialog to connect to a server.
///
void MainWindow::on_actionConnect_triggered()
{
    openConnectionDialog();
}

///
/// \brief Disconnects from the current endpoint.
///
void MainWindow::on_actionDisconnect_triggered()
{
    _clientService->disconnectFromEndpoint();
}

///
/// \brief Initialises browsing of the server address space.
///
void MainWindow::on_actionBrowse_triggered()
{
    initializeAddressSpace();
}

///
/// \brief Initialises browsing of the server address space.
///
void MainWindow::on_actionBrowseAddressSpace_triggered()
{
    initializeAddressSpace();
}

///
/// \brief Re-browses the selected node, or the root when nothing is selected.
///
void MainWindow::on_actionRefresh_triggered()
{
    const QString nodeId = ui->addressSpaceWidget->selectedNode().nodeId;
    browseNodeOrRoot(nodeId);
    if (!nodeId.isEmpty())
        _clientService->browseReferences(nodeId);
}

///
/// \brief Reads the currently selected node.
///
void MainWindow::on_actionRead_triggered()
{
    const OpcUaNodeInfo selected = ui->addressSpaceWidget->selectedNode();
    if (!selected.nodeId.isEmpty())
        _clientService->readNode(selected.nodeId);
}

///
/// \brief Reads the currently selected node.
///
void MainWindow::on_actionReadSelected_triggered()
{
    on_actionRead_triggered();
}

///
/// \brief Opens the write dialog for the selected node's current value.
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
/// \brief Opens the write dialog for the selected node's current value.
///
void MainWindow::on_actionWriteValue_triggered()
{
    on_actionWrite_triggered();
}

///
/// \brief Adds the selected variable node to the data-access view.
///
void MainWindow::on_actionAddToDataAccess_triggered()
{
    if (OpcUa::isVariable(_selectedNodeDetails.nodeClass))
        ui->dataAccessWidget->addNode(_selectedNodeDetails);
}

///
/// \brief Closes the main window.
///
void MainWindow::on_actionExit_triggered()
{
    close();
}

///
/// \brief Toggles between the light and dark colour scheme.
///
void MainWindow::on_actionTheme_triggered()
{
    toggleTheme();
}

///
/// \brief Shows the About dialog.
///
void MainWindow::on_actionAbout_triggered()
{
    DialogAbout dialog(this);
    dialog.exec();
}

///
/// \brief Shows or hides the address-space dock.
/// \param checked True to show the dock.
///
void MainWindow::on_actionViewAddressSpace_toggled(bool checked)
{
    ui->addressSpaceDock->setVisible(checked);
}

///
/// \brief Keeps the address-space view action in sync with the dock's hidden state.
/// \param visible Dock visibility signal value.
///
void MainWindow::on_addressSpaceDock_visibilityChanged(bool visible)
{
    Q_UNUSED(visible)
    if (!ui) return;
    ui->actionViewAddressSpace->setChecked(!ui->addressSpaceDock->isHidden());
}

///
/// \brief Shows or hides the node-details dock.
/// \param checked True to show the dock.
///
void MainWindow::on_actionViewNodeDetails_toggled(bool checked)
{
    ui->nodeDetailsDock->setVisible(checked);
}

///
/// \brief Keeps the node-details view action in sync with the dock's hidden state.
/// \param visible Dock visibility signal value.
///
void MainWindow::on_nodeDetailsDock_visibilityChanged(bool visible)
{
    Q_UNUSED(visible)
    if (!ui) return;
    ui->actionViewNodeDetails->setChecked(!ui->nodeDetailsDock->isHidden());
}

///
/// \brief Shows or hides the activity-log dock.
/// \param checked True to show the dock.
///
void MainWindow::on_actionViewActivity_toggled(bool checked)
{
    ui->logDock->setVisible(checked);
}

///
/// \brief Keeps the activity view action in sync with the log dock's hidden state.
/// \param visible Dock visibility signal value.
///
void MainWindow::on_logDock_visibilityChanged(bool visible)
{
    Q_UNUSED(visible)
    ui->actionViewActivity->setChecked(!ui->logDock->isHidden());
}

///
/// \brief Switches the data-access widget to the Data Access page.
///
void MainWindow::on_actionViewDataAccess_triggered()
{
    ui->dataAccessWidget->setCurrentPage(DataAccessWidget::DataAccessPage);
}

///
/// \brief Switches the data-access widget to the Subscriptions page.
///
void MainWindow::on_actionViewSubscriptions_triggered()
{
    ui->dataAccessWidget->setCurrentPage(DataAccessWidget::SubscriptionsPage);
}

///
/// \brief Switches the data-access widget to the Events page.
///
void MainWindow::on_actionViewEvents_triggered()
{
    ui->dataAccessWidget->setCurrentPage(DataAccessWidget::EventsPage);
}

///
/// \brief Switches the data-access widget to the History page.
///
void MainWindow::on_actionViewHistory_triggered()
{
    ui->dataAccessWidget->setCurrentPage(DataAccessWidget::HistoryPage);
}

///
/// \brief Restores the default dock layout.
///
void MainWindow::on_actionResetLayout_triggered()
{
    resetLayout();
}

///
/// \brief Runs the connection dialog and connects (optionally saving) the chosen profile.
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
/// \brief Initialises the view actions' checked state from the docks.
///
void MainWindow::setupMainMenu()
{
    ui->actionViewAddressSpace->setChecked(!ui->addressSpaceDock->isHidden());
    ui->actionViewNodeDetails->setChecked(!ui->nodeDetailsDock->isHidden());
    ui->actionViewActivity->setChecked(!ui->logDock->isHidden());
}

///
/// \brief Assigns the window corners to the left dock area.
///
void MainWindow::setupDockOptions()
{
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
}

///
/// \brief Restores docks to their default positions and sizes.
///
void MainWindow::resetLayout()
{
    ui->addressSpaceDock->show();
    ui->nodeDetailsDock->show();
    ui->attributesDock->show();
    ui->logDock->show();

    addDockWidget(Qt::LeftDockWidgetArea, ui->addressSpaceDock);
    addDockWidget(Qt::LeftDockWidgetArea, ui->nodeDetailsDock);
    splitDockWidget(ui->addressSpaceDock, ui->nodeDetailsDock, Qt::Vertical);
    addDockWidget(Qt::RightDockWidgetArea, ui->attributesDock);
    addDockWidget(Qt::BottomDockWidgetArea, ui->logDock);

    ui->centralSplitter->setSizes({360, 310});
    resizeDocks({ui->addressSpaceDock, ui->attributesDock}, {300, 390}, Qt::Horizontal);
    resizeDocks({ui->addressSpaceDock, ui->nodeDetailsDock}, {520, 360}, Qt::Vertical);
    resizeDocks({ui->logDock}, {245}, Qt::Vertical);

    ui->actionViewAddressSpace->setChecked(true);
    ui->actionViewNodeDetails->setChecked(true);
    ui->actionViewActivity->setChecked(true);
}

///
/// \brief Toggles the application colour scheme.
///
void MainWindow::toggleTheme()
{
    theApp()->theme().toggle();
}

///
/// \brief Wires the client service and connection controller signals to the UI.
///
void MainWindow::setupOpcUaClient()
{
    connect(_clientService, &OpcUaClientService::stateChanged,
            this, &MainWindow::updateClientUi);
    connect(_clientService, &OpcUaClientService::errorOccurred,
            this, &MainWindow::onClientError);
    connect(_clientService, &OpcUaClientService::browseFinished,
            ui->addressSpaceWidget, &AddressSpaceWidget::setBrowseChildren);
    connect(_clientService, &OpcUaClientService::referencesBrowseFinished,
            ui->addressSpaceWidget, &AddressSpaceWidget::setBrowseReferences);
    connect(ui->addressSpaceWidget, &AddressSpaceWidget::browseRequested,
            _clientService, &OpcUaClientService::browse);
    connect(ui->addressSpaceWidget, &AddressSpaceWidget::referencesRequested,
            _clientService, &OpcUaClientService::browseReferences);
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
    ui->statusbar->setConnectionController(_connectionController);
    updateClientUi(_clientService->state());
}

///
/// \brief Logs an error reported by the client service.
/// \param message Error reported by the OPC UA client service.
///
void MainWindow::onClientError(const QString &message)
{
    qCWarning(lcClient) << message;
}

///
/// \brief Browses a node, defaulting to the Objects folder when none is given.
/// \param nodeId Node to browse, or empty to browse the Objects folder.
///
void MainWindow::browseNodeOrRoot(const QString &nodeId)
{
    _clientService->browse(nodeId.isEmpty()
        ? QString::fromLatin1(StandardNodeId::ObjectsFolder) : nodeId);
}

///
/// \brief Reads the node selected in the address space.
/// \param node Node selected in the address space.
///
void MainWindow::onNodeSelected(const OpcUaNodeInfo &node)
{
    _clientService->readNode(node.nodeId);
}

///
/// \brief Shows the read node details and enables the matching actions.
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
/// \brief Pushes the latest data-access values into the view.
/// \param values Latest data access values.
/// \param error Read error, if any.
///
void MainWindow::onDataValuesReady(const QVector<OpcUaDataValue> &values, const QString &error)
{
    if (error.isEmpty())
        ui->dataAccessWidget->updateValues(values);
}

///
/// \brief Re-reads the node on success, or warns the user on failure.
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
/// \brief Enables/disables actions and refreshes the views for the client state.
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
/// \brief Seeds the address-space view with the Objects-folder root node.
///
void MainWindow::initializeAddressSpace()
{
    OpcUaNodeInfo root;
    root.nodeId = QString::fromLatin1(StandardNodeId::ObjectsFolder);
    root.browseName = tr("Root");
    root.displayName = tr("Root");
    root.nodeClass = 1;
    root.hasChildren = true;
    ui->addressSpaceWidget->setRootNode(root);
}

///
/// \brief Opens the write dialog and writes the entered value on accept.
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
/// \brief Rebuilds the Recent Connections menu from the saved profiles.
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
/// \brief Binds themed icons to the window and its actions.
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
