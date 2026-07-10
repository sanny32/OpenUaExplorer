// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file eventsmodel.h
/// \brief Declares the OPC UA events table model.
///

#pragma once

#include <QAbstractTableModel>
#include <QVector>

#include "columnalignmentstore.h"
#include "subscriptionitem.h"

///
/// \brief Table model for OPC UA event entries.
///
class EventsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs an empty events model.
    /// \param parent Owning QObject.
    ///
    explicit EventsModel(QObject *parent = nullptr);

    ///
    /// \brief Returns the number of event rows.
    /// \param parent Parent index; non-root parents have no rows.
    /// \return Event count, or 0 for non-root parents.
    ///
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    ///
    /// \brief Returns the fixed column count.
    /// \param parent Parent index; non-root parents have no columns.
    /// \return Column count, or 0 for non-root parents.
    ///
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    ///
    /// \brief Returns the time/message text and column alignment for an event row.
    /// \param index Cell to query.
    /// \param role Requested data role.
    /// \return Value for the role, or an invalid variant.
    ///
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    ///
    /// \brief Returns the Time/Message column titles.
    /// \param section Column index.
    /// \param orientation Header orientation.
    /// \param role Display role.
    /// \return Column title, or the base implementation otherwise.
    ///
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    ///
    /// \brief Replaces all event rows.
    /// \param items New events to display.
    ///
    void setItems(const QVector<EventItem> &items);

    ///
    /// \brief Appends streamed events, capping the table at a maximum row count.
    /// \param items Events to add in arrival order.
    ///
    void addEvents(const QVector<EventItem> &items);

    ///
    /// \brief Removes all events.
    ///
    void clear();

    ///
    /// \brief Exports the visible event table data as CSV text.
    /// \return CSV document with a header row.
    ///
    QString toCsv() const;

    ///
    /// \brief Sets the text alignment for a column.
    /// \param column Column index.
    /// \param alignment Alignment to apply.
    ///
    void setColumnAlignment(int column, Qt::Alignment alignment);

    ///
    /// \brief Columns exposed by the events table.
    ///
    enum Column {
        ColTime      = 0,
        ColSeverity  = 1,
        ColSource    = 2,
        ColMessage   = 3,
        ColEventType = 4,
        ColCount     = 5
    };

private:
    QVector<EventItem>         _items;
    ColumnAlignmentStore _columnAlignments;
};
