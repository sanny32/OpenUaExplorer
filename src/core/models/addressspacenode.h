// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>
#include <vector>

#include "opcua/opcuatypes.h"

///
/// \brief Tree node used internally by AddressSpaceModel.
///
class AddressSpaceNode
{
public:
    ///
    /// \brief Constructs a tree node wrapping OPC UA node info.
    /// \param info OPC UA node information.
    /// \param parent Parent tree node.
    ///
    explicit AddressSpaceNode(const OpcUaNodeInfo &info, AddressSpaceNode *parent = nullptr);

    ///
    /// \brief Destroys the node and its children.
    ///
    ~AddressSpaceNode();

    ///
    /// \brief Appends a child node.
    /// \param child Child node.
    ///
    void appendChild(std::unique_ptr<AddressSpaceNode> child);

    ///
    /// \brief Removes all child nodes.
    ///
    void clearChildren();

    ///
    /// \brief Returns the child at a row.
    /// \param row Child row.
    /// \return Child node.
    ///
    AddressSpaceNode *child(int row) const;

    ///
    /// \brief Returns the number of loaded children.
    /// \return Number of loaded children.
    ///
    int childCount() const;

    ///
    /// \brief Returns this node's index within its parent.
    /// \return Row in the parent.
    ///
    int row() const;

    ///
    /// \brief Returns the parent node.
    /// \return Parent node.
    ///
    AddressSpaceNode *parent() const;

    ///
    /// \brief Returns the wrapped OPC UA node information.
    /// \return OPC UA node information.
    ///
    const OpcUaNodeInfo &info() const;

    ///
    /// \brief Reports whether a browse request was issued for this node.
    /// \return True after a browse request was issued.
    ///
    bool browseStarted() const;

    ///
    /// \brief Reports whether browse results were received for this node.
    /// \return True after browse results were received.
    ///
    bool browseComplete() const;

    ///
    /// \brief Sets whether a browse request was issued.
    /// \param value New state.
    ///
    void setBrowseStarted(bool value);

    ///
    /// \brief Sets whether browse results were received.
    /// \param value New state.
    ///
    void setBrowseComplete(bool value);

private:
    OpcUaNodeInfo _info;
    AddressSpaceNode *_parent;
    std::vector<std::unique_ptr<AddressSpaceNode>> _children;
    bool _browseStarted = false;
    bool _browseComplete = false;
};
