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

    ///
    /// \brief Creates a combo box listing the named values of an enumeration cell.
    /// \param parent Parent for the editor widget.
    /// \param option Style options for the cell.
    /// \param index Model index being edited.
    /// \return Combo-box editor, or the base editor for every other cell.
    ///
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

    ///
    /// \brief Selects the combo entry matching the cell's current value.
    /// \param editor Editor widget.
    /// \param index Model index being edited.
    ///
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;

    ///
    /// \brief Reports the picked value instead of storing it in the model.
    /// \param editor Editor widget.
    /// \param model Model behind the view.
    /// \param index Model index being edited.
    ///
    /// The cell keeps showing what the server last sent: the picked value becomes the
    /// row's value only once the write succeeded and the value came back.
    ///
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;

signals:
    ///
    /// \brief Emitted when the user picks a named value for an enumeration cell.
    /// \param index Cell that was edited.
    /// \param value Picked enumeration value, as an Int32.
    ///
    void enumValuePicked(const QModelIndex &index, int value) const;

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

    ///
    /// \brief Commits and closes the combo box as soon as the user picks an entry.
    ///
    void commitAndCloseEditor();

    QAbstractItemView *_view = nullptr;
    /// \brief Frame timer, driven from the const paint() and therefore mutable.
    mutable QTimer _flashTimer;
    /// \brief Set by paint() while a cell still has a visible wash left to draw.
    mutable bool _flashSeen = false;
};
