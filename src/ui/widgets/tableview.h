// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file tableview.h
/// \brief Declares the custom table view with built-in wrapped header.
///

#pragma once

#include <QList>
#include <QMetaObject>
#include <QTableView>

class HeaderView;

///
/// \brief Table view with a built-in HeaderView horizontal header.
///
class TableView : public QTableView
{
    Q_OBJECT
    Q_PROPERTY(bool fullTextHorizontalScroll READ fullTextHorizontalScroll
               WRITE setFullTextHorizontalScroll)

public:
    ///
    /// \brief Constructs the table view with a wrapped header.
    /// \param parent Parent widget.
    ///
    explicit TableView(QWidget *parent = nullptr);

    ///
    /// \brief Sets the model shown by the table and reconnects full-text resizing.
    /// \param model Model to display.
    ///
    void setModel(QAbstractItemModel *model) override;

    ///
    /// \brief Enables column sizing that keeps full cell text reachable through horizontal scrolling.
    /// \param enabled Whether full-text horizontal scrolling is enabled.
    ///
    void setFullTextHorizontalScroll(bool enabled);

    ///
    /// \brief Reports whether full-text horizontal scrolling is enabled.
    /// \return True when columns auto-size to their contents behind a horizontal scrollbar.
    ///
    bool fullTextHorizontalScroll() const;

    ///
    /// \brief Returns the custom horizontal header.
    /// \return The header view, or nullptr when not set.
    ///
    HeaderView *headerView() const;

protected:
    bool viewportEvent(QEvent *event) override;

private:
    void connectFullTextModel(QAbstractItemModel *model);
    void disconnectFullTextModel();
    void applyFullTextHorizontalScroll();

    bool _fullTextHorizontalScroll = false;
    QList<QMetaObject::Connection> _fullTextModelConnections;
};
