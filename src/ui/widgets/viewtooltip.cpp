// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file viewtooltip.cpp
/// \brief Implements the shared elided-cell tooltip used by the item views.
///

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QEvent>
#include <QFontMetrics>
#include <QHelpEvent>
#include <QStyle>
#include <QToolTip>

#include "elidedtextdelegate.h"
#include "viewtooltip.h"

namespace ViewToolTip {

///
/// \brief Shows a cell's full text as a tooltip, but only when the cell elides it.
/// \param view View the event was sent to.
/// \param event Viewport event to handle.
/// \return True when the event was consumed and needs no further handling.
///
/// The viewer button of a truncated cell is a control of its own, so the pointer resting
/// on it is asking about the button and not about the text it sits next to.
///
bool handleViewportEvent(QAbstractItemView *view, QEvent *event)
{
    if (event->type() != QEvent::ToolTip)
        return false;

    auto *helpEvent = static_cast<QHelpEvent *>(event);
    const QModelIndex index = view->indexAt(helpEvent->pos());
    if (!index.isValid())
        return false;

    auto *delegate = qobject_cast<ElidedTextDelegate *>(view->itemDelegateForIndex(index));
    if (delegate && delegate->coversButton(helpEvent->pos(), index)) {
        QToolTip::hideText();
        return true;
    }

    const QString text = view->model()->data(index, Qt::DisplayRole).toString();
    if (!text.isEmpty()) {
        const QRect cellRect = view->visualRect(index);
        const int margin = view->style()->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, view) + 1;
        const int availWidth = cellRect.width() - margin * 2;
        const QFontMetrics fm(view->font());
        if (fm.horizontalAdvance(text) > availWidth) {
            QToolTip::showText(helpEvent->globalPos(), text, view, cellRect);
            return true;
        }
    }
    QToolTip::hideText();
    return true;
}

} // namespace ViewToolTip
