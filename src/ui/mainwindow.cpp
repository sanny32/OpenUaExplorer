// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file mainwindow.cpp
/// \brief Implements the main application window.
///

#include <utility>

#include <QAction>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QEvent>
#include <QFileDialog>
#include <QTimer>
#include <QUrl>

#include "appicons.h"
#include "application.h"
#include "appsettings.h"
#include "dialogs/callmethoddialog.h"
#include "dialogs/certificatesdialog.h"
#include "dialogs/messageboxdialog.h"
#include "dialogs/namespaceinspectordialog.h"
#include "dialogs/nodemonitordialog.h"
#include "dialogs/dialogabout.h"
#include "dialogs/dialogopcuainfo.h"
#include "widgets/dataview.h"
#include "widgets/subscriptionswidget.h"
#include "dialogs/settingsdialog.h"
#include "features/builtinfeatures.h"
#include "features/featurecommands.h"
#include "features/featurehost.h"
#include "features/featuremanager.h"
#include "features/selectioncontext.h"
#include "mainwindow.h"
#include "connectioncoordinator.h"
#include "dataaccesscoordinator.h"
#include "sessioncoordinator.h"
#include "themecoordinator.h"
#include "opcua/connectioncontroller.h"
#include "opcua/opcuabackend.h"
#include "addressspacemodule.h"
#include "attributemodule.h"
#include "dataaccessmodule.h"
#include "eventsmodule.h"
#include "servicecontext.h"
#include "servicemodulemanager.h"
#include "referencemodule.h"
#include "servermodule.h"
#include "updatechecker.h"
#include "ui_mainwindow.h"
#include "widgets/maintoolbar.h"
#include "widgets/themedtoolbutton.h"

///
/// \brief Builds the window, wires up menus, docks, icons, and the OPC UA client.
/// \param parent Parent widget.
///
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , _connectionController(new ConnectionController(this))
    , _backend(_connectionController->backend())
{
    ui->setupUi(this);

    const bool manualThemeSupported = theApp()->theme().isManualToggleSupported();
    ui->actionTheme->setVisible(manualThemeSupported);
    ui->menuTheme->menuAction()->setVisible(manualThemeSupported);
    configureHistoryUi();

    ui->mainToolBar->setupFromDesignerActions();

    setupDockOptions();
    bindIcons();
    _themeCoordinator = new ThemeCoordinator(ui->actionTheme,
                                             ui->actionThemeLight,
                                             ui->actionThemeDark,
                                             ui->actionThemeSystem,
                                             this);

    setupOpcUaClient();
    setupUpdateChecker();
    setupModules();
    setupSessionCoordinator();
    resetLayout();
    restoreSettings();
    ui->actionOpenSession->setEnabled(true);
    ui->actionExportLog->setEnabled(true);
    updateClientUi(_backend->state());
    _sessionCoordinator->rebuildRecentSessionsMenu();

    auto *modifiedTimer = new QTimer(this);
    modifiedTimer->setInterval(500);
    connect(modifiedTimer, &QTimer::timeout,
            _sessionCoordinator, &SessionCoordinator::updateModifiedState);
    modifiedTimer->start();
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
        setWindowIcon(AppIcons::application());
    }
}

///
/// \brief Persists the window layout and element state before the window closes.
/// \param event Close event being handled.
///
void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!_sessionCoordinator->maybeSaveSession()) {
        event->ignore();
        return;
    }
    saveSettings();
    QMainWindow::closeEvent(event);
}

///
/// \brief Opens the connection dialog to create a new connection.
///
void MainWindow::on_actionNewConnection_triggered()
{
    _connectionCoordinator->openConnectionDialog();
}

///
/// \brief Prompts for a session file and restores its connection and workspace.
///
void MainWindow::on_actionOpenSession_triggered()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Open Session"), QString(),
        tr("Session Files (*.ouas);;All Files (*)"));
    if (!path.isEmpty())
        _sessionCoordinator->openSessionFromFile(path);
}

///
/// \brief Loads a session file the desktop shell asked the application to open.
/// \param path Path to the session file.
///
void MainWindow::openSessionFile(const QString &path)
{
    if (path.isEmpty())
        return;

    _sessionCoordinator->openSessionFromFile(path);
}

