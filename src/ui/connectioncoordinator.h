// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file connectioncoordinator.h
/// \brief Declares the coordinator of the connection, favourites and recents flows.
///

#pragma once

#include <QObject>

#include "opcua/certificatetrustdecider.h"
#include "opcua/connectioncredentialsprovider.h"
#include "opcua/opcuatypes.h"

class ConnectionController;
class FavoritesCoordinator;
class OpcUaBackend;
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
/// certificate trust and credential prompts.
///
class ConnectionCoordinator : public QObject,
                              public CertificateTrustDecider,
                              public ConnectionCredentialsProvider
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the coordinator and wires the connection flows.
    /// \param controller Connection controller owning profiles and recents.
    /// \param backend Backend driving the connection state.
    /// \param recentMenu Recent Connections menu rebuilt from the history.
    /// \param favoritesButton Toolbar button anchoring the favourites popup.
    /// \param actions Menu and toolbar actions steered by the coordinator.
    /// \param dialogParent Parent widget for dialogs; also the QObject owner.
    ///
    ConnectionCoordinator(ConnectionController *controller,
                          OpcUaBackend *backend,
                          QMenu *recentMenu,
                          QToolButton *favoritesButton,
                          const ConnectionActions &actions,
                          QWidget *dialogParent);

    ///
    /// \brief Releases a connection-attempt cursor still owned by the coordinator.
    ///
    ~ConnectionCoordinator() override;

    ///
    /// \brief Runs the connection dialog and connects (optionally saving) the chosen profile.
    /// \param preset Profile used to pre-fill the dialog, or nullptr for a blank dialog.
    /// \return True when the user accepted the dialog and a connection was started.
    ///
    bool openConnectionDialog(const ConnectionProfile *preset = nullptr);

    ///
    /// \brief Disconnects from the current endpoint.
    ///
    void disconnectFromServer();

    ///
    /// \brief Reports whether the connection dropped instead of being closed by the user.
    /// \return True from a lost connection until it is back or the user starts another one.
    ///
    bool connectionLost() const;

    ///
    /// \brief Reports whether a reconnect attempt is scheduled or running.
    /// \return True while the lost connection is being retried.
    ///
    bool isReconnecting() const;

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

    ///
    /// \brief Asks for the credentials a profile needs but has none stored for.
    /// \param profile Profile waiting to connect.
    /// \param rejection Reason the previous credentials were turned down, empty on a first ask.
    /// \return The entered credentials, or a rejected result when the user cancels.
    ///
    ConnectionCredentials requestCredentials(const ConnectionProfile &profile,
                                             const QString &rejection) override;

signals:
    ///
    /// \brief Emitted when the user gives up on a lost connection instead of retrying it.
    ///
    void sessionAbandoned();

private:
    void openFavorites();
    void addCurrentToFavorites();
    void connectFavorite(const ConnectionProfile &favorite);
    void editFavorite(const ConnectionProfile &favorite);
    void connectRecentProfile();
    void rebuildRecentMenu();
    void updateActions(OpcUaConnectionState state);
    void trackConnectionState(OpcUaConnectionState state);
    void scheduleReconnect();
    void attemptReconnect();
    void stopReconnect();
    void beginConnectionAttempt();
    void endConnectionAttempt();
    void onClientError(const QString &message);

    ConnectionController *_controller;
    OpcUaBackend *_backend;
    QMenu *_recentMenu;
    QToolButton *_favoritesButton;
    ConnectionActions _actions;
    QWidget *_dialogParent;
    FavoritesCoordinator *_favorites;
    class QTimer *_reconnectTimer;
    bool _disconnectRequested = false;
    bool _wasConnected = false;
    bool _connectionLost = false;
    bool _retryInProgress = false;
    bool _connectionCursorActive = false;
};
