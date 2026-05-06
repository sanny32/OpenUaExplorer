// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file logmodel.h
/// \brief Declares the application log table model.
///

#pragma once

#include <QAbstractTableModel>
#include <QHash>
#include <QVector>

#include "logitem.h"

///
/// \brief Table model for application log messages.
///
class LogModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit LogModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    void addItem(const LogItem &item);
    void clear();

    LogItem::Level filterLevel() const;
    void setFilterLevel(LogItem::Level level);
    void clearFilterLevel();

    void setColumnAlignment(int column, Qt::Alignment alignment);

    enum Column {
        ColTimestamp = 0,
        ColLevel     = 1,
        ColSource    = 2,
        ColMessage   = 3,
        ColCount     = 4
    };

private:
    QVector<LogItem> visibleItems() const;

    QVector<LogItem>      _items;
    LogItem::Level        _filterLevel;
    bool                  _filtered = false;
    QHash<int, Qt::Alignment> _columnAlignments;
};
