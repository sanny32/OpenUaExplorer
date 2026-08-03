// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file treeview.cpp
/// \brief Implements the chevron-disclosure tree view.
///

#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QPainter>
#include <QPen>
#include <QPolygonF>

#include "treeview.h"

///
/// \brief Constructs the tree view.
/// \param parent Parent widget.
///
TreeView::TreeView(QWidget *parent)
    : QTreeView(parent)
{
}

///
/// \brief Paints the disclosure chevron for expandable rows in the row's text colour.
/// \param painter Painter to draw with.
/// \param rect Branch area for the row.
/// \param index Row being decorated.
///
void TreeView::drawBranches(QPainter *painter, const QRect &rect,
                            const QModelIndex &index) const
{
    if (!model() || !model()->hasChildren(index))
        return;

    constexpr qreal viewBox = 24.0;
    constexpr int chevronSize = 16;
    const int centerX = rect.right() - indentation() / 2;
    const QRectF iconRect(centerX - chevronSize / 2.0, rect.center().y() - chevronSize / 2.0,
                          chevronSize, chevronSize);
    auto point = [&](qreal x, qreal y) {
        return QPointF(iconRect.left() + x / viewBox * iconRect.width(),
                       iconRect.top() + y / viewBox * iconRect.height());
    };
    QPolygonF chevron;
    if (isExpanded(index))
        chevron << point(6, 9) << point(12, 15) << point(18, 9);
    else
        chevron << point(9, 6) << point(15, 12) << point(9, 18);

    const bool selected = selectionModel()->isRowSelected(index.row(), index.parent());
    QPen pen(palette().color(selected ? QPalette::HighlightedText : QPalette::Text));
    pen.setWidthF(2.0 * iconRect.width() / viewBox);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(pen);
    painter->drawPolyline(chevron);
    painter->restore();
}
