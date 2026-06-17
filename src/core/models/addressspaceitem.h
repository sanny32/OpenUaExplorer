// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file addressspaceitem.h
/// \brief Declares address space item data types.
///

#pragma once

#include <QIcon>
#include <QString>
#include <QVector>

///
/// \brief Describes one node in the sample OPC UA address space tree.
///
struct AddressSpaceItem
{
    ///
    /// \brief Node category used by the sample address space tree.
    ///
    enum class NodeType { Folder, Node, Variable, Method };

    /// \brief Display name shown in the tree.
    QString  displayName;
    /// \brief Sample node category.
    NodeType nodeType;
    /// \brief Child sample nodes.
    QVector<AddressSpaceItem> children;
};