///
/// \brief Prompts for a destination file and saves the current session.
///
void MainWindow::on_actionSaveSession_triggered()
{
    _sessionCoordinator->saveCurrentSession();
}

///
/// \brief Exports the currently visible central-area view to a file.
///
void MainWindow::on_actionExportData_triggered()
{
    _dataAccessCoordinator->exportActiveView();
}

///
/// \brief Exports the activity log to a file.
///
void MainWindow::on_actionExportLog_triggered()
{
    _featureManager->triggerCommand(FeatureCommandIds::logExport());
}

///
/// \brief Opens the connection dialog to connect to a server.
///
void MainWindow::on_actionConnect_triggered()
{
    _connectionCoordinator->openConnectionDialog();
}

///
/// \brief Disconnects from the current endpoint.
///
void MainWindow::on_actionDisconnect_triggered()
{
    _connectionCoordinator->disconnectFromServer();
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
    const QString nodeId = _selectionContext->currentNode().nodeId;
    _addressSpaceModule->refresh(nodeId);
    if (!nodeId.isEmpty())
        _referenceModule->browseReferences(nodeId);
}

///
/// \brief Shows a read-only summary of the active connection's endpoint settings.
///
void MainWindow::on_actionEndpointSettings_triggered()
{
    _connectionCoordinator->showEndpointSettings();
}

///
/// \brief Opens the PKI certificate management dialog.
///
void MainWindow::on_actionCertificates_triggered()
{
    CertificatesDialog dialog(this);
    dialog.exec();
}

///
/// \brief Opens the namespace inspector for the active connection.
///
void MainWindow::on_actionNamespaceInspector_triggered()
{
    NamespaceInspectorDialog dialog(_backend, &_namespaceCache, this);
    dialog.exec();
}

///
/// \brief Opens a new, empty node monitor window ready to accept a dropped node.
///
void MainWindow::on_actionNodeMonitor_triggered()
{
    NodeMonitorDialog *monitor = createNodeMonitor();
    monitor->show();
    monitor->raise();
    monitor->activateWindow();
}

///
/// \brief Creates an independent modeless node monitor bound to the backend.
/// \return The new node monitor instance, tracked until it is closed.
///
NodeMonitorDialog *MainWindow::createNodeMonitor()
{
    auto *monitor = new NodeMonitorDialog(_backend, this);
    monitor->setAttribute(Qt::WA_DeleteOnClose, true);
    if (SubscriptionsWidget *subscriptions = ui->dataView->subscriptions()) {
        monitor->setSubscriptions(subscriptions->subscriptions());
        connect(subscriptions, &SubscriptionsWidget::subscriptionsChanged,
                monitor, &NodeMonitorDialog::setSubscriptions);
        connect(monitor, &NodeMonitorDialog::subscriptionCreationRequested,
                subscriptions, &SubscriptionsWidget::createSubscription);
    }
    connect(monitor, &QObject::destroyed, this, [this, monitor]() {
        _nodeMonitors.removeOne(monitor);
    });
    _nodeMonitors.append(monitor);
    return monitor;
}

///
/// \brief Monitors a variable node, raising an existing window for it when one is open.
/// \param node Variable node to monitor.
///
void MainWindow::openNodeMonitor(const OpcUaNodeInfo &node)
{
    NodeMonitorDialog *monitor = nullptr;
    for (NodeMonitorDialog *candidate : std::as_const(_nodeMonitors)) {
        if (candidate->nodeId() == node.nodeId) {
            monitor = candidate;
            break;
        }
    }
    if (!monitor) {
        monitor = createNodeMonitor();
        monitor->setTarget(node);
    }
    monitor->show();
    monitor->raise();
    monitor->activateWindow();
}

///
/// \brief Closes every open node monitor window.
///
void MainWindow::closeNodeMonitors()
{
    const QList<NodeMonitorDialog *> monitors = _nodeMonitors;
    for (NodeMonitorDialog *monitor : monitors)
        monitor->close();
}

