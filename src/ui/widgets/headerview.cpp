// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file headerview.cpp
/// \brief Implements the custom wrapped header view.
///

#include <QAbstractItemModel>
#include <QDataStream>
#include <QIODevice>
#include <QPainter>
#include <QShowEvent>
#include <QRegularExpression>
#include <QStyle>
#include <QStyleOptionHeader>
#include <QVector>

#include "headerview.h"

namespace {
constexpr quint32 headerLayoutMagic = 0x4F554148; // "OUAH"
constexpr quint16 headerLayoutVersion = 1;
}

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
                updateGeometry();
            });
}

///
/// \brief Re-computes the wrapped header height whenever the header labels change.
/// \param model Model to display.
///
void HeaderView::setModel(QAbstractItemModel *model)
{
    QHeaderView::setModel(model);
    if (model) {
        connect(model, &QAbstractItemModel::headerDataChanged, this, [this]() {
            enforceMinimumWidths();
            updateGeometry();
        });
    }
}

///
/// \brief Widens any section narrower than its wrapped label so no word is clipped.
///
void HeaderView::enforceMinimumWidths()
{
    const QSignalBlocker blocker(this);
    for (int section = 0; section < count(); ++section) {
        if (isSectionHidden(section))
            continue;
        const int minWidth = minimumWrappedSectionWidth(section);
        if (minWidth > 0 && sectionSize(section) < minWidth)
            resizeSection(section, minWidth);
    }
}

///
/// \brief Clamps section widths to fit their wrapped labels once the header is shown.
/// \param event Show event being handled.
///
void HeaderView::showEvent(QShowEvent *event)
{
    enforceMinimumWidths();
    QHeaderView::showEvent(event);
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
/// \brief Serialises the base header state together with the per-section alignments.
/// \return Opaque state blob suitable for restoreLayout().
///
QByteArray HeaderView::saveLayout() const
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << headerLayoutMagic << headerLayoutVersion;
    stream << QHeaderView::saveState();
    stream << static_cast<quint32>(_alignment);
    stream << static_cast<quint32>(_sectionAlignments.size());
    for (auto it = _sectionAlignments.cbegin(); it != _sectionAlignments.cend(); ++it)
        stream << static_cast<qint32>(it.key()) << static_cast<quint32>(it.value());
    return data;
}

///
/// \brief Restores the base header state and per-section alignments from a blob.
/// \param state Blob produced by saveLayout().
///
void HeaderView::restoreLayout(const QByteArray &state)
{
    if (state.isEmpty())
        return;

    QDataStream stream(state);
    quint32 magic = 0;
    quint16 version = 0;
    stream >> magic >> version;
    if (magic != headerLayoutMagic || version != headerLayoutVersion)
        return;

    QByteArray baseState;
    stream >> baseState;

    QVector<QHeaderView::ResizeMode> modes;
    modes.reserve(count());
    for (int i = 0; i < count(); ++i)
        modes.append(sectionResizeMode(i));

    QHeaderView::restoreState(baseState);

    for (int i = 0; i < count() && i < modes.size(); ++i)
        setSectionResizeMode(i, modes.at(i));

    quint32 defaultAlignment = 0;
    stream >> defaultAlignment;
    _alignment = Qt::Alignment(defaultAlignment);

    quint32 count = 0;
    stream >> count;
    for (quint32 i = 0; i < count; ++i) {
        qint32 section = 0;
        quint32 alignment = 0;
        stream >> section >> alignment;
        setSectionAlignment(section, Qt::Alignment(alignment));
    }
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

    QFont boldFont = font();
    boldFont.setBold(true);
    const QFontMetrics fm(boldFont);
    int wordWidth = 0;
    for (const QString &word : text.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts))
        wordWidth = qMax(wordWidth, fm.horizontalAdvance(word));

    const int margin = style()->pixelMetric(QStyle::PM_HeaderMargin, nullptr, this);
    return qMax(minimumSectionSize(), wordWidth + margin * 2 + 12);
}

///
/// \brief Returns a size hint tall enough for the tallest word-wrapped section label.
/// \return Header size hint.
///
QSize HeaderView::sizeHint() const
{
    QSize size = QHeaderView::sizeHint();

    const QFontMetrics fm(font());
    int textHeight = fm.lineSpacing();

    if (orientation() == Qt::Horizontal && model()) {
        QFont boldFont = font();
        boldFont.setBold(true);
        const QFontMetrics boldMetrics(boldFont);
        const int margin = style()->pixelMetric(QStyle::PM_HeaderMargin, nullptr, this);
        for (int section = 0; section < count(); ++section) {
            if (isSectionHidden(section))
                continue;
            const QString text =
                model()->headerData(section, orientation(), Qt::DisplayRole).toString();
            if (text.isEmpty())
                continue;
            const int available = sectionSize(section) - margin * 2 - 12;
            if (available <= 0)
                continue;
            const QRect bounds = boldMetrics.boundingRect(QRect(0, 0, available, 100000),
                                                          Qt::TextWordWrap, text);
            textHeight = qMax(textHeight, bounds.height());
        }
    }

    size.setHeight(qMax(size.height(), textHeight + 14));
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
