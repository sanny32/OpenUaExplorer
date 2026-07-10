// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file severitydelegate.cpp
/// \brief Implements the event-severity gradient-bar item delegate.
///

#include <QApplication>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>

#include "severitydelegate.h"

namespace {

/// \brief Smallest OPC UA event severity.
constexpr int minSeverity = 1;
/// \brief Largest OPC UA event severity.
constexpr int maxSeverity = 1000;
/// \brief Horizontal/vertical inset of the bar within the cell, in pixels.
constexpr int barMargin = 3;
/// \brief Corner radius of the bar, in pixels.
constexpr qreal barRadius = 3.0;
/// \brief Horizontal padding around the value text inside the bar, in pixels.
constexpr int textPadding = 5;

///
/// \brief Maps a severity fraction onto the green-amber-red gradient stops.
/// \param gradient Gradient to populate; spans the bar's full width.
///
void applySeverityStops(QLinearGradient &gradient)
{
    gradient.setColorAt(0.0, QColor(0x4c, 0xaf, 0x50));
    gradient.setColorAt(0.5, QColor(0xff, 0xc1, 0x07));
    gradient.setColorAt(1.0, QColor(0xe5, 0x39, 0x35));
}

} // namespace

///
/// \brief Constructs the delegate.
/// \param parent Owning QObject.
///
SeverityDelegate::SeverityDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

///
/// \brief Paints the severity value as a proportional gradient bar.
/// \param painter Painter to draw with.
/// \param option Style options for the cell.
/// \param index Model index being painted.
///
void SeverityDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    const QString text = opt.text;
    opt.text.clear();
    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

    bool ok = false;
    const int severity = text.toInt(&ok);
    if (!ok) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    const int clamped = qBound(minSeverity, severity, maxSeverity);
    const qreal fraction = static_cast<qreal>(clamped) / maxSeverity;

    const QRect cell = opt.rect.adjusted(barMargin, barMargin, -barMargin, -barMargin);
    if (cell.width() <= 0 || cell.height() <= 0)
        return;

    const int textWidth = opt.fontMetrics.horizontalAdvance(text) + 2 * textPadding;
    const int proportional = static_cast<int>(cell.width() * fraction);
    QRect bar = cell;
    bar.setWidth(qBound(qMin(textWidth, cell.width()), proportional, cell.width()));

    QLinearGradient gradient(cell.topLeft(), cell.topRight());
    applySeverityStops(gradient);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    QPainterPath path;
    path.addRoundedRect(bar, barRadius, barRadius);
    painter->fillPath(path, gradient);
    painter->setPen(QColor(0x21, 0x25, 0x29));
    painter->drawText(bar, Qt::AlignCenter, text);
    painter->restore();
}
