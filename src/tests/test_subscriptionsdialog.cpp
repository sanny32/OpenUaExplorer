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
#include <QSignalSpy>
#include <QTableView>
#include <QTemporaryDir>
#include <QTest>
#include <QTranslator>

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
    void builtinNamesFollowTheInterfaceLanguage();
    void factoryNamesAreNeverStoredAsRenames();
    void aNameSavedInAnotherLanguageIsDroppedOnLoad();
    void restoreDefaultsClearsANameSavedInAnotherLanguage();
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

namespace {

///
/// \brief Translator that suffixes every SubscriptionsWidget string, standing in for a language.
///
class SuffixTranslator : public QTranslator
{
public:
    QString translate(const char *context, const char *sourceText,
                      const char *disambiguation = nullptr, int n = -1) const override
    {
        Q_UNUSED(disambiguation)
        Q_UNUSED(n)
        if (qstrcmp(context, "SubscriptionsWidget") != 0)
            return QString();
        return QString::fromUtf8(sourceText) + QStringLiteral("-xx");
    }

    bool isEmpty() const override { return false; }
};

///
/// \brief Installs or removes a translator and delivers the language change.
/// \param translator Translator to switch on or off.
/// \param install True to install, false to remove.
///
void switchLanguage(QTranslator *translator, bool install)
{
    if (install)
        QCoreApplication::installTranslator(translator);
    else
        QCoreApplication::removeTranslator(translator);
    QCoreApplication::sendPostedEvents();
    QCoreApplication::processEvents();
}

} // namespace

///
/// \brief A language change renames the untouched built-ins and spares the ones the user named.
///
void TestSubscriptionsDialog::builtinNamesFollowTheInterfaceLanguage()
{
    SubscriptionsDialog dialog;
    SubscriptionsWidget *widget = dialog.subscriptions();
    auto *table = dialog.findChild<QTableView *>(QStringLiteral("subscriptionsTable"));
    QVERIFY(widget);
    QVERIFY(table);

    QVERIFY(table->model()->setData(table->model()->index(0, 0),
                                    QStringLiteral("Telemetry"), Qt::EditRole));

    QSignalSpy renameSpy(widget, &SubscriptionsWidget::subscriptionRenamed);
    SuffixTranslator translator;
    switchLanguage(&translator, true);

    QVector<SubscriptionItem> items = widget->subscriptions();
    QCOMPARE(items.at(0).name, QStringLiteral("Telemetry"));
    QCOMPARE(items.at(1).name, QStringLiteral("Fast-xx"));
    QCOMPARE(items.at(2).name, QStringLiteral("Slow-xx"));

    QCOMPARE(renameSpy.size(), 2);
    QCOMPARE(renameSpy.first().first().toString(), QStringLiteral("Fast"));
    QCOMPARE(renameSpy.first().at(1).toString(), QStringLiteral("Fast-xx"));

    switchLanguage(&translator, false);
    items = widget->subscriptions();
    QCOMPARE(items.at(0).name, QStringLiteral("Telemetry"));
    QCOMPARE(items.at(1).name, QStringLiteral("Fast"));
    QCOMPARE(items.at(2).name, QStringLiteral("Slow"));
}

///
/// \brief A factory name is never persisted, whatever language it was shown in.
///
void TestSubscriptionsDialog::factoryNamesAreNeverStoredAsRenames()
{
    SuffixTranslator translator;
    {
        SubscriptionsDialog dialog;
        switchLanguage(&translator, true);
        QCOMPARE(dialog.subscriptions()->subscriptions().at(0).name,
                 QStringLiteral("Default-xx"));

        AppSettings settings;
        dialog.subscriptions()->saveSubscriptions(settings);
    }
    QVERIFY(AppSettings().builtinSubscriptionOverrides().isEmpty());

    switchLanguage(&translator, false);
    SubscriptionsDialog restored;
    AppSettings settings;
    restored.subscriptions()->loadSubscriptions(settings);
    QCOMPARE(restored.subscriptions()->subscriptions().at(0).name, QStringLiteral("Default"));
}

///
/// \brief A built-in name frozen by an older run in another language is dropped when loaded.
///
void TestSubscriptionsDialog::aNameSavedInAnotherLanguageIsDroppedOnLoad()
{
    SubscriptionItem stale;
    stale.id = SlowSubscriptionId;
    stale.builtin = true;
    stale.name = QStringLiteral("Slow");
    stale.publishingInterval = 9000.0;

    SubscriptionItem renamed;
    renamed.id = FastSubscriptionId;
    renamed.builtin = true;
    renamed.name = QStringLiteral("Telemetry");
    renamed.publishingInterval = 250.0;

    AppSettings().setBuiltinSubscriptionOverrides({stale, renamed});

    SubscriptionsDialog dialog;
    AppSettings settings;
    dialog.subscriptions()->loadSubscriptions(settings);

    const QVector<SubscriptionItem> items = dialog.subscriptions()->subscriptions();
    QCOMPARE(items.at(2).name, QStringLiteral("Slow"));
    QCOMPARE(items.at(2).publishingInterval, 9000.0);
    QCOMPARE(items.at(1).name, QStringLiteral("Telemetry"));

    dialog.subscriptions()->saveSubscriptions(settings);
    const QVector<SubscriptionItem> stored = AppSettings().builtinSubscriptionOverrides();
    QCOMPARE(stored.size(), 2);
    for (const SubscriptionItem &item : stored) {
        if (item.id == SlowSubscriptionId)
            QVERIFY(item.name.isEmpty());
        else
            QCOMPARE(item.name, QStringLiteral("Telemetry"));
    }
}

///
/// \brief Restore Defaults clears a built-in name stored by an older run in another language.
///
void TestSubscriptionsDialog::restoreDefaultsClearsANameSavedInAnotherLanguage()
{
    SubscriptionItem stale;
    stale.id = DefaultSubscriptionId;
    stale.builtin = true;
    stale.name = QString::fromUtf8("默认");
    stale.publishingInterval = 1000.0;
    AppSettings().setBuiltinSubscriptionOverrides({stale});

    SubscriptionsDialog dialog;
    AppSettings settings;
    dialog.subscriptions()->loadSubscriptions(settings);
    QCOMPARE(dialog.subscriptions()->subscriptions().at(0).name, QString::fromUtf8("默认"));

    auto *restoreButton = dialog.findChild<QAbstractButton *>(QStringLiteral("restoreDefaultsButton"));
    QVERIFY(restoreButton);
    restoreButton->click();

    QCOMPARE(dialog.subscriptions()->subscriptions().at(0).name, QStringLiteral("Default"));

    dialog.subscriptions()->saveSubscriptions(settings);
    QVERIFY(AppSettings().builtinSubscriptionOverrides().isEmpty());
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
