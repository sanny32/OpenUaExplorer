// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file mainwindow.cpp
/// \brief Implements the main application window.
///

#include <algorithm>

#include <QAction>
#include <QActionGroup>
#include <QCloseEvent>
#include <QDateTime>
#include <QDockWidget>
#include <QEvent>
#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <QSplitter>

#include "appicons.h"
#include "application.h"
#include "appsettings.h"
#include "dialogs/certificatetrustdialog.h"
#include "dialogs/dialogabout.h"
#include "dialogs/connectiondialog.h"
#include "dialogs/editfavoritedialog.h"
#include "dialogs/settingsdialog.h"
#include "dialogs/writevaluedialog.h"
#include "loggingcategories.h"
#include "mainwindow.h"
#include "opcua/connectioncontroller.h"
#include "opcua/opcuaclientservice.h"
#include "opcua/standardnodeid.h"
#include "addressspaceplugin.h"
#include "attributeplugin.h"
#include "dataaccessplugin.h"
#include "plugincontext.h"
#include "pluginmanager.h"
#include "referenceplugin.h"
#include "serverplugin.h"
#include "ui_mainwindow.h"
#include "widgets/addressspacewidget.h"
#include "widgets/attributeswidget.h"
#include "widgets/dataaccesswidget.h"
#include "widgets/favoriteswidget.h"
#include "widgets/logwidget.h"
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
    ui->nodeDetailsDock->setWidget(ui->addressSpaceWidget->takeNodeDetailsPanel());

    const bool manualThemeSupported = theApp()->theme().isManualToggleSupported();
    ui->actionTheme->setVisible(manualThemeSupported);
    ui->menuTheme->menuAction()->setVisible(manualThemeSupported);

    setupMainMenu();
    ui->mainToolBar->setupFromDesignerActions();

    setupDockOptions();
    bindIcons();
    setupThemeControls();

    resetLayout();
    setupOpcUaClient();
    setupPlugins();
    rebuildRecentConnections();
    restoreSettings();
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
    _addressSpacePlugin->refresh(nodeId);
    if (!nodeId.isEmpty())
        _referencePlugin->browseReferences(nodeId);
}

