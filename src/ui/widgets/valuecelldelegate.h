// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file valuecelldelegate.h
/// \brief Declares the data-access value and status cell delegate.
///

#pragma once

#include <QTimer>

#include "elidedtextdelegate.h"

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
/// The quality colour is applied in initStyleOption() so that the base class, which owns
/// the viewer button and the elided text, keeps drawing the cell; only the wash is painted
/// on top afterwards.
///
class ValueCellDelegate : public ElidedTextDelegate
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the delegate.
    /// \param view View whose viewport is repainted while a cell animates.
    ///
    explicit ValueCellDelegate(QAbstractItemView *view);

    ///
    /// \brief Paints the cell and washes it over while its value is still fresh.
    /// \param painter Painter to draw with.
    /// \param option Style options for the cell.
    /// \param index Model index being painted.
    ///
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

protected:
    ///
    /// \brief Fills the style option and recolours its text to the cell's quality.
    /// \param option Style option to fill.
    /// \param index Model index being painted.
    ///
    void initStyleOption(QStyleOptionViewItem *option,
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
