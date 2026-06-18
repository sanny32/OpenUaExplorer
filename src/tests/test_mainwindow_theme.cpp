// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_mainwindow_theme.cpp
/// \brief Tests that the main window's theme controls track manual-toggle support.
///

#include <QAction>
#include <QDockWidget>
#include <QMenu>
#include <QTableView>
#include <QTest>
#include <QtGlobal>

#include "application.h"
#include "mainwindow.h"

///
/// \brief Verifies the theme action and menu visibility against AppTheme's capabilities.
///
class TestMainWindowTheme : public QObject
{
    Q_OBJECT

private slots:
    void themeControlsFollowManualThemeSupport();
    void nodeDetailsDockFollowsViewAction();
};

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
