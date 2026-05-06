// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file trendgraphwidget.cpp
/// \brief Implements the trend graph widget.
///

#include <QPainter>
#include <QPaintEvent>
#include <QPalette>
#include <QPen>
#include <QSizePolicy>
#include <QtGlobal>
#include <QVector>

#include "trendgraphwidget.h"

///
/// \brief TrendGraphWidget::TrendGraphWidget
/// \param parent
///
TrendGraphWidget::TrendGraphWidget(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

///
/// \brief TrendGraphWidget::paintEvent
/// \param event
///
void TrendGraphWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QPalette pal = palette();
    const QColor textColor = pal.color(QPalette::WindowText);
    const QColor gridColor = pal.color(QPalette::Mid);
    const QColor axisColor = pal.color(QPalette::Dark);
    const QColor lineColor(0, 180, 70);

    painter.fillRect(rect(), pal.color(QPalette::Base));

    const QRectF plotRect = rect().adjusted(44, 18, -36, -36);
    painter.setPen(QPen(gridColor, 1));
    for (int i = 0; i <= 3; ++i) {
        const qreal y = plotRect.top() + plotRect.height() * i / 3.0;
        painter.drawLine(QPointF(plotRect.left(), y), QPointF(plotRect.right(), y));
    }

    painter.setPen(QPen(axisColor, 1));
    painter.drawLine(plotRect.bottomLeft(), plotRect.bottomRight());
    painter.drawLine(plotRect.bottomLeft(), plotRect.topLeft());

    painter.setPen(textColor);
    painter.drawText(QRectF(4, plotRect.top() - 8, 34, 18), Qt::AlignRight | Qt::AlignVCenter, "30");
    painter.drawText(QRectF(4, plotRect.center().y() - 9, 34, 18), Qt::AlignRight | Qt::AlignVCenter, "25");
    painter.drawText(QRectF(4, plotRect.bottom() - 18, 34, 18), Qt::AlignRight | Qt::AlignVCenter, "15");
    painter.drawText(QRectF(plotRect.left(), plotRect.bottom() + 8, 80, 18), Qt::AlignLeft, "12:14:30");
    painter.drawText(QRectF(plotRect.center().x() - 40, plotRect.bottom() + 8, 80, 18), Qt::AlignCenter, "12:15:15");
    painter.drawText(QRectF(plotRect.right() - 80, plotRect.bottom() + 8, 80, 18), Qt::AlignRight, "12:16:00");

    const QVector<QPointF> points = trendPoints(plotRect);
    painter.setPen(QPen(lineColor, 1.6));
    for (int i = 1; i < points.size(); ++i) {
        painter.drawLine(points.at(i - 1), points.at(i));
    }

    painter.setPen(textColor);
    painter.drawText(QRectF(plotRect.right() - 120, plotRect.top() - 2, 120, 20), Qt::AlignRight, "Temperature");
    painter.setPen(QPen(lineColor, 1.6));
    painter.drawLine(QPointF(plotRect.right() - 116, plotRect.top() + 8), QPointF(plotRect.right() - 98, plotRect.top() + 8));
}

///
/// \brief TrendGraphWidget::trendPoints
/// \param plotRect
/// \return
///
QVector<QPointF> TrendGraphWidget::trendPoints(const QRectF &plotRect) const
{
    const QVector<qreal> values = {
        24.1, 24.3, 22.8, 23.5, 22.9, 24.8, 23.2, 24.1, 22.6, 24.5,
        23.6, 25.0, 24.7, 24.9, 25.1, 21.8, 20.4, 21.2, 23.8, 24.9,
        22.7, 23.5, 25.2, 22.6, 23.1, 23.7, 24.4, 22.8, 24.2, 23.0,
        23.4, 24.7, 25.1, 24.9, 22.0, 23.2, 24.5, 21.1, 23.8, 25.0,
        24.0, 22.8, 23.1, 24.2, 23.4, 25.0
    };

    QVector<QPointF> points;
    points.reserve(values.size());
    for (int i = 0; i < values.size(); ++i) {
        const qreal x = plotRect.left() + plotRect.width() * i / qMax(1, values.size() - 1);
        const qreal normalized = (values.at(i) - 15.0) / 15.0;
        const qreal y = plotRect.bottom() - plotRect.height() * normalized;
        points.append(QPointF(x, y));
    }
    return points;
}
