// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file servermodule.cpp
/// \brief Implements connection lifecycle logging.
///

#include "servermodule.h"

#include <QLoggingCategory>

#include "opcua/connectioncontroller.h"
#include "opcua/connectionprofile.h"
#include "opcua/opcuabackend.h"
#include "servicecontext.h"

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

ServerModule::ServerModule(QObject *parent)
    : ServiceModule(parent)
{
}

///
/// \brief Returns the module name shown in the startup log.
///
QString ServerModule::name() const
{
    return tr("Server Module");
}

///
/// \brief Returns the Server logging category.
///
const QLoggingCategory &ServerModule::logCategory() const
{
    return lcServer();
}

///
/// \brief Subscribes to the backend connection state changes.
/// \param context Host context providing the backend and controller.
///
void ServerModule::initialize(ServiceContext &context)
{
    _backend = context.backend();
    _connectionController = context.connectionController();
    connect(_backend, &OpcUaBackend::stateChanged,
            this, &ServerModule::handleStateChanged);
}

///
/// \brief Logs the endpoint block on connect and every connection state transition.
/// \param state New connection state.
///
void ServerModule::handleStateChanged(OpcUaConnectionState state)
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
