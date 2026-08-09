// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file viewtooltip.h
/// \brief Declares the shared elided-cell tooltip used by the item views.
///

#pragma once

class QAbstractItemView;
class QEvent;

///
/// \brief Tooltip behaviour shared by the table and tree views.
///
namespace ViewToolTip {

///
/// \brief Shows a cell's full text as a tooltip, but only when the cell elides it.
/// \param view View the event was sent to.
/// \param event Viewport event to handle.
/// \return True when the event was consumed and needs no further handling.
///
bool handleViewportEvent(QAbstractItemView *view, QEvent *event);

} // namespace ViewToolTip
