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
