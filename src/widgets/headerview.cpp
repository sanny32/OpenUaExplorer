// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file headerview.cpp
/// \brief Implements the custom wrapped header view.
///

#include <QPainter>
#include <QRegularExpression>
#include <QStyle>
#include <QStyleOptionHeader>

#include "headerview.h"

///
/// \brief HeaderView::HeaderView
/// \param orientation
/// \param parent
///
HeaderView::HeaderView(Qt::Orientation orientation, QWidget *parent)
    : QHeaderView(orientation, parent)
{
    connect(this, &QHeaderView::sectionResized, this,
            [this](int logicalIndex, int, int newSize) {
                const int minWidth = minimumWrappedSectionWidth(logicalIndex);
                if (minWidth > 0 && newSize < minWidth) {
                    const QSignalBlocker blocker(this);
                    resizeSection(logicalIndex, minWidth);
                }
            });
}

///
/// \brief HeaderView::defaultAlignment
///
Qt::Alignment HeaderView::defaultAlignment() const
{
    return _alignment;
}

///
/// \brief HeaderView::setDefaultAlignment
/// \param alignment
///
void HeaderView::setDefaultAlignment(Qt::Alignment alignment)
{
    _alignment = alignment;
}

///
/// \brief HeaderView::sectionAlignment
/// \param logicalIndex
///
Qt::Alignment HeaderView::sectionAlignment(int logicalIndex) const
{
    return _sectionAlignments.contains(logicalIndex) ?
        _sectionAlignments.value(logicalIndex, _alignment) :
        _alignment;
}

///
/// \brief HeaderView::setSectionAlignment
/// \param logicalIndex
/// \param alignment
///
void HeaderView::setSectionAlignment(int logicalIndex, Qt::Alignment alignment)
{
    _sectionAlignments[logicalIndex] = alignment;
    headerDataChanged(orientation(), logicalIndex, logicalIndex);
    emit sectionAlignmentChanged(logicalIndex, alignment);
}

///
/// \brief HeaderView::minimumWrappedSectionWidth
/// \param logicalIndex
///
int HeaderView::minimumWrappedSectionWidth(int logicalIndex) const
{
    const QString text = model()
        ? model()->headerData(logicalIndex, orientation(), Qt::DisplayRole).toString()
        : QString();
    if (text.isEmpty())
        return minimumSectionSize();

    const QFontMetrics fm(font());
    int wordWidth = 0;
    for (const QString &word : text.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts))
        wordWidth = qMax(wordWidth, fm.horizontalAdvance(word));

    const int margin = style()->pixelMetric(QStyle::PM_HeaderMargin, nullptr, this);
    return qMax(minimumSectionSize(), wordWidth + margin * 2 + 12);
}

///
/// \brief HeaderView::sizeHint
///
QSize HeaderView::sizeHint() const
{
    QSize size = QHeaderView::sizeHint();
    const QFontMetrics fm(font());
    size.setHeight(qMax(size.height(), fm.lineSpacing() * 2 + 14));
    return size;
}

///
/// \brief HeaderView::paintSection
/// \param painter
/// \param rect
/// \param logicalIndex
///
void HeaderView::paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const
{
    if (!painter || !rect.isValid())
        return;

    QStyleOptionHeader option;
    initStyleOption(&option);
    option.rect = rect;
    option.section = logicalIndex;
    option.text.clear();

    painter->save();
    style()->drawControl(QStyle::CE_Header, &option, painter, this);
    painter->restore();

    const QString text = model()
        ? model()->headerData(logicalIndex, orientation(), Qt::DisplayRole).toString()
        : QString();
    if (text.isEmpty())
        return;

    const QRect textRect = style()->subElementRect(QStyle::SE_HeaderLabel, &option, this);

    QFont boldFont = font();
    boldFont.setBold(true);

    painter->save();
    painter->setFont(boldFont);
    painter->setPen(option.palette.color(QPalette::ButtonText));
    painter->drawText(textRect, sectionAlignment(logicalIndex) | Qt::TextWordWrap, text);
    painter->restore();
}
