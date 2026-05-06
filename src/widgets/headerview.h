// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file headerview.h
/// \brief Declares the custom wrapped header view.
///

#pragma once

#include <QHash>
#include <QHeaderView>

///
/// \brief Header view with wrapped labels and per-section alignment.
///
class HeaderView : public QHeaderView
{
    Q_OBJECT

public:
    explicit HeaderView(Qt::Orientation orientation, QWidget *parent = nullptr);

    Qt::Alignment defaultAlignment() const;
    void setDefaultAlignment(Qt::Alignment alignment);

    Qt::Alignment sectionAlignment(int logicalIndex) const;
    void setSectionAlignment(int logicalIndex, Qt::Alignment alignment);

signals:
    void sectionAlignmentChanged(int logicalIndex, Qt::Alignment alignment);

protected:
    QSize sizeHint() const override;
    void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const override;

private:
    int minimumWrappedSectionWidth(int logicalIndex) const;

    Qt::Alignment _alignment = Qt::AlignLeft | Qt::AlignVCenter;
    QHash<int, Qt::Alignment> _sectionAlignments;
};
