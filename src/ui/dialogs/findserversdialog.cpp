// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file findserversdialog.cpp
/// \brief Implements the discovery-server browser dialog.
///

#include <QGuiApplication>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QStringList>

#include "appcolors.h"
#include "findserversdialog.h"
#include "models/serverdiscoverymodel.h"
#include "opcua/opcuabackend.h"
#include "ui_findserversdialog.h"
#include "widgets/historycombobox.h"
#include "widgets/serverdiscoverywidget.h"

namespace {

/// \brief Settings group keeping the discovery-server URLs apart from endpoint URLs.
constexpr auto discoveryHistoryGroup = "findServersDialog";

}

///
/// \brief Builds the dialog and restores the discovery URL history.
/// \param parent Parent widget.
///
FindServersDialog::FindServersDialog(QWidget *parent)
    : AppBaseDialog(parent)
    , ui(new Ui::FindServersDialog)
    , _discoveryHistoryStore(QString::fromLatin1(discoveryHistoryGroup))
{
    ui->setupUi(this);
    ui->useEndpointButton->setColors(
        { AppColors::accent(), AppColors::accentHover(), AppColors::accentPressed() });

    setupHistory();
    setupConnections();
    updateEndpointSelection();
    setStatus(tr("%n server(s) found.", "", 0), QStringLiteral("info"));
}

///
/// \brief Saves the last discovery URL and destroys the dialog.
///
FindServersDialog::~FindServersDialog()
{
    setBusy(false);
    saveDiscoveryUrl();
    delete ui;
}

///
/// \brief Sets the backend used for discovery and subscribes to its results.
/// \param backend OPC UA backend.
///
void FindServersDialog::setBackend(OpcUaBackend *backend)
{
    if (_service)
        disconnect(_service, nullptr, this, nullptr);
    _service = backend;
    if (!_service)
        return;

    connect(_service, &OpcUaBackend::serversDiscovered,
            this, &FindServersDialog::handleServers);
    connect(_service, &OpcUaBackend::endpointsDiscovered,
            this, &FindServersDialog::handleEndpoints);
}

///
/// \brief Sets the backend name and timeout used for every request.
/// \param backendName Preferred backend plugin name.
/// \param timeoutMs Request timeout in milliseconds.
///
void FindServersDialog::setRequestDefaults(const QString &backendName, int timeoutMs)
{
    _backendName = backendName;
    _timeoutMs = timeoutMs;
}

///
/// \brief Returns the endpoint the user accepted the dialog with.
/// \return Selected endpoint, or a default-constructed value when cancelled.
///
EndpointInfo FindServersDialog::selectedEndpoint() const
{
    return _selectedEndpoint;
}

///
/// \brief Populates the discovery URL combo box from the persisted history.
///
void FindServersDialog::setupHistory()
{
    QStringList defaults;
    for (int index = 0; index < ui->discoveryUrlComboBox->count(); ++index)
        defaults.append(ui->discoveryUrlComboBox->itemText(index));

    _discoveryHistoryStore.seedIfUninitialized(defaults);
    const QStringList history = _discoveryHistoryStore.history();

    ui->discoveryUrlComboBox->setHistory(history);
    ui->discoveryUrlComboBox->setEditText(history.isEmpty() ? QString()
                                                            : history.constFirst());
}

///
/// \brief Wires the buttons, the combo box and the server tree.
///
void FindServersDialog::setupConnections()
{
    connect(ui->findServersButton, &QPushButton::clicked, this, &FindServersDialog::findServers);
    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->useEndpointButton, &QPushButton::clicked,
            this, &FindServersDialog::acceptSelectedEndpoint);
    connect(ui->discoveryUrlComboBox, &HistoryComboBox::itemRemoved,
            this, &FindServersDialog::forgetDiscoveryUrl);
    connect(ui->discoveryUrlComboBox->lineEdit(), &QLineEdit::returnPressed,
            this, &FindServersDialog::findServers);
    connect(ui->serversWidget, &ServerDiscoveryWidget::currentEndpointChanged,
            this, &FindServersDialog::updateEndpointSelection);
    connect(ui->serversWidget, &ServerDiscoveryWidget::serverExpanded,
            this, &FindServersDialog::handleServerExpanded);
    connect(ui->serversWidget, &ServerDiscoveryWidget::endpointActivated,
            this, &FindServersDialog::acceptSelectedEndpoint);
}

///
/// \brief Starts a FindServers request for the entered discovery URL.
///
void FindServersDialog::findServers()
{
    if (!_service) {
        setStatus(tr("The OPC UA backend is unavailable."), QStringLiteral("alert-circle"));
        return;
    }
    if (_findingServers)
        return;

    saveDiscoveryUrl();
    _pendingServerRows.clear();
    _activeServerRow = -1;
    ui->serversWidget->clear();
    updateEndpointSelection();

    _findingServers = true;
    setBusy(true);
    ui->findServersButton->setEnabled(false);
    setStatus(tr("Searching for servers…"), QStringLiteral("info"));
    _service->findServers(ui->discoveryUrlComboBox->currentText(), _backendName, _timeoutMs);
}

