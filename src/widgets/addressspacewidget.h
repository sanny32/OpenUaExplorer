// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file addressspacewidget.h
/// \brief Declares the OPC UA address space browser widget.
///

#pragma once

#include <QWidget>

#include "addressspaceitem.h"
#include "itestdatapopulatable.h"

namespace Ui {
class AddressSpaceWidget;
}

class AddressSpaceModel;
class NodeInfoModel;
class ReferencesModel;

///
/// \brief Widget for browsing the OPC UA address space and selected node details.
///
class AddressSpaceWidget : public QWidget, public ITestDataPopulatable
{
    Q_OBJECT

public:
    explicit AddressSpaceWidget(QWidget *parent = nullptr);
    ~AddressSpaceWidget() override;

    void populateWithTestData() override;

private:
    void setupTreeView();
    void setupNodeInfoView();
    void setupReferencesView();

    Ui::AddressSpaceWidget *ui;
    AddressSpaceModel      *_treeModel;
    NodeInfoModel          *_nodeInfoModel;
    ReferencesModel        *_referencesModel;
};
