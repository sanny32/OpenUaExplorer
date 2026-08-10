// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_connectiondialog_history_auth.cpp
/// \brief Tests ConnectionDialog history and authentication behavior.
///

#include <QTest>

#include "test_connectiondialog_support.h"

///
/// \brief Tests the related behavior in an isolated Qt Test executable.
///
class TestConnectionDialogHistoryAuth : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();
    void designerEndpointsSeedEmptyHistory();
    void clearedEndpointHistoryStaysEmpty();
    void usernamePasswordDefaultsAreEmpty();
    void rememberOptionFollowsUsernameAuthentication();
    void anonymousAuthenticationHidesTheCredentialFields();
    void reopeningRestoresTheAuthenticationOfTheLastConnection();
    void reopeningFillsInTheRememberedPassword();

private:
    QTemporaryDir _settingsDirectory;
};

void TestConnectionDialogHistoryAuth::initTestCase()
{
    QVERIFY(_settingsDirectory.isValid());
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("OpenUaExplorerTests"));
    QCoreApplication::setApplicationName(QStringLiteral("TestConnectionDialogHistoryAuth"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       _settingsDirectory.path());
}

void TestConnectionDialogHistoryAuth::cleanup()
{
    while (QGuiApplication::overrideCursor())
        QGuiApplication::restoreOverrideCursor();
    SettingsStore settings;
    settings.clear();
}

///
/// \brief Verifies that every endpoint declared in the UI seeds an empty history.
///
void TestConnectionDialogHistoryAuth::designerEndpointsSeedEmptyHistory()
{
    ConnectionDialog dialog;
    auto *endpoints = dialog.findChild<HistoryComboBox *>(
        QStringLiteral("discoveryUrlComboBox"));
    QVERIFY(endpoints);

    QCOMPARE(endpoints->count(), 4);
    QCOMPARE(endpoints->itemText(0),
             QStringLiteral("opc.tcp://uademo.prosysopc.com:53530/OPCUA/SimulationServer"));
    QCOMPARE(endpoints->itemText(1),
             QStringLiteral("opc.tcp://opcua.demo-this.com:51210/UA/SampleServer"));
    QCOMPARE(endpoints->itemData(2, Qt::AccessibleDescriptionRole).toString(),
             QStringLiteral("separator"));
    QVERIFY(endpoints->isActionEntry(3));
}

///
/// \brief Verifies that removing every initial endpoint leaves later dialogs empty.
///
void TestConnectionDialogHistoryAuth::clearedEndpointHistoryStaysEmpty()
{
    ConnectionDialog initialDialog;
    auto *initialEndpoints = initialDialog.findChild<HistoryComboBox *>(
        QStringLiteral("discoveryUrlComboBox"));
    QVERIFY(initialEndpoints);

    EndpointHistoryStore store;
    for (int index = 0; index < initialEndpoints->count(); ++index) {
        if (!initialEndpoints->isActionEntry(index))
            store.remove(initialEndpoints->itemText(index));
    }

    ConnectionDialog reopenedDialog;
    auto *reopenedEndpoints = reopenedDialog.findChild<HistoryComboBox *>(
        QStringLiteral("discoveryUrlComboBox"));
    QVERIFY(reopenedEndpoints);
    QCOMPARE(reopenedEndpoints->count(), 1);
    QVERIFY(reopenedEndpoints->isActionEntry(0));
    QVERIFY(reopenedEndpoints->currentText().isEmpty());
}

void TestConnectionDialogHistoryAuth::usernamePasswordDefaultsAreEmpty()
{
    ConnectionDialog dialog;
    auto *username = dialog.findChild<QLineEdit *>(QStringLiteral("usernameEdit"));
    auto *password = dialog.findChild<QLineEdit *>(QStringLiteral("passwordEdit"));
    QVERIFY(username);
    QVERIFY(password);

    QVERIFY(username->text().isEmpty());
    QVERIFY(password->text().isEmpty());
}

