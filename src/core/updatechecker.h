// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file updatechecker.h
/// \brief Declares the GitHub release update checker.
///

#pragma once

#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

///
/// \brief Queries the GitHub releases API for a newer application version.
///
/// The checker compares the latest published release tag against APP_VERSION,
/// understanding semantic pre-release suffixes (rc/beta/alpha). Results are
/// reported through signals so callers can present them however they like.
///
class UpdateChecker : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the checker and its network manager.
    /// \param parent Parent object.
    ///
    explicit UpdateChecker(QObject *parent = nullptr);

    ///
    /// \brief Builds the checker with a supplied network manager.
    /// \param networkManager Network manager used for release requests.
    /// \param parent Parent object.
    ///
    explicit UpdateChecker(QNetworkAccessManager *networkManager, QObject *parent = nullptr);

    ///
    /// \brief Starts an asynchronous check against the GitHub releases API.
    ///
    /// Does nothing when a check is already in flight. Emits checkStarted() and
    /// then exactly one of newVersionAvailable(), noUpdatesAvailable() or
    /// checkFailed() when the request completes.
    ///
    void checkForUpdates();

    ///
    /// \brief Returns the newest version discovered by the last successful check.
    /// \return Version string without a leading 'v', or empty when none is newer.
    ///
    QString latestVersion() const { return _latestVersion; }

    ///
    /// \brief Returns the release page URL for the newest discovered version.
    /// \return Release URL, or empty when no newer version is available.
    ///
    QString releaseUrl() const { return _releaseUrl; }

    ///
    /// \brief Reports whether the last check found a newer version.
    /// \return True when a newer version is available.
    ///
    bool hasNewVersion() const { return _hasNewVersion; }

    ///
    /// \brief Compares two version strings, honouring pre-release suffixes.
    /// \param candidate Version to test.
    /// \param current Version to compare against.
    /// \return True when candidate is strictly newer than current.
    ///
    static bool isVersionNewer(const QString &candidate, const QString &current);

signals:
    ///
    /// \brief Emitted when a check begins.
    ///
    void checkStarted();

    ///
    /// \brief Emitted when the current version is up to date.
    ///
    void noUpdatesAvailable();

    ///
    /// \brief Emitted when the check could not be completed.
    /// \param errorString Human-readable failure reason.
    ///
    void checkFailed(const QString &errorString);

    ///
    /// \brief Emitted when a newer version is published.
    /// \param version Newer version string without a leading 'v'.
    /// \param url Release page URL.
    ///
    void newVersionAvailable(const QString &version, const QString &url);

private:
    void onReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *_networkManager;
    QString _latestVersion;
    QString _releaseUrl;
    bool _isChecking = false;
    bool _hasNewVersion = false;
};
