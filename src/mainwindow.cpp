// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file mainwindow.cpp
/// \brief Implements the main application window.
///

#include <QAction>
#include <QDockWidget>
#include <QEvent>
#include <QList>
#include <QMessageBox>

#include "appicons.h"
#include "application.h"
#include "dialogs/connectiondialog.h"
#include "itestdatapopulatable.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "widgets/dataaccesswidget.h"


///
/// \brief MainWindow::MainWindow
/// \param parent
///
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->actionTheme->setVisible(theApp()->theme().isManualToggleSupported());

    setupMainMenu();
    ui->mainToolBar->setupFromDesignerActions();

    setupDockOptions();
    bindIcons();

    connect(ui->actionNewConnection, &QAction::triggered, this, &MainWindow::openConnectionDialog);
    connect(ui->actionConnect, &QAction::triggered, this, &MainWindow::openConnectionDialog);
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::close);
    connect(ui->actionTheme, &QAction::triggered, this, &MainWindow::toggleTheme);
    connect(ui->actionAbout, &QAction::triggered, this, [this] {
        QMessageBox::about(this, tr("About OpenUaExplorer"),
                           tr("OpenUaExplorer\n\nOpen source OPC UA client."));
    });

    resetLayout();

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
/// \brief MainWindow::setupMainMenu
///
void MainWindow::setupMainMenu()
{
    ui->actionViewAddressSpace->setChecked(!ui->addressSpaceDock->isHidden());
    ui->actionViewActivity->setChecked(!ui->logDock->isHidden());

    connect(ui->actionViewAddressSpace, &QAction::toggled,
            ui->addressSpaceDock, &QDockWidget::setVisible);
    connect(ui->addressSpaceDock, &QDockWidget::visibilityChanged,
            ui->actionViewAddressSpace, &QAction::setChecked);

    connect(ui->actionViewActivity, &QAction::toggled,
            ui->logDock, &QDockWidget::setVisible);
    connect(ui->logDock, &QDockWidget::visibilityChanged,
            ui->actionViewActivity, &QAction::setChecked);

    connect(ui->actionViewDataAccess, &QAction::triggered, this, [this] {
        ui->dataAccessWidget->setCurrentPage(DataAccessWidget::DataAccessPage);
    });
    connect(ui->actionViewSubscriptions, &QAction::triggered, this, [this] {
        ui->dataAccessWidget->setCurrentPage(DataAccessWidget::SubscriptionsPage);
    });
    connect(ui->actionViewEvents, &QAction::triggered, this, [this] {
        ui->dataAccessWidget->setCurrentPage(DataAccessWidget::EventsPage);
    });
    connect(ui->actionViewHistory, &QAction::triggered, this, [this] {
        ui->dataAccessWidget->setCurrentPage(DataAccessWidget::HistoryPage);
    });
    connect(ui->actionResetLayout, &QAction::triggered, this, &MainWindow::resetLayout);
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
/// \brief MainWindow::resetLayout
///
void MainWindow::resetLayout()
{
    ui->addressSpaceDock->show();
    ui->attributesDock->show();
    ui->logDock->show();

    addDockWidget(Qt::LeftDockWidgetArea, ui->addressSpaceDock);
    addDockWidget(Qt::RightDockWidgetArea, ui->attributesDock);
    addDockWidget(Qt::BottomDockWidgetArea, ui->logDock);

    ui->centralSplitter->setSizes({360, 310});
    resizeDocks({ui->addressSpaceDock, ui->attributesDock}, {300, 390}, Qt::Horizontal);
    resizeDocks({ui->logDock}, {245}, Qt::Vertical);
}

///
/// \brief MainWindow::toggleTheme
///
void MainWindow::toggleTheme()
{
    theApp()->theme().toggle();
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
