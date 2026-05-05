#pragma once

#include <QIcon>
#include <QString>
#include <QVector>

struct AddressSpaceItem
{
    enum class NodeType { Folder, Node, Variable, Method };

    QString  displayName;
    NodeType nodeType;
    QVector<AddressSpaceItem> children;
};
