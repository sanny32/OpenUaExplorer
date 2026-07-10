// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file historycombobox.h
/// \brief Declares an editable combo box whose popup entries can be removed.
///

#pragma once

#include <QComboBox>

class QAbstractItemView;
class HistoryComboBoxDelegate;

///
/// \brief Editable combo box that offers a remove button on each popup entry.
///
/// Each row in the popup paints a small cross on its right edge while hovered or
/// current; clicking it drops the entry without activating it or closing the popup,
/// and pressing Delete on the highlighted row does the same. The widget only edits
/// its own items and reports each removal through itemRemoved, leaving persistence
/// to the owner.
///
class HistoryComboBox : public QComboBox
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the combo box and attaches the remove-button delegate.
    /// \param parent Parent widget.
    ///
    explicit HistoryComboBox(QWidget *parent = nullptr);

    ///
    /// \brief Widens the popup to fit the longest entry, then shows it.
    ///
    void showPopup() override;

    ///
    /// \brief Returns the remove button's rectangle inside a popup entry's rectangle.
    /// \param itemRect Entry rectangle in popup viewport coordinates.
    /// \return Rectangle the entry's remove button occupies.
    ///
    static QRect removeButtonRect(const QRect &itemRect);

signals:
    ///
    /// \brief Emitted after the user removes an entry from the popup.
    /// \param text Text of the removed entry.
    ///
    void itemRemoved(const QString &text);

protected:
    ///
    /// \brief Consumes clicks on a remove button and tracks which button is hovered.
    /// \param watched Object the event was sent to.
    /// \param event Event being delivered.
    /// \return True when the event was handled and must not reach the popup.
    ///
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void attachToView();
    void updatePopupWidth();
    void removeRow(int row);
    int rowUnderRemoveButton(const QPoint &viewportPosition) const;
    void setHoveredRemoveRow(int row);

    HistoryComboBoxDelegate *_delegate = nullptr;
    QAbstractItemView *_view = nullptr;
};