///
/// \brief Remembering a password is offered for username authentication and nothing else.
///
void TestConnectionDialogHistoryAuth::rememberOptionFollowsUsernameAuthentication()
{
    ConnectionDialog dialog;
    auto *remember = dialog.findChild<QCheckBox *>(QStringLiteral("rememberCheckBox"));
    auto *authentication = dialog.findChild<QComboBox *>(
        QStringLiteral("authenticationComboBox"));
    QVERIFY(remember);
    QVERIFY(authentication);

    // The designer items are ordered like the authentication enum.
    authentication->setCurrentIndex(
        static_cast<int>(ConnectionProfile::Authentication::Anonymous));
    QVERIFY(!remember->isEnabled());
    QVERIFY(!dialog.rememberCredentials());

    authentication->setCurrentIndex(
        static_cast<int>(ConnectionProfile::Authentication::Username));
    QVERIFY(remember->isEnabled());
    QVERIFY(!dialog.rememberCredentials());

    remember->setChecked(true);
    QVERIFY(dialog.rememberCredentials());

    // A mode without a password to store must not report a leftover tick as an answer.
    authentication->setCurrentIndex(
        static_cast<int>(ConnectionProfile::Authentication::Certificate));
    QVERIFY(!dialog.rememberCredentials());
}

///
/// \brief Anonymous shows its notice instead of credential fields, without growing the stack.
///
void TestConnectionDialogHistoryAuth::anonymousAuthenticationHidesTheCredentialFields()
{
    ConnectionDialog dialog;
    auto *stack = dialog.findChild<QStackedWidget *>(QStringLiteral("authStack"));
    auto *anonymousPanel = dialog.findChild<QWidget *>(QStringLiteral("anonymousPanel"));
    auto *usernamePanel = dialog.findChild<QWidget *>(QStringLiteral("usernamePanel"));
    auto *certificatePanel = dialog.findChild<QWidget *>(QStringLiteral("certificatePanel"));
    auto *authentication = dialog.findChild<QComboBox *>(
        QStringLiteral("authenticationComboBox"));
    QVERIFY(stack);
    QVERIFY(anonymousPanel);
    QVERIFY(usernamePanel);
    QVERIFY(certificatePanel);
    QVERIFY(authentication);

    // The designer items are ordered like the authentication enum.
    authentication->setCurrentIndex(
        static_cast<int>(ConnectionProfile::Authentication::Anonymous));
    QCOMPARE(stack->currentWidget(), anonymousPanel);

    authentication->setCurrentIndex(
        static_cast<int>(ConnectionProfile::Authentication::Username));
    QCOMPARE(stack->currentWidget(), usernamePanel);

    authentication->setCurrentIndex(
        static_cast<int>(ConnectionProfile::Authentication::Certificate));
    QCOMPARE(stack->currentWidget(), certificatePanel);

    authentication->setCurrentIndex(
        static_cast<int>(ConnectionProfile::Authentication::Anonymous));

    // The notice fills the space the credential fields leave behind, and must fit in it: a
    // taller page would make the stack reserve extra height on the other pages as well.
    auto *notice = dialog.findChild<QWidget *>(QStringLiteral("anonymousNotice"));
    QVERIFY(notice);
    QVERIFY(!notice->isHidden());
    QVERIFY(anonymousPanel->sizeHint().height() <= usernamePanel->sizeHint().height());
    QVERIFY(anonymousPanel->sizeHint().height() <= certificatePanel->sizeHint().height());

    // It reads as part of the form: left edge where the labels start, right edge where the
    // combo box above it ends.
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    auto *label = dialog.findChild<QLabel *>(QStringLiteral("authenticationLabel"));
    QVERIFY(label);
    const QRect noticeRect(notice->mapTo(&dialog, QPoint(0, 0)), notice->size());
    const QRect labelRect(label->mapTo(&dialog, QPoint(0, 0)), label->size());
    const QRect comboRect(authentication->mapTo(&dialog, QPoint(0, 0)), authentication->size());
    QCOMPARE(noticeRect.left(), labelRect.left());
    QCOMPARE(noticeRect.right(), comboRect.right());

    // Hiding a shown dialog only once it is destroyed sends the focus-out through slots of a
    // half-destroyed object, so close it while it is still whole.
    dialog.hide();
}

