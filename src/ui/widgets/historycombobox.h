// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file historycombobox.h
/// \brief Declares an editable combo box whose popup entries can be removed.
///

#pragma once

#include <QComboBox>
#include <QString>
#include <QStringList>

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
/// An optional action entry can be appended below a separator. It is never removable
/// and never becomes the current entry: choosing it only emits actionTriggered(), so
/// the label cannot leak into the editable line edit.
///
class HistoryComboBox : public QComboBox
{
    Q_OBJECT

public:
    /// \brief Item-data role flagging the entry that only triggers an action.
    static constexpr int ActionRole = Qt::UserRole + 1;

    ///
    /// \brief Constructs the combo box and attaches the remove-button delegate.
    /// \param parent Parent widget.
    ///
    explicit HistoryComboBox(QWidget *parent = nullptr);

    ///
    /// \brief Replaces the history entries, keeping the action entry last.
    /// \param entries History entries in display order.
    ///
    void setHistory(const QStringList &entries);

    ///
    /// \brief Appends an action entry below a separator, replacing any previous one.
    /// \param text Action entry label; an empty text removes the entry.
    ///
    void setActionEntry(const QString &text);

    ///
    /// \brief Reports whether an entry only triggers the action.
    /// \param index Entry index.
    /// \return True for the action entry.
    ///
    bool isActionEntry(int index) const;

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

    ///
    /// \brief Emitted when the user chooses the action entry.
    ///
    void actionTriggered();

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
    void appendActionEntry();
    void triggerAction();
    void skipActionEntry(int index);

    HistoryComboBoxDelegate *_delegate = nullptr;
    QAbstractItemView *_view = nullptr;
    QString _actionText;
    int _lastSelectableIndex = -1;
};
