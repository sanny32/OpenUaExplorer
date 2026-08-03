// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_subscriptionsdialog.cpp
/// \brief Tests the subscriptions management dialog.
///

#include <QAbstractButton>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QSettings>
#include <QTableView>
#include <QTemporaryDir>
#include <QTest>

#include "application.h"
#include "appsettings.h"
#include "dialogs/subscriptionsdialog.h"
#include "settingsstore.h"
#include "widgets/dialogbuttonbox.h"
#include "widgets/subscriptionswidget.h"

///
/// \brief UI tests for SubscriptionsDialog.
///
class TestSubscriptionsDialog : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void referenceControlsArePresent();
    void addRemoveAndClose();
    void builtinEditsSurviveSaveAndLoad();
    void restoreDefaultsKeepsCustomSubscriptions();
    void customIdsNeverCollideWithBuiltinIds();

private:
    QTemporaryDir _settingsDirectory;
};

///
/// \brief Routes QSettings to a throwaway directory so tests never touch the
///        real user configuration.
///
void TestSubscriptionsDialog::initTestCase()
{
    QVERIFY(_settingsDirectory.isValid());
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       _settingsDirectory.path());
}

///
/// \brief Clears all stored settings between tests to keep them independent.
///
void TestSubscriptionsDialog::cleanup()
{
    SettingsStore settings;
    settings.clear();
}

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
    QCOMPARE(table->model()->rowCount(), 3);
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
    QCOMPARE(table->model()->rowCount(), 4);

    table->selectRow(3);
    QVERIFY(removeButton->isEnabled());
    removeButton->click();
    QCOMPARE(table->model()->rowCount(), 3);

    dialog.show();
    QVERIFY(dialog.isVisible());
    QAbstractButton *closeButton = buttonBox->button(QDialogButtonBox::Close);
    QVERIFY(closeButton);
    closeButton->click();
    QVERIFY(!dialog.isVisible());
}

///
/// \brief Edits to a built-in subscription are persisted and reapplied on the next run.
///
void TestSubscriptionsDialog::builtinEditsSurviveSaveAndLoad()
{
    {
        SubscriptionsDialog dialog;
        SubscriptionsWidget *widget = dialog.subscriptions();
        auto *table = dialog.findChild<QTableView *>(QStringLiteral("subscriptionsTable"));
        QVERIFY(widget);
        QVERIFY(table);

        // Rename and retime the built-in Default, and retime Slow without renaming it.
        QVERIFY(table->model()->setData(table->model()->index(0, 0),
                                        QStringLiteral("Telemetry"), Qt::EditRole));
        QVERIFY(table->model()->setData(table->model()->index(0, 1), 50.0, Qt::EditRole));
        QVERIFY(table->model()->setData(table->model()->index(2, 1), 9000.0, Qt::EditRole));

        AppSettings settings;
        widget->saveSubscriptions(settings);
    }

    // Fast was left untouched, so it must not be written out at all.
    QCOMPARE(AppSettings().builtinSubscriptionOverrides().size(), 2);

    SubscriptionsDialog restored;
    AppSettings settings;
    restored.subscriptions()->loadSubscriptions(settings);

    const QVector<SubscriptionItem> items = restored.subscriptions()->subscriptions();
    QCOMPARE(items.size(), 3);
    QCOMPARE(items.at(0).name, QStringLiteral("Telemetry"));
    QCOMPARE(items.at(0).publishingInterval, 50.0);
    QVERIFY(items.at(0).isBuiltin());
    QCOMPARE(items.at(1).name, QStringLiteral("Fast"));
    QCOMPARE(items.at(1).publishingInterval, 250.0);
    QCOMPARE(items.at(2).name, QStringLiteral("Slow"));
    QCOMPARE(items.at(2).publishingInterval, 9000.0);
}

///
/// \brief Restore Defaults resets the built-in rows and leaves custom subscriptions alone.
///
void TestSubscriptionsDialog::restoreDefaultsKeepsCustomSubscriptions()
{
    SubscriptionsDialog dialog;
    SubscriptionsWidget *widget = dialog.subscriptions();
    auto *table = dialog.findChild<QTableView *>(QStringLiteral("subscriptionsTable"));
    QVERIFY(widget);
    QVERIFY(table);

    widget->createSubscription(QStringLiteral("Custom"), 750.0);
    QVERIFY(table->model()->setData(table->model()->index(0, 0),
                                    QStringLiteral("Telemetry"), Qt::EditRole));
    QVERIFY(table->model()->setData(table->model()->index(0, 1), 50.0, Qt::EditRole));

    widget->restoreBuiltinDefaults();

    const QVector<SubscriptionItem> items = widget->subscriptions();
    QCOMPARE(items.size(), 4);
    QCOMPARE(items.at(0).name, QStringLiteral("Default"));
    QCOMPARE(items.at(0).publishingInterval, 1000.0);
    QCOMPARE(items.at(1).publishingInterval, 250.0);
    QCOMPARE(items.at(2).publishingInterval, 5000.0);
    QCOMPARE(items.at(3).name, QStringLiteral("Custom"));
    QCOMPARE(items.at(3).publishingInterval, 750.0);

    // Back at factory values, nothing needs to be stored.
    AppSettings settings;
    widget->saveSubscriptions(settings);
    QVERIFY(AppSettings().builtinSubscriptionOverrides().isEmpty());
}

///
/// \brief Adding a subscription after removals never reuses a built-in identifier.
///
void TestSubscriptionsDialog::customIdsNeverCollideWithBuiltinIds()
{
    SubscriptionsDialog dialog;
    SubscriptionsWidget *widget = dialog.subscriptions();
    QVERIFY(widget);

    widget->createSubscription(QStringLiteral("First"), 500.0);
    widget->createSubscription(QStringLiteral("Second"), 600.0);
    QCOMPARE(widget->subscriptions().at(3).id, 3);
    QCOMPARE(widget->subscriptions().at(4).id, 4);

    auto *table = dialog.findChild<QTableView *>(QStringLiteral("subscriptionsTable"));
    QVERIFY(table);
    auto *removeButton = dialog.findChild<QAbstractButton *>(
        QStringLiteral("removeSubscriptionButton"));
    QVERIFY(removeButton);
    table->selectRow(3);
    removeButton->click();

    widget->createSubscription(QStringLiteral("Third"), 700.0);
    const QVector<SubscriptionItem> items = widget->subscriptions();
    QCOMPARE(items.size(), 5);
    QCOMPARE(items.at(4).name, QStringLiteral("Third"));
    QCOMPARE(items.at(4).id, 5);
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
