#include <QHeaderView>
#include <QTableWidgetItem>

#include "attributeswidget.h"
#include "testdata.h"
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
    const auto rows = TestData::attributeItems();

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
