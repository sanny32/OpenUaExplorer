#include <QBrush>
#include <QColor>
#include <QHeaderView>
#include <QStringList>
#include <QTableWidgetItem>
#include <QVector>

#include "logwidget.h"
#include "ui_logwidget.h"

LogWidget::LogWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LogWidget)
{
    ui->setupUi(this);
    populateLog();
}

LogWidget::~LogWidget()
{
    delete ui;
}

void LogWidget::populateLog()
{
    const QVector<QStringList> rows = {
        {"12:14:58.123", "INFO", "Client", "Connected to opc.tcp://localhost:4840"},
        {"12:14:58.456", "INFO", "Client", "Browse completed in 120 ms"},
        {"12:15:01.789", "INFO", "Client", "Subscription created"},
        {"12:15:02.001", "INFO", "Client", "Monitored items: 6"},
        {"12:15:10.234", "INFO", "Client", "Write succeeded: ns=2;s=Device1.Commands.Start = true"},
        {"12:15:10.235", "INFO", "Client", "Write succeeded: ns=2;s=Device1.Commands.Start = false"},
        {"12:15:23.250", "INFO", "Client", "Data change: Temperature = 23.45"}
    };

    ui->logTable->setRowCount(rows.size());
    ui->logTable->setColumnCount(4);
    ui->logTable->setHorizontalHeaderLabels({
        "Time",
        "Level",
        "Source",
        "Message"
    });
    ui->logTable->horizontalHeader()->setStretchLastSection(true);
    ui->logTable->verticalHeader()->hide();

    for (int row = 0; row < rows.size(); ++row) {
        for (int column = 0; column < rows.at(row).size(); ++column) {
            QTableWidgetItem *item = new QTableWidgetItem(rows.at(row).at(column));
            if (column == 1) {
                item->setForeground(QBrush(QColor(0, 150, 64)));
            }
            ui->logTable->setItem(row, column, item);
        }
    }
}
