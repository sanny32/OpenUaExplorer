// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file valuecelldelegate.cpp
/// \brief Implements the data-access value and status cell delegate.
///

#include <QAbstractItemView>
#include <QDateTime>
#include <QPainter>
#include <QStyle>

#include "appcolors.h"
#include "formatters/attributeformatter.h"
#include "models/dataaccessmodel.h"
#include "valuecelldelegate.h"

namespace {

/// \brief Longest lifetime of the change wash, in milliseconds.
constexpr qint64 FlashDurationMs = 800;
/// \brief Shortest lifetime of the change wash, in milliseconds.
constexpr qint64 MinFlashDurationMs = 150;
/// \brief Interval between wash frames, in milliseconds.
constexpr int FlashFrameMs = 40;
/// \brief Opacity of the wash at the moment of the change, out of 255.
constexpr int FlashMaxAlpha = 64;

///
/// \brief Returns how long a row's change wash may last.
/// \param index Cell to inspect.
/// \return Wash lifetime in milliseconds.
///
/// A wash outliving the publishing interval would be restamped before it faded, leaving
/// fast rows permanently tinted, so it is capped to one interval. Below the floor the
/// updates outrun the eye and merging into a steady tint is the honest rendering.
///
qint64 flashDuration(const QModelIndex &index)
{
    const double interval = index.data(DataAccessModel::ExpectedIntervalRole).toDouble();
    if (interval <= 0.0)
        return FlashDurationMs;
    return qBound(MinFlashDurationMs, static_cast<qint64>(interval), FlashDurationMs);
}

///
/// \brief Picks the text colour a cell's status quality calls for.
/// \param index Cell to inspect.
/// \param column Column being painted.
/// \return Colour to draw the text in, or an invalid colour to keep the palette's.
///
QColor stateColor(const QModelIndex &index, int column)
{
    const auto severity = static_cast<OpcUaFormat::StatusSeverity>(
        index.data(DataAccessModel::StatusSeverityRole).toInt());
    switch (severity) {
    case OpcUaFormat::StatusSeverity::Bad:       return AppColors::statusError();
    case OpcUaFormat::StatusSeverity::Uncertain: return AppColors::statusWarning();
    case OpcUaFormat::StatusSeverity::Good:
        if (column == DataAccessModel::ColStatus)
            return AppColors::statusSuccess();
        break;
    case OpcUaFormat::StatusSeverity::Unknown:
        break;
    }
    return QColor();
}

} // namespace

///
/// \brief Constructs the delegate.
/// \param view View whose viewport is repainted while a cell animates.
///
ValueCellDelegate::ValueCellDelegate(QAbstractItemView *view)
    : ElidedTextDelegate(view)
    , _view(view)
{
    _flashTimer.setInterval(FlashFrameMs);
    connect(&_flashTimer, &QTimer::timeout, this, &ValueCellDelegate::onFlashTick);
}

///
/// \brief Fills the style option and recolours its text to the cell's quality.
/// \param option Style option to fill.
/// \param index Model index being painted.
///
/// The model greys whole rows that are offline, pending, or unmonitored; that foreground
/// outranks the per-cell state colour. Which role the style reads is its own business, so
/// both carry the colour rather than the delegate guessing.
///
void ValueCellDelegate::initStyleOption(QStyleOptionViewItem *option,
                                        const QModelIndex &index) const
{
    ElidedTextDelegate::initStyleOption(option, index);

    if (index.data(Qt::ForegroundRole).isValid())
        return;

    QColor color = stateColor(index, index.column());
    if (!color.isValid())
        return;

    if (option->state.testFlag(QStyle::State_Selected)) {
        color = AppColors::mostLegible(
            option->palette.color(QPalette::Highlight), color,
            option->palette.color(QPalette::HighlightedText));
    }
    option->palette.setColor(QPalette::Text, color);
    option->palette.setColor(QPalette::HighlightedText, color);
}

///
/// \brief Paints the cell and washes it over while its value is still fresh.
/// \param painter Painter to draw with.
/// \param option Style options for the cell.
/// \param index Model index being painted.
///
void ValueCellDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                              const QModelIndex &index) const
{
    ElidedTextDelegate::paint(painter, option, index);

    if (index.column() != DataAccessModel::ColValue
        || !index.data(DataAccessModel::HighlightChangesRole).toBool()) {
        return;
    }

    const qint64 changedAt = index.data(DataAccessModel::ValueChangedAtRole).toLongLong();
    const qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - changedAt;
    const qint64 duration = flashDuration(index);
    if (changedAt <= 0 || elapsed < 0 || elapsed >= duration)
        return;

    // Washing over the finished cell keeps the style in charge of the text; at this
    // opacity the tint reads on the background without dulling the glyphs.
    const qreal remaining = 1.0 - static_cast<qreal>(elapsed) / duration;
    QColor wash = AppColors::signalChangeWash(
        option.palette, option.state.testFlag(QStyle::State_Selected));
    wash.setAlpha(qRound(FlashMaxAlpha * remaining));
    painter->fillRect(option.rect, wash);

    _flashSeen = true;
    if (!_flashTimer.isActive())
        _flashTimer.start();
}

///
/// \brief Repaints the viewport, stopping the flash timer once nothing animates.
///
void ValueCellDelegate::onFlashTick()
{
    if (!_flashSeen) {
        _flashTimer.stop();
        return;
    }
    _flashSeen = false;
    _view->viewport()->update();
}
