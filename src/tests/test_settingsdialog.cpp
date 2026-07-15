// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_settingsdialog.cpp
/// \brief Tests the settings dialog layout and preference controls.
///

#include <QAbstractButton>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QSettings>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>

#include "application.h"
#include "appsettings.h"
#include "dialogs/settingsdialog.h"
#include "settingsstore.h"
#include "widgets/dialogbuttonbox.h"
#include "widgets/themepreviewbutton.h"

///
/// \brief Verifies the settings dialog's section layout and interactions.
///
class TestSettingsDialog : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();
    void referenceControlsArePresent();
    void themeCardsSelectOneMode();
    void applyPersistsWithoutClosing();
    void timestampModeComboPersists();
    void layoutResetWaitsForAcceptance();

private:
    QTemporaryDir _settingsDirectory;
};

///
/// \brief Routes settings to a temporary directory.
///
void TestSettingsDialog::initTestCase()
{
    QVERIFY(_settingsDirectory.isValid());
    QCoreApplication::setOrganizationName(QStringLiteral("OpenUaExplorerTests"));
    QCoreApplication::setApplicationName(QStringLiteral("SettingsDialog"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       _settingsDirectory.path());
}

///
/// \brief Clears preferences between test cases.
///
void TestSettingsDialog::cleanup()
{
    SettingsStore settings;
    settings.clear();
}

///
/// \brief Verifies the theme cards, action buttons, and grouped logging grids.
///
void TestSettingsDialog::referenceControlsArePresent()
{
    SettingsDialog dialog;

    QVERIFY(!dialog.windowFlags().testFlag(Qt::FramelessWindowHint));
    QCOMPARE(dialog.windowTitle(), QStringLiteral("Settings"));
    auto *themeGroup = dialog.findChild<QGroupBox *>(QStringLiteral("themeGroup"));
    QVERIFY(themeGroup);
    QCOMPARE(!themeGroup->isHidden(), theApp()->theme().isManualToggleSupported());

    auto *lightCard = dialog.findChild<ThemePreviewButton *>(
        QStringLiteral("lightThemeButton"));
    QVERIFY(lightCard);
    QCOMPARE(lightCard->sizeHint(), QSize(150, 118));
    QVERIFY(dialog.findChild<ThemePreviewButton *>(QStringLiteral("darkThemeButton")));
    QVERIFY(dialog.findChild<ThemePreviewButton *>(QStringLiteral("systemThemeButton")));
    QVERIFY(dialog.findChild<QPushButton *>(QStringLiteral("resetButton")));

    auto *buttons = dialog.findChild<DialogButtonBox *>(QStringLiteral("buttonBox"));
    auto *loggingTabs = dialog.findChild<QTabWidget *>(QStringLiteral("logCategoryTabs"));
    auto *applicationGrid = dialog.findChild<QGridLayout *>(QStringLiteral("applicationLogGroupLayout"));
    auto *open62541Grid = dialog.findChild<QGridLayout *>(QStringLiteral("open62541LogGroupLayout"));
    QVERIFY(buttons);
    QVERIFY(buttons->button(QDialogButtonBox::Apply));
    QVERIFY(!buttons->button(QDialogButtonBox::Apply)->isEnabled());
    QVERIFY(loggingTabs);
    QCOMPARE(loggingTabs->count(), 2);
    QCOMPARE(loggingTabs->tabText(0), QStringLiteral("Application"));
    QCOMPARE(loggingTabs->tabText(1), QStringLiteral("open62541 backend"));
    QVERIFY(applicationGrid);
    QVERIFY(open62541Grid);
    QVERIFY(!qobject_cast<QGroupBox *>(
        dialog.findChild<QWidget *>(QStringLiteral("logCategoriesGroup"))));
    QCOMPARE(applicationGrid->count(), AppSettings::availableApplicationLogCategories().size());
    QCOMPARE(open62541Grid->count(),
             AppSettings::availableQtOpcUaLogCategories().size()
                 + AppSettings::availableOpen62541LogCategories().size());
    QCOMPARE(open62541Grid->columnCount(), 3);
    auto *plugin = dialog.findChild<QCheckBox *>(QStringLiteral("logCategory_plugin"));
    QVERIFY(plugin);
    QCOMPARE(plugin->text(), QStringLiteral("plugin"));
    auto *securityPolicy = dialog.findChild<QCheckBox *>(
        QStringLiteral("logCategory_securitypolicy"));
    QVERIFY(securityPolicy);
    QCOMPARE(securityPolicy->text(), QStringLiteral("security policy"));
}

///
/// \brief Verifies that theme cards behave as one exclusive preference group.
///
void TestSettingsDialog::themeCardsSelectOneMode()
{
    SettingsDialog dialog;
    auto *light = dialog.findChild<QAbstractButton *>(QStringLiteral("lightThemeButton"));
    auto *dark = dialog.findChild<QAbstractButton *>(QStringLiteral("darkThemeButton"));
    auto *system = dialog.findChild<QAbstractButton *>(QStringLiteral("systemThemeButton"));
    auto *buttons = dialog.findChild<DialogButtonBox *>(QStringLiteral("buttonBox"));
    QVERIFY(light);
    QVERIFY(dark);
    QVERIFY(system);
    QVERIFY(buttons);

    QVERIFY(system->isChecked());
    QVERIFY(!buttons->button(QDialogButtonBox::Apply)->isEnabled());

    dark->click();
    QVERIFY(dark->isChecked());
    QVERIFY(!light->isChecked());
    QVERIFY(!system->isChecked());
    QVERIFY(buttons->button(QDialogButtonBox::Apply)->isEnabled());

    light->click();
    QVERIFY(light->isChecked());
    QVERIFY(!dark->isChecked());
    QVERIFY(!system->isChecked());

    system->click();
    QVERIFY(system->isChecked());
    QVERIFY(!light->isChecked());
    QVERIFY(!dark->isChecked());
}

///
/// \brief Verifies that Apply saves logging preferences without closing the dialog.
///
void TestSettingsDialog::applyPersistsWithoutClosing()
{
    SettingsDialog dialog;
    auto *app = dialog.findChild<QCheckBox *>(QStringLiteral("logCategory_application.app"));
    auto *plugin = dialog.findChild<QCheckBox *>(QStringLiteral("logCategory_plugin"));
    auto *dark = dialog.findChild<QAbstractButton *>(QStringLiteral("darkThemeButton"));
    auto *buttons = dialog.findChild<DialogButtonBox *>(QStringLiteral("buttonBox"));
    QVERIFY(app);
    QVERIFY(plugin);
    QVERIFY(dark);
    QVERIFY(buttons);

    app->setChecked(false);
    plugin->setChecked(false);
    if (theApp()->theme().isManualToggleSupported())
        dark->click();
    QVERIFY(buttons->button(QDialogButtonBox::Apply)->isEnabled());
    buttons->button(QDialogButtonBox::Apply)->click();

    QVERIFY(!AppSettings().logCategoryStates().value(QStringLiteral("application.app")));
    QVERIFY(!AppSettings().logCategoryStates().value(QStringLiteral("plugin")));
    if (theApp()->theme().isManualToggleSupported())
        QCOMPARE(AppSettings().themeMode(), AppSettings::ThemeMode::Dark);
    QCOMPARE(dialog.result(), 0);
    QVERIFY(!buttons->button(QDialogButtonBox::Apply)->isEnabled());
}

///
/// \brief Verifies the timestamp-mode combo reflects and persists the preference.
///
void TestSettingsDialog::timestampModeComboPersists()
{
    SettingsDialog dialog;
    auto *combo = dialog.findChild<QComboBox *>(QStringLiteral("timestampModeCombo"));
    auto *buttons = dialog.findChild<DialogButtonBox *>(QStringLiteral("buttonBox"));
    QVERIFY(combo);
    QVERIFY(buttons);

    // The default preference is UTC (index 1).
    QCOMPARE(combo->currentIndex(), 1);
    QVERIFY(!buttons->button(QDialogButtonBox::Apply)->isEnabled());

    combo->setCurrentIndex(0);
    QVERIFY(buttons->button(QDialogButtonBox::Apply)->isEnabled());
    buttons->button(QDialogButtonBox::Apply)->click();

    QCOMPARE(AppSettings().timestampMode(), AppSettings::TimestampMode::LocalTime);
    QCOMPARE(dialog.result(), 0);
}

///
/// \brief Verifies that layout reset is cancelled or committed with the dialog.
///
void TestSettingsDialog::layoutResetWaitsForAcceptance()
{
    const QByteArray geometry("saved-geometry");
    AppSettings().setWindowGeometry(geometry);

    SettingsDialog cancelled;
    cancelled.findChild<QPushButton *>(QStringLiteral("resetButton"))->click();
    cancelled.reject();
    QCOMPARE(AppSettings().windowGeometry(), geometry);

    SettingsDialog accepted;
    accepted.findChild<QPushButton *>(QStringLiteral("resetButton"))->click();
    auto *buttons = accepted.findChild<DialogButtonBox *>(QStringLiteral("buttonBox"));
    buttons->button(QDialogButtonBox::Ok)->click();
    QCOMPARE(accepted.result(), static_cast<int>(QDialog::Accepted));
    QVERIFY(accepted.layoutResetRequested());
    QVERIFY(AppSettings().windowGeometry().isEmpty());
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
    TestSettingsDialog test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_settingsdialog.moc"
