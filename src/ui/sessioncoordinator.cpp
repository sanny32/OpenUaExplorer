// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file sessioncoordinator.cpp
/// \brief Implements working-session persistence and recent-session UI coordination.
///

#include "sessioncoordinator.h"

#include <QAction>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMenu>
#include <QRegularExpression>
#include <QStringList>

#include "connectioncoordinator.h"
#include "dataaccesscoordinator.h"
#include "dialogs/messageboxdialog.h"
#include "features/featuremanager.h"
#include "opcua/connectioncontroller.h"
#include "opcua/opcuabackend.h"
#include "session/recentsessionstore.h"
#include "session/sessionstore.h"
#include "utils.h"
#include "widgets/dialogbuttonbox.h"

namespace {

///
/// \brief Builds an order-independent fingerprint of saveable workspace content.
///
/// Cosmetic navigation state and the connection profile are excluded so browsing the tree
/// or reconnecting to the same server does not mark the session dirty.
///
QByteArray sessionFingerprint(const SessionData &data)
{
    QStringList subscriptions;
    subscriptions.reserve(data.subscriptions.size());
    for (const SubscriptionItem &item : data.subscriptions) {
        subscriptions.append(item.name + QLatin1Char('\x1f')
                             + QString::number(item.publishingInterval, 'g', 10));
    }
    subscriptions.sort();

    QStringList nodes;
    nodes.reserve(data.dataAccessNodes.size());
    for (const SessionNode &node : data.dataAccessNodes)
        nodes.append(node.nodeId + QLatin1Char('\x1f') + node.subscriptionName);
    nodes.sort();

    QStringList trends;
    trends.reserve(data.trendTabs.size());
    for (const SessionTrendTab &tab : data.trendTabs) {
        QString entry;
        entry += QString::number(tab.autoScale) + QString::number(tab.showLegend)
            + QString::number(tab.showGrid) + QString::number(tab.smoothLines)
            + QString::number(tab.lineType) + QString::number(tab.showPoints)
            + QString::number(tab.showValueTooltip) + QString::number(tab.labelMode)
            + QString::number(tab.autoScrollLive) + QLatin1Char('\x1f') + tab.liveSubscription
            + QLatin1Char('\x1f') + QString::number(tab.mode)
            + QLatin1Char('\x1f') + QString::number(tab.windowMs);
        for (const SessionTrendSeries &series : tab.series) {
            entry += QLatin1Char('\x1f') + series.nodeId + QLatin1Char('\x1d')
                + series.displayName + QLatin1Char('\x1d') + series.displayPath
                + QLatin1Char('\x1d') + series.color + QLatin1Char('\x1d')
                + QString::number(series.visible);
        }
        trends.append(entry);
    }

    QStringList monitors;
    monitors.reserve(data.nodeMonitors.size());
    for (const SessionNodeMonitor &monitor : data.nodeMonitors) {
        monitors.append(monitor.nodeId + QLatin1Char('\x1f') + monitor.subscriptionName
            + QLatin1Char('\x1f') + QString::number(monitor.alwaysOnTop)
            + QString::number(monitor.autoScale) + QString::number(monitor.stepLines)
            + QString::number(monitor.showGrid) + QString::number(monitor.showLegend)
            + QString::number(monitor.showPoints) + QString::number(monitor.showValueTooltip));
    }
    monitors.sort();

    const QString blob = subscriptions.join(QLatin1Char('\n')) + QLatin1Char('\x1e')
        + nodes.join(QLatin1Char('\n')) + QLatin1Char('\x1e')
        + trends.join(QLatin1Char('\n')) + QLatin1Char('\x1e')
        + monitors.join(QLatin1Char('\n'));
    return blob.toUtf8();
}

} // namespace

///
/// \brief Uses the supplied UI/services for prompts and session snapshot collection.
///
SessionCoordinator::SessionCoordinator(const SessionCoordinatorContext &context, QObject *parent)
    : QObject(parent)
    , _context(context)
{
    connect(_context.backend, &OpcUaBackend::stateChanged,
            this, &SessionCoordinator::handleConnectionState);
    updateWindowTitle();
}

///
/// \brief Releases a session-restore cursor still owned by the coordinator.
///
SessionCoordinator::~SessionCoordinator()
{
    endSessionRestore();
}

///
/// \brief Writes to the current session file, or prompts for one derived from the active profile.
///
bool SessionCoordinator::saveCurrentSession()
{
    if (!_sessionPath.isEmpty())
        return saveSessionToFile(_sessionPath);

    const ConnectionProfile &profile = _context.connectionController->activeProfile();
    QString base = !profile.sessionName.isEmpty() ? profile.sessionName : profile.name;
    base.remove(QRegularExpression(QStringLiteral(R"(^[A-Za-z][A-Za-z0-9.+-]*://)")));
    const QString suggested = Utils::fileNameSegment(base, tr("session"))
        + QStringLiteral(".ouas");
    const QString path = QFileDialog::getSaveFileName(
        _context.window, tr("Save Session"), suggested,
        tr("Session Files (*.ouas);;All Files (*)"));
    if (path.isEmpty())
        return false;
    return saveSessionToFile(path);
}

