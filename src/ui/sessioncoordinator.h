// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <functional>

#include <QObject>
#include <QString>

#include "session/sessiondata.h"

class ConnectionController;
class ConnectionCoordinator;
class DataAccessCoordinator;
class FeatureManager;
class OpcUaBackend;
class QMenu;
class QWidget;
enum class OpcUaConnectionState;

///
/// \brief Dependencies supplied by MainWindow so session state can be coordinated outside it.
///
/// Modeless node-monitor windows are still owned by MainWindow, so their session state is
/// exchanged through callbacks instead of making SessionCoordinator own UI windows.
///
struct SessionCoordinatorContext
{
    QWidget *window = nullptr;
    QMenu *recentSessionsMenu = nullptr;
    ConnectionController *connectionController = nullptr;
    ConnectionCoordinator *connectionCoordinator = nullptr;
    DataAccessCoordinator *dataAccessCoordinator = nullptr;
    FeatureManager *featureManager = nullptr;
    OpcUaBackend *backend = nullptr;
    std::function<QVector<SessionNodeMonitor>()> captureNodeMonitors;
    std::function<void(const SessionNodeMonitor &)> restoreNodeMonitor;
};

///
/// \brief Owns working-session persistence, recent-session menu state, and dirty tracking.
///
/// The coordinator keeps MainWindow responsible for visible widgets while centralising the
/// rules that decide when a session can be restored, saved, or considered modified.
///
class SessionCoordinator : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief Uses the supplied UI/services for prompts and session snapshot collection.
    ///
    explicit SessionCoordinator(const SessionCoordinatorContext &context,
                                QObject *parent = nullptr);

    ///
    /// \brief Releases a session-restore cursor still owned by the coordinator.
    ///
    ~SessionCoordinator() override;

    ///
    /// \brief Writes to the current session file, or prompts for one derived from the active profile.
    /// \return True when the session was written; false when the user cancels.
    ///
    bool saveCurrentSession();

    ///
    /// \brief Loads a session file and starts the connection flow needed before workspace restore.
    ///
    /// Saved workspaces are applied later by applyPendingSession(), after the active endpoint
    /// matches the loaded profile.
    ///
    void openSessionFromFile(const QString &path);

    ///
    /// \brief Rebuilds the menu from stored recent paths, hiding files that no longer exist.
    ///
    void rebuildRecentSessionsMenu();

    ///
    /// \brief Applies a loaded workspace only after the connection reaches the expected endpoint.
    ///
    void applyPendingSession();

    ///
    /// \brief Drops the current session identity after disconnect so close does not prompt to save.
    ///
    void closeCurrentSession();

    ///
    /// \brief Maintains the window's modified marker from the connected saveable workspace.
    ///
    void updateModifiedState();

    ///
    /// \brief Offers Save/Discard/Cancel for connected sessions that are unsaved or changed.
    /// \return True when the caller may continue closing or discarding the session.
    ///
    bool maybeSaveSession();

private:
    bool saveSessionToFile(const QString &path);
    SessionData sessionWorkspace() const;
    SessionData collectSessionData() const;
    void recordRecentSession(const QString &path);
    void setCurrentSessionPath(const QString &path);
    QString sessionDisplayName() const;
    void updateWindowTitle();
    void beginSessionRestore();
    void endSessionRestore();

private slots:
    void openRecentSession();
    void handleConnectionState(OpcUaConnectionState state);

private:
    SessionCoordinatorContext _context;
    SessionData _pendingSession;
    QString _pendingSessionPath;
    bool _hasPendingSession = false;
    QString _sessionPath;
    QByteArray _savedSessionFingerprint;
    bool _sessionRestoreCursorActive = false;
};
