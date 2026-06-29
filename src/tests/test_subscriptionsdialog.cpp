// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_subscriptionsdialog.cpp
/// \brief Tests the subscriptions management dialog.
///

#include <QAbstractButton>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTableView>
#include <QTest>

#include "application.h"
#include "dialogs/subscriptionsdialog.h"
#include "widgets/dialogbuttonbox.h"
#include "widgets/subscriptionswidget.h"

///
/// \brief UI tests for SubscriptionsDialog.
///
class TestSubscriptionsDialog : public QObject
{
    Q_OBJECT

private slots:
    void referenceControlsArePresent();
    void addRemoveAndClose();
};

///
/// \brief Verifies the hosted subscriptions widget and table are present.
///
void TestSubscriptionsDialog::referenceControlsArePresent()
{
    SubscriptionsDialog dialog;

    QCOMPARE(dialog.windowTitle(), QStringLiteral("Subscriptions"));
    QVERIFY(dialog.subscriptions());
    QVERIFY(dialog.findChild<SubscriptionsWidget *>(QStringLiteral("subscriptionsWidget")));
    auto *table = dialog.findChild<QTableView *>(QStringLiteral("subscriptionsTable"));
    QVERIFY(table);
    QCOMPARE(table->model()->rowCount(), 1);
}

///
/// \brief Verifies subscription editing works and Close hides the modeless dialog.
///
void TestSubscriptionsDialog::addRemoveAndClose()
{
    SubscriptionsDialog dialog;
    auto *table = dialog.findChild<QTableView *>(QStringLiteral("subscriptionsTable"));
    auto *addButton = dialog.findChild<QAbstractButton *>(QStringLiteral("addSubscriptionButton"));
    auto *removeButton = dialog.findChild<QAbstractButton *>(QStringLiteral("removeSubscriptionButton"));
    auto *buttonBox = dialog.findChild<DialogButtonBox *>(QStringLiteral("buttonBox"));
    QVERIFY(table);
    QVERIFY(addButton);
    QVERIFY(removeButton);
    QVERIFY(buttonBox);

    addButton->click();
    QCOMPARE(table->model()->rowCount(), 2);

    table->selectRow(1);
    QVERIFY(removeButton->isEnabled());
    removeButton->click();
    QCOMPARE(table->model()->rowCount(), 1);

    dialog.show();
    QVERIFY(dialog.isVisible());
    QAbstractButton *closeButton = buttonBox->button(QDialogButtonBox::Close);
    QVERIFY(closeButton);
    closeButton->click();
    QVERIFY(!dialog.isVisible());
}

///
/// \brief Runs the suite under Application so theme services are available.
/// \param argc Argument count.
/// \param argv Argument vector.
/// \return Test exit code.
///
int main(int argc, char *argv[])
{
    Application app(argc, argv);
    TestSubscriptionsDialog test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_subscriptionsdialog.moc"
