// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_dialogabout.cpp
/// \brief Unit tests for the About dialog.
///

#include <QApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QTextBrowser>
#include <QTextDocument>
#include <QTest>
#include <QWidget>
#include <QtGlobal>

#include "dialogs/dialogabout.h"

///
/// \brief Tests the About dialog content.
///
class TestDialogAbout : public QObject
{
    Q_OBJECT

private slots:
    void loadsAuthorsFromAboutJson();
    void listsComponents();
    void formatsLicenseInformation();
    void resizesTabAreaInsteadOfHeader();
};

///
/// \brief Verifies the authors tab is populated from about.json.
///
void TestDialogAbout::loadsAuthorsFromAboutJson()
{
    DialogAbout dialog;

    QFile file(QStringLiteral(":/res/about.json"));
    QVERIFY(file.open(QFile::ReadOnly));

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    QVERIFY(document.isObject());

    QStringList names;
    for (const QString &sectionName :
         { QStringLiteral("authors"), QStringLiteral("contributors") }) {
        const QJsonArray people = document.object().value(sectionName).toArray();
        for (const QJsonValue &value : people) {
            names.append(value.toObject().value(QStringLiteral("name")).toString());
        }
    }
    QCOMPARE(document.object().value(QStringLiteral("contributors")).toArray().size(), 0);
    QCOMPARE(names.size(), 1);

    auto *nameLabel0 = dialog.findChild<QLabel *>(QStringLiteral("authorNameLabel0"));
    auto *roleLabel0 = dialog.findChild<QLabel *>(QStringLiteral("authorRoleLabel0"));
    QVERIFY(nameLabel0);
    QVERIFY(roleLabel0);

    QCOMPARE(nameLabel0->text(), names.at(0));
    QCOMPARE(roleLabel0->text(), QStringLiteral("Author"));
    QVERIFY(!dialog.findChild<QLabel *>(QStringLiteral("authorNameLabel1")));

    auto *mailButton = dialog.findChild<QPushButton *>(QStringLiteral("authorContactButton0"));
    QVERIFY(mailButton);
    QCOMPARE(mailButton->text(), QStringLiteral("@"));
}

///
/// \brief Verifies the components tab lists runtime and framework components.
///
void TestDialogAbout::listsComponents()
{
    DialogAbout dialog;

    auto *tabWidget = dialog.findChild<QTabWidget *>(QStringLiteral("tabWidget"));
    QVERIFY(tabWidget);
    QVERIFY(tabWidget->tabText(3).contains(QStringLiteral("Components")));
    QVERIFY(!tabWidget->tabText(3).contains(QStringLiteral("Credits")));

    auto *componentsScrollArea = dialog.findChild<QScrollArea *>(QStringLiteral("componentsScrollArea"));
    auto *qtTitle = dialog.findChild<QLabel *>(QStringLiteral("componentTitleLabel0"));
    auto *qtVersion = dialog.findChild<QLabel *>(QStringLiteral("componentVersionLabel0"));
    auto *qtDescription = dialog.findChild<QLabel *>(QStringLiteral("componentDescriptionLabel0"));
    auto *opcUaTitle = dialog.findChild<QLabel *>(QStringLiteral("componentTitleLabel1"));
    auto *qtKeychainTitle = dialog.findChild<QLabel *>(QStringLiteral("componentTitleLabel2"));
    auto *qtKeychainVersion = dialog.findChild<QLabel *>(QStringLiteral("componentVersionLabel2"));
    auto *openSslTitle = dialog.findChild<QLabel *>(QStringLiteral("componentTitleLabel3"));
    auto *openSslVersion = dialog.findChild<QLabel *>(QStringLiteral("componentVersionLabel3"));
    auto *platformDescription =
        dialog.findChild<QLabel *>(QStringLiteral("componentDescriptionLabel4"));
    auto *qtButton = dialog.findChild<QPushButton *>(QStringLiteral("componentContactButton0"));
    auto *platformButton =
        dialog.findChild<QPushButton *>(QStringLiteral("componentContactButton4"));
    QVERIFY(componentsScrollArea);
    QVERIFY(componentsScrollArea->widgetResizable());
    QCOMPARE(componentsScrollArea->frameShape(), QFrame::NoFrame);
    QVERIFY(qtTitle);
    QVERIFY(qtVersion);
    QVERIFY(qtDescription);
    QVERIFY(opcUaTitle);
    QVERIFY(qtKeychainTitle);
    QVERIFY(qtKeychainVersion);
    QVERIFY(openSslTitle);
    QVERIFY(openSslVersion);
    QVERIFY(platformDescription);
    QVERIFY(qtButton);
    QVERIFY(platformButton);

    QCOMPARE(qtTitle->text(), QStringLiteral("Qt"));
    QVERIFY(qtVersion->text().contains(QString::fromLatin1(qVersion())));
    QVERIFY(qtVersion->text().contains(QStringLiteral(QT_VERSION_STR)));
    QVERIFY(qtDescription->text().contains(QStringLiteral("Cross-platform")));
    QCOMPARE(opcUaTitle->text(), QStringLiteral("Qt OPC UA"));
    QCOMPARE(qtKeychainTitle->text(), QStringLiteral("QtKeychain"));
    QVERIFY(qtKeychainVersion->text().contains(QStringLiteral(APP_QTKEYCHAIN_VERSION)));
    QCOMPARE(openSslTitle->text(), QStringLiteral("OpenSSL"));
    QVERIFY(openSslVersion->text().contains(QStringLiteral(APP_OPENSSL_VERSION)));
    QCOMPARE(platformDescription->text(), QStringLiteral("Underlying platform."));
    QVERIFY(qtButton->isVisibleTo(qtButton->parentWidget()));
    QVERIFY(!platformButton->isVisibleTo(platformButton->parentWidget()));
}

