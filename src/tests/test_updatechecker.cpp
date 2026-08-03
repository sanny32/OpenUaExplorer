// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_updatechecker.cpp
/// \brief Unit tests for update version comparison and release replies.
///

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>

#include "updatechecker.h"

namespace {

struct FakeReplyData
{
    QByteArray body;
    QNetworkReply::NetworkError error = QNetworkReply::NoError;
    int status = 200;
    QString errorString;
};

class FakeNetworkReply final : public QNetworkReply
{
public:
    FakeNetworkReply(const QNetworkRequest &request, const FakeReplyData &data, QObject *parent = nullptr)
        : QNetworkReply(parent)
        , _body(data.body)
        , _errorString(data.errorString)
    {
        setRequest(request);
        setUrl(request.url());
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, data.status);
        if (data.error != QNetworkReply::NoError)
            setError(data.error, _errorString);
        open(QIODevice::ReadOnly | QIODevice::Unbuffered);
        QTimer::singleShot(0, this, [this]() {
            if (!_body.isEmpty())
                emit readyRead();
            emit finished();
        });
    }

    void abort() override {}

    qint64 bytesAvailable() const override
    {
        return _body.size() - _offset + QNetworkReply::bytesAvailable();
    }

protected:
    qint64 readData(char *data, qint64 maxSize) override
    {
        const qint64 remaining = _body.size() - _offset;
        const qint64 size = qMin(maxSize, remaining);
        if (size <= 0)
            return -1;
        memcpy(data, _body.constData() + _offset, static_cast<size_t>(size));
        _offset += size;
        return size;
    }

private:
    QByteArray _body;
    QString _errorString;
    qint64 _offset = 0;
};

class FakeNetworkAccessManager final : public QNetworkAccessManager
{
public:
    QVector<QNetworkRequest> requests;
    QVector<FakeReplyData> replies;

protected:
    QNetworkReply *createRequest(Operation operation,
                                 const QNetworkRequest &request,
                                 QIODevice *outgoingData = nullptr) override
    {
        Q_UNUSED(operation)
        Q_UNUSED(outgoingData)

        requests.append(request);
        const FakeReplyData replyData = replies.isEmpty() ? FakeReplyData{} : replies.takeFirst();
        auto *reply = new FakeNetworkReply(request, replyData, this);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            emit finished(reply);
        });
        return reply;
    }
};

FakeReplyData jsonReply(const QByteArray &body)
{
    FakeReplyData data;
    data.body = body;
    return data;
}

QByteArray releaseList(const QByteArray &entries)
{
    return "[" + entries + "]";
}

QByteArray release(const char *tag, const char *url, bool draft = false, bool prerelease = false)
{
    return QByteArray("{\"tag_name\":\"") + tag + "\",\"html_url\":\"" + url
           + "\",\"draft\":" + (draft ? "true" : "false")
           + ",\"prerelease\":" + (prerelease ? "true" : "false") + "}";
}

} // namespace

///
/// \brief Tests the semantic version comparison used to detect updates.
///
class TestUpdateChecker : public QObject
{
    Q_OBJECT

private slots:
    void comparesNumericVersions();
    void comparesPreReleaseSuffixes();
    void comparesShortPreReleaseSuffixes();
    void ranksDevBuildsBelowPreReleases();
    void rejectsInvalidCandidates();
    void sendsGithubReleaseRequest();
    void ignoresConcurrentChecks();
    void reportsNewerRelease();
    void reportsNewestOfSeveralReleases();
    void skipsDraftReleases();
    void reportsEmptyReleaseListAsNoUpdate();
    void reportsNotFoundAsNoUpdate();
    void reportsNetworkError();
    void rejectsMalformedResponses_data();
    void rejectsMalformedResponses();
};

///
/// \brief Verifies ordering of plain numeric versions.
///
void TestUpdateChecker::comparesNumericVersions()
{
    QVERIFY(UpdateChecker::isVersionNewer(QStringLiteral("1.2.1"), QStringLiteral("1.2.0")));
    QVERIFY(UpdateChecker::isVersionNewer(QStringLiteral("2.0.0"), QStringLiteral("1.9.9")));
    QVERIFY(!UpdateChecker::isVersionNewer(QStringLiteral("1.2.0"), QStringLiteral("1.2.0")));
    QVERIFY(!UpdateChecker::isVersionNewer(QStringLiteral("1.1.0"), QStringLiteral("1.2.0")));
}

///
/// \brief Verifies pre-release suffixes rank below the matching stable release.
///
void TestUpdateChecker::comparesPreReleaseSuffixes()
{
    QVERIFY(UpdateChecker::isVersionNewer(QStringLiteral("1.2.0"), QStringLiteral("1.2.0-rc1")));
    QVERIFY(UpdateChecker::isVersionNewer(QStringLiteral("1.2.0-rc2"), QStringLiteral("1.2.0-rc1")));
    QVERIFY(UpdateChecker::isVersionNewer(QStringLiteral("1.2.0-beta1"), QStringLiteral("1.2.0-alpha3")));
    QVERIFY(!UpdateChecker::isVersionNewer(QStringLiteral("1.2.0-beta1"), QStringLiteral("1.2.0")));
    QVERIFY(!UpdateChecker::isVersionNewer(QStringLiteral("1.2.0-alpha1"), QStringLiteral("1.2.0-rc1")));
}

