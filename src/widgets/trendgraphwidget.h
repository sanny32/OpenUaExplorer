// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file trendgraphwidget.h
/// \brief Declares the trend graph widget.
///

#pragma once

#include <QPointF>
#include <QRectF>
#include <QVector>
#include <QWidget>

///
/// \brief Renders a compact sample trend graph.
///
class TrendGraphWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TrendGraphWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<QPointF> trendPoints(const QRectF &plotRect) const;
};
