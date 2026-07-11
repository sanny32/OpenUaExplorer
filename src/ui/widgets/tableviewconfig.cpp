// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file tableviewconfig.cpp
/// \brief Implements shared setup for TableView columns backed by HeaderView.
///

#include "tableviewconfig.h"

#include "headerview.h"
#include "tableview.h"

namespace TableViewConfig {

///
/// \brief Applies resize, width, and alignment defaults while forwarding user alignment changes.
///
void apply(TableView *view,
           const QList<Column> &columns,
           const std::function<void(int, Qt::Alignment)> &setColumnAlignment)
{
    auto *header = view->headerView();
    QObject::connect(header, &HeaderView::sectionAlignmentChanged, view,
                     [setColumnAlignment](int logicalIndex, Qt::Alignment alignment) {
        setColumnAlignment(logicalIndex, alignment | Qt::AlignVCenter);
    });

    header->setStretchLastSection(false);
    for (const Column &column : columns) {
        header->setSectionResizeMode(column.section, column.resizeMode);
        if (column.alignment != Qt::Alignment{})
            header->setSectionAlignment(column.section, column.alignment);
        if (column.width >= 0)
            view->setColumnWidth(column.section, column.width);
    }
}

} // namespace TableViewConfig
