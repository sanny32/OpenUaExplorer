#pragma once

#include <QIcon>
#include <QWidget>

#include "addressspaceitem.h"

namespace Ui {
class AddressSpaceWidget;
}

class AddressSpaceModel;
class NodeInfoModel;
class ReferencesModel;

class AddressSpaceWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AddressSpaceWidget(QWidget *parent = nullptr);
    ~AddressSpaceWidget() override;

private:
    void setupTreeView();
    void setupNodeInfoView();
    void setupReferencesView();

    QIcon themedIcon(const QString &name) const;

    Ui::AddressSpaceWidget *ui;
    AddressSpaceModel      *_treeModel;
    NodeInfoModel          *_nodeInfoModel;
    ReferencesModel        *_referencesModel;
};
