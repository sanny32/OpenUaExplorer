#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QHeaderView>
#include <QIcon>
#include <QPalette>
#include <QStringList>
#include <QTableWidgetItem>
#include <QVector>

#include "dataaccesswidget.h"
#include "ui_dataaccesswidget.h"

DataAccessWidget::DataAccessWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DataAccessWidget)
{
    ui->setupUi(this);
    configureToolbar();
    populateDataTable();
    ui->dataTable->setMinimumHeight(190);
}

DataAccessWidget::~DataAccessWidget()
{
    delete ui;
}

void DataAccessWidget::configureToolbar()
{
    ui->addNodeButton->setIcon(themedIcon("add"));
    ui->removeButton->setIcon(themedIcon("remove"));
    ui->readButton->setIcon(themedIcon("read"));
    ui->writeButton->setIcon(themedIcon("write"));
    ui->subscribeButton->setIcon(themedIcon("subscribe"));
}

void DataAccessWidget::populateDataTable()
{
    const QVector<QStringList> rows = {
        {"1", "ns=2;s=Device1.Measurements.Temperature", "Temperature", "23.45", "Double", "12:15:23.250", "Good"},
        {"2", "ns=2;s=Device1.Measurements.Pressure", "Pressure", "1.013", "Double", "12:15:23.250", "Good"},
        {"3", "ns=2;s=Device1.Measurements.Humidity", "Humidity", "45.2", "Double", "12:15:23.250", "Good"},
        {"4", "ns=2;s=Device1.Measurements.FlowRate", "FlowRate", "12.4", "Double", "12:15:23.250", "Good"},
        {"5", "ns=2;s=Device1.Status.Running", "Running", "true", "Boolean", "12:15:23.250", "Good"},
        {"6", "ns=2;s=Device1.Status.ErrorCode", "ErrorCode", "0", "UInt32", "12:15:23.250", "Good"}
    };

    ui->dataTable->setRowCount(rows.size());
    ui->dataTable->setColumnCount(7);
    ui->dataTable->setHorizontalHeaderLabels({
        "#",
        "Node Id",
        "Display Name",
        "Value",
        "Data Type",
        "Source Timestamp",
        "Status"
    });
    ui->dataTable->verticalHeader()->hide();
    ui->dataTable->horizontalHeader()->setStretchLastSection(false);
    ui->dataTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    ui->dataTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->dataTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    ui->dataTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    ui->dataTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    ui->dataTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
    ui->dataTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Fixed);
    ui->dataTable->setColumnWidth(0, 36);
    ui->dataTable->setColumnWidth(2, 120);
    ui->dataTable->setColumnWidth(3, 70);
    ui->dataTable->setColumnWidth(4, 82);
    ui->dataTable->setColumnWidth(5, 150);
    ui->dataTable->setColumnWidth(6, 86);

    for (int row = 0; row < rows.size(); ++row) {
        for (int column = 0; column < rows.at(row).size(); ++column) {
            QTableWidgetItem *item = new QTableWidgetItem(rows.at(row).at(column));
            if (column == 3 || column == 6) {
                item->setForeground(QBrush(QColor(0, 150, 64)));
            }
            ui->dataTable->setItem(row, column, item);
        }
    }
}

QIcon DataAccessWidget::themedIcon(const QString &name) const
{
    const QColor windowColor = qApp->palette().color(QPalette::Window);
    const QString themeName = windowColor.lightness() < 128 ? "dark" : "light";
    return QIcon(QString(":/icons/%1/%2.svg").arg(themeName, name));
}
