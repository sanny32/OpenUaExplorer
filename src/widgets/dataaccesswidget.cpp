#include <QApplication>
#include <QColor>
#include <QHeaderView>
#include <QIcon>
#include <QPalette>
#include <QTableView>

#include "dataaccessmodel.h"
#include "dataaccesswidget.h"
#include "subscriptiondelegate.h"
#include "ui_dataaccesswidget.h"

///
/// \brief DataAccessWidget::DataAccessWidget
/// \param parent
///
DataAccessWidget::DataAccessWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DataAccessWidget)
    , _model(new DataAccessModel(this))
{
    ui->setupUi(this);
    configureToolbar();
    setupDataView();
    ui->dataView->setMinimumHeight(190);
}

///
/// \brief DataAccessWidget::~DataAccessWidget
///
DataAccessWidget::~DataAccessWidget()
{
    delete ui;
}

///
/// \brief DataAccessWidget::setupDataView
///
void DataAccessWidget::setupDataView()
{
    ui->dataView->setModel(_model);
    ui->dataView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->dataView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);
    ui->dataView->verticalHeader()->hide();

    auto *delegate = new SubscriptionDelegate(_model->subscriptionNames(), ui->dataView);
    ui->dataView->setItemDelegateForColumn(DataAccessModel::ColSubscription, delegate);

    QHeaderView *header = ui->dataView->horizontalHeader();
    header->setStretchLastSection(false);
    header->setSectionResizeMode(0, QHeaderView::Fixed);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::Fixed);
    header->setSectionResizeMode(3, QHeaderView::Fixed);
    header->setSectionResizeMode(4, QHeaderView::Fixed);
    header->setSectionResizeMode(5, QHeaderView::Fixed);
    header->setSectionResizeMode(6, QHeaderView::Fixed);
    header->setSectionResizeMode(7, QHeaderView::Fixed);

    ui->dataView->setColumnWidth(0, 36);
    ui->dataView->setColumnWidth(2, 120);
    ui->dataView->setColumnWidth(3, 70);
    ui->dataView->setColumnWidth(4, 82);
    ui->dataView->setColumnWidth(5, 150);
    ui->dataView->setColumnWidth(6, 86);
    ui->dataView->setColumnWidth(7, 100);
}

///
/// \brief DataAccessWidget::configureToolbar
///
void DataAccessWidget::configureToolbar()
{
    ui->addNodeButton->setIcon(themedIcon("add"));
    ui->removeButton->setIcon(themedIcon("remove"));
    ui->readButton->setIcon(themedIcon("read"));
    ui->writeButton->setIcon(themedIcon("write"));
    ui->subscribeButton->setIcon(themedIcon("subscribe"));
}

///
/// \brief DataAccessWidget::themedIcon
/// \param name
/// \return
///
QIcon DataAccessWidget::themedIcon(const QString &name) const
{
    const QColor windowColor = qApp->palette().color(QPalette::Window);
    const QString themeName = windowColor.lightness() < 128 ? "dark" : "light";
    return QIcon(QString(":/icons/%1/%2.svg").arg(themeName, name));
}