///
/// \brief Verifies the short b/a suffixes rank like beta/alpha.
///
void TestUpdateChecker::comparesShortPreReleaseSuffixes()
{
    QVERIFY(UpdateChecker::isVersionNewer(QStringLiteral("1.0.0-b2"), QStringLiteral("1.0.0-b1")));
    QVERIFY(UpdateChecker::isVersionNewer(QStringLiteral("1.0.0-b1"), QStringLiteral("1.0.0-a9")));
    QVERIFY(UpdateChecker::isVersionNewer(QStringLiteral("1.0.0-rc1"), QStringLiteral("1.0.0-b3")));
    QVERIFY(UpdateChecker::isVersionNewer(QStringLiteral("1.0.0"), QStringLiteral("1.0.0-b2")));
    QVERIFY(!UpdateChecker::isVersionNewer(QStringLiteral("1.0.0-b2"), QStringLiteral("1.0.0")));
    QVERIFY(!UpdateChecker::isVersionNewer(QStringLiteral("1.0.0-b1"), QStringLiteral("1.0.0-beta1")));
}

///
/// \brief Verifies a dev build ranks below every published pre-release.
///
/// Dev builds carry a "-dev.g<sha>" suffix of unknown rank, so any published
/// release is offered to them. This keeps the feature testable from a dev build.
///
void TestUpdateChecker::ranksDevBuildsBelowPreReleases()
{
    QVERIFY(UpdateChecker::isVersionNewer(QStringLiteral("1.0.0-b2"),
                                          QStringLiteral("1.0-dev.gabc1234")));
    QVERIFY(!UpdateChecker::isVersionNewer(QStringLiteral("1.0-dev.gabc1234"),
                                           QStringLiteral("1.0.0-b2")));
}

///
/// \brief Verifies unparseable candidates are never treated as newer.
///
void TestUpdateChecker::rejectsInvalidCandidates()
{
    QVERIFY(!UpdateChecker::isVersionNewer(QString(), QStringLiteral("1.0.0")));
    QVERIFY(!UpdateChecker::isVersionNewer(QStringLiteral("not-a-version"), QStringLiteral("1.0.0")));
}

///
/// \brief Verifies the GitHub API request URL and headers.
///
void TestUpdateChecker::sendsGithubReleaseRequest()
{
    auto *manager = new FakeNetworkAccessManager;
    manager->replies.append(jsonReply(releaseList(release("v0.0.0", "https://example.test/release"))));

    UpdateChecker checker(manager);
    QSignalSpy done(&checker, &UpdateChecker::noUpdatesAvailable);
    checker.checkForUpdates();

    QVERIFY(done.wait(1000));
    QCOMPARE(manager->requests.size(), 1);
    const QNetworkRequest request = manager->requests.constFirst();
    QCOMPARE(request.url(), QUrl(QStringLiteral("https://api.github.com/repos/sanny32/OpenUaExplorer/releases?per_page=20")));
    QCOMPARE(request.header(QNetworkRequest::UserAgentHeader).toString(), QStringLiteral("OpenUaExplorer"));
    QCOMPARE(request.rawHeader("Accept"), QByteArray("application/vnd.github.v3+json"));
}

///
/// \brief Verifies a second check is ignored while the first reply is pending.
///
void TestUpdateChecker::ignoresConcurrentChecks()
{
    auto *manager = new FakeNetworkAccessManager;
    manager->replies.append(jsonReply(releaseList(release("v0.0.0", "https://example.test/release"))));

    UpdateChecker checker(manager);
    QSignalSpy done(&checker, &UpdateChecker::noUpdatesAvailable);
    checker.checkForUpdates();
    checker.checkForUpdates();

    QCOMPARE(manager->requests.size(), 1);
    QVERIFY(done.wait(1000));
}

///
/// \brief Verifies a newer release emits the version and URL.
///
void TestUpdateChecker::reportsNewerRelease()
{
    auto *manager = new FakeNetworkAccessManager;
    manager->replies.append(jsonReply(releaseList(release("v9999.0.0", "https://example.test/new"))));

    UpdateChecker checker(manager);
    QSignalSpy found(&checker, &UpdateChecker::newVersionAvailable);
    checker.checkForUpdates();

    QVERIFY(found.wait(1000));
    QCOMPARE(found.size(), 1);
    QCOMPARE(found.takeFirst().at(0).toString(), QStringLiteral("9999.0.0"));
    QCOMPARE(checker.latestVersion(), QStringLiteral("9999.0.0"));
    QCOMPARE(checker.releaseUrl(), QStringLiteral("https://example.test/new"));
    QVERIFY(checker.hasNewVersion());
}

