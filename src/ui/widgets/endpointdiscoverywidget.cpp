// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file endpointdiscoverywidget.cpp
/// \brief Implements the discovered-endpoint table widget.
///

#include <QApplication>
#include <QEvent>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOptionButton>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QVBoxLayout>

#include "endpointdiscoverywidget.h"
#include "models/endpointmodel.h"

namespace {

///
/// \brief Dynamic property used to share the hovered row across delegates.
///
const char *const kHoveredRowProperty = "hoveredRow";

///
/// \brief Forces a whole-row hover state so all cells highlight together.
/// \param option Style option to adjust in place.
/// \param view View carrying the shared hovered-row property.
/// \param index Cell being painted.
///
void applyRowHover(QStyleOptionViewItem &option, const QAbstractItemView *view,
                   const QModelIndex &index)
{
    const int hovered = view ? view->property(kHoveredRowProperty).toInt() : -1;
    if (index.row() == hovered)
        option.state |= QStyle::State_MouseOver;
    else
        option.state &= ~QStyle::State_MouseOver;
}

///
/// \brief Returns true when a row is selected or hovered.
///
bool isRowActive(const QStyleOptionViewItem &option, const QAbstractItemView *view,
                 const QModelIndex &index)
{
    if (option.state.testFlag(QStyle::State_Selected))
        return true;
    return view && view->property(kHoveredRowProperty).toInt() == index.row();
}

///
/// \brief Base delegate that paints a consistent whole-row background.
///
class EndpointCellDelegate : public QStyledItemDelegate
{
public:
    explicit EndpointCellDelegate(QAbstractItemView *view)
        : QStyledItemDelegate(view)
        , _view(view)
    {
    }

protected:
    void paintBackground(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const
    {
        QStyleOptionViewItem backgroundOption(option);
        initStyleOption(&backgroundOption, index);
        backgroundOption.text.clear();
        backgroundOption.icon = QIcon();
        applyRowHover(backgroundOption, _view, index);
        const QStyle *style = option.widget ? option.widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &backgroundOption, painter, option.widget);
    }

    QAbstractItemView *_view;
};

///
/// \brief Draws a radio indicator that follows the table's current row.
///
class RadioColumnDelegate final : public EndpointCellDelegate
{
public:
    using EndpointCellDelegate::EndpointCellDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        paintBackground(painter, option, index);

        const QStyle *style = option.widget ? option.widget->style() : QApplication::style();
        QStyleOptionButton button;
        button.state = QStyle::State_Enabled
            | (_view->currentIndex().row() == index.row() ? QStyle::State_On
                                                          : QStyle::State_Off);
        const int size = style->pixelMetric(QStyle::PM_ExclusiveIndicatorWidth, &button);
        button.rect = QRect(option.rect.center().x() - size / 2 + 1,
                            option.rect.center().y() - size / 2,
                            size, size);
        style->drawPrimitive(QStyle::PE_IndicatorRadioButton, &button, painter, option.widget);
    }
};

///
/// \brief Draws single-line endpoint text that follows the selection palette.
///
class EndpointTextDelegate final : public EndpointCellDelegate
{
public:
    using EndpointCellDelegate::EndpointCellDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        paintBackground(painter, option, index);

        const bool active = isRowActive(option, _view, index);
        const QColor color = option.palette.color(
            active ? QPalette::HighlightedText : QPalette::Text);
        const QRect textRect = option.rect.adjusted(kMarginH, 0, -kMarginH, 0);
        const QFontMetrics metrics(option.font);

        painter->save();
        painter->setPen(color);
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter,
                          metrics.elidedText(index.data(Qt::DisplayRole).toString(),
                                             Qt::ElideRight, textRect.width()));
        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override
    {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        const QFontMetrics metrics(option.font);
        size.setWidth(metrics.horizontalAdvance(index.data(Qt::DisplayRole).toString())
                      + kMarginH * 2 + 2);
        size.setHeight(qMax(size.height(), 34));
        return size;
    }

private:
    static constexpr int kMarginH = 6;
};

///
/// \brief Draws the endpoint security status as a coloured pill badge.
///
class StatusBadgeDelegate final : public EndpointCellDelegate
{
public:
    using EndpointCellDelegate::EndpointCellDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        paintBackground(painter, option, index);

        const QString text = index.data(EndpointModel::StatusRole).toString();
        if (text.isEmpty())
            return;
        const QColor color = index.data(EndpointModel::StatusColorRole).value<QColor>();

        QFont font = option.font;
        font.setWeight(QFont::DemiBold);
        const QFontMetrics metrics(font);
        QRect pill(0, 0, metrics.horizontalAdvance(text) + kPaddingH * 2,
                   metrics.height() + kPaddingV * 2);
        pill.moveTo(option.rect.left() + kMarginH,
                    option.rect.center().y() - pill.height() / 2);

        // Paint an opaque, lightly tinted chip so the badge stays legible over
        // the row selection / hover background instead of blending into it.
        const QColor background = blend(color, Qt::white, 0.16);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);
        painter->setBrush(background);
        painter->drawRoundedRect(pill, pill.height() / 2.0, pill.height() / 2.0);
        painter->setPen(color);
        painter->setFont(font);
        painter->drawText(pill, Qt::AlignCenter, text);
        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override
    {
        QFont font = option.font;
        font.setWeight(QFont::DemiBold);
        const QFontMetrics metrics(font);
        const QString text = index.data(EndpointModel::StatusRole).toString();
        return QSize(metrics.horizontalAdvance(text) + kPaddingH * 2 + kMarginH * 2,
                     qMax(32, metrics.height() + kPaddingV * 2 + 6));
    }

