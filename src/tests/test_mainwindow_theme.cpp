// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_mainwindow_theme.cpp
/// \brief Tests the main window's theme controls and AppTheme scheme switching.
///

#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QDockWidget>
#include <QGuiApplication>
#include <QMenu>
#include <QSettings>
#include <QSignalSpy>
#include <QStyleHints>
#include <QTableView>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QtGlobal>

#include "appsettings.h"
#include "application.h"
#include "mainwindow.h"
#include "opcua/opcuatypes.h"
#include "settingsstore.h"

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
    void opcUaInfoActionOpensDialog();
    void historyActionsFollowQtSupport();

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
    SettingsStore settings;
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

    const bool platformDark =
        QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
    QCOMPARE(theme.isDark(), platformDark);
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
    QVERIFY(dock);
    QAction *action = dock->toggleViewAction();
    QVERIFY(action);
    QVERIFY(dock->widget());
    QVERIFY(dock->widget()->findChild<QTableView *>(QStringLiteral("nodeInfoTable")));
    QVERIFY(dock->widget()->findChild<QTableView *>(QStringLiteral("referencesTable")));

    QVERIFY(action->isChecked());
    dock->hide();
    QVERIFY(!action->isChecked());
    action->trigger();
    QVERIFY(!dock->isHidden());
    action->trigger();
    QVERIFY(dock->isHidden());
}

///
/// \brief Help exposes the OPC UA information dialog.
///
void TestMainWindowTheme::opcUaInfoActionOpensDialog()
{
    MainWindow window;
    auto *action = window.findChild<QAction *>(QStringLiteral("actionOpcUaInfo"));
    QVERIFY(action);
    QVERIFY(action->isEnabled());

    bool dialogSeen = false;
    QTimer::singleShot(0, action, &QAction::trigger);
    QTimer::singleShot(50, [&dialogSeen]() {
        for (QWidget *widget : QApplication::topLevelWidgets()) {
            if (widget->windowTitle() != QStringLiteral("OPC UA Info"))
                continue;
            dialogSeen = true;
            if (auto *dialog = qobject_cast<QDialog *>(widget))
                dialog->accept();
        }
    });

    QTRY_VERIFY(dialogSeen);
}

///
/// \brief HistoryRead menu actions are visible only with supporting Qt OPC UA APIs.
///
void TestMainWindowTheme::historyActionsFollowQtSupport()
{
    MainWindow window;
    auto *readDataHistory = window.findChild<QAction *>(QStringLiteral("actionReadDataHistory"));
    auto *readEventsHistory = window.findChild<QAction *>(QStringLiteral("actionReadEventsHistory"));
    auto *viewDataHistory = window.findChild<QAction *>(QStringLiteral("actionViewDataHistory"));
    auto *viewEventsHistory = window.findChild<QAction *>(QStringLiteral("actionViewEventsHistory"));
    auto *dataMenu = window.findChild<QMenu *>(QStringLiteral("menuData"));
    auto *viewMenu = window.findChild<QMenu *>(QStringLiteral("menuView"));
    auto *toolbar = window.findChild<QToolBar *>(QStringLiteral("mainToolBar"));
    QVERIFY(readDataHistory);
    QVERIFY(readEventsHistory);
    QVERIFY(viewDataHistory);
    QVERIFY(viewEventsHistory);
    QVERIFY(dataMenu);
    QVERIFY(viewMenu);
    QVERIFY(toolbar);

    const bool supported = OpcUa::isHistoryReadSupported();
    QCOMPARE(readDataHistory->isVisible(), supported);
    QCOMPARE(readEventsHistory->isVisible(), supported);
    QCOMPARE(viewDataHistory->isVisible(), supported);
    QCOMPARE(viewEventsHistory->isVisible(), supported);
    QCOMPARE(dataMenu->actions().contains(readDataHistory), supported);
    QCOMPARE(dataMenu->actions().contains(readEventsHistory), supported);
    QCOMPARE(viewMenu->actions().contains(viewDataHistory), supported);
    QCOMPARE(viewMenu->actions().contains(viewEventsHistory), supported);

    bool readDataHistoryInToolbar = false;
    bool readEventsHistoryInToolbar = false;
    for (const QToolButton *button : toolbar->findChildren<QToolButton *>()) {
        readDataHistoryInToolbar = readDataHistoryInToolbar
                                || button->defaultAction() == readDataHistory;
        readEventsHistoryInToolbar = readEventsHistoryInToolbar
                                  || button->defaultAction() == readEventsHistory;
    }
    QVERIFY(!readDataHistoryInToolbar);
    QVERIFY(!readEventsHistoryInToolbar);
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
