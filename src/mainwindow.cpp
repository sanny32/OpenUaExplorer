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

#include "appicons.h"
#include "application.h"
#include "dialogs/dialogabout.h"
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
/// \brief MainWindow::on_actionNewConnection_triggered
///
void MainWindow::on_actionNewConnection_triggered()
{
    openConnectionDialog();
}

///
/// \brief MainWindow::on_actionConnect_triggered
///
void MainWindow::on_actionConnect_triggered()
{
    openConnectionDialog();
}

///
/// \brief MainWindow::on_actionExit_triggered
///
void MainWindow::on_actionExit_triggered()
{
    close();
}

///
/// \brief MainWindow::on_actionTheme_triggered
///
void MainWindow::on_actionTheme_triggered()
{
    toggleTheme();
}

///
/// \brief MainWindow::on_actionAbout_triggered
///
void MainWindow::on_actionAbout_triggered()
{
    DialogAbout dialog(this);
    dialog.exec();
}

///
/// \brief MainWindow::on_actionViewAddressSpace_toggled
///
void MainWindow::on_actionViewAddressSpace_toggled(bool checked)
{
    ui->addressSpaceDock->setVisible(checked);
}

///
/// \brief MainWindow::on_addressSpaceDock_visibilityChanged
///
void MainWindow::on_addressSpaceDock_visibilityChanged(bool visible)
{
    if (!ui) return;
    ui->actionViewAddressSpace->setChecked(visible);
}

///
/// \brief MainWindow::on_actionViewActivity_toggled
///
void MainWindow::on_actionViewActivity_toggled(bool checked)
{
    ui->logDock->setVisible(checked);
}

///
/// \brief MainWindow::on_logDock_visibilityChanged
///
void MainWindow::on_logDock_visibilityChanged(bool visible)
{
    ui->actionViewActivity->setChecked(visible);
}

///
/// \brief MainWindow::on_actionViewDataAccess_triggered
///
void MainWindow::on_actionViewDataAccess_triggered()
{
    ui->dataAccessWidget->setCurrentPage(DataAccessWidget::DataAccessPage);
}

///
/// \brief MainWindow::on_actionViewSubscriptions_triggered
///
void MainWindow::on_actionViewSubscriptions_triggered()
{
    ui->dataAccessWidget->setCurrentPage(DataAccessWidget::SubscriptionsPage);
}

///
/// \brief MainWindow::on_actionViewEvents_triggered
///
void MainWindow::on_actionViewEvents_triggered()
{
    ui->dataAccessWidget->setCurrentPage(DataAccessWidget::EventsPage);
}

///
/// \brief MainWindow::on_actionViewHistory_triggered
///
void MainWindow::on_actionViewHistory_triggered()
{
    ui->dataAccessWidget->setCurrentPage(DataAccessWidget::HistoryPage);
}

///
/// \brief MainWindow::on_actionResetLayout_triggered
///
void MainWindow::on_actionResetLayout_triggered()
{
    resetLayout();
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
