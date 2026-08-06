// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_connectioncredentialsdialog.cpp
/// \brief Unit tests for the dialog that collects the credentials a profile is missing.
///

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QTest>

#include "dialogs/connectioncredentialsdialog.h"
#include "opcua/connectionprofile.h"

///
/// \brief Tests the credential panels, the remember option and the explanation line.
///
class TestConnectionCredentialsDialog : public QObject
{
    Q_OBJECT

private slots:
    void usernameProfileOffersToRememberThePassword();
    void certificateProfileHidesTheRememberOption();
    void reasonReplacesTheHintAndIsRestored();
};

namespace {

/// \brief Returns a profile using username authentication.
ConnectionProfile usernameProfile()
{
    ConnectionProfile profile;
    profile.endpointUrl = QStringLiteral("opc.tcp://localhost:4840");
    profile.authentication = ConnectionProfile::Authentication::Username;
    profile.username = QStringLiteral("operator");
    return profile;
}

/// \brief Returns a profile using certificate authentication.
ConnectionProfile certificateProfile()
{
    ConnectionProfile profile;
    profile.endpointUrl = QStringLiteral("opc.tcp://localhost:4840");
    profile.authentication = ConnectionProfile::Authentication::Certificate;
    profile.clientCertificateFile = QStringLiteral("client.der");
    profile.privateKeyFile = QStringLiteral("client.pem");
    return profile;
}

} // namespace

///
/// \brief The username panel carries the remember option, since a password can be stored.
///
void TestConnectionCredentialsDialog::usernameProfileOffersToRememberThePassword()
{
    ConnectionCredentialsDialog dialog;
    dialog.setProfile(usernameProfile());

    auto *remember = dialog.findChild<QCheckBox *>(QStringLiteral("rememberCheckBox"));
    QVERIFY(remember);
    QVERIFY(!remember->isHidden());
    QVERIFY(!dialog.rememberCredentials());

    auto *username = dialog.findChild<QLineEdit *>(QStringLiteral("usernameEdit"));
    QVERIFY(username);
    QCOMPARE(username->text(), QStringLiteral("operator"));
}

///
/// \brief The certificate panel hides the remember option: its key password is never stored.
///
void TestConnectionCredentialsDialog::certificateProfileHidesTheRememberOption()
{
    ConnectionCredentialsDialog dialog;
    dialog.setProfile(certificateProfile());

    auto *remember = dialog.findChild<QCheckBox *>(QStringLiteral("rememberCheckBox"));
    QVERIFY(remember);
    QVERIFY(remember->isHidden());
    QVERIFY(!dialog.rememberCredentials());

    // The key password is still collected; it just lives for this attempt only.
    auto *keyPassword = dialog.findChild<QLineEdit *>(QStringLiteral("privateKeyPasswordEdit"));
    QVERIFY(keyPassword);
    QCOMPARE(dialog.profile().clientCertificateFile, QStringLiteral("client.der"));
}

///
/// \brief An explanation takes the place of the hint, and an empty one brings the hint back.
///
void TestConnectionCredentialsDialog::reasonReplacesTheHintAndIsRestored()
{
    ConnectionCredentialsDialog dialog;
    dialog.setProfile(usernameProfile());

    auto *subtitle = dialog.findChild<QLabel *>(QStringLiteral("subtitleLabel"));
    QVERIFY(subtitle);
    const QString hint = subtitle->text();
    QVERIFY(!hint.isEmpty());

    auto *password = dialog.findChild<QLineEdit *>(QStringLiteral("passwordEdit"));
    QVERIFY(password);
    password->setText(QStringLiteral("stale"));

    const QString reason = QStringLiteral("The server rejected the credentials.");
    dialog.setReason(reason);

    QCOMPARE(subtitle->text(), reason);
    QVERIFY(password->text().isEmpty());

    dialog.setReason(QString());
    QCOMPARE(subtitle->text(), hint);
}

QTEST_MAIN(TestConnectionCredentialsDialog)

#include "test_connectioncredentialsdialog.moc"
