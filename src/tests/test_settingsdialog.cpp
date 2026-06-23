// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_settingsdialog.cpp
/// \brief Tests the settings dialog layout and preference controls.
///

#include <QAbstractButton>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

#include "application.h"
#include "appsettings.h"
#include "dialogs/settingsdialog.h"
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
    QSettings settings;
    settings.clear();
}

///
/// \brief Verifies the theme cards, action buttons, and three-column logging grid.
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
    QCOMPARE(lightCard->sizeHint(), QSize(190, 150));
    QVERIFY(dialog.findChild<ThemePreviewButton *>(QStringLiteral("darkThemeButton")));
    QVERIFY(dialog.findChild<ThemePreviewButton *>(QStringLiteral("systemThemeButton")));
    QVERIFY(dialog.findChild<QPushButton *>(QStringLiteral("resetButton")));

    auto *buttons = dialog.findChild<DialogButtonBox *>(QStringLiteral("buttonBox"));
    auto *loggingGrid = dialog.findChild<QGridLayout *>(QStringLiteral("logGroupLayout"));
    QVERIFY(buttons);
    QVERIFY(buttons->button(QDialogButtonBox::Apply));
    QVERIFY(!buttons->button(QDialogButtonBox::Apply)->isEnabled());
    QVERIFY(loggingGrid);
    QVERIFY(!qobject_cast<QGroupBox *>(
        dialog.findChild<QWidget *>(QStringLiteral("logCategoriesGroup"))));
    QCOMPARE(loggingGrid->count(), AppSettings::availableLogCategories().size());
    QCOMPARE(loggingGrid->columnCount(), 3);
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
    auto *plugin = dialog.findChild<QCheckBox *>(QStringLiteral("logCategory_plugin"));
    auto *dark = dialog.findChild<QAbstractButton *>(QStringLiteral("darkThemeButton"));
    auto *buttons = dialog.findChild<DialogButtonBox *>(QStringLiteral("buttonBox"));
    QVERIFY(plugin);
    QVERIFY(dark);
    QVERIFY(buttons);

    plugin->setChecked(false);
    if (theApp()->theme().isManualToggleSupported())
        dark->click();
    QVERIFY(buttons->button(QDialogButtonBox::Apply)->isEnabled());
    buttons->button(QDialogButtonBox::Apply)->click();

    QVERIFY(!AppSettings().logCategoryStates().value(QStringLiteral("plugin")));
    if (theApp()->theme().isManualToggleSupported())
        QCOMPARE(AppSettings().themeMode(), AppSettings::ThemeMode::Dark);
    QCOMPARE(dialog.result(), 0);
    QVERIFY(!buttons->button(QDialogButtonBox::Apply)->isEnabled());
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
