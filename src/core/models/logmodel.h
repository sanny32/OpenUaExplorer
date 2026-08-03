// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file logmodel.h
/// \brief Declares the application log table model.
///

#pragma once

#include <QAbstractTableModel>
#include <QVector>

#include "columnalignmentstore.h"
#include "logitem.h"

///
/// \brief Table model for application log messages.
///
class LogModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs an empty log model.
    /// \param parent Owning QObject.
    ///
    explicit LogModel(QObject *parent = nullptr);

    ///
    /// \brief Returns the number of visible (filtered) log rows.
    /// \param parent Parent index; non-root parents have no rows.
    /// \return Visible item count, or 0 for non-root parents.
    ///
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    ///
    /// \brief Returns the fixed column count.
    /// \param parent Parent index; non-root parents have no columns.
    /// \return Column count, or 0 for non-root parents.
    ///
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    ///
    /// \brief Returns cell text, the level colour, and column alignment for a log row.
    /// \param index Cell to query.
    /// \param role Requested data role.
    /// \return Value for the role, or an invalid variant.
    ///
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    ///
    /// \brief Returns the Time/Level/Source/Message column titles.
    /// \param section Column index.
    /// \param orientation Header orientation.
    /// \param role Display role.
    /// \return Column title, or the base implementation otherwise.
    ///
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    ///
    /// \brief Re-emits the header titles after a UI language change.
    ///
    void retranslate();

    ///
    /// \brief Appends a log entry, inserting a row only when it passes the active filters.
    /// \param item Log entry to add.
    ///
    void addItem(const LogItem &item);

    ///
    /// \brief Removes all log entries.
    ///
    void clear();

    ///
    /// \brief Returns the level the log is filtered to.
    /// \return Active filter level.
    ///
    LogItem::Level filterLevel() const;

    ///
    /// \brief Filters the log to a single level.
    /// \param level Level to show.
    ///
    void setFilterLevel(LogItem::Level level);

    ///
    /// \brief Removes the level filter, showing all levels.
    ///
    void clearFilterLevel();

    ///
    /// \brief Filters log rows to those whose message contains the search text.
    /// \param text Case-insensitive substring filter; empty clears it.
    ///
    void setSearchFilter(const QString &text);

    ///
    /// \brief Filters log rows to those originating from a single source.
    /// \param source Exact source to show; empty clears the filter.
    ///
    void setSourceFilter(const QString &source);

    ///
    /// \brief Sets the text alignment for a column.
    /// \param column Column index.
    /// \param alignment Alignment to apply.
    ///
    void setColumnAlignment(int column, Qt::Alignment alignment);

    ///
    /// \brief Columns exposed by the log table.
    ///
    enum Column {
        ColTimestamp = 0,
        ColLevel     = 1,
        ColSource    = 2,
        ColMessage   = 3,
        ColCount     = 4
    };

private:
    QVector<LogItem> visibleItems() const;

    QVector<LogItem>          _items;
    LogItem::Level            _filterLevel;
    bool                      _filtered = false;
    QString                   _searchText;
    QString                   _sourceFilter;
    ColumnAlignmentStore     _columnAlignments;
};
