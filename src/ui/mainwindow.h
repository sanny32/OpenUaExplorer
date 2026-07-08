// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file mainwindow.h
/// \brief Declares the main application window.
///

#pragma once

#include <QByteArray>
#include <QList>
#include <QMainWindow>
#include <QString>

#include "dialogs/namespaceinspectordialog.h"
#include "opcua/opcuatypes.h"
#include "session/sessiondata.h"

namespace Ui {
class MainWindow;
}

///
/// \brief Main application window that coordinates docks, toolbar and theme actions.
///
class MainWindow : public QMainWindow
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
    void on_actionOpenSession_triggered();
    void on_actionSaveSession_triggered();
    void on_actionExportData_triggered();
    void on_actionExportLog_triggered();
    void on_actionConnect_triggered();
    void on_actionDisconnect_triggered();
    void on_actionBrowse_triggered();
    void on_actionBrowseAddressSpace_triggered();
    void on_actionRefresh_triggered();
    void on_actionEndpointSettings_triggered();
    void on_actionCertificates_triggered();
    void on_actionNamespaceInspector_triggered();
    void on_actionNodeMonitor_triggered();
    void on_actionRead_triggered();
    void on_actionReadSelected_triggered();
    void on_actionWrite_triggered();
    void on_actionWriteValue_triggered();
    void on_actionSubscribe_triggered();
    void on_actionUnsubscribe_triggered();
    void on_actionAddToDataAccess_triggered();
    void on_actionRemoveFromDataAccess_triggered();
    void on_actionClearDataAccess_triggered();
    void on_actionSetSubscriptionNone_triggered();
    void on_actionSetSubscriptionDefault_triggered();
    void on_actionSetSubscriptionFast_triggered();
    void on_actionSetSubscriptionCustom_triggered();
    void on_actionReadDataHistory_triggered();
    void on_actionReadEventsHistory_triggered();
    void on_actionExit_triggered();
    void on_actionSettings_triggered();
    void on_actionTheme_triggered();
    void on_actionAbout_triggered();
    void on_actionViewDataAccess_triggered();
    void on_actionManageSubscriptions_triggered();
    void on_actionViewEvents_triggered();
    void on_actionViewDataHistory_triggered();
    void on_actionViewEventsHistory_triggered();
    void on_actionResetLayout_triggered();

private:
    void openSettingsDialog();
    void setupDockOptions();
    void resetLayout();
    void saveSettings();
    void restoreSettings();
    void bindIcons();
    void configureHistoryUi();
    void setupOpcUaClient();
    void setupPlugins();
    void updateClientUi(OpcUaConnectionState state);
    bool saveSessionToFile(const QString &path);
    bool saveCurrentSession();
    void openSessionFromFile(const QString &path);
    void applyPendingSession();
    SessionData sessionWorkspace() const;
    SessionData collectSessionData() const;
    void setCurrentSessionPath(const QString &path);
    void closeCurrentSession();
    QString sessionDisplayName() const;
    void updateWindowTitle();
    void updateModifiedState();
    bool maybeSaveSession();
    void initializeAddressSpace();
    class NodeMonitorDialog *createNodeMonitor();
    void openNodeMonitor(const OpcUaNodeInfo &node);
    void closeNodeMonitors();
    void openCallMethod(const OpcUaNodeInfo &object, const OpcUaNodeInfo &method);

private:
    Ui::MainWindow *ui;
    class ConnectionController *_connectionController;
    class OpcUaClientService *_clientService;
    class ServiceModuleManager *_pluginManager = nullptr;
    class FeatureManager *_featureManager = nullptr;
    class SelectionContext *_selectionContext = nullptr;
    class ServerModule *_serverPlugin = nullptr;
    class AddressSpaceModule *_addressSpacePlugin = nullptr;
    class AttributeModule *_attributePlugin = nullptr;
    class ReferenceModule *_referencePlugin = nullptr;
    class DataAccessModule *_dataAccessPlugin = nullptr;
    class EventsModule *_eventsPlugin = nullptr;
    class ThemeCoordinator *_themeCoordinator = nullptr;
    class ConnectionCoordinator *_connectionCoordinator = nullptr;
    class DataAccessCoordinator *_dataAccessCoordinator = nullptr;
    QList<class NodeMonitorDialog *> _nodeMonitors;
    NamespaceInspectorCache _namespaceCache;
    SessionData _pendingSession;
    QString _pendingSessionPath;
    bool _hasPendingSession = false;
    QString _sessionPath;
    QByteArray _savedSessionFingerprint;
};
