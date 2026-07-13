// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file connectioncoordinator.cpp
/// \brief Implements the coordinator of the connection, favourites and recents flows.
///

#include <algorithm>

#include <QAction>
#include <QDateTime>
#include <QMenu>
#include <QToolButton>

#include "connectioncoordinator.h"
#include "dialogs/certificatetrustdialog.h"
#include "dialogs/connectioncredentialsdialog.h"
#include "dialogs/connectiondialog.h"
#include "dialogs/editfavoritedialog.h"
#include "dialogs/endpointsettingsdialog.h"
#include "favoritescoordinator.h"
#include "loggingcategories.h"
#include "opcua/connectioncontroller.h"
#include "opcua/opcuabackend.h"

///
/// \brief Builds the coordinator and wires the connection flows.
/// \param controller Connection controller owning profiles and recents.
/// \param backend Backend driving the connection state.
/// \param recentMenu Recent Connections menu rebuilt from the history.
/// \param favoritesButton Toolbar button anchoring the favourites popup.
/// \param actions Menu and toolbar actions steered by the coordinator.
/// \param dialogParent Parent widget for dialogs; also the QObject owner.
///
ConnectionCoordinator::ConnectionCoordinator(ConnectionController *controller,
                                             OpcUaBackend *backend,
                                             QMenu *recentMenu,
                                             QToolButton *favoritesButton,
                                             const ConnectionActions &actions,
                                             QWidget *dialogParent)
    : QObject(dialogParent)
    , _controller(controller)
    , _backend(backend)
    , _recentMenu(recentMenu)
    , _favoritesButton(favoritesButton)
    , _actions(actions)
    , _dialogParent(dialogParent)
    , _favorites(new FavoritesCoordinator(controller, backend, dialogParent))
{
    connect(_controller, &ConnectionController::recentsChanged,
            this, &ConnectionCoordinator::rebuildRecentMenu);
    connect(_controller, &ConnectionController::errorOccurred,
            this, &ConnectionCoordinator::onClientError);
    connect(_backend, &OpcUaBackend::stateChanged,
            this, &ConnectionCoordinator::updateActions);
    connect(_favoritesButton, &QToolButton::clicked,
            this, &ConnectionCoordinator::openFavorites);
    connect(_favorites, &FavoritesCoordinator::connectRequested,
            this, &ConnectionCoordinator::connectFavorite);
    connect(_favorites, &FavoritesCoordinator::editRequested,
            this, &ConnectionCoordinator::editFavorite);
    connect(_favorites, &FavoritesCoordinator::addFavoriteRequested,
            this, &ConnectionCoordinator::addCurrentToFavorites);
    _controller->setCertificateTrustDecider(this);
    rebuildRecentMenu();
    updateActions(_backend->state());
}

///
/// \brief Runs the connection dialog and connects (optionally saving) the chosen profile.
/// \param preset Profile used to pre-fill the dialog, or nullptr for a blank dialog.
///
void ConnectionCoordinator::openConnectionDialog(const ConnectionProfile *preset)
{
    ConnectionDialog dialog(_dialogParent);
    dialog.setBackend(_backend);
    if (preset)
        dialog.setProfile(*preset);
    if (dialog.exec() != QDialog::Accepted)
        return;

    // Editing an existing favourite saves the changes back to it; a plain connect does not
    // touch favourites, so reconnecting a server with different security never overwrites it.
    const QList<ConnectionProfile> favorites = _controller->profiles();
    const bool editingFavorite = preset && std::any_of(
        favorites.cbegin(), favorites.cend(), [preset](const ConnectionProfile &favorite) {
            return favorite.id == preset->id;
        });
    const ConnectionProfile profile = dialog.profile();
    if (editingFavorite) {
        _controller->saveProfile(
            profile, dialog.password(), dialog.privateKeyPassword());
    }
    _controller->connectNewProfile(
        profile, dialog.password(), dialog.privateKeyPassword());
}

///
/// \brief Disconnects from the current endpoint.
///
void ConnectionCoordinator::disconnectFromServer()
{
    _backend->disconnectFromEndpoint();
}

///
/// \brief Shows a read-only summary of the active connection's endpoint settings.
///
void ConnectionCoordinator::showEndpointSettings()
{
    EndpointSettingsDialog dialog(_dialogParent);
    dialog.setProfile(_controller->activeProfile());
    dialog.setServerCertificate(_backend->activeServerCertificate());
    dialog.exec();
}