///
/// \brief Loads a session file and starts the connection flow needed before workspace restore.
///
void SessionCoordinator::openSessionFromFile(const QString &path)
{
    SessionData data;
    QString error;
    if (!SessionStore::load(path, data, &error)) {
        MessageBoxDialog::warning(_context.window, tr("Open Session"),
                                  tr("Could not open the session:\n%1").arg(error),
                                  DialogButtonBox::Ok);
        return;
    }

    _pendingSession = data;
    _pendingSessionPath = path;
    _hasPendingSession = true;
    recordRecentSession(path);

    if (data.profile.authentication == ConnectionProfile::Authentication::Anonymous) {
        beginSessionRestore();
        _context.connectionController->connectSavedProfileWithCredentials(data.profile,
                                                                          QString(),
                                                                          QString());
    } else if (_context.connectionCoordinator->openConnectionDialog(&data.profile)) {
        if (_hasPendingSession) {
            beginSessionRestore();
            handleConnectionState(_context.backend->state());
        }
    } else {
        _pendingSession = {};
        _pendingSessionPath.clear();
        _hasPendingSession = false;
    }
}

///
/// \brief Rebuilds the Recent Sessions menu from stored paths that still exist on disk.
///
void SessionCoordinator::rebuildRecentSessionsMenu()
{
    _context.recentSessionsMenu->clear();

    QStringList existing;
    const QStringList recent = RecentSessionStore().sessions();
    for (const QString &path : recent) {
        if (QFileInfo::exists(path))
            existing.append(path);
    }

    if (existing.isEmpty()) {
        _context.recentSessionsMenu->addAction(tr("No Recent Sessions"))->setEnabled(false);
        return;
    }

    for (const QString &path : existing) {
        QAction *action = _context.recentSessionsMenu->addAction(QDir::toNativeSeparators(path));
        action->setData(path);
        action->setToolTip(QDir::toNativeSeparators(path));
        connect(action, &QAction::triggered, this, &SessionCoordinator::openRecentSession);
    }
}

///
/// \brief Applies a pending workspace only after the active endpoint matches the saved profile.
///
void SessionCoordinator::applyPendingSession()
{
    if (!_hasPendingSession)
        return;

    if (!_context.connectionController->activeProfile().isSameEndpoint(_pendingSession.profile))
        return;

    _hasPendingSession = false;
    const SessionData session = _pendingSession;
    const QString sessionPath = _pendingSessionPath;
    _pendingSession = {};
    _pendingSessionPath.clear();

    _context.dataAccessCoordinator->restoreSubscriptions(session.subscriptions);

    QVector<QPair<QString, QString>> nodes;
    nodes.reserve(session.dataAccessNodes.size());
    for (const SessionNode &node : session.dataAccessNodes)
        nodes.append({node.nodeId, node.subscriptionName});
    _context.dataAccessCoordinator->restoreMonitoredNodes(nodes);

    if (!session.trendTabs.isEmpty())
        _context.dataAccessCoordinator->restoreTrendTabs(session.trendTabs);
    else
        _context.dataAccessCoordinator->restoreTrendNodes(session.trendNodes);

    _context.featureManager->restoreSession(session);

    for (const SessionNodeMonitor &monitorState : session.nodeMonitors)
        _context.restoreNodeMonitor(monitorState);

    _savedSessionFingerprint = sessionFingerprint(session);
    setCurrentSessionPath(sessionPath);
    endSessionRestore();
}

///
/// \brief Drops the current session identity after disconnect so close does not prompt to save.
///
void SessionCoordinator::closeCurrentSession()
{
    if (_sessionPath.isEmpty() && _savedSessionFingerprint.isEmpty())
        return;
    _savedSessionFingerprint.clear();
    setCurrentSessionPath(QString());
}

///
/// \brief Maintains the window modified marker from the connected saveable workspace.
///
void SessionCoordinator::updateModifiedState()
{
    const bool connected = _context.backend->state() == OpcUaConnectionState::Connected;
    const bool modified = connected
        && (_sessionPath.isEmpty()
            || sessionFingerprint(sessionWorkspace()) != _savedSessionFingerprint);
    if (modified != _context.window->isWindowModified()) {
        _context.window->setWindowModified(modified);
#ifdef Q_OS_MAC
        updateWindowTitle();
#endif
    }
}