///
/// \brief Verifies the highest version wins regardless of list order, including pre-releases.
///
void TestUpdateChecker::reportsNewestOfSeveralReleases()
{
    auto *manager = new FakeNetworkAccessManager;
    manager->replies.append(jsonReply(releaseList(
        release("v9999.0.0-b1", "https://example.test/b1", false, true) + ","
        + release("v9999.0.0-b2", "https://example.test/b2", false, true) + ","
        + release("v0.9.0", "https://example.test/old"))));

    UpdateChecker checker(manager);
    QSignalSpy found(&checker, &UpdateChecker::newVersionAvailable);
    checker.checkForUpdates();

    QVERIFY(found.wait(1000));
    QCOMPARE(found.takeFirst().at(0).toString(), QStringLiteral("9999.0.0-b2"));
    QCOMPARE(checker.releaseUrl(), QStringLiteral("https://example.test/b2"));
}

///
/// \brief Verifies draft releases are never offered as updates.
///
void TestUpdateChecker::skipsDraftReleases()
{
    auto *manager = new FakeNetworkAccessManager;
    manager->replies.append(jsonReply(releaseList(
        release("v9999.0.0", "https://example.test/draft", true) + ","
        + release("v0.0.1", "https://example.test/published"))));

    UpdateChecker checker(manager);
    QSignalSpy noUpdate(&checker, &UpdateChecker::noUpdatesAvailable);
    checker.checkForUpdates();

    QVERIFY(noUpdate.wait(1000));
    QVERIFY(!checker.hasNewVersion());
}

///
/// \brief Verifies a repository without releases reports no available update.
///
void TestUpdateChecker::reportsEmptyReleaseListAsNoUpdate()
{
    auto *manager = new FakeNetworkAccessManager;
    manager->replies.append(jsonReply("[]"));

    UpdateChecker checker(manager);
    QSignalSpy noUpdate(&checker, &UpdateChecker::noUpdatesAvailable);
    QSignalSpy failed(&checker, &UpdateChecker::checkFailed);
    checker.checkForUpdates();

    QVERIFY(noUpdate.wait(1000));
    QCOMPARE(failed.size(), 0);
    QVERIFY(!checker.hasNewVersion());
}

///
/// \brief Verifies a 404 release response is treated as no available update.
///
void TestUpdateChecker::reportsNotFoundAsNoUpdate()
{
    auto *manager = new FakeNetworkAccessManager;
    FakeReplyData response;
    response.error = QNetworkReply::ContentNotFoundError;
    response.status = 404;
    response.errorString = QStringLiteral("not found");
    manager->replies.append(response);

    UpdateChecker checker(manager);
    QSignalSpy noUpdate(&checker, &UpdateChecker::noUpdatesAvailable);
    QSignalSpy failed(&checker, &UpdateChecker::checkFailed);
    checker.checkForUpdates();

    QVERIFY(noUpdate.wait(1000));
    QCOMPARE(failed.size(), 0);
    QVERIFY(!checker.hasNewVersion());
}

///
/// \brief Verifies non-404 network errors are reported as failures.
///
void TestUpdateChecker::reportsNetworkError()
{
    auto *manager = new FakeNetworkAccessManager;
    FakeReplyData response;
    response.error = QNetworkReply::HostNotFoundError;
    response.status = 0;
    response.errorString = QStringLiteral("host not found");
    manager->replies.append(response);

    UpdateChecker checker(manager);
    QSignalSpy failed(&checker, &UpdateChecker::checkFailed);
    checker.checkForUpdates();

    QVERIFY(failed.wait(1000));
    QCOMPARE(failed.takeFirst().at(0).toString(), QStringLiteral("Unknown error"));
}

///
/// \brief Provides malformed successful release responses.
///
void TestUpdateChecker::rejectsMalformedResponses_data()
{
    QTest::addColumn<QByteArray>("body");

    QTest::newRow("not-json") << QByteArray("not json");
    QTest::newRow("not-an-array") << QByteArray("{\"tag_name\":\"v9999.0.0\"}");
    QTest::newRow("missing-tag") << releaseList("{\"html_url\":\"https://example.test/release\"}");
    QTest::newRow("missing-url") << releaseList("{\"tag_name\":\"v9999.0.0\"}");
    QTest::newRow("unparseable-tag") << releaseList(release("nightly", "https://example.test/release"));
}

///
/// \brief Verifies malformed release responses emit checkFailed().
///
void TestUpdateChecker::rejectsMalformedResponses()
{
    QFETCH(QByteArray, body);

    auto *manager = new FakeNetworkAccessManager;
    manager->replies.append(jsonReply(body));

    UpdateChecker checker(manager);
    QSignalSpy failed(&checker, &UpdateChecker::checkFailed);
    checker.checkForUpdates();

    QVERIFY(failed.wait(1000));
    QVERIFY(!failed.takeFirst().at(0).toString().isEmpty());
    QVERIFY(!checker.hasNewVersion());
}

QTEST_MAIN(TestUpdateChecker)

#include "test_updatechecker.moc"