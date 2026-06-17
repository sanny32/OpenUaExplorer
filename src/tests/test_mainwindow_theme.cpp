// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_mainwindow_theme.cpp
/// \brief Tests that the main window's theme controls track manual-toggle support.
///

#include <QAction>
#include <QMenu>
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
