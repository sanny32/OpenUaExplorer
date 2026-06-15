// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QHash>

class ColumnAlignmentStore
{
public:
    Qt::Alignment alignment(int column) const
    {
        return _alignments.value(column, Qt::AlignLeft | Qt::AlignVCenter);
    }

    void setAlignment(int column, Qt::Alignment alignment)
    {
        _alignments.insert(column, alignment);
    }

private:
    QHash<int, Qt::Alignment> _alignments;
};
