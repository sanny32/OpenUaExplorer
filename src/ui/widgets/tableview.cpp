// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file tableview.cpp
/// \brief Implements the custom table view with built-in wrapped header.
///

#include <QAbstractItemModel>
#include <QAbstractScrollArea>
#include <QEvent>
#include <QFontMetrics>
#include <QHeaderView>
#include <QHelpEvent>
#include <QStyle>
#include <QToolTip>

#include "headerview.h"
#include "tableview.h"

///
/// \brief Constructs the table view with a wrapped header.
/// \param parent Parent widget.
///
TableView::TableView(QWidget *parent)
    : QTableView(parent)
{
    auto *header = new HeaderView(Qt::Horizontal, this);
    setHorizontalHeader(header);
}

///
/// \brief Sets the model shown by the table and reconnects full-text resizing.
/// \param model Model to display.
///
void TableView::setModel(QAbstractItemModel *model)
{
    disconnectFullTextModel();
    QTableView::setModel(model);
    if (_fullTextHorizontalScroll)
        connectFullTextModel(model);
    applyFullTextHorizontalScroll();
}

///
/// \brief Enables column sizing that keeps full cell text reachable through horizontal scrolling.
/// \param enabled Whether full-text horizontal scrolling is enabled.
///
void TableView::setFullTextHorizontalScroll(bool enabled)
{
    if (_fullTextHorizontalScroll == enabled)
        return;

    _fullTextHorizontalScroll = enabled;
    disconnectFullTextModel();
    if (_fullTextHorizontalScroll)
        connectFullTextModel(model());
    applyFullTextHorizontalScroll();
}

///
/// \brief Reports whether full-text horizontal scrolling is enabled.
/// \return True when columns auto-size to their contents behind a horizontal scrollbar.
///
bool TableView::fullTextHorizontalScroll() const
{
    return _fullTextHorizontalScroll;
}

///
/// \brief Returns the custom horizontal header.
/// \return The header view, or nullptr when not set.
///
HeaderView *TableView::headerView() const
{
    return qobject_cast<HeaderView *>(horizontalHeader());
}

///
/// \brief Connects model changes that can affect full-text column widths.
/// \param model Model to observe.
///
void TableView::connectFullTextModel(QAbstractItemModel *model)
{
    if (!model)
        return;

    _fullTextModelConnections = {
        connect(model, &QAbstractItemModel::columnsInserted,
                this, &TableView::applyFullTextHorizontalScroll),
        connect(model, &QAbstractItemModel::columnsRemoved,
                this, &TableView::applyFullTextHorizontalScroll),
        connect(model, &QAbstractItemModel::dataChanged,
                this, &TableView::applyFullTextHorizontalScroll),
        connect(model, &QAbstractItemModel::headerDataChanged,
                this, &TableView::applyFullTextHorizontalScroll),
        connect(model, &QAbstractItemModel::layoutChanged,
                this, &TableView::applyFullTextHorizontalScroll),
        connect(model, &QAbstractItemModel::modelReset,
                this, &TableView::applyFullTextHorizontalScroll),
        connect(model, &QAbstractItemModel::rowsInserted,
                this, &TableView::applyFullTextHorizontalScroll),
        connect(model, &QAbstractItemModel::rowsRemoved,
                this, &TableView::applyFullTextHorizontalScroll)
    };
}

///
/// \brief Disconnects model change hooks used by full-text resizing.
///
void TableView::disconnectFullTextModel()
{
    for (const QMetaObject::Connection &connection : _fullTextModelConnections)
        disconnect(connection);
    _fullTextModelConnections.clear();
}

///
/// \brief Applies full-text column sizing when the property is enabled.
///
void TableView::applyFullTextHorizontalScroll()
{
    if (!_fullTextHorizontalScroll)
        return;

    setSizeAdjustPolicy(QAbstractScrollArea::AdjustIgnored);
    setTextElideMode(Qt::ElideNone);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QHeaderView *header = horizontalHeader();
    if (!header || !model())
        return;

    header->setStretchLastSection(false);
    header->setResizeContentsPrecision(-1);
    for (int column = 0; column < model()->columnCount(); ++column)
        header->setSectionResizeMode(column, QHeaderView::Interactive);
    resizeColumnsToContents();

    const int textMargin = style()->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, this) + 1;
    const int columnPadding = textMargin * 2 + 16;
    for (int column = 0; column < model()->columnCount(); ++column)
        header->resizeSection(column, header->sectionSize(column) + columnPadding);
}

///
/// \brief Shows a tooltip only when a cell's text is elided.
/// \param event Viewport event being handled.
/// \return True when the event was consumed.
///
bool TableView::viewportEvent(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        auto *helpEvent = static_cast<QHelpEvent *>(event);
        const QModelIndex index = indexAt(helpEvent->pos());
        if (index.isValid()) {
            const QString text = model()->data(index, Qt::DisplayRole).toString();
            if (!text.isEmpty()) {
                const QRect cellRect = visualRect(index);
                const int margin = style()->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, this) + 1;
                const int availWidth = cellRect.width() - margin * 2;
                const QFontMetrics fm(font());
                if (fm.horizontalAdvance(text) > availWidth) {
                    QToolTip::showText(helpEvent->globalPos(), text, this, cellRect);
                    return true;
                }
            }
            QToolTip::hideText();
            return true;
        }
    }
    return QTableView::viewportEvent(event);
}
