// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file treetableview.cpp
/// \brief Implements the tree view carrying the wrapped column header of the tables.
///

#include <QEvent>

#include "headerview.h"
#include "treetableview.h"
#include "viewtooltip.h"

///
/// \brief Constructs the tree view with a wrapped header.
/// \param parent Parent widget.
///
TreeTableView::TreeTableView(QWidget *parent)
    : TreeView(parent)
{
    setHeader(new HeaderView(Qt::Horizontal, this));
}

///
/// \brief Returns the custom header.
/// \return The header view, or nullptr when not set.
///
HeaderView *TreeTableView::headerView() const
{
    return qobject_cast<HeaderView *>(header());
}

///
/// \brief Shows a tooltip only when a cell's text is elided.
/// \param event Viewport event being handled.
/// \return True when the event was consumed.
///
bool TreeTableView::viewportEvent(QEvent *event)
{
    if (ViewToolTip::handleViewportEvent(this, event))
        return true;
    return TreeView::viewportEvent(event);
}
