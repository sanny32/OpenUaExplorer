// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_connectiondialog_support.h
/// \brief Shared backend and helpers for ConnectionDialog tests.
///

#pragma once

#include <algorithm>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QCursor>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QSizePolicy>
#include <QSpinBox>
#include <QStandardPaths>
#include <QSslCertificate>
#include <QStackedWidget>
#include <QStyleFactory>
#include <QTableView>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QUuid>

#include "appsettings.h"
#include "dialogs/connectiondialog.h"
#include "models/endpointmodel.h"
#include "opcua/endpointhistorystore.h"
#include "opcua/opcuabackend.h"
#include "opcua/pkimanager.h"
#include "opcua/recentconnectionstore.h"
#include "opcua/secretstore.h"
#include "settingsstore.h"
#include "widgets/certificatesummarywidget.h"
#include "widgets/dialogbuttonbox.h"
#include "widgets/endpointdiscoverywidget.h"
#include "widgets/historycombobox.h"

///
/// \brief Minimal OPC UA backend double that only counts discovery calls.
///
class DialogFakeBackend : public OpcUaBackend
{
public:
    using OpcUaBackend::OpcUaBackend;

    bool isAvailable() const override { return true; }
    QStringList availableBackends() const override { return {QStringLiteral("fake")}; }
    OpcUaConnectionState state() const override
    {
        return OpcUaConnectionState::Disconnected;
    }
    QString lastError() const override { return {}; }
    void setCertificateTrustDecider(CertificateTrustDecider *) override {}

    void discoverEndpoints(const QString &, const QString &, int) override
    {
        ++discoveryCalls;
    }
    void connectToEndpoint(const ConnectionProfile &, const QString &,
                           const QString &) override {}
    void disconnectFromEndpoint() override {}
    void browse(const QString &) override {}
    void browseReferences(const QString &) override {}
    void readNode(const QString &) override {}
    void readValues(const QStringList &) override {}
    void writeValue(const QString &, const QVariant &, int) override {}

    int discoveryCalls = 0;
};

///
/// \brief Drives the dialog through discovery, certificate, and layout scenarios.
///
class TestConnectionDialog : public QObject
{
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
    void discoveryPopulatesEndpointModelAndAuthentication();
    void endpointsWidgetSelectsAPolicyAndModePair();
    void closingDuringDiscoveryRestoresCursor();
    void clientCertificateActionFollowsSelection();
    void clientCertificateSelectorFillsRow();
    void certificateStatusRowsAlignBadgeToRight();
    void advancedSettingsControlsAlignToGrid();
    void advancedSettingsSeedFromStoredDefaults();
    void serverTrustStateFollowsTrustList();
    void trustIsRefusedForACertificateOutsideItsValidity();
    void endpointHoverDoesNotUseSelectionBackgroundInFusion();

private:
    QTemporaryDir _settingsDirectory;
};

