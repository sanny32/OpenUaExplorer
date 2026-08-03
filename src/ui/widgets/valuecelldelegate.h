// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file valuecelldelegate.h
/// \brief Declares the data-access value and status cell delegate.
///

#pragma once

#include <QStyledItemDelegate>
#include <QTimer>

class QAbstractItemView;

///
/// \brief Paints data-access values and status codes as a live quality indicator.
///
/// Two signals share the cell: a fading accent wash right after a value changed,
/// and a quality text colour driven by the status code. Selected status text adapts
/// to the system highlight when its semantic colour would be less legible. The colours
/// live here rather than in DataAccessModel because the model sits in the core library,
/// which cannot reach the theme-aware AppColors palette.
///
class ValueCellDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the delegate.
    /// \param view View whose viewport is repainted while a cell animates.
    ///
    explicit ValueCellDelegate(QAbstractItemView *view);

    ///
    /// \brief Paints the cell background wash and its state-coloured text.
    /// \param painter Painter to draw with.
    /// \param option Style options for the cell.
    /// \param index Model index being painted.
    ///
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

private:
    ///
    /// \brief Repaints the viewport, stopping the flash timer once nothing animates.
    ///
    void onFlashTick();

    QAbstractItemView *_view = nullptr;
    /// \brief Frame timer, driven from the const paint() and therefore mutable.
    mutable QTimer _flashTimer;
    /// \brief Set by paint() while a cell still has a visible wash left to draw.
    mutable bool _flashSeen = false;
};
