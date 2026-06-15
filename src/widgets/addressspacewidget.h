// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file addressspacewidget.h
/// \brief Declares the OPC UA address space browser widget.
///

#pragma once

#include <QWidget>

#include "addressspaceitem.h"
#include "opcua/opcuatypes.h"

namespace Ui {
class AddressSpaceWidget;
}

class AddressSpaceModel;
class NodeInfoModel;
class ReferencesModel;

///
/// \brief Widget for browsing the OPC UA address space and selected node details.
///
class AddressSpaceWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AddressSpaceWidget(QWidget *parent = nullptr);
    ~AddressSpaceWidget() override;

    void setRootNode(const OpcUaNodeInfo &root);
    void setBrowseChildren(const QString &parentNodeId,
                           const QVector<OpcUaNodeInfo> &children,
                           const QString &error);
    void setNodeDetails(const OpcUaNodeDetails &details);
    void clear();
    OpcUaNodeInfo selectedNode() const;

signals:
    void browseRequested(QString nodeId);
    void nodeSelected(OpcUaNodeInfo node);
    void refreshRequested(QString nodeId);

private:
    void setupTreeView();
    void setupNodeInfoView();
    void setupReferencesView();

    Ui::AddressSpaceWidget *ui;
    AddressSpaceModel      *_treeModel;
    NodeInfoModel          *_nodeInfoModel;
    ReferencesModel        *_referencesModel;
    QString                 _selectedNodeId;
};