///
/// \brief Reads the currently selected node.
///
void MainWindow::on_actionRead_triggered()
{
    const OpcUaNodeInfo selected = ui->addressSpaceWidget->selectedNode();
    if (!selected.nodeId.isEmpty())
        _attributePlugin->read(selected.nodeId);
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
/// \brief Starts monitoring the selected variable and adds it to Data Access.
///
void MainWindow::on_actionSubscribe_triggered()
{
    if (!OpcUa::isVariable(_selectedNodeDetails.nodeClass)
        || _selectedNodeDetails.nodeId.isEmpty()) {
        return;
    }
    ui->dataAccessWidget->addNode(_selectedNodeDetails);
    _pendingMonitoringNodeIds.insert(_selectedNodeDetails.nodeId);
    updateMonitoringActions();
    _dataAccessPlugin->subscribe(_selectedNodeDetails.nodeId);
}

///
/// \brief Stops monitoring the selected variable.
///
void MainWindow::on_actionUnsubscribe_triggered()
{
    if (_selectedNodeDetails.nodeId.isEmpty()
        || !_subscribedNodeIds.contains(_selectedNodeDetails.nodeId)) {
        return;
    }
    _pendingMonitoringNodeIds.insert(_selectedNodeDetails.nodeId);
    updateMonitoringActions();
    _dataAccessPlugin->unsubscribe(_selectedNodeDetails.nodeId);
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
    cycleTheme();
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
void MainWindow::openConnectionDialog(const ConnectionProfile *preset)
{
    ConnectionDialog dialog(this);
    dialog.setClientService(_clientService);
    if (preset)
        dialog.setProfile(*preset);
    if (dialog.exec() != QDialog::Accepted)
        return;

    // Editing an existing favourite saves the changes back to it; a plain connect does not
    // touch favourites, so reconnecting a server with different security never overwrites it.
    const QList<ConnectionProfile> favorites = _connectionController->profiles();
    const bool editingFavorite = preset && std::any_of(
        favorites.cbegin(), favorites.cend(), [preset](const ConnectionProfile &favorite) {
            return favorite.id == preset->id;
        });
    const ConnectionProfile profile = dialog.profile();
    if (editingFavorite) {
        _connectionController->saveProfile(
            profile, dialog.password(), dialog.privateKeyPassword());
    }
    _connectionController->connectNewProfile(
        profile, dialog.password(), dialog.privateKeyPassword());
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
/// \brief Saves the window geometry, dock layout, splitter, page, and view element state.
///
void MainWindow::saveSettings()
{
    AppSettings settings;
    settings.setWindowGeometry(saveGeometry());
    settings.setWindowState(saveState());
    settings.setCentralSplitterState(ui->centralSplitter->saveState());
    settings.setDataAccessPage(ui->dataAccessWidget->currentPage());
    ui->addressSpaceWidget->saveViewState(settings);
    ui->attributesWidget->saveViewState(settings);
    ui->dataAccessWidget->saveViewState(settings);
    ui->logWidget->saveViewState(settings);
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

    ui->dataAccessWidget->setCurrentPage(
        static_cast<DataAccessWidget::Page>(settings.dataAccessPage()));

    ui->addressSpaceWidget->restoreViewState(settings);
    ui->attributesWidget->restoreViewState(settings);
    ui->dataAccessWidget->restoreViewState(settings);
    ui->logWidget->restoreViewState(settings);

    ui->actionViewAddressSpace->setChecked(!ui->addressSpaceDock->isHidden());
    ui->actionViewNodeDetails->setChecked(!ui->nodeDetailsDock->isHidden());
    ui->actionViewActivity->setChecked(!ui->logDock->isHidden());
}

///
/// \brief Wires the toolbar button and Theme submenu to the three colour-scheme modes.
///
void MainWindow::setupThemeControls()
{
    QActionGroup *group = new QActionGroup(this);
    group->setExclusive(true);

    const bool manualThemeSupported = theApp()->theme().isManualToggleSupported();
    const QList<QAction *> modeActions = {
        ui->actionThemeLight, ui->actionThemeDark, ui->actionThemeSystem};
    for (QAction *action : modeActions) {
        action->setCheckable(true);
        action->setEnabled(manualThemeSupported);
        group->addAction(action);
    }

    connect(ui->actionThemeLight, &QAction::triggered, this,
            [this] { applyThemeMode(AppSettings::ThemeMode::Light); });
    connect(ui->actionThemeDark, &QAction::triggered, this,
            [this] { applyThemeMode(AppSettings::ThemeMode::Dark); });
    connect(ui->actionThemeSystem, &QAction::triggered, this,
            [this] { applyThemeMode(AppSettings::ThemeMode::System); });

    connect(&theApp()->theme(), &AppTheme::colorSchemeChanged, this,
            &MainWindow::updateThemeControls);

    updateThemeControls();
}

///
/// \brief Reflects the saved colour-scheme mode in the toolbar icon and Theme submenu.
///
void MainWindow::updateThemeControls()
{
    if (!ui)
        return;

    switch (AppSettings().themeMode()) {
    case AppSettings::ThemeMode::Light:
        ui->actionTheme->setIcon(AppIcons::themed(QStringLiteral("theme-light")));
        ui->actionTheme->setToolTip(tr("Theme: Light — click to switch to Dark"));
        ui->actionThemeLight->setChecked(true);
        break;
    case AppSettings::ThemeMode::Dark:
        ui->actionTheme->setIcon(AppIcons::themed(QStringLiteral("theme-dark")));
        ui->actionTheme->setToolTip(tr("Theme: Dark — click to switch to System"));
        ui->actionThemeDark->setChecked(true);
        break;
    case AppSettings::ThemeMode::System:
        ui->actionTheme->setIcon(AppIcons::themed(QStringLiteral("theme-system")));
        ui->actionTheme->setToolTip(tr("Theme: System — click to switch to Light"));
        ui->actionThemeSystem->setChecked(true);
        break;
    }
}

///
/// \brief Advances the colour scheme to the next mode: Light → Dark → System → Light.
///
void MainWindow::cycleTheme()
{
    AppSettings::ThemeMode next = AppSettings::ThemeMode::Light;
    switch (AppSettings().themeMode()) {
    case AppSettings::ThemeMode::Light:
        next = AppSettings::ThemeMode::Dark;
        break;
    case AppSettings::ThemeMode::Dark:
        next = AppSettings::ThemeMode::System;
        break;
    case AppSettings::ThemeMode::System:
        next = AppSettings::ThemeMode::Light;
        break;
    }
    applyThemeMode(next);
}

///
/// \brief Applies and persists a colour-scheme mode, then refreshes the theme controls.
/// \param mode Light, Dark or System mode to activate.
///
void MainWindow::applyThemeMode(AppSettings::ThemeMode mode)
{
    theApp()->theme().setColorSchemePreference(mode);
    updateThemeControls();
}

///
/// \brief Wires the client service and connection controller signals to the UI.
///
void MainWindow::setupOpcUaClient()
{
    connect(_clientService, &OpcUaClientService::stateChanged,
            this, &MainWindow::updateClientUi);
    connect(ui->dataAccessWidget, &DataAccessWidget::addSelectedNodeRequested,
            this, &MainWindow::on_actionAddToDataAccess_triggered);
    connect(ui->dataAccessWidget, &DataAccessWidget::nodeDropRequested,
            this, &MainWindow::addNodeToDataAccess);
    connect(ui->dataAccessWidget, &DataAccessWidget::writeRequested,
            this, &MainWindow::showWriteDialog);
    connect(_connectionController, &ConnectionController::recentsChanged,
            this, &MainWindow::rebuildRecentConnections);

    _favoritesWidget = new FavoritesWidget(this);
    connect(_favoritesWidget, &FavoritesWidget::connectRequested,
            this, [this](const ConnectionProfile &profile) {
        _connectionController->connectSavedProfile(profile);
    });
    connect(_favoritesWidget, &FavoritesWidget::editRequested,
            this, &MainWindow::editFavorite);
    connect(_favoritesWidget, &FavoritesWidget::removeRequested,
            this, [this](const QString &id) {
        _connectionController->removeFavorite(id);
    });
    connect(_favoritesWidget, &FavoritesWidget::addFavoriteRequested,
            this, &MainWindow::addCurrentToFavorites);
    connect(_favoritesWidget, &FavoritesWidget::reorderRequested,
            this, [this](const QStringList &orderedIds) {
        _connectionController->reorderFavorites(orderedIds);
    });
    connect(_connectionController, &ConnectionController::profilesChanged, this, [this] {
        if (_favoritesWidget->isVisible())
            _favoritesWidget->setFavorites(_connectionController->profiles());
    });
    connect(ui->mainToolBar->favoritesButton(), &QToolButton::clicked,
            this, &MainWindow::openFavorites);
    connect(_connectionController, &ConnectionController::errorOccurred,
            this, &MainWindow::onClientError);
    _connectionController->setCertificateTrustDecider(this);
    ui->statusbar->setConnectionController(_connectionController);
    updateClientUi(_clientService->state());
}

///
/// \brief Registers the static plugins and wires their data API to the widgets.
///
/// Each plugin exposes an API for its slice of server data and logs that area's operations
/// under its own source. MainWindow is the composition root: it connects widget signals to
/// plugin API calls and plugin result signals back to the widgets.
///
void MainWindow::setupPlugins()
{
    _pluginManager = new PluginManager(this);
    _serverPlugin = new ServerPlugin;
    _addressSpacePlugin = new AddressSpacePlugin;
    _attributePlugin = new AttributePlugin;
    _referencePlugin = new ReferencePlugin;
    _dataAccessPlugin = new DataAccessPlugin;
    _pluginManager->registerPlugin(_serverPlugin);
    _pluginManager->registerPlugin(_addressSpacePlugin);
    _pluginManager->registerPlugin(_attributePlugin);
    _pluginManager->registerPlugin(_referencePlugin);
    _pluginManager->registerPlugin(_dataAccessPlugin);

    PluginContext context(_clientService, _connectionController);
    _pluginManager->initializeAll(context);

    connect(ui->addressSpaceWidget, &AddressSpaceWidget::browseRequested,
            _addressSpacePlugin, &AddressSpacePlugin::browse);
    connect(ui->addressSpaceWidget, &AddressSpaceWidget::refreshRequested,
            _addressSpacePlugin, &AddressSpacePlugin::refresh);
    connect(_addressSpacePlugin, &AddressSpacePlugin::childrenReady,
            ui->addressSpaceWidget, &AddressSpaceWidget::setBrowseChildren);
    connect(ui->addressSpaceWidget, &AddressSpaceWidget::nodeSelected,
            this, [this](const OpcUaNodeInfo &node) { _attributePlugin->read(node.nodeId); });

    connect(ui->addressSpaceWidget, &AddressSpaceWidget::referencesRequested,
            _referencePlugin, &ReferencePlugin::browseReferences);
    connect(_referencePlugin, &ReferencePlugin::referencesReady,
            ui->addressSpaceWidget, &AddressSpaceWidget::setBrowseReferences);

    connect(ui->attributesWidget, &AttributesWidget::writeRequested,
            _attributePlugin, &AttributePlugin::write);
    connect(_attributePlugin, &AttributePlugin::attributesReady,
            this, &MainWindow::onNodeDetailsReady);
    connect(_attributePlugin, &AttributePlugin::writeFinished,
            this, &MainWindow::onWriteFinished);

    connect(ui->dataAccessWidget, &DataAccessWidget::readRequested,
            _dataAccessPlugin, &DataAccessPlugin::read);
    connect(ui->dataAccessWidget, &DataAccessWidget::monitoringRequested,
            _dataAccessPlugin, &DataAccessPlugin::subscribe);
    connect(ui->dataAccessWidget, &DataAccessWidget::monitoringCancelled,
            _dataAccessPlugin, &DataAccessPlugin::unsubscribe);
    connect(_dataAccessPlugin, &DataAccessPlugin::valuesReady,
            this, &MainWindow::onDataValuesReady);
    connect(_dataAccessPlugin, &DataAccessPlugin::monitoringFinished,
            this, &MainWindow::onMonitoringFinished);
}

///
/// \brief Logs an error reported by the connection controller.
///
/// Backend and client-service errors are already logged at their source in
/// QtOpcUaBackend::setError(); this slot covers controller-level errors only.
///
/// \param message Error reported by the connection controller.
///
void MainWindow::onClientError(const QString &message)
{
    qCWarning(lcClient) << message;
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

    const bool variable = OpcUa::isVariable(details.nodeClass);
    if (_pendingDataAccessNodeIds.remove(details.nodeId) && variable)
        ui->dataAccessWidget->addNodeWithDefaultSubscription(details);

    if (details.nodeId != ui->addressSpaceWidget->selectedNode().nodeId)
        return;

    _selectedNodeDetails = details;
    ui->addressSpaceWidget->setNodeDetails(details);
    ui->attributesWidget->setNodeDetails(details);
    const bool writable = variable && OpcUa::isWritable(details.userAccessLevel);
    ui->actionRead->setEnabled(variable);
    ui->actionReadSelected->setEnabled(variable);
    ui->actionWrite->setEnabled(writable);
    ui->actionWriteValue->setEnabled(writable);
    ui->actionAddToDataAccess->setEnabled(variable);
    updateMonitoringActions();
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
        _attributePlugin->read(nodeId);
        _dataAccessPlugin->read({nodeId});
    } else {
        QMessageBox::warning(this, tr("Write Failed"), error);
    }
}

///
/// \brief Applies the result of a subscribe or unsubscribe request to the UI.
/// \param nodeId Affected node.
/// \param subscribed True for subscribe and false for unsubscribe.
/// \param success Whether the request succeeded.
/// \param error Error description, empty on success.
///
void MainWindow::onMonitoringFinished(const QString &nodeId, bool subscribed,
                                      bool success, const QString &error)
{
    _pendingMonitoringNodeIds.remove(nodeId);
    if (success) {
        if (subscribed)
            _subscribedNodeIds.insert(nodeId);
        else
            _subscribedNodeIds.remove(nodeId);
        ui->dataAccessWidget->setNodeSubscribed(nodeId, subscribed);
    } else {
        QMessageBox::warning(this,
                             subscribed ? tr("Subscribe Failed") : tr("Unsubscribe Failed"),
                             error);
    }
    updateMonitoringActions();
}

///
/// \brief Enables the monitoring actions for the selected variable's current state.
///
void MainWindow::updateMonitoringActions()
{
    const bool connected = _clientService->state() == OpcUaConnectionState::Connected;
    const bool variable = connected && OpcUa::isVariable(_selectedNodeDetails.nodeClass)
        && !_selectedNodeDetails.nodeId.isEmpty();
    const bool subscribed = variable
        && _subscribedNodeIds.contains(_selectedNodeDetails.nodeId);
    const bool pending = variable
        && _pendingMonitoringNodeIds.contains(_selectedNodeDetails.nodeId);
    ui->actionSubscribe->setEnabled(variable && !subscribed && !pending);
    ui->actionUnsubscribe->setEnabled(subscribed && !pending);
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
    updateMonitoringActions();
    if (connected) {
        initializeAddressSpace();
    } else if (state == OpcUaConnectionState::Disconnected
               || state == OpcUaConnectionState::Unavailable) {
        ui->addressSpaceWidget->clear();
        ui->attributesWidget->clear();
        ui->dataAccessWidget->clearRuntimeData();
        _selectedNodeDetails = {};
        _subscribedNodeIds.clear();
        _pendingMonitoringNodeIds.clear();
        _pendingDataAccessNodeIds.clear();
        updateMonitoringActions();
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
/// \brief Reads a node so it can be added to Data Access after its attributes arrive.
/// \param nodeId Node to add.
///
void MainWindow::addNodeToDataAccess(const QString &nodeId)
{
    if (nodeId.isEmpty())
        return;
    _pendingDataAccessNodeIds.insert(nodeId);
    _attributePlugin->read(nodeId);
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
        _attributePlugin->write(nodeId, dialog.value(), dialog.valueType());
}

///
/// \brief Rebuilds the Recent Connections menu from the recent-connection history.
///
void MainWindow::rebuildRecentConnections()
{
    ui->menuRecentConnections->clear();
    const QList<ConnectionProfile> recent = _connectionController->recentConnections();
    if (recent.isEmpty()) {
        ui->menuRecentConnections->addAction(tr("No Recent Connections"))->setEnabled(false);
        return;
    }
    for (const ConnectionProfile &profile : recent) {
        ui->menuRecentConnections->addAction(
            profile.name.isEmpty() ? profile.endpointUrl : profile.name,
            this, [this, profile]() {
                _connectionController->connectSavedProfile(profile);
            });
    }
}

///
/// \brief Opens the favourites widget beneath the toolbar button.
///
void MainWindow::openFavorites()
{
    const QList<ConnectionProfile> profiles = _connectionController->profiles();
    const ConnectionProfile active = _connectionController->activeProfile();
    const bool connected = _clientService->state() == OpcUaConnectionState::Connected;
    // The same server may be saved with different security, so a connection only counts as
    // already favourited when its endpoint, policy, and mode all match a saved profile.
    const bool alreadyFavorite = std::any_of(
        profiles.cbegin(), profiles.cend(), [&active](const ConnectionProfile &profile) {
            return profile.endpointUrl == active.endpointUrl
                && profile.securityPolicy == active.securityPolicy
                && profile.securityMode == active.securityMode;
        });
    _favoritesWidget->setCanAddFavorite(
        connected && !active.endpointUrl.isEmpty() && !alreadyFavorite);
    _favoritesWidget->showFor(profiles, ui->mainToolBar->favoritesButton());
}

///
/// \brief Saves the current connection as a favourite, or opens the dialog if none is active.
///
void MainWindow::addCurrentToFavorites()
{
    ConnectionProfile profile = _connectionController->activeProfile();
    if (profile.endpointUrl.isEmpty()) {
        openConnectionDialog();
        return;
    }
    profile.saveProfile = true;
    profile.lastUsed = QDateTime::currentDateTime();
    _connectionController->saveProfile(profile, QString(), QString());
}

///
/// \brief Edits a favourite's server URL and security policy/mode, saving the changes.
/// \param favorite Favourite to edit.
///
void MainWindow::editFavorite(const ConnectionProfile &favorite)
{
    EditFavoriteDialog dialog(this);
    dialog.setProfile(favorite);
    if (dialog.exec() != QDialog::Accepted)
        return;
    _connectionController->saveProfile(dialog.profile(), QString(), QString());
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
    AppIcons::bindIcon(ui->actionWrite,       "write");
    AppIcons::bindIcon(ui->actionSubscribe,   "subscribe");
    AppIcons::bindIcon(ui->actionUnsubscribe, "unsubscribe");
    AppIcons::bindIcon(ui->actionSettings,    "settings");
}
