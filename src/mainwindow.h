// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file mainwindow.h
/// \brief Declares the main application window.
///

#pragma once

#include <QMainWindow>

#include "opcua/connectionprofile.h"
#include "opcua/opcuatypes.h"
#include "opcua/secretstore.h"

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
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void changeEvent(QEvent *event) override;

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
    void on_actionAddToDataAccess_triggered();
    void on_actionExit_triggered();
    void on_actionTheme_triggered();
    void on_actionAbout_triggered();
    void on_actionViewAddressSpace_toggled(bool checked);
    void on_addressSpaceDock_visibilityChanged(bool visible);
    void on_actionViewActivity_toggled(bool checked);
    void on_logDock_visibilityChanged(bool visible);
    void on_actionViewDataAccess_triggered();
    void on_actionViewSubscriptions_triggered();
    void on_actionViewEvents_triggered();
    void on_actionViewHistory_triggered();
    void on_actionResetLayout_triggered();

private:
    void openConnectionDialog();
    void setupMainMenu();
    void setupDockOptions();
    void resetLayout();
    void bindIcons();
    void toggleTheme();
    void populateWithTestData();
    void setupOpcUaClient();
    void updateClientUi(OpcUaConnectionState state);
    void initializeAddressSpace();
    void showWriteDialog(const QString &nodeId, const QVariant &value, int valueType,
                         const QString &dataTypeId, bool writable);
    void saveProfile(const ConnectionProfile &profile,
                     const QString &password,
                     const QString &privateKeyPassword);
    void rebuildRecentConnections();
    void connectProfile(const ConnectionProfile &profile);
    void discoverThenConnect(const ConnectionProfile &profile,
                             const QString &password = {},
                             const QString &privateKeyPassword = {});

    // OPC UA client signal handlers (wired up in setupOpcUaClient).
    void onClientError(const QString &message);
    void browseNodeOrRoot(const QString &nodeId);
    void onNodeSelected(const OpcUaNodeInfo &node);
    void onNodeDetailsReady(const OpcUaNodeDetails &details, const QString &error);
    void onDataValuesReady(const QVector<OpcUaDataValue> &values, const QString &error);
    void onWriteFinished(const QString &nodeId, bool success, const QString &error);
    void onCertificateValidationRequired(const QByteArray &certificate,
                                         const QString &message, int *decision);
    void onSecretReadFinished(const QString &profileId, SecretStore::Secret secret,
                              const QString &value, const QString &error);

private:
    Ui::MainWindow *ui;
    class OpcUaClientService *_clientService;
    class SecretStore *_secretStore;
    class ConnectionProfileStore *_profileStore;
    ConnectionProfile _activeProfile;
    ConnectionProfile _pendingProfile;
    OpcUaNodeDetails _selectedNodeDetails;
    QString _pendingPassword;
    QString _pendingPrivateKeyPassword;
    int _pendingSecretReads = 0;
};
