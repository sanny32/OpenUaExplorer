// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_mainstatusbarwidget.cpp
/// \brief Tests the main status bar widget.
///

#include <QLabel>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

#include "mainstatusbarwidget.h"
#include "widgets/elidedlabel.h"
#include "opcua/connectioncontroller.h"
#include "opcua/connectionprofilestore.h"
#include "opcua/opcuabackend.h"
#include "opcua/recentconnectionstore.h"
#include "opcua/secretstore.h"

///
/// \brief Backend stub whose connection state the test drives directly.
///
class StatusBarFakeBackend : public OpcUaBackend
{
    Q_OBJECT

public:
    using OpcUaBackend::OpcUaBackend;

    bool isAvailable() const override { return true; }
    QStringList availableBackends() const override { return {QStringLiteral("fake")}; }
    OpcUaConnectionState state() const override { return currentState; }
    QString lastError() const override { return {}; }
    void setCertificateTrustDecider(CertificateTrustDecider *) override {}
    void discoverEndpoints(const QString &, const QString &, int) override
    {
        setState(OpcUaConnectionState::Discovering);
    }
    void connectToEndpoint(const ConnectionProfile &, const QString &, const QString &) override {}
    void disconnectFromEndpoint() override {}
    void browse(const QString &) override {}
    void browseReferences(const QString &) override {}
    void readNode(const QString &) override {}
    void readValues(const QStringList &) override {}
    void writeValue(const QString &, const QVariant &, int) override {}

    void setState(OpcUaConnectionState state)
    {
        currentState = state;
        emit stateChanged(state);
    }

    OpcUaConnectionState currentState = OpcUaConnectionState::Disconnected;
};

///
/// \brief UI tests for the main status bar.
///
class TestMainStatusBarWidget : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void clockLabelsReserveStableWidths();
    void clockWidthsFollowFontChanges();
    void lostConnectionKeepsTheSessionParameters();
    void fieldsElideOnlyWhenTheBarRunsOut();

private:
    QTemporaryDir _settingsDirectory;
};