///
/// \brief Verifies the license tab presents structured MIT license details.
///
void TestDialogAbout::formatsLicenseInformation()
{
    DialogAbout dialog;

    auto *licenseBrowser = dialog.findChild<QTextBrowser *>(QStringLiteral("licenseBrowser"));
    QVERIFY(licenseBrowser);

    const QString documentStyle = licenseBrowser->document()->defaultStyleSheet();
    QVERIFY(!documentStyle.contains(QStringLiteral("background-color")));
    QCOMPARE(licenseBrowser->horizontalScrollBarPolicy(), Qt::ScrollBarAlwaysOff);

    const QString licenseText = licenseBrowser->toPlainText();
    QVERIFY(licenseText.contains(QStringLiteral("MIT License")));
    QVERIFY(licenseText.contains(
        QStringLiteral("Copyright %1 Alexandr Ananev").arg(QStringLiteral(BUILD_YEAR))));
    QVERIFY(licenseText.contains(QStringLiteral("Permission is hereby granted, free of charge")));
    QVERIFY(licenseText.contains(QStringLiteral("copyright notice and this permission notice")));
    QVERIFY(licenseText.contains(QStringLiteral("THE SOFTWARE IS PROVIDED \"AS IS\"")));
}

///
/// \brief Verifies vertical resize space is assigned to the tab widget.
///
void TestDialogAbout::resizesTabAreaInsteadOfHeader()
{
    DialogAbout dialog;
    dialog.resize(600, 500);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));

    auto *headerWidget = dialog.findChild<QWidget *>(QStringLiteral("headerWidget"));
    auto *tabWidget = dialog.findChild<QTabWidget *>(QStringLiteral("tabWidget"));
    QVERIFY(headerWidget);
    QVERIFY(tabWidget);

    const int headerHeight = headerWidget->height();
    const int tabHeight = tabWidget->height();

    dialog.resize(dialog.width(), dialog.height() + 200);
    QApplication::processEvents();

    QCOMPARE(headerWidget->height(), headerHeight);
    QVERIFY(tabWidget->height() > tabHeight);
}

QTEST_MAIN(TestDialogAbout)

#include "test_dialogabout.moc"
