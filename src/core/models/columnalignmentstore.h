// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QHash>

///
/// \brief Stores per-column text alignment overrides.
///
class ColumnAlignmentStore
{
public:
    ///
    /// \brief Returns the stored alignment for a column, or the default left/vertical-centre alignment.
    /// \param column Column index.
    /// \return Stored or default alignment.
    ///
    Qt::Alignment alignment(int column) const
    {
        return _alignments.value(column, Qt::AlignLeft | Qt::AlignVCenter);
    }

    ///
    /// \brief Stores a text alignment for a column.
    /// \param column Column index.
    /// \param alignment Alignment to store.
    ///
    void setAlignment(int column, Qt::Alignment alignment)
    {
        _alignments.insert(column, alignment);
    }

private:
    QHash<int, Qt::Alignment> _alignments;
};
