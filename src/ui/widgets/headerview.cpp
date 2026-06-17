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
/// \brief Constructs the header and clamps sections to their wrapped minimum width.
/// \param orientation Header orientation.
/// \param parent Parent widget.
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
/// \brief Returns the alignment used for sections without an explicit override.
/// \return Default header alignment.
///
Qt::Alignment HeaderView::defaultAlignment() const
{
    return _alignment;
}

///
/// \brief Sets the alignment used for sections without an explicit override.
/// \param alignment Default header alignment.
///
void HeaderView::setDefaultAlignment(Qt::Alignment alignment)
{
    _alignment = alignment;
}

///
/// \brief Returns the alignment for a section, falling back to the default.
/// \param logicalIndex Section index.
/// \return Section alignment.
///
Qt::Alignment HeaderView::sectionAlignment(int logicalIndex) const
{
    return _sectionAlignments.contains(logicalIndex) ?
        _sectionAlignments.value(logicalIndex, _alignment) :
        _alignment;
}

///
/// \brief Overrides the alignment for a single section.
/// \param logicalIndex Section index.
/// \param alignment Alignment to apply.
///
void HeaderView::setSectionAlignment(int logicalIndex, Qt::Alignment alignment)
{
    _sectionAlignments[logicalIndex] = alignment;
    headerDataChanged(orientation(), logicalIndex, logicalIndex);
    emit sectionAlignmentChanged(logicalIndex, alignment);
}

///
/// \brief Computes the narrowest width that keeps the longest header word unbroken.
/// \param logicalIndex Section index.
/// \return Minimum section width in pixels.
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
/// \brief Returns a size hint tall enough for two wrapped header lines.
/// \return Header size hint.
///
QSize HeaderView::sizeHint() const
{
    QSize size = QHeaderView::sizeHint();
    const QFontMetrics fm(font());
    size.setHeight(qMax(size.height(), fm.lineSpacing() * 2 + 14));
    return size;
}

///
/// \brief Paints a header section with bold, word-wrapped, section-aligned text.
/// \param painter Painter to draw with.
/// \param rect Section rectangle.
/// \param logicalIndex Section index.
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