namespace {

///
/// \brief Asserts a certificate panel's validity row right-aligns its icon and badge.
/// \param dialog Dialog under test.
/// \param panelName Object name of the certificate summary panel.
///
void verifyRightAlignedCertificateStatus(ConnectionDialog &dialog, const QString &panelName)
{
    auto *panel = dialog.findChild<QWidget *>(panelName);
    QVERIFY(panel);

    // The Subject/Issuer/Valid/Fingerprint rows now live inside the reusable
    // CertificateSummaryWidget, so scope the lookups to that panel.
    auto *layout = panel->findChild<QHBoxLayout *>(QStringLiteral("validLayout"));
    auto *dateLabel = panel->findChild<QLabel *>(QStringLiteral("validEdit"));
    auto *iconLabel = panel->findChild<QLabel *>(QStringLiteral("validIcon"));
    auto *badgeLabel = panel->findChild<QLabel *>(QStringLiteral("validBadge"));

    QVERIFY(layout);
    QVERIFY(dateLabel);
    QVERIFY(iconLabel);
    QVERIFY(badgeLabel);
    QCOMPARE(layout->count(), 4);
    QCOMPARE(dateLabel->sizePolicy().horizontalPolicy(), QSizePolicy::Maximum);
    QCOMPARE(iconLabel->sizePolicy().horizontalPolicy(), QSizePolicy::Maximum);
    QCOMPARE(badgeLabel->sizePolicy().horizontalPolicy(), QSizePolicy::Maximum);
    QVERIFY(layout->itemAt(1)->spacerItem());
    QVERIFY(layout->itemAt(1)->expandingDirections().testFlag(Qt::Horizontal));
    QCOMPARE(layout->itemAt(2)->widget(), iconLabel);
    QCOMPARE(layout->itemAt(3)->widget(), badgeLabel);
}

///
/// \brief Reports whether a named Qt style can be created.
/// \param styleName Style name to look up.
/// \return True when Qt lists the style.
///
bool hasStyle(const QString &styleName)
{
    for (const QString &key : QStyleFactory::keys()) {
        if (key.compare(styleName, Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
}

///
/// \brief Builds a minimal endpoint row for dialog UI tests.
/// \param url Endpoint URL to show.
/// \return Endpoint row data.
///
EndpointInfo makeDialogEndpoint(const QString &url)
{
    EndpointInfo endpoint;
    endpoint.endpointUrl = url;
    endpoint.securityPolicy = QStringLiteral("None");
    endpoint.securityMode = QStringLiteral("None");
    endpoint.securityModeValue = 1;
    endpoint.supportsAnonymous = true;
    return endpoint;
}

///
/// \brief Samples a cell background away from its text.
/// \param view Table view to render.
/// \param row Model row to sample.
/// \param column Model column to sample.
/// \return Rendered pixel colour.
///
QColor cellBackgroundColor(QTableView *view, int row, int column)
{
    const QModelIndex index = view->model()->index(row, column);
    const QRect rect = view->visualRect(index);
    QImage image(view->viewport()->size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    view->viewport()->render(&painter);
    painter.end();

    return image.pixelColor(rect.right() - 4, rect.center().y());
}

///
/// \brief Generates a client certificate and returns its DER bytes.
/// \return DER-encoded certificate, or an empty array when generation is unavailable.
///
QByteArray generateCertificate()
{
    PkiManager pki;
    QString certificateFile;
    QString privateKeyFile;
    QString error;
    if (!pki.generateClientCertificate(PkiManager::clientCertificateCommonName(),
                                       PkiManager::applicationUri(),
                                       &certificateFile, &privateKeyFile, &error)) {
        return {};
    }
    QFile file(certificateFile);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

///
/// \brief Answers the next modal question dialog, waiting for it to appear.
/// \param answer Standard button to click once the dialog is up.
///
void answerNextQuestion(DialogButtonBox::StandardButton answer)
{
    QTimer::singleShot(0, qApp, [answer]() {
        auto *modal = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!modal) {
            answerNextQuestion(answer);
            return;
        }
        auto *buttons = modal->findChild<DialogButtonBox *>();
        QVERIFY(buttons);
        QPushButton *button = buttons->button(answer);
        QVERIFY(button);
        QTest::mouseClick(button, Qt::LeftButton);
    });
}

///
/// \brief Reports whether the trust list holds a certificate.
/// \param certificate DER-encoded certificate.
/// \return True when the trusted store holds this certificate.
///
bool isInTrustList(const QByteArray &certificate)
{
    PkiManager pki;
    const QString wanted = PkiManager::fingerprint(certificate);
    const QList<QByteArray> trusted = pki.certificates(PkiManager::Category::Trusted);
    return std::any_of(trusted.cbegin(), trusted.cend(), [&wanted](const QByteArray &stored) {
        return PkiManager::fingerprint(stored) == wanted;
    });
}

}