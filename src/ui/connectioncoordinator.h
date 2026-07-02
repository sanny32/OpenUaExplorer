// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file connectioncoordinator.h
/// \brief Declares the coordinator of the connection, favourites and recents flows.
///

#pragma once

#include <QObject>

#include "opcua/certificatetrustdecider.h"
#include "opcua/opcuatypes.h"

class ConnectionController;
class FavoritesCoordinator;
class OpcUaClientService;
class QAction;
class QMenu;
class QToolButton;
struct ConnectionProfile;

///
/// \brief Menu and toolbar actions steered by the connection coordinator.
///
struct ConnectionActions
{
    QAction *connect = nullptr;
    QAction *newConnection = nullptr;
    QAction *disconnect = nullptr;
    QAction *browse = nullptr;
    QAction *browseAddressSpace = nullptr;
    QAction *refresh = nullptr;
    QAction *endpointSettings = nullptr;
};

///
/// \brief Coordinates the connection dialogs, favourites, recents and trust prompts.
///
/// Owns the favourites popup coordinator, rebuilds the Recent Connections menu,
/// enables the connection actions for the client state, and answers the server
/// certificate trust prompt.
///
class ConnectionCoordinator : public QObject, public CertificateTrustDecider
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the coordinator and wires the connection flows.
    /// \param controller Connection controller owning profiles and recents.
    /// \param clientService Client service driving the connection state.
    /// \param recentMenu Recent Connections menu rebuilt from the history.
    /// \param favoritesButton Toolbar button anchoring the favourites popup.
    /// \param actions Menu and toolbar actions steered by the coordinator.
    /// \param dialogParent Parent widget for dialogs; also the QObject owner.
    ///
    ConnectionCoordinator(ConnectionController *controller,
                          OpcUaClientService *clientService,
                          QMenu *recentMenu,
                          QToolButton *favoritesButton,
                          const ConnectionActions &actions,
                          QWidget *dialogParent);

    ///
    /// \brief Runs the connection dialog and connects (optionally saving) the chosen profile.
    /// \param preset Profile used to pre-fill the dialog, or nullptr for a blank dialog.
    ///
    void openConnectionDialog(const ConnectionProfile *preset = nullptr);

    ///
    /// \brief Disconnects from the current endpoint.
    ///
    void disconnectFromServer();

    ///
    /// \brief Shows a read-only summary of the active connection's endpoint settings.
    ///
    void showEndpointSettings();

    ///
    /// \brief Shows the certificate prompt and returns the selected trust policy.
    /// \param certificate Server certificate awaiting a trust decision.
    /// \param message Validation message to display.
    /// \return Selected certificate trust policy.
    ///
    CertificateTrustDecision decide(const QByteArray &certificate,
                                    const QString &message) override;

private:
    void openFavorites();
    void addCurrentToFavorites();
    void connectFavorite(const ConnectionProfile &favorite);
    void editFavorite(const ConnectionProfile &favorite);
    void connectRecentProfile();
    void rebuildRecentMenu();
    void updateActions(OpcUaConnectionState state);
    void onClientError(const QString &message);

    ConnectionController *_controller;
    OpcUaClientService *_clientService;
    QMenu *_recentMenu;
    QToolButton *_favoritesButton;
    ConnectionActions _actions;
    QWidget *_dialogParent;
    FavoritesCoordinator *_favorites;
};
