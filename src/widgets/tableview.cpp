// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file tableview.cpp
/// \brief Implements the custom table view with built-in wrapped header.
///

#include "headerview.h"
#include "tableview.h"

///
/// \brief TableView::TableView
/// \param parent
///
TableView::TableView(QWidget *parent)
    : QTableView(parent)
{
    auto *header = new HeaderView(Qt::Horizontal, this);
    setHorizontalHeader(header);
}

///
/// \brief TableView::headerView
///
HeaderView *TableView::headerView() const
{
    return qobject_cast<HeaderView *>(horizontalHeader());
}
