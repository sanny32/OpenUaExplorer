// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <functional>

#include <QHeaderView>
#include <QList>
#include <Qt>

class TableView;

namespace TableViewConfig {

///
/// \brief Column header setup used by the table widgets that share HeaderView behavior.
///
struct Column {
    /// Model column index.
    int section;

    /// Initial resize mode for the section.
    QHeaderView::ResizeMode resizeMode = QHeaderView::Interactive;

    /// Initial width in pixels; negative leaves the current width untouched.
    int width = -1;

    /// Initial header/model alignment; empty leaves the HeaderView default.
    Qt::Alignment alignment = {};
};

///
/// \brief Applies a compact column spec while preserving each model's alignment storage.
///
/// HeaderView owns the interactive alignment UI; the callback lets each model persist the
/// resulting alignment without coupling this helper to concrete model classes.
///
void apply(TableView *view,
           const QList<Column> &columns,
           const std::function<void(int, Qt::Alignment)> &setColumnAlignment);

} // namespace TableViewConfig
