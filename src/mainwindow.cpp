// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file mainwindow.cpp
/// \brief Implements the main application window.
///

#include <QApplication>
#include <QAction>
#include <QDockWidget>
#include <QEvent>
#include <QGuiApplication>
#include <QList>
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#include <QStyleHints>
#endif

#include "appicons.h"
#include "dialogs/connectiondialog.h"
#include "itestdatapopulatable.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "widgets/addressspacewidget.h"
#include "widgets/attributeswidget.h"
#include "widgets/dataaccesswidget.h"
#include "widgets/logwidget.h"
#include "widgets/mainstatusbarwidget.h"

namespace {

///
/// \brief isThemeSwitchingSupported
/// \return
///
bool isThemeSwitchingSupported()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    return QGuiApplication::styleHints()->colorScheme() != Qt::ColorScheme::Unknown;
#else
    return false;
#endif
}

}

///
/// \brief MainWindow::MainWindow
/// \param parent
///
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , _mainStatusBarWidget(new MainStatusBarWidget(this))
{
    ui->setupUi(this);
    ui->actionTheme->setVisible(isThemeSwitchingSupported());

    ui->mainToolBar->setupFromDesignerActions();
    ui->statusbar->addWidget(_mainStatusBarWidget, 1);

    setupDockOptions();
    bindIcons();

    connect(ui->actionConnect, &QAction::triggered, this, &MainWindow::openConnectionDialog);
    connect(ui->actionTheme, &QAction::triggered, this, &MainWindow::toggleTheme);

    ui->centralSplitter->setSizes({360, 310});
    resizeDocks({ui->addressSpaceDock, ui->attributesDock}, {300, 390}, Qt::Horizontal);
    resizeDocks({ui->logDock}, {245}, Qt::Vertical);

    populateWithTestData();
}

///
/// \brief MainWindow::~MainWindow
///
MainWindow::~MainWindow()
{
    delete ui;
}

///
/// \brief MainWindow::changeEvent
/// \param event
///
void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);

    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange) {
        setWindowIcon(AppIcons::themed("app.ico"));
    }
}

///
/// \brief MainWindow::openConnectionDialog
///
void MainWindow::openConnectionDialog()
{
    ConnectionDialog dialog(this);
    dialog.exec();
}

///
/// \brief MainWindow::setupDockOptions
///
void MainWindow::setupDockOptions()
{
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);

}

///
/// \brief MainWindow::toggleTheme
///
void MainWindow::toggleTheme()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    auto *hints = QGuiApplication::styleHints();
    hints->setColorScheme(AppIcons::isDarkTheme() ? Qt::ColorScheme::Light : Qt::ColorScheme::Dark);
#endif
}

///
/// \brief MainWindow::populateWithTestData
///
void MainWindow::populateWithTestData()
{
    const QList<ITestDataPopulatable *> targets = {
        ui->addressSpaceWidget,
        ui->attributesWidget,
        ui->dataAccessWidget,
        ui->logWidget,
    };
    for (ITestDataPopulatable *t : targets)
        t->populateWithTestData();
}

///
/// \brief MainWindow::bindIcons
///
void MainWindow::bindIcons()
{
    setWindowIcon(AppIcons::themed("app.ico"));
    AppIcons::bindIcon(ui->actionConnect,     "connect.svg");
    AppIcons::bindIcon(ui->actionDisconnect,  "disconnect.svg");
    AppIcons::bindIcon(ui->actionBrowse,      "browse.svg");
    AppIcons::bindIcon(ui->actionRefresh,     "refresh.svg");
    AppIcons::bindIcon(ui->actionRead,        "read.svg");
    AppIcons::bindIcon(ui->actionWrite,       "write.svg");
    AppIcons::bindIcon(ui->actionSubscribe,   "subscribe.svg");
    AppIcons::bindIcon(ui->actionUnsubscribe, "unsubscribe.svg");
    AppIcons::bindIcon(ui->actionSettings,    "settings.svg");
    AppIcons::bindIcon(ui->actionTheme,       "theme.svg");
}
