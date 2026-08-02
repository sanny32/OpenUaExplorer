// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file findserversdialog.h
/// \brief Declares the discovery-server browser dialog.
///

#pragma once

#include <QList>
#include <QQueue>

#include "dialogs/appbasedialog.h"
#include "opcua/endpointhistorystore.h"
#include "opcua/opcuatypes.h"

namespace Ui {
class FindServersDialog;
}

///
/// \brief Lists the servers registered with a discovery server and their endpoints.
///
/// FindServers only returns application descriptions, so the endpoints of a server are
/// fetched with a separate GetEndpoints request when its node is expanded. The backend
/// drives a single client, so those requests are queued and issued one at a time.
///
class FindServersDialog : public AppBaseDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the dialog and restores the discovery URL history.
    /// \param parent Parent widget.
    ///
    explicit FindServersDialog(QWidget *parent = nullptr);

    ///
    /// \brief Saves the last discovery URL and destroys the dialog.
    ///
    ~FindServersDialog() override;

    ///
    /// \brief Sets the backend used for discovery and subscribes to its results.
    /// \param backend OPC UA backend.
    ///
    void setBackend(class OpcUaBackend *backend);

    ///
    /// \brief Sets the backend name and timeout used for every request.
    /// \param backendName Preferred backend plugin name.
    /// \param timeoutMs Request timeout in milliseconds.
    ///
    void setRequestDefaults(const QString &backendName, int timeoutMs);

    ///
    /// \brief Returns the endpoint the user accepted the dialog with.
    /// \return Selected endpoint, or a default-constructed value when cancelled.
    ///
    EndpointInfo selectedEndpoint() const;

private slots:
    void findServers();
    void handleServers(QList<ServerInfo> servers, const QString &error);
    void handleEndpoints(QList<EndpointInfo> endpoints, const QString &error);
    void handleServerExpanded(int serverRow);
    void updateEndpointSelection();
    void acceptSelectedEndpoint();

private:
    void setupHistory();
    void setupConnections();
    void saveDiscoveryUrl();
    void forgetDiscoveryUrl(const QString &discoveryUrl);
    void startNextEndpointRequest();
    void setBusy(bool busy);
    void setStatus(const QString &text, const QString &iconName);

    Ui::FindServersDialog *ui;
    class OpcUaBackend *_service = nullptr;
    EndpointHistoryStore _discoveryHistoryStore;
    QString _backendName;
    int _timeoutMs = 5000;
    QQueue<int> _pendingServerRows;
    int _activeServerRow = -1;
    bool _findingServers = false;
    bool _busyCursorActive = false;
    EndpointInfo _selectedEndpoint;
};
