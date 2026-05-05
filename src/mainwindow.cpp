#include <QApplication>
#include <QAction>
#include <QDockWidget>
#include <QEvent>
#include <QList>
#include <QPalette>

#include "apptheme.h"
#include "dialogs/connectiondialog.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "widgets/mainstatusbarwidget.h"

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

    ui->mainToolBar->setupFromDesignerActions();
    ui->statusbar->addWidget(_mainStatusBarWidget, 1);
    setupDockOptions();
    applyThemeIcons();

    connect(ui->actionConnect, &QAction::triggered, this, &MainWindow::openConnectionDialog);
    connect(ui->actionTheme, &QAction::triggered, this, &MainWindow::toggleTheme);

    ui->centralSplitter->setSizes({360, 310});
    resizeDocks({ui->addressSpaceDock, ui->attributesDock}, {300, 390}, Qt::Horizontal);
    resizeDocks({ui->logDock}, {245}, Qt::Vertical);
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
        applyThemeIcons();
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

    ui->addressSpaceDock->setMinimumWidth(300);
    ui->addressSpaceDock->setMaximumWidth(300);
    ui->attributesDock->setMinimumWidth(390);
    ui->attributesDock->setMaximumWidth(390);
    ui->logDock->setMinimumHeight(245);
    ui->logDock->setMaximumHeight(245);
}

///
/// \brief MainWindow::toggleTheme
///
void MainWindow::toggleTheme()
{
    const IconTheme next = currentIconTheme() == IconTheme::Light ? IconTheme::Dark : IconTheme::Light;
    applyForcedTheme(next);
}

///
/// \brief MainWindow::applyForcedTheme
/// \param theme
///
void MainWindow::applyForcedTheme(IconTheme theme)
{
    _forcedTheme = theme;

    const QPalette palette = theme == IconTheme::Dark
        ? AppTheme::darkPalette()
        : AppTheme::lightPalette();

    qApp->setPalette(palette);
}

///
/// \brief MainWindow::applyThemeIcons
///
void MainWindow::applyThemeIcons()
{
    const QString themeName = currentIconTheme() == IconTheme::Dark
        ? "dark"
        : "light";

    setWindowIcon(QIcon(QString(":/icons/%1/app.ico").arg(themeName)));

    ui->actionConnect->setIcon(themedIcon("connect"));
    ui->actionDisconnect->setIcon(themedIcon("disconnect"));
    ui->actionBrowse->setIcon(themedIcon("browse"));
    ui->actionRefresh->setIcon(themedIcon("refresh"));
    ui->actionRead->setIcon(themedIcon("read"));
    ui->actionWrite->setIcon(themedIcon("write"));
    ui->actionSubscribe->setIcon(themedIcon("subscribe"));
    ui->actionUnsubscribe->setIcon(themedIcon("unsubscribe"));
    ui->actionSettings->setIcon(themedIcon("settings"));
    ui->actionTheme->setIcon(themedIcon("theme"));

    _mainStatusBarWidget->setConnectionIcon(themedIcon("connected"));
}

///
/// \brief MainWindow::currentIconTheme
/// \return
///
MainWindow::IconTheme MainWindow::currentIconTheme() const
{
    if (_forcedTheme.has_value()) {
        return _forcedTheme.value();
    }
    const QColor windowColor = qApp->palette().color(QPalette::Window);
    return windowColor.lightness() < 128 ? IconTheme::Dark : IconTheme::Light;
}

///
/// \brief MainWindow::themedIcon
/// \param name
/// \return
///
QIcon MainWindow::themedIcon(const QString &name) const
{
    const QString themeName = currentIconTheme() == IconTheme::Dark
        ? "dark"
        : "light";
    return QIcon(QString(":/icons/%1/%2.svg").arg(themeName, name));
}