///
/// \brief Routes QSettings to a throwaway directory so tests never touch real configuration.
///
void TestMainStatusBarWidget::initTestCase()
{
    QVERIFY(_settingsDirectory.isValid());
    QCoreApplication::setOrganizationName(QStringLiteral("OpenUaExplorerStatusBarTests"));
    QCoreApplication::setApplicationName(QStringLiteral("OpenUaExplorerStatusBarTests"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, _settingsDirectory.path());
}

///
/// \brief Clock labels reserve enough width for wide digit combinations.
///
void TestMainStatusBarWidget::clockLabelsReserveStableWidths()
{
    MainStatusBarWidget widget;
    auto *serverLabel = widget.findChild<QLabel *>(QStringLiteral("serverTimeLabel"));
    auto *localLabel = widget.findChild<QLabel *>(QStringLiteral("localTimeLabel"));
    QVERIFY(serverLabel);
    QVERIFY(localLabel);

    const int serverMinimumWidth = serverLabel->minimumWidth();
    const int localMinimumWidth = localLabel->minimumWidth();
    QVERIFY(serverMinimumWidth >= serverLabel->fontMetrics().horizontalAdvance(serverLabel->text()));
    QVERIFY(localMinimumWidth >= localLabel->fontMetrics().horizontalAdvance(localLabel->text()));

    serverLabel->setText(QStringLiteral("Server Time: 11:11:11 UTC"));
    localLabel->setText(QStringLiteral("Local Time: 11:11:11 UTC+3"));
    QCOMPARE(serverLabel->minimumWidth(), serverMinimumWidth);
    QCOMPARE(localLabel->minimumWidth(), localMinimumWidth);
}

///
/// \brief Clock width reservations are refreshed after a font change.
///
void TestMainStatusBarWidget::clockWidthsFollowFontChanges()
{
    MainStatusBarWidget widget;
    auto *serverLabel = widget.findChild<QLabel *>(QStringLiteral("serverTimeLabel"));
    auto *localLabel = widget.findChild<QLabel *>(QStringLiteral("localTimeLabel"));
    QVERIFY(serverLabel);
    QVERIFY(localLabel);

    const int serverMinimumWidth = serverLabel->minimumWidth();
    const int localMinimumWidth = localLabel->minimumWidth();
    QFont largerFont = widget.font();
    largerFont.setPointSize(largerFont.pointSize() + 8);
    widget.setFont(largerFont);

    QVERIFY(serverLabel->minimumWidth() > serverMinimumWidth);
    QVERIFY(localLabel->minimumWidth() > localMinimumWidth);
}

///
/// \brief A lost connection keeps showing what was connected; a closed one does not (issue #7).
///
void TestMainStatusBarWidget::lostConnectionKeepsTheSessionParameters()
{
    StatusBarFakeBackend backend;
    SecretStore secrets;
    ConnectionProfileStore profiles;
    RecentConnectionStore recents;
    ConnectionController controller(&backend, &secrets, &profiles, &recents);

    MainStatusBarWidget widget;
    widget.setConnectionController(&controller);
    auto *connectionLabel = widget.findChild<QLabel *>(QStringLiteral("connectionLabel"));
    auto *securityLabel = widget.findChild<QLabel *>(QStringLiteral("securityLabel"));
    auto *sessionLabel = widget.findChild<QLabel *>(QStringLiteral("sessionLabel"));
    auto *authenticationLabel = widget.findChild<QLabel *>(QStringLiteral("authenticationLabel"));
    QVERIFY(connectionLabel);
    QVERIFY(securityLabel);
    QVERIFY(sessionLabel);
    QVERIFY(authenticationLabel);

    ConnectionProfile profile;
    profile.id = QStringLiteral("probe");
    profile.endpointUrl = QStringLiteral("opc.tcp://probe.invalid:4840");
    profile.securityPolicy = QStringLiteral("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");
    profile.securityMode = 3;
    profile.sessionName = QStringLiteral("Probe Session");
    profile.authentication = ConnectionProfile::Authentication::Anonymous;
    controller.connectNewProfile(profile, QString(), QString());
    backend.setState(OpcUaConnectionState::Connected);

    const QString connectedSecurity = securityLabel->text();
    const QString connectedSession = sessionLabel->text();
    const QString connectedAuthentication = authenticationLabel->text();
    QCOMPARE(connectionLabel->text(), profile.endpointUrl);
    QVERIFY(connectedSecurity != QStringLiteral("-"));
    QVERIFY(connectedSession != QStringLiteral("-"));

    // A disconnect the user asked for leaves nothing behind.
    backend.setState(OpcUaConnectionState::Disconnected);
    QCOMPARE(securityLabel->text(), QStringLiteral("-"));
    QCOMPARE(sessionLabel->text(), QStringLiteral("-"));

    // A lost connection keeps the parameters of the session that dropped.
    widget.setConnectionLost(true);
    QCOMPARE(connectionLabel->text(), profile.endpointUrl);
    QCOMPARE(securityLabel->text(), connectedSecurity);
    QCOMPARE(sessionLabel->text(), connectedSession);
    QCOMPARE(authenticationLabel->text(), connectedAuthentication);

    // Retry attempts pass through discovery, which must not drop what is shown.
    backend.setState(OpcUaConnectionState::Discovering);
    QCOMPARE(sessionLabel->text(), connectedSession);
    backend.setState(OpcUaConnectionState::Disconnected);
    QCOMPARE(sessionLabel->text(), connectedSession);

    // Once the window reports the session gone, the fields are cleared.
    widget.setConnectionLost(false);
    QCOMPARE(sessionLabel->text(), QStringLiteral("-"));
    QVERIFY(connectionLabel->text() != profile.endpointUrl);
}

///
/// \brief Fields keep their full value and give it up only when the bar is too narrow.
///
void TestMainStatusBarWidget::fieldsElideOnlyWhenTheBarRunsOut()
{
    StatusBarFakeBackend backend;
    SecretStore secrets;
    ConnectionProfileStore profiles;
    RecentConnectionStore recents;
    ConnectionController controller(&backend, &secrets, &profiles, &recents);

    MainStatusBarWidget widget;
    widget.setConnectionController(&controller);
    auto *connectionLabel = widget.findChild<ElidedLabel *>(QStringLiteral("connectionLabel"));
    QVERIFY(connectionLabel);

    ConnectionProfile profile;
    profile.id = QStringLiteral("wide");
    profile.endpointUrl =
        QStringLiteral("opc.tcp://a-long-host-name.invalid:53530/OPCUA/SimulationServer");
    profile.securityPolicy =
        QStringLiteral("http://opcfoundation.org/UA/SecurityPolicy#Aes256_Sha256_RsaPss");
    profile.securityMode = 2;
    profile.sessionName = QStringLiteral("OpenUaExplorer@a-long-host-name");
    controller.connectNewProfile(profile, QString(), QString());
    backend.setState(OpcUaConnectionState::Connected);

    const QString category = connectionLabel->property("fieldTooltip").toString();
    QVERIFY(!category.isEmpty());

    widget.resize(4000, widget.sizeHint().height());
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    widget.grab();
    QCOMPARE(connectionLabel->size().width(), connectionLabel->sizeHint().width());
    QCOMPARE(connectionLabel->text(), profile.endpointUrl);
    QVERIFY(!connectionLabel->isElided());
    QCOMPARE(connectionLabel->toolTip(), category);

    widget.resize(360, widget.height());
    QTest::qWait(100);
    widget.grab();
    QVERIFY(connectionLabel->isElided());
    QCOMPARE(connectionLabel->text(), profile.endpointUrl);
    QVERIFY(connectionLabel->toolTip().contains(profile.endpointUrl));
}

QTEST_MAIN(TestMainStatusBarWidget)

#include "test_mainstatusbarwidget.moc"
