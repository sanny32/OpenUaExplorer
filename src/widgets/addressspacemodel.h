// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file addressspacemodel.h
/// \brief Declares the OPC UA address space tree model.
///

#pragma once

#include <QAbstractItemModel>
#include <QIcon>
#include <QString>
#include <QVector>

#include "addressspaceitem.h"

///
/// \brief Tree node used internally by AddressSpaceModel.
///
class AddressSpaceNode
{
public:
    explicit AddressSpaceNode(const QString &displayName,
                              AddressSpaceItem::NodeType nodeType,
                              AddressSpaceNode *parent = nullptr);
    ~AddressSpaceNode();

    void appendChild(AddressSpaceNode *child);

    AddressSpaceNode *child(int row) const;
    int childCount() const;
    int row() const;
    AddressSpaceNode *parent() const;

    QString displayName() const;
    AddressSpaceItem::NodeType nodeType() const;

private:
    QString                    _displayName;
    AddressSpaceItem::NodeType _nodeType;
    AddressSpaceNode          *_parent;
    QVector<AddressSpaceNode *> _children;
};

///
/// \brief Tree model for the OPC UA address space browser.
///
class AddressSpaceModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit AddressSpaceModel(QObject *parent = nullptr);
    ~AddressSpaceModel() override;

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void setItems(const QVector<AddressSpaceItem> &items);
    void clear();

    QModelIndex findFirst(const QString &displayName) const;

    void setIconProvider(std::function<QIcon(AddressSpaceItem::NodeType)> provider);

private:
    void buildNode(AddressSpaceNode *parent, const QVector<AddressSpaceItem> &items);
    QModelIndex findFirstRecursive(AddressSpaceNode *node, const QString &displayName) const;

    AddressSpaceNode *_root;
    std::function<QIcon(AddressSpaceItem::NodeType)> _iconProvider;
};
