#pragma once

#include <QIcon>
#include <QString>
#include <QVector>

///
/// \brief Describes one node in the sample OPC UA address space tree.
///
struct AddressSpaceItem
{
    enum class NodeType { Folder, Node, Variable, Method };

    QString  displayName;
    NodeType nodeType;
    QVector<AddressSpaceItem> children;
};