private:
    static QColor blend(const QColor &color, const QColor &other, qreal factor)
    {
        return QColor::fromRgbF(
            color.redF() * factor + other.redF() * (1.0 - factor),
            color.greenF() * factor + other.greenF() * (1.0 - factor),
            color.blueF() * factor + other.blueF() * (1.0 - factor));
    }

    static constexpr int kPaddingH = 9;
    static constexpr int kPaddingV = 3;
    static constexpr int kMarginH = 8;
};

}

///
/// \brief Builds the endpoint table with its custom row delegates and hover tracking.
/// \param parent Owning widget.
///
EndpointDiscoveryWidget::EndpointDiscoveryWidget(QWidget *parent)
    : QWidget(parent)
    , _titleLabel(new QLabel(tr("Discovered Endpoints"), this))
    , _view(new QTableView(this))
    , _model(new EndpointModel(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    layout->addWidget(_titleLabel);
    layout->addWidget(_view);

    _view->setObjectName(QStringLiteral("endpointListWidget"));
    _view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _view->setMinimumSize(300, 180);
    _view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _view->setSelectionMode(QAbstractItemView::SingleSelection);
    _view->setSelectionBehavior(QAbstractItemView::SelectRows);
    _view->setAlternatingRowColors(true);
    _view->setShowGrid(false);
    _view->setWordWrap(false);
    _view->setModel(_model);
    _view->setItemDelegateForColumn(EndpointModel::SelectColumn,
                                    new RadioColumnDelegate(_view));
    _view->setItemDelegateForColumn(EndpointModel::PolicyColumn,
                                    new EndpointTextDelegate(_view));
    _view->setItemDelegateForColumn(EndpointModel::ModeColumn,
                                    new EndpointTextDelegate(_view));
    _view->setItemDelegateForColumn(EndpointModel::StatusColumn,
                                    new StatusBadgeDelegate(_view));

    QHeaderView *header = _view->horizontalHeader();
    header->setHighlightSections(false);
    header->setSectionResizeMode(EndpointModel::SelectColumn, QHeaderView::Fixed);
    header->setSectionResizeMode(EndpointModel::PolicyColumn, QHeaderView::Stretch);
    header->setSectionResizeMode(EndpointModel::ModeColumn, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(EndpointModel::StatusColumn, QHeaderView::ResizeToContents);
    _view->setColumnWidth(EndpointModel::SelectColumn, 34);
    _view->verticalHeader()->setVisible(false);
    _view->verticalHeader()->setDefaultSectionSize(34);

    _view->setProperty(kHoveredRowProperty, -1);
    _view->setMouseTracking(true);
    _view->viewport()->setMouseTracking(true);
    _view->viewport()->installEventFilter(this);

    connect(_view->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &EndpointDiscoveryWidget::currentEndpointChanged);
}

///
/// \brief Shows the discovered endpoints, updating the count and selecting the first row.
/// \param endpoints Discovered endpoints to display.
///
void EndpointDiscoveryWidget::setEndpoints(const QList<EndpointInfo> &endpoints)
{
    _model->setEndpoints(endpoints);
    _titleLabel->setText(endpoints.isEmpty()
        ? tr("Discovered Endpoints")
        : tr("Discovered Endpoints (%1)").arg(endpoints.size()));
    if (!endpoints.isEmpty())
        _view->setCurrentIndex(_model->index(0, 0));
}

///
/// \brief Removes all endpoints.
///
void EndpointDiscoveryWidget::clear()
{
    setEndpoints({});
}

///
/// \brief Returns how many endpoints are shown.
/// \return Number of discovered endpoints.
///
int EndpointDiscoveryWidget::endpointCount() const
{
    return _model->rowCount();
}

///
/// \brief Returns the selected row index.
/// \return Selected endpoint row, or -1 when none is selected.
///
int EndpointDiscoveryWidget::currentRow() const
{
    return _view->currentIndex().row();
}

///
/// \brief Reports whether a valid endpoint is selected.
/// \return True when a valid endpoint row is selected.
///
bool EndpointDiscoveryWidget::hasSelection() const
{
    const int row = currentRow();
    return row >= 0 && row < _model->rowCount();
}

///
/// \brief Returns the selected endpoint.
/// \return Endpoint for the selected row.
///
EndpointInfo EndpointDiscoveryWidget::currentEndpoint() const
{
    return _model->endpointAt(currentRow());
}

///
/// \brief Tracks the hovered row from viewport mouse-move and leave events.
/// \param watched Watched object.
/// \param event Event being delivered.
/// \return True if the event was consumed.
///
bool EndpointDiscoveryWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == _view->viewport()) {
        if (event->type() == QEvent::MouseMove) {
            const auto *mouseEvent = static_cast<QMouseEvent *>(event);
            setHoveredRow(_view->indexAt(mouseEvent->pos()).row());
        } else if (event->type() == QEvent::Leave) {
            setHoveredRow(-1);
        }
    }
    return QWidget::eventFilter(watched, event);
}

///
/// \brief Updates the shared hovered-row property and repaints the viewport.
/// \param row Endpoint row under the cursor, or -1 for none.
///
void EndpointDiscoveryWidget::setHoveredRow(int row)
{
    if (_view->property(kHoveredRowProperty).toInt() == row)
        return;
    _view->setProperty(kHoveredRowProperty, row);
    _view->viewport()->update();
}
