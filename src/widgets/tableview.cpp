// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file tableview.cpp
/// \brief Implements the custom table view with built-in wrapped header.
///

#include <QEvent>
#include <QFontMetrics>
#include <QHelpEvent>
#include <QToolTip>

#include "headerview.h"
#include "tableview.h"

///
/// \brief TableView::TableView
/// \param parent
///
TableView::TableView(QWidget *parent)
    : QTableView(parent)
{
    auto *header = new HeaderView(Qt::Horizontal, this);
    setHorizontalHeader(header);
}

///
/// \brief TableView::headerView
///
HeaderView *TableView::headerView() const
{
    return qobject_cast<HeaderView *>(horizontalHeader());
}

///
/// \brief TableView::viewportEvent
/// \param event
///
bool TableView::viewportEvent(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        auto *helpEvent = static_cast<QHelpEvent *>(event);
        const QModelIndex index = indexAt(helpEvent->pos());
        if (index.isValid()) {
            const QString text = model()->data(index, Qt::DisplayRole).toString();
            if (!text.isEmpty()) {
                const QRect cellRect = visualRect(index);
                const int margin = style()->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, this) + 1;
                const int availWidth = cellRect.width() - margin * 2;
                const QFontMetrics fm(font());
                if (fm.horizontalAdvance(text) > availWidth) {
                    QToolTip::showText(helpEvent->globalPos(), text, this, cellRect);
                    return true;
                }
            }
            QToolTip::hideText();
            return true;
        }
    }
    return QTableView::viewportEvent(event);
}
