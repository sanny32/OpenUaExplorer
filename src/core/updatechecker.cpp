// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file updatechecker.cpp
/// \brief Implements the GitHub release update checker.
///

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslSocket>
#include <QUrl>
#include <QVersionNumber>

#include "loggingcategories.h"
#include "updatechecker.h"

namespace {

///
/// \brief GitHub releases API endpoint for the latest published release.
///
constexpr auto ReleasesApiUrl =
    "https://api.github.com/repos/sanny32/OpenUaExplorer/releases/latest";

///
/// \brief Parsed version with pre-release suffix support.
///
/// Suffix rank orders pre-releases below stable releases:
/// 4 = stable, 3 = rc, 2 = beta, 1 = alpha, 0 = dev/unknown.
///
struct ParsedVersion
{
    QVersionNumber numeric;
    int suffixRank = 4;
    int suffixNum = 0;

    bool isNull() const { return numeric.isNull(); }

    bool operator>(const ParsedVersion &other) const
    {
        if (numeric != other.numeric)
            return numeric > other.numeric;
        if (suffixRank != other.suffixRank)
            return suffixRank > other.suffixRank;
        return suffixNum > other.suffixNum;
    }
};

///
/// \brief Parses a version string into a comparable value.
/// \param str Version string, optionally with a pre-release suffix.
/// \return Parsed version.
///
ParsedVersion parseVersion(const QString &str)
{
    ParsedVersion v;
    const int dash = str.indexOf('-');
    const QString numPart = dash >= 0 ? str.left(dash) : str;
    const QString suffix = dash >= 0 ? str.mid(dash + 1).toLower() : QString();

    v.numeric = QVersionNumber::fromString(numPart);
    if (suffix.isEmpty()) {
        v.suffixRank = 4;
    } else if (suffix.startsWith(QLatin1String("rc"))) {
        v.suffixRank = 3;
        v.suffixNum = suffix.mid(2).toInt();
    } else if (suffix.startsWith(QLatin1String("beta"))) {
        v.suffixRank = 2;
        v.suffixNum = suffix.mid(4).toInt();
    } else if (suffix.startsWith(QLatin1String("alpha"))) {
        v.suffixRank = 1;
        v.suffixNum = suffix.mid(5).toInt();
    } else {
        v.suffixRank = 0;
    }
    return v;
}

} // namespace

///
/// \brief Builds the checker and its network manager.
/// \param parent Parent object.
///
UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent)
    , _networkManager(new QNetworkAccessManager(this))
{
    connect(_networkManager, &QNetworkAccessManager::finished, this, &UpdateChecker::onReplyFinished);
}

///
/// \brief Compares two version strings, honouring pre-release suffixes.
/// \param candidate Version to test.
/// \param current Version to compare against.
/// \return True when candidate is strictly newer than current.
///
bool UpdateChecker::isVersionNewer(const QString &candidate, const QString &current)
{
    const ParsedVersion candidateVersion = parseVersion(candidate);
    if (candidateVersion.isNull())
        return false;

    return candidateVersion > parseVersion(current);
}

///
/// \brief Starts an asynchronous check against the GitHub releases API.
///
void UpdateChecker::checkForUpdates()
{
    if (_isChecking)
        return;

    if (!QSslSocket::supportsSsl()) {
        emit checkFailed(tr("Secure connections are not supported in this build."));
        return;
    }

    _isChecking = true;
    emit checkStarted();

    QNetworkRequest request{QUrl(QString::fromLatin1(ReleasesApiUrl))};
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("OpenUaExplorer"));
    request.setRawHeader("Accept", "application/vnd.github.v3+json");
    _networkManager->get(request);
}

///
/// \brief Handles the completed release-query reply.
/// \param reply Network reply for the release query.
///
void UpdateChecker::onReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();
    _isChecking = false;

    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(lcApp) << "Update check failed:" << reply->errorString();
        emit checkFailed(reply->errorString());
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isObject()) {
        emit checkFailed(tr("Failed to parse update information."));
        return;
    }

    const QJsonObject obj = doc.object();
    const QString tagName = obj.value(QStringLiteral("tag_name")).toString();
    const QString htmlUrl = obj.value(QStringLiteral("html_url")).toString();

    if (tagName.isEmpty() || htmlUrl.isEmpty()) {
        emit checkFailed(tr("Update information is incomplete."));
        return;
    }

    QString versionStr = tagName;
    if (versionStr.startsWith(QLatin1Char('v'), Qt::CaseInsensitive))
        versionStr = versionStr.mid(1);

    if (isVersionNewer(versionStr, QStringLiteral(APP_VERSION))) {
        _hasNewVersion = true;
        _latestVersion = versionStr;
        _releaseUrl = htmlUrl;
        emit newVersionAvailable(versionStr, htmlUrl);
    } else {
        _hasNewVersion = false;
        _latestVersion.clear();
        _releaseUrl.clear();
        emit noUpdatesAvailable();
    }
}
