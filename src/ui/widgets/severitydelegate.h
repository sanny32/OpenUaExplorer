// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file severitydelegate.h
/// \brief Declares the event-severity gradient-bar item delegate.
///

#pragma once

#include <QStyledItemDelegate>

///
/// \brief Renders an OPC UA event severity (1-1000) as a colored gradient bar with its value.
///
class SeverityDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the delegate.
    /// \param parent Owning QObject.
    ///
    explicit SeverityDelegate(QObject *parent = nullptr);

    ///
    /// \brief Paints the severity value as a proportional gradient bar.
    /// \param painter Painter to draw with.
    /// \param option Style options for the cell.
    /// \param index Model index being painted.
    ///
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
};
