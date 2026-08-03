// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file treeview.h
/// \brief Declares a tree view whose disclosure indicator follows the row colour.
///

#pragma once

#include <QTreeView>

///
/// \brief Tree view that paints the branch disclosure as a palette-coloured chevron.
///
/// The default branch indicator keeps its colour on a highlighted row, so it fades against
/// the selection fill. This view draws the chevron in the row's text colour and switches to
/// the highlighted-text colour when the row is selected, matching the row's label.
///
class TreeView : public QTreeView
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the tree view.
    /// \param parent Parent widget.
    ///
    explicit TreeView(QWidget *parent = nullptr);

protected:
    void drawBranches(QPainter *painter, const QRect &rect,
                      const QModelIndex &index) const override;
};
