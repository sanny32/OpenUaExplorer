#include <QHeaderView>
#include <QPair>
#include <QTableWidgetItem>
#include <QVector>

#include "attributeswidget.h"
#include "ui_attributeswidget.h"

///
/// \brief AttributesWidget::AttributesWidget
/// \param parent
///
AttributesWidget::AttributesWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AttributesWidget)
{
    ui->setupUi(this);
    populateAttributes();
}

///
/// \brief AttributesWidget::~AttributesWidget
///
AttributesWidget::~AttributesWidget()
{
    delete ui;
}

///
/// \brief AttributesWidget::populateAttributes
///
void AttributesWidget::populateAttributes()
{
    const QVector<QPair<QString, QString>> rows = {
        {"NodeId", "ns=2;s=Device1.Measurements.Temperature"},
        {"NamespaceIndex", "2"},
        {"IdentifierType", "String"},
        {"Identifier", "Device1.Measurements.Temperature"},
        {"NodeClass", "Variable"},
        {"BrowseName", "2, \"Temperature\""},
        {"DisplayName", "\"Temperature\""},
        {"Description", "\"Temperature of device 1\""},
        {"WriteMask", "0"},
        {"UserWriteMask", "0"},
        {"DataType", "Double"},
        {"ValueRank", "-1 (Scalar)"},
        {"ArrayDimensions", ""},
        {"AccessLevel", "Read | Write"},
        {"UserAccessLevel", "Read | Write"}
    };

    ui->attributesTable->setRowCount(rows.size());
    ui->attributesTable->setColumnCount(2);
    ui->attributesTable->setHorizontalHeaderLabels({"Attribute", "Value"});
    ui->attributesTable->horizontalHeader()->setStretchLastSection(true);
    ui->attributesTable->verticalHeader()->hide();
    ui->attributesTable->setColumnWidth(0, 125);

    for (int row = 0; row < rows.size(); ++row) {
        ui->attributesTable->setItem(row, 0, new QTableWidgetItem(rows.at(row).first));
        ui->attributesTable->setItem(row, 1, new QTableWidgetItem(rows.at(row).second));
    }
}