///
/// \brief Offers Save/Discard/Cancel for connected sessions that are unsaved or changed.
///
bool SessionCoordinator::maybeSaveSession()
{
    if (_context.backend->state() != OpcUaConnectionState::Connected)
        return true;

    QString message;
    if (_sessionPath.isEmpty()) {
        message = tr("The current session has not been saved.\nDo you want to save it?");
    } else {
        if (sessionFingerprint(sessionWorkspace()) == _savedSessionFingerprint)
            return true;
        message = tr("The session \"%1\" has unsaved changes.\nDo you want to save them?")
                      .arg(sessionDisplayName());
    }

    const DialogButtonBox::StandardButton choice = MessageBoxDialog::warning(
        _context.window, tr("Save Session"), message,
        DialogButtonBox::Save | DialogButtonBox::Discard | DialogButtonBox::Cancel,
        DialogButtonBox::Save);

    switch (choice) {
    case DialogButtonBox::Save:
        return saveCurrentSession();
    case DialogButtonBox::Discard:
        return true;
    default:
        return false;
    }
}

///
/// \brief Serializes the active connection and workspace, then updates dirty tracking.
///
bool SessionCoordinator::saveSessionToFile(const QString &path)
{
    const SessionData data = collectSessionData();

    QString error;
    if (!SessionStore::save(path, data, &error)) {
        MessageBoxDialog::warning(_context.window, tr("Save Session"),
                                  tr("Could not save the session:\n%1").arg(error),
                                  DialogButtonBox::Ok);
        return false;
    }

    _savedSessionFingerprint = sessionFingerprint(data);
    setCurrentSessionPath(path);
    recordRecentSession(path);
    return true;
}

///
/// \brief Gathers only content that participates in dirty tracking and save prompts.
///
SessionData SessionCoordinator::sessionWorkspace() const
{
    SessionData data;
    data.subscriptions = _context.dataAccessCoordinator->sessionSubscriptions();
    const QVector<QPair<QString, QString>> nodes = _context.dataAccessCoordinator->monitoredNodes();
    for (const QPair<QString, QString> &node : nodes)
        data.dataAccessNodes.append({node.first, node.second});
    data.trendTabs = _context.dataAccessCoordinator->trendTabs();
    data.nodeMonitors = _context.captureNodeMonitors();
    return data;
}

///
/// \brief Extends the saveable workspace with connection and feature navigation state.
///
SessionData SessionCoordinator::collectSessionData() const
{
    SessionData data = sessionWorkspace();
    data.profile = _context.connectionController->activeProfile();
    _context.featureManager->saveSession(data);
    return data;
}

///
/// \brief Stores a session path as most-recent and immediately reflects it in the menu.
///
void SessionCoordinator::recordRecentSession(const QString &path)
{
    RecentSessionStore().record(path);
    rebuildRecentSessionsMenu();
}

///
/// \brief Records the file backing the open session and refreshes title/dirty state.
///
void SessionCoordinator::setCurrentSessionPath(const QString &path)
{
    _sessionPath = path;
    updateWindowTitle();
    updateModifiedState();
}

///
/// \brief Returns the title fragment for the current session file or the untitled placeholder.
///
QString SessionCoordinator::sessionDisplayName() const
{
    if (_sessionPath.isEmpty())
        return tr("Untitled");
    return QFileInfo(_sessionPath).completeBaseName();
}

///
/// \brief Re-applies the translated window title after a language change.
///
void SessionCoordinator::retranslate()
{
    updateWindowTitle();
}

///
/// \brief Updates the product/session title while preserving Qt's modified marker placeholder.
///
/// macOS suppresses the [*] placeholder in favour of the close-button dot, so there the
/// asterisk is spelled out whenever the window is marked modified.
///
void SessionCoordinator::updateWindowTitle()
{
#ifdef Q_OS_MAC
    const QString marker = _context.window->isWindowModified()
        ? QStringLiteral("*") : QString();
#else
    const QString marker = QStringLiteral("[*]");
#endif
    _context.window->setWindowTitle(QStringLiteral("%1 — %2%3")
                                        .arg(QString::fromUtf8(APP_PRODUCT_NAME),
                                             sessionDisplayName(), marker));
}

///
/// \brief Shows the application wait cursor while a loaded session is being restored.
///
void SessionCoordinator::beginSessionRestore()
{
    if (_sessionRestoreCursorActive)
        return;
    QGuiApplication::setOverrideCursor(Qt::WaitCursor);
    _sessionRestoreCursorActive = true;
}

///
/// \brief Removes the wait cursor installed for session restoration.
///
void SessionCoordinator::endSessionRestore()
{
    if (!_sessionRestoreCursorActive)
        return;
    QGuiApplication::restoreOverrideCursor();
    _sessionRestoreCursorActive = false;
}

///
/// \brief Stops waiting after restoration or when its connection attempt becomes idle.
///
void SessionCoordinator::handleConnectionState(OpcUaConnectionState state)
{
    if (state == OpcUaConnectionState::Connected
        || state == OpcUaConnectionState::Disconnected
        || state == OpcUaConnectionState::Unavailable)
        endSessionRestore();
}

///
/// \brief Opens the session path stored on the triggering recent-session action.
///
void SessionCoordinator::openRecentSession()
{
    auto *action = qobject_cast<QAction *>(sender());
    if (!action)
        return;
    const QString path = action->data().toString();
    if (!path.isEmpty())
        openSessionFromFile(path);
}
