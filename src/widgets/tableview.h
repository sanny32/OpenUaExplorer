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
    ///
    /// \brief Constructs the table view with a wrapped header.
    /// \param parent Parent widget.
    ///
    explicit TableView(QWidget *parent = nullptr);

    ///
    /// \brief Returns the custom horizontal header.
    /// \return The header view, or nullptr when not set.
    ///
    HeaderView *headerView() const;

protected:
    bool viewportEvent(QEvent *event) override;
};
