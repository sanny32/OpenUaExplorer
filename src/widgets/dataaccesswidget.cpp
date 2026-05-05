#include <QApplication>
#include <QColor>
#include <QHeaderView>
#include <QIcon>
#include <QItemSelectionModel>
#include <QMenu>
#include <QPalette>
#include <QTableView>

#include "headerview.h"
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

    auto delegate = new SubscriptionDelegate(_model->subscriptionNames(), ui->dataView);
    ui->dataView->setItemDelegateForColumn(DataAccessModel::ColSubscription, delegate);

    auto header = new HeaderView(Qt::Horizontal, ui->dataView);
    connect(header, &HeaderView::sectionAlignmentChanged, this,
            [this](int logicalIndex, Qt::Alignment alignment) {
                _model->setColumnAlignment(logicalIndex, alignment | Qt::AlignVCenter);
            });

    ui->dataView->setHorizontalHeader(header);

    header->setStretchLastSection(false);
    header->setSectionResizeMode(DataAccessModel::ColNumber,       QHeaderView::Fixed);
    header->setSectionResizeMode(DataAccessModel::ColNodeId,       QHeaderView::Stretch);
    header->setSectionResizeMode(DataAccessModel::ColDisplayName,  QHeaderView::Fixed);
    header->setSectionResizeMode(DataAccessModel::ColValue,        QHeaderView::Fixed);
    header->setSectionResizeMode(DataAccessModel::ColDataType,     QHeaderView::Fixed);
    header->setSectionResizeMode(DataAccessModel::ColTimestamp,    QHeaderView::Fixed);
    header->setSectionResizeMode(DataAccessModel::ColStatus,       QHeaderView::Fixed);
    header->setSectionResizeMode(DataAccessModel::ColSubscription, QHeaderView::Fixed);

    header->setSectionAlignment(DataAccessModel::ColNumber,     Qt::AlignCenter);
    header->setSectionAlignment(DataAccessModel::ColValue,      Qt::AlignCenter);
    header->setSectionAlignment(DataAccessModel::ColDataType,   Qt::AlignCenter);
    header->setSectionAlignment(DataAccessModel::ColTimestamp,  Qt::AlignCenter);
    header->setSectionAlignment(DataAccessModel::ColStatus,     Qt::AlignCenter);

    ui->dataView->setColumnWidth(DataAccessModel::ColNumber,        36  );
    ui->dataView->setColumnWidth(DataAccessModel::ColDisplayName,   120 );
    ui->dataView->setColumnWidth(DataAccessModel::ColValue,         70  );
    ui->dataView->setColumnWidth(DataAccessModel::ColDataType,      82  );
    ui->dataView->setColumnWidth(DataAccessModel::ColTimestamp,     150 );
    ui->dataView->setColumnWidth(DataAccessModel::ColStatus,        86  );
    ui->dataView->setColumnWidth(DataAccessModel::ColSubscription,  100 );

    connect(ui->dataView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this] {
        ui->subscribeButton->setEnabled(
            !ui->dataView->selectionModel()->selectedRows().isEmpty());
    });
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
    ui->subscribeButton->setPopupMode(QToolButton::InstantPopup);
    ui->subscribeButton->setEnabled(false);

    QMenu *menu = new QMenu(ui->subscribeButton);
    for (const QString &name : _model->subscriptionNames()) {
        menu->addAction(name, this, [this, name] {
            applySubscriptionToSelection(name);
        });
    }
    menu->addSeparator();
    menu->addAction(tr("Unsubscribe"), this, [this] {
        applySubscriptionToSelection(QString());
    });
    ui->subscribeButton->setMenu(menu);
}

///
/// \brief DataAccessWidget::applySubscriptionToSelection
/// \param subscriptionName
///
void DataAccessWidget::applySubscriptionToSelection(const QString &subscriptionName)
{
    const QModelIndexList rows = ui->dataView->selectionModel()->selectedRows();
    for (const QModelIndex &idx : rows) {
        _model->setData(_model->index(idx.row(), DataAccessModel::ColSubscription),
                        subscriptionName, Qt::EditRole);
    }
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