///
/// \brief Opens a method-call dialog for a method selected in the address space.
/// \param object Object node that owns the method.
/// \param method Method node to call.
///
void MainWindow::openCallMethod(const OpcUaNodeInfo &object, const OpcUaNodeInfo &method)
{
    auto *dialog = new CallMethodDialog(_backend, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    dialog->setTarget(object, method);
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}

///
/// \brief Reads the currently selected node.
///
void MainWindow::on_actionRead_triggered()
{
    _dataAccessCoordinator->readSelected();
}

///
/// \brief Reads the currently selected node.
///
void MainWindow::on_actionReadSelected_triggered()
{
    _dataAccessCoordinator->readSelected();
}

///
/// \brief Opens the write dialog for the selected node's current value.
///
void MainWindow::on_actionWrite_triggered()
{
    _dataAccessCoordinator->writeSelected();
}

///
/// \brief Opens the write dialog for the selected node's current value.
///
void MainWindow::on_actionWriteValue_triggered()
{
    _dataAccessCoordinator->writeSelected();
}

///
/// \brief Starts monitoring the selected variable and adds it to Data Access.
///
void MainWindow::on_actionSubscribe_triggered()
{
    _dataAccessCoordinator->subscribeSelected();
}

///
/// \brief Stops monitoring the selected variable.
///
void MainWindow::on_actionUnsubscribe_triggered()
{
    _dataAccessCoordinator->unsubscribeSelected();
}

///
/// \brief Adds the selected variable node to the data-access view.
///
void MainWindow::on_actionAddToDataAccess_triggered()
{
    _dataAccessCoordinator->addSelectedToView();
}

///
/// \brief Removes the selected data-access nodes from the data-access view.
///
void MainWindow::on_actionRemoveFromDataAccess_triggered()
{
    _dataAccessCoordinator->removeSelectionFromView();
}

///
/// \brief Removes every node from the data-access view.
///
void MainWindow::on_actionClearDataAccess_triggered()
{
    _dataAccessCoordinator->clearView();
}

///
/// \brief Unsubscribes the selected data-access nodes, leaving them in the view.
///
void MainWindow::on_actionSetSubscriptionNone_triggered()
{
    _dataAccessCoordinator->applyNoSubscription();
}

///
/// \brief Assigns the selected data-access nodes to the built-in Default subscription.
///
void MainWindow::on_actionSetSubscriptionDefault_triggered()
{
    _dataAccessCoordinator->applyDefaultSubscription();
}

///
/// \brief Assigns the selected data-access nodes to the built-in Fast subscription.
///
void MainWindow::on_actionSetSubscriptionFast_triggered()
{
    _dataAccessCoordinator->applyFastSubscription();
}

///
/// \brief Creates a new subscription and assigns the selected data-access nodes to it.
///
void MainWindow::on_actionSetSubscriptionCustom_triggered()
{
    _dataAccessCoordinator->promptCustomSubscription();
}

///
/// \brief Reads the data history of the selected variable node.
///
void MainWindow::on_actionReadDataHistory_triggered()
{
    _dataAccessCoordinator->readDataHistoryForSelected();
}

///
/// \brief Reads the event history of the selected node.
///
void MainWindow::on_actionReadEventsHistory_triggered()
{
    _dataAccessCoordinator->readEventsHistoryForSelected();
}

///
/// \brief Closes the main window.
///
void MainWindow::on_actionExit_triggered()
{
    close();
}

///
/// \brief Opens the application settings dialog.
///
void MainWindow::on_actionSettings_triggered()
{
    openSettingsDialog();
}

///
/// \brief Cycles the toolbar button through the Light, Dark and System schemes.
///
void MainWindow::on_actionTheme_triggered()
{
    _themeCoordinator->cycle();
}

///
/// \brief Shows the OPC UA information dialog.
///
void MainWindow::on_actionOpcUaInfo_triggered()
{
    DialogOpcUaInfo dialog(this);
    dialog.exec();
}

///
/// \brief Starts a check for a newer application release.
///
void MainWindow::on_actionCheckForUpdates_triggered()
{
    _updateChecker->checkForUpdates();
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
/// \brief Switches the data-access widget to the Data Access page.
///
void MainWindow::on_actionViewDataAccess_triggered()
{
    _dataAccessCoordinator->showDataAccessPage();
}

///
/// \brief Opens the subscriptions management dialog.
///
void MainWindow::on_actionManageSubscriptions_triggered()
{
    _dataAccessCoordinator->showSubscriptionsDialog();
}

///
/// \brief Switches the data-access widget to the Events page.
///
void MainWindow::on_actionViewEvents_triggered()
{
    _dataAccessCoordinator->showEventsPage();
}

///
/// \brief Switches the data-access widget to the Data History page.
///
void MainWindow::on_actionViewDataHistory_triggered()
{
    _dataAccessCoordinator->showDataHistoryPage();
}

///
/// \brief Switches the data-access widget to the Events History page.
///
void MainWindow::on_actionViewEventsHistory_triggered()
{
    _dataAccessCoordinator->showEventsHistoryPage();
}

///
/// \brief Restores the default dock layout.
///
void MainWindow::on_actionResetLayout_triggered()
{
    resetLayout();
}

///
/// \brief Opens the settings dialog and restores the default layout if requested.
///
void MainWindow::openSettingsDialog()
{
    SettingsDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted && dialog.layoutResetRequested())
        resetLayout();
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
    _featureManager->resetDockLayout(*this);
    ui->centralSplitter->setSizes({360, 310});
}

///
/// \brief Saves the window geometry, dock layout, splitter, page, and view element state.
///
void MainWindow::saveSettings()
{
    AppSettings settings;
    settings.setWindowGeometry(saveGeometry());
    settings.setWindowState(saveState());
    settings.setCentralSplitterState(ui->centralSplitter->saveState());
    _featureManager->saveState(settings);
    _dataAccessCoordinator->saveState(settings);
}

///
/// \brief Restores the saved window layout and element state over the default layout.
///
/// Falls back to the default layout established by resetLayout() when nothing is
/// stored or the user disabled layout restoration.
///
void MainWindow::restoreSettings()
{
    AppSettings settings;
    _dataAccessCoordinator->loadSubscriptions(settings);

    if (!settings.restoreLayoutOnStartup())
        return;

    const QByteArray geometry = settings.windowGeometry();
    if (!geometry.isEmpty())
        restoreGeometry(geometry);

    const QByteArray state = settings.windowState();
    if (!state.isEmpty())
        restoreState(state);

    const QByteArray splitterState = settings.centralSplitterState();
    if (!splitterState.isEmpty())
        ui->centralSplitter->restoreState(splitterState);

    _featureManager->restoreState(settings);
    _dataAccessCoordinator->restoreState(settings);
}

///
/// \brief Removes HistoryRead entry points when the linked Qt OPC UA API cannot serve them.
///
void MainWindow::configureHistoryUi()
{
    const bool supported = OpcUa::isHistoryReadSupported();
    ui->actionReadDataHistory->setVisible(supported);
    ui->actionReadEventsHistory->setVisible(supported);
    ui->actionViewDataHistory->setVisible(supported);
    ui->actionViewEventsHistory->setVisible(supported);
    if (supported)
        return;

    ui->menuView->removeAction(ui->actionViewDataHistory);
    ui->menuView->removeAction(ui->actionViewEventsHistory);
    ui->menuData->removeAction(ui->actionReadDataHistory);
    ui->menuData->removeAction(ui->actionReadEventsHistory);
    ui->mainToolBar->removeAction(ui->actionReadDataHistory);
    ui->mainToolBar->removeAction(ui->actionReadEventsHistory);
}

///
/// \brief Wires the backend and connection controller signals to the UI.
///
void MainWindow::setupOpcUaClient()
{
    connect(_backend, &OpcUaBackend::stateChanged,
            this, &MainWindow::updateClientUi);

    ConnectionActions connectionActions;
    connectionActions.connect = ui->actionConnect;
    connectionActions.newConnection = ui->actionNewConnection;
    connectionActions.disconnect = ui->actionDisconnect;
    connectionActions.browse = ui->actionBrowse;
    connectionActions.browseAddressSpace = ui->actionBrowseAddressSpace;
    connectionActions.refresh = ui->actionRefresh;
    connectionActions.endpointSettings = ui->actionEndpointSettings;
    _connectionCoordinator = new ConnectionCoordinator(_connectionController,
                                                       _backend,
                                                       ui->menuRecentConnections,
                                                       ui->mainToolBar->favoritesButton(),
                                                       connectionActions,
                                                       this);
    ui->statusbar->setConnectionController(_connectionController);

    connect(ui->actionFileDisconnect, &QAction::triggered,
            ui->actionDisconnect, &QAction::trigger);
    connect(ui->actionDisconnect, &QAction::changed, this, [this] {
        ui->actionFileDisconnect->setEnabled(ui->actionDisconnect->isEnabled());
    });
    ui->actionFileDisconnect->setEnabled(ui->actionDisconnect->isEnabled());
}

///
/// \brief Creates the update checker and wires it to the Help menu action.
///
void MainWindow::setupUpdateChecker()
{
    _updateChecker = new UpdateChecker(this);
    ui->actionCheckForUpdates->setEnabled(true);

    connect(_updateChecker, &UpdateChecker::checkStarted, this, [this] {
        ui->actionCheckForUpdates->setEnabled(false);
    });

    connect(_updateChecker, &UpdateChecker::noUpdatesAvailable, this, [this] {
        ui->actionCheckForUpdates->setEnabled(true);
        MessageBoxDialog::information(this,
                                     tr("Check for Updates"),
                                     tr("You are running the latest version."));
    });

    connect(_updateChecker, &UpdateChecker::checkFailed, this, [this](const QString &errorString) {
        ui->actionCheckForUpdates->setEnabled(true);
        MessageBoxDialog::warning(this,
                                  tr("Check for Updates"),
                                  tr("Failed to check for updates.\n\n%1").arg(errorString),
                                  DialogButtonBox::Ok);
    });

    connect(_updateChecker, &UpdateChecker::newVersionAvailable, this,
            [this](const QString &version, const QString &url) {
                ui->actionCheckForUpdates->setEnabled(true);
                const auto answer = MessageBoxDialog::question(
                    this,
                    tr("Update Available"),
                    tr("Version %1 is available.\n\nOpen the download page?").arg(version),
                    DialogButtonBox::Yes | DialogButtonBox::No,
                    DialogButtonBox::Yes);
                if (answer == DialogButtonBox::Yes)
                    QDesktopServices::openUrl(QUrl(url));
            });
}

///
/// \brief Registers data modules, initializes UI features, and wires shared flows.
///
void MainWindow::setupModules()
{
    _moduleManager = new ServiceModuleManager(this);
    _featureManager = new FeatureManager(this);
    _selectionContext = new SelectionContext(this);

    _serverModule = new ServerModule;
    _addressSpaceModule = new AddressSpaceModule;
    _attributeModule = new AttributeModule;
    _referenceModule = new ReferenceModule;
    _dataAccessModule = new DataAccessModule;
    _eventsModule = new EventsModule;

    _moduleManager->registerModule(_serverModule);
    _moduleManager->registerModule(_addressSpaceModule);
    _moduleManager->registerModule(_attributeModule);
    _moduleManager->registerModule(_referenceModule);
    _moduleManager->registerModule(_dataAccessModule);
    _moduleManager->registerModule(_eventsModule);

    FeatureHost host(this,
                     ui->menuView,
                     ui->mainToolBar,
                     _backend,
                     _connectionController,
                     _moduleManager,
                     _featureManager,
                     _selectionContext);
    registerBuiltinFeatures(*_featureManager);
    _featureManager->initializeAll(host);

    ServiceContext context(_backend, _connectionController);
    _moduleManager->initializeAll(context);

    DataAccessActions dataAccessActions;
    dataAccessActions.read = ui->actionRead;
    dataAccessActions.readSelected = ui->actionReadSelected;
    dataAccessActions.write = ui->actionWrite;
    dataAccessActions.writeValue = ui->actionWriteValue;
    dataAccessActions.subscribe = ui->actionSubscribe;
    dataAccessActions.unsubscribe = ui->actionUnsubscribe;
    dataAccessActions.addToDataAccess = ui->actionAddToDataAccess;
    dataAccessActions.removeFromDataAccess = ui->actionRemoveFromDataAccess;
    dataAccessActions.clearDataAccess = ui->actionClearDataAccess;
    dataAccessActions.setSubscriptionNone = ui->actionSetSubscriptionNone;
    dataAccessActions.setSubscriptionDefault = ui->actionSetSubscriptionDefault;
    dataAccessActions.setSubscriptionFast = ui->actionSetSubscriptionFast;
    dataAccessActions.setSubscriptionCustom = ui->actionSetSubscriptionCustom;
    dataAccessActions.readDataHistory = ui->actionReadDataHistory;
    dataAccessActions.readEventsHistory = ui->actionReadEventsHistory;
    _dataAccessCoordinator = new DataAccessCoordinator(ui->dataView,
                                                       ui->trendPanelWidget,
                                                       _dataAccessModule,
                                                       _eventsModule,
                                                       _attributeModule,
                                                       _selectionContext,
                                                       _backend,
                                                       dataAccessActions,
                                                       this);

    connect(_selectionContext, &SelectionContext::monitorNodeRequested,
            this, &MainWindow::openNodeMonitor);
    connect(_selectionContext, &SelectionContext::callMethodRequested,
            this, &MainWindow::openCallMethod);
}

void MainWindow::setupSessionCoordinator()
{
    SessionCoordinatorContext context;
    context.window = this;
    context.recentSessionsMenu = ui->menuRecentSessions;
    context.connectionController = _connectionController;
    context.connectionCoordinator = _connectionCoordinator;
    context.dataAccessCoordinator = _dataAccessCoordinator;
    context.featureManager = _featureManager;
    context.backend = _backend;
    context.captureNodeMonitors = [this] {
        QVector<SessionNodeMonitor> monitors;
        for (NodeMonitorDialog *monitor : _nodeMonitors) {
            if (!monitor->nodeId().isEmpty())
                monitors.append(monitor->captureSession());
        }
        return monitors;
    };
    context.restoreNodeMonitor = [this](const SessionNodeMonitor &monitorState) {
        NodeMonitorDialog *monitor = createNodeMonitor();
        monitor->restoreSession(monitorState);
        monitor->show();
    };
    _sessionCoordinator = new SessionCoordinator(context, this);
}

///
/// \brief Refreshes the views for the client state.
/// \param state Current OPC UA client state.
///
void MainWindow::updateClientUi(OpcUaConnectionState state)
{
    const bool connected = state == OpcUaConnectionState::Connected;
    const bool idle = state == OpcUaConnectionState::Disconnected
        || state == OpcUaConnectionState::Unavailable;
    ui->actionNamespaceInspector->setEnabled(connected);
    ui->actionNodeMonitor->setEnabled(connected);
    ui->actionSaveSession->setEnabled(connected);
    ui->actionExportData->setEnabled(connected);
    if (connected) {
        initializeAddressSpace();
        _sessionCoordinator->applyPendingSession();
    } else if (idle) {
        _dataAccessCoordinator->clearRuntimeState();
        _selectionContext->clear();
        _featureManager->clearRuntimeState();
        _namespaceCache = {};
        closeNodeMonitors();
        _sessionCoordinator->closeCurrentSession();
    }
}

///
/// \brief Seeds the address-space view with the Objects-folder root node.
///
void MainWindow::initializeAddressSpace()
{
    _featureManager->triggerCommand(FeatureCommandIds::addressSpaceBrowse());
}

///
/// \brief Binds themed icons to the window and its actions.
///
void MainWindow::bindIcons()
{
    setWindowIcon(AppIcons::application());
    AppIcons::bindIcon(ui->actionConnect,     "connect");
    AppIcons::bindIcon(ui->actionDisconnect,  "disconnect");
    AppIcons::bindIcon(ui->actionBrowse,      "browse");
    AppIcons::bindIcon(ui->actionRefresh,     "refresh");
    AppIcons::bindIcon(ui->actionRead,        "read");
    AppIcons::bindIcon(ui->actionReadDataHistory, "history");
    AppIcons::bindIcon(ui->actionReadEventsHistory, "event-history");
    AppIcons::bindIcon(ui->actionWrite,       "write");
    AppIcons::bindIcon(ui->actionSubscribe,   "subscribe");
    AppIcons::bindIcon(ui->actionUnsubscribe, "unsubscribe");
    AppIcons::bindIcon(ui->actionSettings,    "settings");
    AppIcons::bindIcon(ui->actionCertificates, "certificate");
    AppIcons::bindIcon(ui->actionFavorites,   "star");
    AppIcons::bindIcon(ui->actionNodeMonitor, "trend");
}

