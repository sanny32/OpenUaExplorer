// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file treetableview.h
/// \brief Declares the tree view carrying the wrapped column header of the tables.
///

#pragma once

#include "treeview.h"

class HeaderView;

///
/// \brief Tree view with the same wrapped, alignable header the tables use.
///
/// A table of monitored values whose rows expand is still a table: it keeps the column
/// header, the per-section alignment, and the tooltip that only shows for elided cells.
///
class TreeTableView : public TreeView
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the tree view with a wrapped header.
    /// \param parent Parent widget.
    ///
    explicit TreeTableView(QWidget *parent = nullptr);

    ///
    /// \brief Returns the custom header.
    /// \return The header view, or nullptr when not set.
    ///
    HeaderView *headerView() const;

protected:
    bool viewportEvent(QEvent *event) override;
};
