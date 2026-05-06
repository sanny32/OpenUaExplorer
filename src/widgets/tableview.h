// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file tableview.h
/// \brief Declares the custom table view with built-in wrapped header.
///

#pragma once

#include <QTableView>

class HeaderView;

///
/// \brief Table view with a built-in HeaderView horizontal header.
///
class TableView : public QTableView
{
    Q_OBJECT

public:
    explicit TableView(QWidget *parent = nullptr);

    HeaderView *headerView() const;

protected:
    bool viewportEvent(QEvent *event) override;
};
