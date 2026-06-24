// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file serverplugin.cpp
/// \brief Implements connection lifecycle logging.
///

#include "serverplugin.h"

#include <QLoggingCategory>

#include "opcua/connectioncontroller.h"
#include "opcua/connectionprofile.h"
#include "opcua/opcuaclientservice.h"
#include "plugincontext.h"

namespace {

Q_LOGGING_CATEGORY(lcServer, "ouaexp.Server")

/// \brief Returns the display token for a connection state.
QString connectionStateName(OpcUaConnectionState state)
{
    switch (state) {
    case OpcUaConnectionState::Unavailable: return QStringLiteral("Unavailable");
    case OpcUaConnectionState::Disconnected: return QStringLiteral("Disconnected");
    case OpcUaConnectionState::Discovering: return QStringLiteral("Discovering");
    case OpcUaConnectionState::Connecting: return QStringLiteral("Connecting");
    case OpcUaConnectionState::Connected: return QStringLiteral("Connected");
    case OpcUaConnectionState::Closing: return QStringLiteral("Closing");
    }
    return QStringLiteral("Unknown");
}

/// \brief Returns the display token for a profile's authentication mode.
QString authenticationName(ConnectionProfile::Authentication authentication)
{
    switch (authentication) {
    case ConnectionProfile::Authentication::Anonymous: return QStringLiteral("Anonymous");
    case ConnectionProfile::Authentication::Username: return QStringLiteral("Username");
    case ConnectionProfile::Authentication::Certificate: return QStringLiteral("Certificate");
    }
    return QStringLiteral("Anonymous");
}

} // namespace

ServerPlugin::ServerPlugin(QObject *parent)
    : Plugin(parent)
{
}

///
/// \brief Returns the plugin name shown in the startup log.
///
QString ServerPlugin::name() const
{
    return tr("Server Plugin");
}

///
/// \brief Returns the Server logging category.
///
const QLoggingCategory &ServerPlugin::logCategory() const
{
    return lcServer();
}

///
/// \brief Subscribes to the client service connection state changes.
/// \param context Host context providing the client service and controller.
///
void ServerPlugin::initialize(PluginContext &context)
{
    _clientService = context.clientService();
    _connectionController = context.connectionController();
    connect(_clientService, &OpcUaClientService::stateChanged,
            this, &ServerPlugin::handleStateChanged);
}

///
/// \brief Logs the endpoint block on connect and every connection state transition.
/// \param state New connection state.
///
void ServerPlugin::handleStateChanged(OpcUaConnectionState state)
{
    if (state == OpcUaConnectionState::Connecting && _connectionController) {
        const ConnectionProfile &profile = _connectionController->activeProfile();
        if (!profile.endpointUrl.isEmpty()) {
            qCInfo(lcServer).noquote() << tr("Endpoint: '%1'").arg(profile.endpointUrl);
            qCInfo(lcServer).noquote() << tr("Security policy: '%1'").arg(profile.securityPolicy);
            qCInfo(lcServer).noquote() << tr("Security mode: %1").arg(profile.securityMode);
            qCInfo(lcServer).noquote() << tr("Authentication: %1")
                                              .arg(authenticationName(profile.authentication));
        }
    }
    qCInfo(lcServer).noquote()
        << tr("Connection state changed to '%1'.").arg(connectionStateName(state));
}
