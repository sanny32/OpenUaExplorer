// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file updatechecker.cpp
/// \brief Implements the GitHub release update checker.
///

#include <QJsonArray>
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
/// \brief GitHub releases API endpoint listing published releases, newest first.
///
/// The /releases/latest endpoint is deliberately not used: it hides releases
/// flagged as pre-release, so beta tags would never be reported.
///
constexpr auto ReleasesApiUrl =
    "https://api.github.com/repos/sanny32/OpenUaExplorer/releases?per_page=20";

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
/// Both the spelled-out suffixes (beta1, alpha2) and their single-letter
/// shorthands (b1, a2) are recognised, since release tags use the short form.
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
    } else if (suffix.startsWith(QLatin1Char('b'))) {
        v.suffixRank = 2;
        v.suffixNum = suffix.mid(1).toInt();
    } else if (suffix.startsWith(QLatin1Char('a'))) {
        v.suffixRank = 1;
        v.suffixNum = suffix.mid(1).toInt();
    } else {
        v.suffixRank = 0;
    }
    return v;
}

///
/// \brief A release entry reduced to what the checker needs.
///
struct ReleaseInfo
{
    QString version;
    QString url;
    ParsedVersion parsed;
};

///
/// \brief Picks the highest-versioned release from a releases array.
/// \param releases Array returned by the GitHub releases endpoint.
/// \return Newest usable release, or an entry with an empty version when none qualifies.
///
/// Drafts are skipped, as are entries without a parseable tag or a page URL.
/// Pre-releases are kept: they are ranked by parseVersion() like any other tag.
///
ReleaseInfo selectNewestRelease(const QJsonArray &releases)
{
    ReleaseInfo newest;
    for (const QJsonValue &value : releases) {
        if (!value.isObject())
            continue;

        const QJsonObject obj = value.toObject();
        if (obj.value(QStringLiteral("draft")).toBool())
            continue;

        const QString url = obj.value(QStringLiteral("html_url")).toString();
        QString version = obj.value(QStringLiteral("tag_name")).toString();
        if (version.startsWith(QLatin1Char('v'), Qt::CaseInsensitive))
            version = version.mid(1);

        if (version.isEmpty() || url.isEmpty())
            continue;

        const ParsedVersion parsed = parseVersion(version);
        if (parsed.isNull())
            continue;

        if (newest.version.isEmpty() || parsed > newest.parsed)
            newest = ReleaseInfo{version, url, parsed};
    }
    return newest;
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
}

///
/// \brief Builds the checker with a supplied network manager.
/// \param networkManager Network manager used for release requests.
/// \param parent Parent object.
///
UpdateChecker::UpdateChecker(QNetworkAccessManager *networkManager, QObject *parent)
    : QObject(parent)
    , _networkManager(networkManager)
{
    if (_networkManager && !_networkManager->parent())
        _networkManager->setParent(this);
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

    QNetworkReply *reply = _networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] { onReplyFinished(reply); });
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
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status == 404) {
            _hasNewVersion = false;
            _latestVersion.clear();
            _releaseUrl.clear();
            emit noUpdatesAvailable();
            return;
        }

        qCWarning(lcApp) << "Update check failed:" << reply->errorString();
        emit checkFailed(reply->errorString());
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isArray()) {
        emit checkFailed(tr("Failed to parse update information."));
        return;
    }

    const QJsonArray releases = doc.array();
    const ReleaseInfo newest = selectNewestRelease(releases);

    if (newest.version.isEmpty() && !releases.isEmpty()) {
        emit checkFailed(tr("Update information is incomplete."));
        return;
    }

    if (!newest.version.isEmpty()
        && isVersionNewer(newest.version, QStringLiteral(APP_VERSION))) {
        _hasNewVersion = true;
        _latestVersion = newest.version;
        _releaseUrl = newest.url;
        emit newVersionAvailable(newest.version, newest.url);
    } else {
        _hasNewVersion = false;
        _latestVersion.clear();
        _releaseUrl.clear();
        emit noUpdatesAvailable();
    }
}