///
/// \brief Shows the discovered servers or the reported error.
/// \param servers Discovered servers.
/// \param error Discovery error.
///
void FindServersDialog::handleServers(QList<ServerInfo> servers, const QString &error)
{
    _findingServers = false;
    setBusy(false);
    ui->findServersButton->setEnabled(true);

    if (!error.isEmpty()) {
        ui->serversWidget->clear();
        updateEndpointSelection();
        setStatus(error, QStringLiteral("alert-circle"));
        return;
    }

    ui->serversWidget->setServers(servers);
    updateEndpointSelection();
    setStatus(tr("%n server(s) found.", "", servers.size()),
              servers.isEmpty() ? QStringLiteral("info") : QStringLiteral("check-circle"));
}

///
/// \brief Queues a GetEndpoints request for a server the user expanded.
/// \param serverRow Row of the expanded server.
///
void FindServersDialog::handleServerExpanded(int serverRow)
{
    ServerDiscoveryModel *model = ui->serversWidget->model();
    if (model->serverState(serverRow) != ServerDiscoveryModel::LoadState::NotLoaded)
        return;

    if (model->discoveryUrl(serverRow).isEmpty()) {
        model->setServerState(serverRow, ServerDiscoveryModel::LoadState::Failed,
                              tr("The server advertises no opc.tcp discovery URL."));
        return;
    }

    model->setServerState(serverRow, ServerDiscoveryModel::LoadState::Loading);
    _pendingServerRows.enqueue(serverRow);
    startNextEndpointRequest();
}

///
/// \brief Fills in the endpoints of the server whose request just completed.
/// \param endpoints Discovered endpoints.
/// \param error Discovery error.
///
void FindServersDialog::handleEndpoints(QList<EndpointInfo> endpoints, const QString &error)
{
    if (_activeServerRow < 0)
        return;

    ServerDiscoveryModel *model = ui->serversWidget->model();
    const int serverRow = _activeServerRow;
    _activeServerRow = -1;

    if (error.isEmpty())
        model->setServerEndpoints(serverRow, endpoints);
    else
        model->setServerState(serverRow, ServerDiscoveryModel::LoadState::Failed, error);

    startNextEndpointRequest();
}

///
/// \brief Issues the next queued GetEndpoints request when the client is free.
///
/// The backend owns a single client, so overlapping requests would cancel each other.
///
void FindServersDialog::startNextEndpointRequest()
{
    if (!_service || _activeServerRow >= 0 || _findingServers || _pendingServerRows.isEmpty())
        return;

    _activeServerRow = _pendingServerRows.dequeue();
    const QString url = ui->serversWidget->model()->discoveryUrl(_activeServerRow);
    _service->discoverEndpoints(url, _backendName, _timeoutMs);
}

///
/// \brief Enables the accept button only while an endpoint row is selected.
///
void FindServersDialog::updateEndpointSelection()
{
    ui->useEndpointButton->setEnabled(ui->serversWidget->hasEndpointSelection());
}

///
/// \brief Stores the selected endpoint and closes the dialog.
///
void FindServersDialog::acceptSelectedEndpoint()
{
    if (!ui->serversWidget->hasEndpointSelection())
        return;
    _selectedEndpoint = ui->serversWidget->currentEndpoint();
    accept();
}

///
/// \brief Records the entered discovery URL and refreshes the combo box history.
///
void FindServersDialog::saveDiscoveryUrl()
{
    const QString discoveryUrl = ui->discoveryUrlComboBox->currentText().trimmed();
    if (discoveryUrl.isEmpty())
        return;

    _discoveryHistoryStore.save(discoveryUrl);

    const QSignalBlocker blocker(ui->discoveryUrlComboBox);
    ui->discoveryUrlComboBox->setHistory(_discoveryHistoryStore.history());
    ui->discoveryUrlComboBox->setEditText(discoveryUrl);
}

///
/// \brief Drops a discovery URL from the persisted history.
/// \param discoveryUrl URL removed from the popup.
///
void FindServersDialog::forgetDiscoveryUrl(const QString &discoveryUrl)
{
    _discoveryHistoryStore.remove(discoveryUrl);
}

///
/// \brief Shows or hides the wait cursor for a running FindServers request.
/// \param busy True while the request is in flight.
///
void FindServersDialog::setBusy(bool busy)
{
    if (busy == _busyCursorActive)
        return;
    if (busy)
        QGuiApplication::setOverrideCursor(Qt::WaitCursor);
    else
        QGuiApplication::restoreOverrideCursor();
    _busyCursorActive = busy;
}

///
/// \brief Updates the footer status line.
/// \param text Status text.
/// \param iconName Themed icon shown next to the text.
///
void FindServersDialog::setStatus(const QString &text, const QString &iconName)
{
    ui->statusIconLabel->setIcon(iconName, QSize(16, 16));
    ui->statusLabel->setText(text);
}
