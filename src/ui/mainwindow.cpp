// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file mainwindow.cpp
/// \brief Implements the main application window.
///

#include <QAction>
#include <QCloseEvent>
#include <QEvent>
#include <QMenu>

#include "appicons.h"
#include "application.h"
#include "appsettings.h"
#include "dialogs/certificatesdialog.h"
#include "dialogs/dialogabout.h"
#include "dialogs/settingsdialog.h"
#include "features/builtinfeatures.h"
#include "features/featurehost.h"
#include "features/featuremanager.h"
#include "features/selectioncontext.h"
#include "mainwindow.h"
#include "connectioncoordinator.h"
#include "dataaccesscoordinator.h"
#include "themecoordinator.h"
#include "opcua/connectioncontroller.h"
#include "opcua/opcuaclientservice.h"
#include "addressspacemodule.h"
#include "attributemodule.h"
#include "dataaccessmodule.h"
#include "eventsmodule.h"
#include "servicecontext.h"
#include "servicemodulemanager.h"
#include "referencemodule.h"
#include "servermodule.h"
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
    , _clientService(_connectionController->clientService())
{
    ui->setupUi(this);
    
    setWindowTitle(QString::fromUtf8(APP_DESCRIPTION));

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
    setupPlugins();
    resetLayout();
    restoreSettings();
    updateClientUi(_clientService->state());
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
/// \brief Persists the window layout and element state before the window closes.
/// \param event Close event being handled.
///
void MainWindow::closeEvent(QCloseEvent *event)
{
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
    _addressSpacePlugin->refresh(nodeId);
    if (!nodeId.isEmpty())
        _referencePlugin->browseReferences(nodeId);
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
/// \brief Wires the client service and connection controller signals to the UI.
///
void MainWindow::setupOpcUaClient()
{
    connect(_clientService, &OpcUaClientService::stateChanged,
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
                                                       _clientService,
                                                       ui->menuRecentConnections,
                                                       ui->mainToolBar->favoritesButton(),
                                                       connectionActions,
                                                       this);
    ui->statusbar->setConnectionController(_connectionController);
}

///
/// \brief Registers data plugins, initializes UI features, and wires shared flows.
///
void MainWindow::setupPlugins()
{
    _pluginManager = new ServiceModuleManager(this);
    _featureManager = new FeatureManager(this);
    _selectionContext = new SelectionContext(this);

    _serverPlugin = new ServerModule;
    _addressSpacePlugin = new AddressSpaceModule;
    _attributePlugin = new AttributeModule;
    _referencePlugin = new ReferenceModule;
    _dataAccessPlugin = new DataAccessModule;
    _eventsPlugin = new EventsModule;

    _pluginManager->registerModule(_serverPlugin);
    _pluginManager->registerModule(_addressSpacePlugin);
    _pluginManager->registerModule(_attributePlugin);
    _pluginManager->registerModule(_referencePlugin);
    _pluginManager->registerModule(_dataAccessPlugin);
    _pluginManager->registerModule(_eventsPlugin);

    FeatureHost host(this,
                     ui->menuView,
                     ui->mainToolBar,
                     _clientService,
                     _connectionController,
                     _pluginManager,
                     _featureManager,
                     _selectionContext);
    registerBuiltinFeatures(*_featureManager);
    _featureManager->initializeAll(host);

    ServiceContext context(_clientService, _connectionController);
    _pluginManager->initializeAll(context);

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
                                                       _dataAccessPlugin,
                                                       _eventsPlugin,
                                                       _attributePlugin,
                                                       _selectionContext,
                                                       _clientService,
                                                       dataAccessActions,
                                                       this);
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
    if (connected) {
        initializeAddressSpace();
    } else if (idle) {
        _dataAccessCoordinator->clearRuntimeState();
        _selectionContext->clear();
        _featureManager->clearRuntimeState();
    }
}

///
/// \brief Seeds the address-space view with the Objects-folder root node.
///
void MainWindow::initializeAddressSpace()
{
    _featureManager->triggerCommand(QStringLiteral("addressSpace.browse"));
}

///
/// \brief Binds themed icons to the window and its actions.
///
void MainWindow::bindIcons()
{
    setWindowIcon(AppIcons::themed("app.ico"));
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
}
