// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file mainwindow.h
/// \brief Declares the main application window.
///

#pragma once

#include <QMainWindow>
#include <QSet>

#include "appsettings.h"
#include "opcua/certificatetrustdecider.h"
#include "opcua/opcuatypes.h"

namespace Ui {
class MainWindow;
}

struct ConnectionProfile;

///
/// \brief Main application window that coordinates docks, toolbar and theme actions.
///
class MainWindow : public QMainWindow, private CertificateTrustDecider
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the window, wires up menus, docks, icons, and the OPC UA client.
    /// \param parent Parent widget.
    ///
    explicit MainWindow(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the window and its generated UI.
    ///
    ~MainWindow() override;

protected:
    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void on_actionNewConnection_triggered();
    void on_actionConnect_triggered();
    void on_actionDisconnect_triggered();
    void on_actionBrowse_triggered();
    void on_actionBrowseAddressSpace_triggered();
    void on_actionRefresh_triggered();
    void on_actionRead_triggered();
    void on_actionReadSelected_triggered();
    void on_actionWrite_triggered();
    void on_actionWriteValue_triggered();
    void on_actionSubscribe_triggered();
    void on_actionUnsubscribe_triggered();
    void on_actionAddToDataAccess_triggered();
    void on_actionExit_triggered();
    void on_actionSettings_triggered();
    void on_actionTheme_triggered();
    void on_actionAbout_triggered();
    void on_actionViewDataAccess_triggered();
    void on_actionViewSubscriptions_triggered();
    void on_actionViewEvents_triggered();
    void on_actionViewHistory_triggered();
    void on_actionResetLayout_triggered();

private:
    void openConnectionDialog(const ConnectionProfile *preset = nullptr);
    void addCurrentToFavorites();
    void editFavorite(const ConnectionProfile &favorite);
    void openSettingsDialog();
    void setupDockOptions();
    void resetLayout();
    void saveSettings();
    void restoreSettings();
    void bindIcons();
    void setupThemeControls();
    void updateThemeControls();
    void cycleTheme();
    void applyThemeMode(AppSettings::ThemeMode mode);
    void setupOpcUaClient();
    void setupPlugins();
    void updateClientUi(OpcUaConnectionState state);
    void initializeAddressSpace();
    void addNodeToDataAccess(const QString &nodeId);
    void showWriteDialog(const QString &nodeId, const QVariant &value, int valueType,
                         const QString &dataTypeId, bool writable);
    void rebuildRecentConnections();
    void openFavorites();

    // OPC UA client signal handlers (wired up in setupOpcUaClient).
    void onClientError(const QString &message);
    void onSelectedNodeChanged(const OpcUaNodeInfo &node);
    void onAttributeDetailsReady(const OpcUaNodeDetails &details, const QString &error);
    void onNodeDetailsReady(const OpcUaNodeDetails &details, const QString &error);
    void onSelectionCleared();
    void onDataValuesReady(const QVector<OpcUaDataValue> &values, const QString &error);
    void onWriteFinished(const QString &nodeId, bool success, const QString &error);
    void onMonitoringFinished(const QString &nodeId, bool subscribed,
                              bool success, const QString &error);
    void updateMonitoringActions();
    CertificateTrustDecision decide(const QByteArray &certificate,
                                    const QString &message) override;

private:
    Ui::MainWindow *ui;
    class ConnectionController *_connectionController;
    class OpcUaClientService *_clientService;
    class PluginManager *_pluginManager = nullptr;
    class FeatureManager *_featureManager = nullptr;
    class SelectionContext *_selectionContext = nullptr;
    class ServerPlugin *_serverPlugin = nullptr;
    class AddressSpacePlugin *_addressSpacePlugin = nullptr;
    class AttributePlugin *_attributePlugin = nullptr;
    class ReferencePlugin *_referencePlugin = nullptr;
    class DataAccessPlugin *_dataAccessPlugin = nullptr;
    class FavoritesWidget *_favoritesWidget = nullptr;
    OpcUaNodeDetails _selectedNodeDetails;
    QSet<QString> _subscribedNodeIds;
    QSet<QString> _pendingMonitoringNodeIds;
    QSet<QString> _pendingDataAccessNodeIds;
};
