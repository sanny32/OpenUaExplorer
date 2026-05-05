#include <QApplication>
#include <QHeaderView>
#include <QPalette>

#include "addressspacemodel.h"
#include "addressspacewidget.h"
#include "nodeinfomodel.h"
#include "referencesmodel.h"
#include "testdata.h"
#include "ui_addressspacewidget.h"

///
/// \brief AddressSpaceWidget::AddressSpaceWidget
/// \param parent
///
AddressSpaceWidget::AddressSpaceWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AddressSpaceWidget)
    , _treeModel(new AddressSpaceModel(this))
    , _nodeInfoModel(new NodeInfoModel(this))
    , _referencesModel(new ReferencesModel(this))
{
    ui->setupUi(this);

    setupTreeView();
    setupNodeInfoView();
    setupReferencesView();

    _treeModel->setIconProvider([this](AddressSpaceItem::NodeType type) {
        switch (type) {
        case AddressSpaceItem::NodeType::Folder:   return themedIcon("folder");
        case AddressSpaceItem::NodeType::Node:     return themedIcon("node");
        case AddressSpaceItem::NodeType::Variable: return themedIcon("variable");
        case AddressSpaceItem::NodeType::Method:   return themedIcon("method");
        }
        return QIcon();
    });

    _treeModel->setItems(TestData::addressSpaceItems());
    _nodeInfoModel->setItems(TestData::nodeInfoItems());
    _referencesModel->setItems(TestData::referenceItems());

    ui->addressTree->expandAll();

    const QModelIndex found = _treeModel->findFirst("Temperature");
    if (found.isValid())
        ui->addressTree->setCurrentIndex(found);

    ui->refreshButton->setIcon(themedIcon("refresh"));
    ui->refreshButton->setToolTip("Refresh");
    ui->refreshButton->setText("");
    ui->refreshButton->setMaximumWidth(34);
    ui->splitter->setSizes({455, 255});
}

///
/// \brief AddressSpaceWidget::~AddressSpaceWidget
///
AddressSpaceWidget::~AddressSpaceWidget()
{
    delete ui;
}

///
/// \brief AddressSpaceWidget::setupTreeView
///
void AddressSpaceWidget::setupTreeView()
{
    ui->addressTree->setModel(_treeModel);
    ui->addressTree->setHeaderHidden(true);
    ui->addressTree->setUniformRowHeights(true);
}

///
/// \brief AddressSpaceWidget::setupNodeInfoView
///
void AddressSpaceWidget::setupNodeInfoView()
{
    ui->nodeInfoTable->setModel(_nodeInfoModel);
    ui->nodeInfoTable->horizontalHeader()->hide();
    ui->nodeInfoTable->verticalHeader()->hide();
    ui->nodeInfoTable->horizontalHeader()->setStretchLastSection(true);
    ui->nodeInfoTable->setColumnWidth(NodeInfoModel::ColLabel, 105);
}

///
/// \brief AddressSpaceWidget::setupReferencesView
///
void AddressSpaceWidget::setupReferencesView()
{
    ui->referencesTable->setModel(_referencesModel);
    ui->referencesTable->verticalHeader()->hide();
    ui->referencesTable->horizontalHeader()->setStretchLastSection(true);
    ui->referencesTable->setColumnWidth(ReferencesModel::ColReference, 150);
}

///
/// \brief AddressSpaceWidget::themedIcon
/// \param name
/// \return
///
QIcon AddressSpaceWidget::themedIcon(const QString &name) const
{
    const QColor windowColor = qApp->palette().color(QPalette::Window);
    const QString themeName = windowColor.lightness() < 128 ? "dark" : "light";
    return QIcon(QString(":/icons/%1/%2.svg").arg(themeName, name));
}
