// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_mainwindow_theme.cpp
/// \brief Tests the main window's theme controls and AppTheme scheme switching.
///

#include <QAction>
#include <QDockWidget>
#include <QGuiApplication>
#include <QMenu>
#include <QSettings>
#include <QSignalSpy>
#include <QStyleHints>
#include <QTableView>
#include <QTemporaryDir>
#include <QTest>
#include <QtGlobal>

#include "appsettings.h"
#include "application.h"
#include "mainwindow.h"

///
/// \brief Verifies the theme action, the Theme submenu and AppTheme's mode switching.
///
class TestMainWindowTheme : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void themeControlsFollowManualThemeSupport();
    void themeButtonCyclesThroughModes();
    void colorSchemePreferenceAppliesAndPersists();
    void systemPreferenceFollowsTheCurrentScheme();
    void nodeDetailsDockFollowsViewAction();

private:
    QTemporaryDir _settingsDirectory;
};

///
/// \brief Routes QSettings to a throwaway directory so tests never touch real configuration.
///
void TestMainWindowTheme::initTestCase()
{
    QVERIFY(_settingsDirectory.isValid());
    QCoreApplication::setOrganizationName(QStringLiteral("OpenUaExplorerTests"));
    QCoreApplication::setApplicationName(QStringLiteral("OpenUaExplorerTests"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       _settingsDirectory.path());
}

///
/// \brief Clears stored settings between tests to keep them independent.
///
void TestMainWindowTheme::cleanup()
{
    QSettings settings;
    settings.clear();
}

void TestMainWindowTheme::themeControlsFollowManualThemeSupport()
{
    MainWindow window;

    QAction *themeAction = window.findChild<QAction *>(QStringLiteral("actionTheme"));
    QMenu *themeMenu = window.findChild<QMenu *>(QStringLiteral("menuTheme"));
    QVERIFY(themeAction);
    QVERIFY(themeMenu);

    QCOMPARE(themeAction->isVisible(), theApp()->theme().isManualToggleSupported());
    QCOMPARE(themeMenu->menuAction()->isVisible(), theApp()->theme().isManualToggleSupported());

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QVERIFY(!theApp()->theme().isManualToggleSupported());
    QVERIFY(!theApp()->theme().isDark());

    theApp()->theme().toggle();

    QVERIFY(!theApp()->theme().isDark());
#endif
}

///
/// \brief The toolbar button steps Light -> Dark -> System -> Light, keeping the
///        submenu selection and the persisted preference in sync.
///
void TestMainWindowTheme::themeButtonCyclesThroughModes()
{
    if (!theApp()->theme().isManualToggleSupported())
        QSKIP("Manual theme toggle is not supported on this platform.");

    theApp()->theme().setColorSchemePreference(AppSettings::ThemeMode::Light);

    MainWindow window;
    auto *themeAction = window.findChild<QAction *>(QStringLiteral("actionTheme"));
    auto *lightAction = window.findChild<QAction *>(QStringLiteral("actionThemeLight"));
    auto *darkAction = window.findChild<QAction *>(QStringLiteral("actionThemeDark"));
    auto *systemAction = window.findChild<QAction *>(QStringLiteral("actionThemeSystem"));
    QVERIFY(themeAction);
    QVERIFY(lightAction);
    QVERIFY(darkAction);
    QVERIFY(systemAction);

    QCOMPARE(AppSettings().themeMode(), AppSettings::ThemeMode::Light);
    QVERIFY(lightAction->isChecked());

    themeAction->trigger();
    QCOMPARE(AppSettings().themeMode(), AppSettings::ThemeMode::Dark);
    QVERIFY(darkAction->isChecked());

    themeAction->trigger();
    QCOMPARE(AppSettings().themeMode(), AppSettings::ThemeMode::System);
    QVERIFY(systemAction->isChecked());

    themeAction->trigger();
    QCOMPARE(AppSettings().themeMode(), AppSettings::ThemeMode::Light);
    QVERIFY(lightAction->isChecked());
}

///
/// \brief A manual Light/Dark preference applies the scheme, emits the change and persists.
///
void TestMainWindowTheme::colorSchemePreferenceAppliesAndPersists()
{
    AppTheme &theme = theApp()->theme();
    if (!theme.isManualToggleSupported())
        QSKIP("Manual theme toggle is not supported on this platform.");

    QSignalSpy spy(&theme, &AppTheme::colorSchemeChanged);
    QVERIFY(spy.isValid());

    theme.setColorSchemePreference(AppSettings::ThemeMode::Dark);
    QVERIFY(theme.isDark());
    QCOMPARE(AppSettings().themeMode(), AppSettings::ThemeMode::Dark);

    theme.setColorSchemePreference(AppSettings::ThemeMode::Light);
    QVERIFY(!theme.isDark());
    QCOMPARE(AppSettings().themeMode(), AppSettings::ThemeMode::Light);

    QVERIFY(spy.size() >= 2);
}

///
/// \brief Switching from a manual scheme to System drops the override and adopts the
///        platform scheme rather than keeping the previous manual choice.
///
void TestMainWindowTheme::systemPreferenceFollowsTheCurrentScheme()
{
    AppTheme &theme = theApp()->theme();
    if (!theme.isManualToggleSupported())
        QSKIP("Manual theme toggle is not supported on this platform.");

    theme.setColorSchemePreference(AppSettings::ThemeMode::Dark);
    QVERIFY(theme.isDark());

    theme.setColorSchemePreference(AppSettings::ThemeMode::System);
    QCOMPARE(AppSettings().themeMode(), AppSettings::ThemeMode::System);

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    const bool platformDark =
        QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
    QCOMPARE(theme.isDark(), platformDark);
#endif
}

///
/// \brief Verifies the selected-node details panel lives in a dock controlled by View.
///
void TestMainWindowTheme::nodeDetailsDockFollowsViewAction()
{
    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    auto *dock = window.findChild<QDockWidget *>(QStringLiteral("nodeDetailsDock"));
    auto *action = window.findChild<QAction *>(QStringLiteral("actionViewNodeDetails"));
    QVERIFY(dock);
    QVERIFY(action);
    QVERIFY(dock->widget());
    QVERIFY(dock->widget()->findChild<QTableView *>(QStringLiteral("nodeInfoTable")));
    QVERIFY(dock->widget()->findChild<QTableView *>(QStringLiteral("referencesTable")));

    QVERIFY(action->isChecked());
    dock->hide();
    QVERIFY(!action->isChecked());
    action->setChecked(true);
    QVERIFY(!dock->isHidden());
    action->setChecked(false);
    QVERIFY(dock->isHidden());
}

///
/// \brief Runs the suite under a real Application so theApp() and the theme are available.
/// \param argc Argument count.
/// \param argv Argument vector.
/// \return Test exit code.
///
int main(int argc, char *argv[])
{
    Application app(argc, argv);
    TestMainWindowTheme test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_mainwindow_theme.moc"
