// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file elidedtextdelegate.h
/// \brief Declares the delegate that offers a viewer button for truncated cells.
///

#pragma once

#include <QPersistentModelIndex>
#include <QStyledItemDelegate>

class QAbstractItemView;

///
/// \brief Paints a viewer button in cells the column cannot show in full.
///
/// Long OPC UA values such as ByteStrings or XmlElements are wider than any sensible
/// column, so the cell can only ever show a prefix. A picture is not text at all and no
/// column width would help it. The button appears in both cases and reports the click
/// through viewRequested(), leaving the choice of viewer to the widget that owns the view.
///
class ElidedTextDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the delegate and starts tracking the view's viewport.
    /// \param view View the delegate paints into.
    ///
    explicit ElidedTextDelegate(QAbstractItemView *view);

    ///
    /// \brief Paints the cell and, for truncated text, its viewer button.
    /// \param painter Painter to draw with.
    /// \param option Style options for the cell.
    /// \param index Model index being painted.
    ///
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

signals:
    ///
    /// \brief Emitted when the viewer button of a cell is clicked.
    /// \param index Cell whose button was clicked.
    ///
    void viewRequested(const QModelIndex &index);

protected:
    ///
    /// \brief Routes clicks that land on the viewer button to viewRequested().
    /// \param event Event delivered to the cell.
    /// \param model Model behind the view.
    /// \param option Style options for the cell.
    /// \param index Model index the event belongs to.
    /// \return True when the event was consumed by the button.
    ///
    bool editorEvent(QEvent *event, QAbstractItemModel *model,
                     const QStyleOptionViewItem &option, const QModelIndex &index) override;

    ///
    /// \brief Tracks the pointer over the viewport to give the button a hover state.
    /// \param watched Object the event was sent to.
    /// \param event Event being delivered.
    /// \return False, so the view keeps handling the event.
    ///
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    ///
    /// \brief Returns the button area inside a cell.
    /// \param cell Rectangle of the whole cell.
    /// \return Square button rectangle at the trailing edge of the cell.
    ///
    QRect buttonRect(const QRect &cell) const;

    ///
    /// \brief Returns the width an overlay scroll bar covers at the trailing edge.
    /// \return Width to keep free, in pixels, or zero when nothing overlaps the cells.
    ///
    /// A transient scroll bar floats above the viewport instead of taking a strip of
    /// its own, so a button drawn flush with the cell edge would sit underneath it.
    ///
    int scrollBarOverlap() const;

    ///
    /// \brief Reports whether a cell's text is too wide to be shown in full.
    /// \param option Style options with the text already filled in.
    /// \return True when the text does not fit the cell.
    ///
    bool isTruncated(const QStyleOptionViewItem &option) const;

    ///
    /// \brief Reports whether a cell carries a value a viewer can show better than the cell.
    /// \param index Model index being inspected.
    /// \return True when the cell holds an encoded picture.
    ///
    bool hasViewer(const QModelIndex &index) const;

    ///
    /// \brief Reports whether a cell should carry the viewer button.
    /// \param option Style options with the text already filled in.
    /// \param index Model index being inspected.
    /// \return True when the cell needs a viewer button.
    ///
    bool needsButton(const QStyleOptionViewItem &option, const QModelIndex &index) const;

    ///
    /// \brief Builds a fully initialized style option for a cell.
    /// \param option Style options handed in by the view.
    /// \param index Model index being inspected.
    /// \return Style option with text, palette, and state applied.
    ///
    QStyleOptionViewItem cellOption(const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const;

    ///
    /// \brief Remembers which button the pointer is over and repaints on a change.
    /// \param index Cell whose button is hovered, or an invalid index for none.
    ///
    void setHoveredIndex(const QModelIndex &index);

    QAbstractItemView     *_view = nullptr;
    QPersistentModelIndex  _hovered;
    QPersistentModelIndex  _pressed;
};