///
/// \brief Shows the certificate prompt and returns the selected trust policy.
/// \param certificate Server certificate awaiting a trust decision.
/// \param message Validation message to display.
/// \return Selected certificate trust policy.
///
CertificateTrustDecision ConnectionCoordinator::decide(const QByteArray &certificate,
                                                       const QString &message)
{
    CertificateTrustDialog dialog(_dialogParent);
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
/// \brief Opens the favourites popup beneath the toolbar button.
///
void ConnectionCoordinator::openFavorites()
{
    _favorites->open(_favoritesButton);
}

///
/// \brief Saves the current connection as a favourite, or opens the dialog if none is active.
///
void ConnectionCoordinator::addCurrentToFavorites()
{
    ConnectionProfile profile = _controller->activeProfile();
    if (profile.endpointUrl.isEmpty()) {
        openConnectionDialog();
        return;
    }
    profile.saveProfile = true;
    profile.lastUsed = QDateTime::currentDateTime();
    _controller->saveProfile(profile, QString(), QString());
}

///
/// \brief Connects a favourite, prompting for its credentials when it needs authentication.
/// \param favorite Favourite to connect to.
///
void ConnectionCoordinator::connectFavorite(const ConnectionProfile &favorite)
{
    if (favorite.authentication == ConnectionProfile::Authentication::Anonymous) {
        _controller->connectSavedProfile(favorite);
        return;
    }

    ConnectionCredentialsDialog dialog(_dialogParent);
    dialog.setProfile(favorite);
    if (dialog.exec() != QDialog::Accepted)
        return;
    _controller->connectSavedProfileWithCredentials(
        dialog.profile(), dialog.password(), dialog.privateKeyPassword());
}

///
/// \brief Edits a favourite's server URL and security policy/mode, saving the changes.
/// \param favorite Favourite to edit.
///
void ConnectionCoordinator::editFavorite(const ConnectionProfile &favorite)
{
    EditFavoriteDialog dialog(_dialogParent);
    dialog.setProfile(favorite);
    if (dialog.exec() != QDialog::Accepted)
        return;
    _controller->saveProfile(dialog.profile(), QString(), QString());
}

///
/// \brief Connects the recent profile carried by the triggering menu action.
///
void ConnectionCoordinator::connectRecentProfile()
{
    auto *action = qobject_cast<QAction *>(sender());
    if (!action)
        return;
    const QString id = action->data().toString();
    const QList<ConnectionProfile> recent = _controller->recentConnections();
    const auto match = std::find_if(
        recent.cbegin(), recent.cend(), [&id](const ConnectionProfile &profile) {
            return profile.id == id;
        });
    if (match != recent.cend())
        _controller->connectSavedProfile(*match);
}

///
/// \brief Rebuilds the Recent Connections menu from the recent-connection history.
///
void ConnectionCoordinator::rebuildRecentMenu()
{
    _recentMenu->clear();
    const QList<ConnectionProfile> recent = _controller->recentConnections();
    if (recent.isEmpty()) {
        _recentMenu->addAction(tr("No Recent Connections"))->setEnabled(false);
        return;
    }
    for (const ConnectionProfile &profile : recent) {
        QAction *action = _recentMenu->addAction(
            profile.name.isEmpty() ? profile.endpointUrl : profile.name);
        action->setData(profile.id);
        connect(action, &QAction::triggered,
                this, &ConnectionCoordinator::connectRecentProfile);
    }
}

///
/// \brief Enables the connection actions for the client state.
/// \param state Current OPC UA client state.
///
void ConnectionCoordinator::updateActions(OpcUaConnectionState state)
{
    const bool connected = state == OpcUaConnectionState::Connected;
    const bool idle = state == OpcUaConnectionState::Disconnected
        || state == OpcUaConnectionState::Unavailable;
    _actions.connect->setEnabled(idle);
    _actions.newConnection->setEnabled(idle);
    _actions.disconnect->setEnabled(connected);
    _actions.browse->setEnabled(connected);
    _actions.browseAddressSpace->setEnabled(connected);
    _actions.refresh->setEnabled(connected);
    _actions.endpointSettings->setEnabled(connected);
}

///
/// \brief Logs an error reported by the connection controller.
///
/// Backend and client-service errors are already logged at their source in
/// QtOpcUaBackend::setError(); this slot covers controller-level errors only.
///
/// \param message Error reported by the connection controller.
///
void ConnectionCoordinator::onClientError(const QString &message)
{
    qCWarning(lcClient) << message;
}
