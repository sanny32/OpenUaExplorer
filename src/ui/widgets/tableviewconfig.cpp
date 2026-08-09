// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file tableviewconfig.cpp
/// \brief Implements shared setup for TableView columns backed by HeaderView.
///

#include "tableviewconfig.h"

#include "headerview.h"
#include "tableview.h"
#include "treetableview.h"

namespace TableViewConfig {

namespace {

///
/// \brief Applies resize, width, and alignment defaults while forwarding user alignment changes.
/// \param header Header owning the sections.
/// \param context Object the alignment connection is scoped to.
/// \param columns Column spec to apply.
/// \param setColumnAlignment Callback persisting an alignment the user picked.
///
/// Widths go through the header rather than the view so tables and trees share the code.
///
void applyToHeader(HeaderView *header, QObject *context,
                   const QList<Column> &columns,
                   const std::function<void(int, Qt::Alignment)> &setColumnAlignment)
{
    QObject::connect(header, &HeaderView::sectionAlignmentChanged, context,
                     [setColumnAlignment](int logicalIndex, Qt::Alignment alignment) {
        setColumnAlignment(logicalIndex, alignment | Qt::AlignVCenter);
    });

    header->setStretchLastSection(false);
    for (const Column &column : columns) {
        header->setSectionResizeMode(column.section, column.resizeMode);
        if (column.alignment != Qt::Alignment{})
            header->setSectionAlignment(column.section, column.alignment);
        if (column.width >= 0)
            header->resizeSection(column.section, column.width);
    }
}

} // namespace

///
/// \brief Applies resize, width, and alignment defaults while forwarding user alignment changes.
///
void apply(TableView *view,
           const QList<Column> &columns,
           const std::function<void(int, Qt::Alignment)> &setColumnAlignment)
{
    applyToHeader(view->headerView(), view, columns, setColumnAlignment);
}

///
/// \brief Applies the same column spec to a tree view carrying the shared header.
///
void apply(TreeTableView *view,
           const QList<Column> &columns,
           const std::function<void(int, Qt::Alignment)> &setColumnAlignment)
{
    applyToHeader(view->headerView(), view, columns, setColumnAlignment);
}

} // namespace TableViewConfig
