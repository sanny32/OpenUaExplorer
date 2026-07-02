// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_endpointsettingsdialog.cpp
/// \brief Unit tests for the read-only endpoint settings inspector dialog.
///

#include <QLabel>
#include <QPushButton>
#include <QFile>
#include <QTest>

#include "dialogs/endpointsettingsdialog.h"
#include "opcua/connectionprofile.h"
#include "opcua/pkimanager.h"
#include "widgets/themediconlabel.h"

///
/// \brief Tests that the inspector renders an active profile's endpoint values.
///
class TestEndpointSettingsDialog : public QObject
{
    Q_OBJECT

private slots:
    void showsProfileValues();
    void showsServerCertificateRows();
    void mapsSecurityAndAnonymousAuthentication();
};

namespace {

/// \brief Returns the text of a named QLabel child, or a marker when it is missing.
QString labelText(const EndpointSettingsDialog &dialog, const QString &name)
{
    auto *label = dialog.findChild<QLabel *>(name);
    return label ? label->text() : QStringLiteral("<missing>");
}

} // namespace

///
/// \brief Verifies each field mirrors the corresponding profile value.
///
void TestEndpointSettingsDialog::showsProfileValues()
{
    ConnectionProfile profile;
    profile.endpointUrl = QStringLiteral("opc.tcp://localhost:4840");
    profile.securityPolicy =
        QStringLiteral("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");
    profile.securityMode = 3;
    profile.authentication = ConnectionProfile::Authentication::Username;
    profile.username = QStringLiteral("operator");
    profile.sessionTimeoutMs = 600000;
    profile.maxMessageSizeBytes = 4194304;

    EndpointSettingsDialog dialog;
    dialog.setProfile(profile);

    QCOMPARE(labelText(dialog, QStringLiteral("serverUrlValue")), profile.endpointUrl);
    QCOMPARE(labelText(dialog, QStringLiteral("securityPolicyValue")),
             QStringLiteral("Basic256Sha256"));
    QCOMPARE(labelText(dialog, QStringLiteral("securityModeValue")),
             QStringLiteral("Sign & Encrypt"));
    QVERIFY(labelText(dialog, QStringLiteral("authenticationValue")).contains(profile.username));
    QVERIFY(labelText(dialog, QStringLiteral("sessionTimeoutValue"))
                .contains(QStringLiteral("(10 min)")));
    QVERIFY(labelText(dialog, QStringLiteral("maxMessageSizeValue"))
                .contains(QStringLiteral("MiB")));
}

///
/// \brief Verifies the certificate rows and details button state.
///
void TestEndpointSettingsDialog::showsServerCertificateRows()
{
    EndpointSettingsDialog dialog;
    auto *viewButton = dialog.findChild<QPushButton *>(QStringLiteral("viewCertificateButton"));
    QVERIFY(viewButton);

    QVERIFY(!viewButton->isEnabled());
    QCOMPARE(labelText(dialog, QStringLiteral("certificateSubjectValue")),
             QStringLiteral("The active endpoint does not use a server certificate."));
    QCOMPARE(labelText(dialog, QStringLiteral("certificateStatusValue")), QString());

    const QByteArray certificate = QByteArrayLiteral("der-server-certificate");
    dialog.setServerCertificate(certificate);

    QVERIFY(viewButton->isEnabled());
    QCOMPARE(labelText(dialog, QStringLiteral("certificateSubjectValue")),
             QStringLiteral("Unable to read certificate"));
    QCOMPARE(labelText(dialog, QStringLiteral("certificateValidUntilValue")),
             QStringLiteral("%1 bytes").arg(certificate.size()));
    QCOMPARE(labelText(dialog, QStringLiteral("certificateStatusValue")),
             QStringLiteral("Invalid"));
    QCOMPARE(labelText(dialog, QStringLiteral("certificateSerialNumberValue")),
             QStringLiteral("Unavailable"));

    PkiManager pki;
    QString certificateFile;
    QString privateKeyFile;
    QString error;
    if (!pki.generateClientCertificate(PkiManager::clientCertificateCommonName(),
                                       PkiManager::applicationUri(),
                                       &certificateFile, &privateKeyFile, &error)) {
        QSKIP(qPrintable(error));
    }

    QFile file(certificateFile);
    QVERIFY(file.open(QIODevice::ReadOnly));
    dialog.setServerCertificate(file.readAll());
    file.close();

    auto *statusIcon =
        dialog.findChild<ThemedIconLabel *>(QStringLiteral("certificateStatusIcon"));
    QVERIFY(statusIcon);
    QVERIFY(!statusIcon->isHidden());
    QCOMPARE(labelText(dialog, QStringLiteral("certificateStatusValue")),
             QStringLiteral("Valid"));

    QVERIFY(QFile::remove(certificateFile));
    QVERIFY(QFile::remove(privateKeyFile));
}

///
/// \brief Verifies the None/Anonymous fallbacks and the short policy name for "None".
///
void TestEndpointSettingsDialog::mapsSecurityAndAnonymousAuthentication()
{
    ConnectionProfile profile;
    profile.endpointUrl = QStringLiteral("opc.tcp://localhost:4840");
    profile.securityPolicy = QStringLiteral("None");
    profile.securityMode = 1;
    profile.authentication = ConnectionProfile::Authentication::Anonymous;

    EndpointSettingsDialog dialog;
    dialog.setProfile(profile);

    QCOMPARE(labelText(dialog, QStringLiteral("securityPolicyValue")), QStringLiteral("None"));
    QCOMPARE(labelText(dialog, QStringLiteral("securityModeValue")), QStringLiteral("None"));
    QCOMPARE(labelText(dialog, QStringLiteral("authenticationValue")),
             QStringLiteral("Anonymous"));
}

QTEST_MAIN(TestEndpointSettingsDialog)

#include "test_endpointsettingsdialog.moc"
