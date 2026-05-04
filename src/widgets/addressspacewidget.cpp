#include "addressspacewidget.h"
#include "ui_addressspacewidget.h"

#include <QApplication>
#include <QHeaderView>
#include <QIcon>
#include <QPalette>
#include <QPair>
#include <QTableWidgetItem>
#include <QTreeWidgetItem>
#include <QVector>

AddressSpaceWidget::AddressSpaceWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AddressSpaceWidget)
{
    ui->setupUi(this);
    populateAddressTree();
    populateNodeInfo();

    ui->refreshButton->setIcon(themedIcon("refresh"));
    ui->refreshButton->setToolTip("Refresh");
    ui->refreshButton->setText("");
    ui->refreshButton->setMaximumWidth(34);
    ui->splitter->setSizes({455, 255});
}

AddressSpaceWidget::~AddressSpaceWidget()
{
    delete ui;
}

void AddressSpaceWidget::populateAddressTree()
{
    ui->addressTree->setHeaderHidden(true);
    ui->addressTree->setUniformRowHeights(true);

    QTreeWidgetItem *root = addItem(nullptr, "Root", "folder");
    QTreeWidgetItem *objects = addItem(root, "Objects", "folder");
    QTreeWidgetItem *deviceSet = addItem(objects, "DeviceSet", "node");
    QTreeWidgetItem *device1 = addItem(deviceSet, "Device1", "node");
    QTreeWidgetItem *status = addItem(device1, "Status", "folder");
    addItem(status, "Running", "variable");
    addItem(status, "ErrorCode", "variable");

    QTreeWidgetItem *measurements = addItem(device1, "Measurements", "folder");
    addItem(measurements, "Temperature", "variable");
    addItem(measurements, "Pressure", "variable");
    addItem(measurements, "Humidity", "variable");
    addItem(measurements, "FlowRate", "variable");

    QTreeWidgetItem *commands = addItem(device1, "Commands", "folder");
    addItem(commands, "Start", "method");
    addItem(commands, "Stop", "method");
    addItem(commands, "Reset", "method");

    addItem(deviceSet, "Device2", "node");
    addItem(deviceSet, "Device3", "node");
    addItem(root, "Types", "folder");
    addItem(root, "Views", "folder");

    ui->addressTree->expandAll();

    const QList<QTreeWidgetItem *> matches = ui->addressTree->findItems("Temperature", Qt::MatchRecursive);
    if (!matches.isEmpty()) {
        ui->addressTree->setCurrentItem(matches.first());
    }
}

void AddressSpaceWidget::populateNodeInfo()
{
    const QVector<QPair<QString, QString>> rows = {
        {"NodeId:", "ns=2;s=Device1.Measurements.Temperature"},
        {"Namespace:", "2"},
        {"Identifier Type:", "String"},
        {"Data Type:", "Double"},
        {"Value Rank:", "-1 (Scalar)"},
        {"Access Level:", "Read | Write"},
        {"User Access Level:", "Read | Write"},
        {"Description:", "Temperature of device 1"}
    };

    ui->nodeInfoTable->setRowCount(rows.size());
    ui->nodeInfoTable->setColumnCount(2);
    ui->nodeInfoTable->horizontalHeader()->hide();
    ui->nodeInfoTable->verticalHeader()->hide();
    ui->nodeInfoTable->horizontalHeader()->setStretchLastSection(true);
    ui->nodeInfoTable->setColumnWidth(0, 105);

    for (int row = 0; row < rows.size(); ++row) {
        ui->nodeInfoTable->setItem(row, 0, new QTableWidgetItem(rows.at(row).first));
        ui->nodeInfoTable->setItem(row, 1, new QTableWidgetItem(rows.at(row).second));
    }
}

QTreeWidgetItem *AddressSpaceWidget::addItem(QTreeWidgetItem *parent, const QString &text, const QString &iconName)
{
    QTreeWidgetItem *item = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(ui->addressTree);
    item->setText(0, text);
    item->setIcon(0, themedIcon(iconName));
    return item;
}

QIcon AddressSpaceWidget::themedIcon(const QString &name) const
{
    const QColor windowColor = qApp->palette().color(QPalette::Window);
    const QString themeName = windowColor.lightness() < 128 ? "dark" : "light";
    return QIcon(QString(":/icons/%1/%2.svg").arg(themeName, name));
}