///
/// \brief Reopening the dialog for a known server brings back how it was connected.
///
void TestConnectionDialogHistoryAuth::reopeningRestoresTheAuthenticationOfTheLastConnection()
{
    const QString endpoint = QStringLiteral("opc.tcp://restored:4840");
    EndpointHistoryStore().save(endpoint);

    ConnectionProfile profile;
    profile.id = QStringLiteral("restored-profile");
    profile.endpointUrl = endpoint;
    profile.authentication = ConnectionProfile::Authentication::Username;
    profile.username = QStringLiteral("operator");
    RecentConnectionStore().record(profile);

    ConnectionDialog dialog;
    auto *authentication = dialog.findChild<QComboBox *>(
        QStringLiteral("authenticationComboBox"));
    auto *username = dialog.findChild<QLineEdit *>(QStringLiteral("usernameEdit"));
    QVERIFY(authentication);
    QVERIFY(username);

    QCOMPARE(authentication->currentIndex(),
             static_cast<int>(ConnectionProfile::Authentication::Username));
    QCOMPARE(username->text(), QStringLiteral("operator"));
    // The connection continues the profile it had, so its stored password keeps being found.
    QCOMPARE(dialog.profile().id, profile.id);
}

///
/// \brief A password kept for the server is filled in, with the remember option ticked.
///
void TestConnectionDialogHistoryAuth::reopeningFillsInTheRememberedPassword()
{
    const QString endpoint = QStringLiteral("opc.tcp://remembered:4840");
    // Unique per run so the credential store of the machine keeps no test leftovers around.
    const QString profileId = QStringLiteral("ouaexp-dialog-test-")
        + QUuid::createUuid().toString(QUuid::WithoutBraces);

    EndpointHistoryStore().save(endpoint);
    ConnectionProfile profile;
    profile.id = profileId;
    profile.endpointUrl = endpoint;
    profile.authentication = ConnectionProfile::Authentication::Username;
    profile.username = QStringLiteral("operator");
    RecentConnectionStore().record(profile);

    // The secret is filed under the endpoint identity of the profile, not under its bare
    // identifier, so the dialog only finds it when it is stored under the same scope.
    const QString scope = profile.secretScope();
    SecretStore secrets;
    QSignalSpy writeSpy(&secrets, &SecretStore::writeFinished);
    secrets.write(scope, SecretStore::Secret::Password, QStringLiteral("kept-secret"));
    if (!writeSpy.wait(5000) || !writeSpy.takeFirst().at(2).toString().isEmpty())
        QSKIP("No usable keychain backend.");

    {
        ConnectionDialog dialog;
        auto *password = dialog.findChild<QLineEdit *>(QStringLiteral("passwordEdit"));
        auto *remember = dialog.findChild<QCheckBox *>(QStringLiteral("rememberCheckBox"));
        QVERIFY(password);
        QVERIFY(remember);

        QTRY_COMPARE(password->text(), QStringLiteral("kept-secret"));
        QVERIFY(remember->isChecked());
        QCOMPARE(dialog.password(), QStringLiteral("kept-secret"));
    }

    QSignalSpy removeSpy(&secrets, &SecretStore::writeFinished);
    secrets.remove(scope, SecretStore::Secret::Password);
    removeSpy.wait(5000);
}

QTEST_MAIN(TestConnectionDialogHistoryAuth)

#include "test_connectiondialog_history_auth.moc"
