// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file serverdiscoverywidget.cpp
/// \brief Implements the discovered-server tree widget.
///

#include <QApplication>
#include <QEvent>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QStackedWidget>
#include <QStyleOptionButton>
#include <QStyledItemDelegate>
#include <QTreeView>
#include <QVBoxLayout>

#if defined(HAVE_QLEMENTINE_APP_STYLE)
#include <oclero/qlementine/style/QlementineStyle.hpp>
#endif

#include "appcolors.h"
#include "appicons.h"
#include "models/serverdiscoverymodel.h"
#include "serverdiscoverywidget.h"
#include "themediconlabel.h"

namespace {

/// \brief Dynamic property used to share the hovered row across delegates.
const char *const kHoveredIndexProperty = "hoveredIndex";

/// \brief Height of an endpoint or placeholder row.
constexpr int kEndpointRowHeight = 34;

/// \brief Height of the two-line server row.
constexpr int kServerRowHeight = 52;

/// \brief Horizontal margin kept inside a cell.
constexpr int kMarginH = 6;

/// \brief Side of the icon drawn on both server and endpoint rows.
constexpr int kIconSize = 20;

///
/// \brief Forces a whole-row hover state so all cells highlight together.
/// \param option Style option to adjust in place.
/// \param view View carrying the shared hovered-row property.
/// \param index Cell being painted.
///
void applyRowHover(QStyleOptionViewItem &option, const QAbstractItemView *view,
                   const QModelIndex &index)
{
    const auto hovered = view
        ? view->property(kHoveredIndexProperty).value<QPersistentModelIndex>()
        : QPersistentModelIndex();
    if (hovered.isValid() && hovered.parent() == index.parent()
        && hovered.row() == index.row()) {
        option.state |= QStyle::State_MouseOver;
    } else {
        option.state &= ~QStyle::State_MouseOver;
    }
}

///
/// \brief Base delegate that paints a consistent whole-row background.
///
class DiscoveryCellDelegate : public QStyledItemDelegate
{
public:
    explicit DiscoveryCellDelegate(QAbstractItemView *view)
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

    static QColor textColor(const QStyleOptionViewItem &option)
    {
        return option.palette.color(option.state.testFlag(QStyle::State_Selected)
                                        ? QPalette::HighlightedText
                                        : QPalette::Text);
    }

    QAbstractItemView *_view;
};

///
/// \brief Draws server rows, endpoint rows and placeholders in the primary column.
///
class PrimaryColumnDelegate final : public DiscoveryCellDelegate
{
public:
    using DiscoveryCellDelegate::DiscoveryCellDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        paintBackground(painter, option, index);

        if (index.data(ServerDiscoveryModel::IsServerRole).toBool())
            paintServer(painter, option, index);
        else if (index.data(ServerDiscoveryModel::IsPlaceholderRole).toBool())
            paintPlaceholder(painter, option, index);
        else
            paintEndpoint(painter, option, index);
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setHeight(index.data(ServerDiscoveryModel::IsServerRole).toBool()
                           ? kServerRowHeight
                           : kEndpointRowHeight);
        return size;
    }

private:
    void paintServer(QPainter *painter, const QStyleOptionViewItem &option,
                     const QModelIndex &index) const
    {
        QRect content = option.rect.adjusted(kMarginH, 0, -kMarginH, 0);
        drawIcon(painter, content, index.data(ServerDiscoveryModel::IconRole).toString());
        content.setLeft(content.left() + kIconSize + kMarginH * 2);

        QFont nameFont = option.font;
        nameFont.setWeight(QFont::DemiBold);
        QFont urlFont = option.font;
        urlFont.setItalic(true);

        const QFontMetrics nameMetrics(nameFont);
        const QFontMetrics urlMetrics(urlFont);
        const int blockHeight = nameMetrics.height() + urlMetrics.height();
        const int top = content.center().y() - blockHeight / 2;
        const QRect nameRect(content.left(), top, content.width(), nameMetrics.height());
        const QRect urlRect(content.left(), top + nameMetrics.height(), content.width(),
                            urlMetrics.height());

        painter->save();
        painter->setFont(nameFont);
        painter->setPen(textColor(option));
        painter->drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter,
                          nameMetrics.elidedText(index.data(Qt::DisplayRole).toString(),
                                                 Qt::ElideRight, nameRect.width()));
        painter->setFont(urlFont);
        painter->setPen(option.state.testFlag(QStyle::State_Selected)
                            ? textColor(option)
                            : AppColors::subtitleText());
        painter->drawText(urlRect, Qt::AlignLeft | Qt::AlignVCenter,
                          urlMetrics.elidedText(
                              index.data(ServerDiscoveryModel::SubtitleRole).toString(),
                              Qt::ElideMiddle, urlRect.width()));
        painter->restore();
    }

    void paintEndpoint(QPainter *painter, const QStyleOptionViewItem &option,
                       const QModelIndex &index) const
    {
        QRect content = option.rect.adjusted(kMarginH, 0, -kMarginH, 0);

        const QStyle *style = option.widget ? option.widget->style() : QApplication::style();
        QStyleOptionButton button;
        const QModelIndex current = _view->currentIndex();
        const bool checked = current.isValid() && current.parent() == index.parent()
            && current.row() == index.row();
        button.state = QStyle::State_Enabled | (checked ? QStyle::State_On : QStyle::State_Off);
        const int indicator = style->pixelMetric(QStyle::PM_ExclusiveIndicatorWidth, &button);
        button.rect = QRect(content.left(), content.center().y() - indicator / 2,
                            indicator, indicator);
#if defined(HAVE_QLEMENTINE_APP_STYLE)
        const bool qlementine =
            qobject_cast<const oclero::qlementine::QlementineStyle *>(style) != nullptr;
#else
        const bool qlementine = false;
#endif
        style->drawPrimitive(QStyle::PE_IndicatorRadioButton, &button, painter,
                             qlementine ? nullptr : option.widget);

        content.setLeft(button.rect.right() + kMarginH * 2);
        drawIcon(painter, content, index.data(ServerDiscoveryModel::IconRole).toString());
        content.setLeft(content.left() + kIconSize + kMarginH);

        const QFontMetrics metrics(option.font);
        painter->save();
        painter->setPen(textColor(option));
        painter->drawText(content, Qt::AlignLeft | Qt::AlignVCenter,
                          metrics.elidedText(index.data(Qt::DisplayRole).toString(),
                                             Qt::ElideRight, content.width()));
        painter->restore();
    }

    void paintPlaceholder(QPainter *painter, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const
    {
        QFont font = option.font;
        font.setItalic(true);
        const QRect content = option.rect.adjusted(kMarginH, 0, -kMarginH, 0);
        const QFontMetrics metrics(font);

        painter->save();
        painter->setFont(font);
        painter->setPen(AppColors::hint());
        painter->drawText(content, Qt::AlignLeft | Qt::AlignVCenter,
                          metrics.elidedText(index.data(Qt::DisplayRole).toString(),
                                             Qt::ElideRight, content.width()));
        painter->restore();
    }

    static void drawIcon(QPainter *painter, const QRect &content, const QString &name)
    {
        if (name.isEmpty())
            return;
        const QRect iconRect(content.left(), content.center().y() - kIconSize / 2,
                             kIconSize, kIconSize);
        AppIcons::themed(name).paint(painter, iconRect);
    }
};

///
/// \brief Draws the right-aligned transport scheme of an endpoint row.
///
class TransportColumnDelegate final : public DiscoveryCellDelegate
{
public:
    using DiscoveryCellDelegate::DiscoveryCellDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        paintBackground(painter, option, index);

        const QString text = index.data(Qt::DisplayRole).toString();
        if (text.isEmpty())
            return;

        QFont font = option.font;
        font.setItalic(true);
        painter->save();
        painter->setFont(font);
        painter->setPen(option.state.testFlag(QStyle::State_Selected)
                            ? textColor(option)
                            : AppColors::hint());
        painter->drawText(option.rect.adjusted(0, 0, -kMarginH * 2, 0),
                          Qt::AlignRight | Qt::AlignVCenter, text);
        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QFont font = option.font;
        font.setItalic(true);
        const QFontMetrics metrics(font);
        return QSize(metrics.horizontalAdvance(index.data(Qt::DisplayRole).toString())
                         + kMarginH * 4,
                     kEndpointRowHeight);
    }
};

///
/// \brief Builds the illustration shown while no server has been discovered.
/// \param parent Owning widget.
/// \return Empty-state page.
///
QWidget *createEmptyPage(QWidget *parent)
{
    auto *page = new QWidget(parent);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(8);
    layout->addStretch();

    auto *icon = new ThemedIconLabel(page);
    icon->setIcon(QStringLiteral("search"), QSize(64, 64));
    icon->setAlignment(Qt::AlignCenter);
    layout->addWidget(icon);
    layout->addSpacing(8);

    auto *title = new QLabel(ServerDiscoveryWidget::tr("No servers found."), page);
    QFont titleFont = title->font();
    titleFont.setPointSizeF(titleFont.pointSizeF() * 1.3);
    titleFont.setWeight(QFont::DemiBold);
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    auto *hint = new QLabel(
        ServerDiscoveryWidget::tr("No OPC UA servers were discovered using the specified "
                                  "discovery server.\nPlease check the Discovery Server URL "
                                  "and try again."),
        page);
    hint->setAlignment(Qt::AlignCenter);
    hint->setWordWrap(true);
    QPalette hintPalette = hint->palette();
    hintPalette.setColor(QPalette::WindowText, AppColors::hint());
    hint->setPalette(hintPalette);
    layout->addWidget(hint);

    layout->addStretch();
    return page;
}

}

///
/// \brief Builds the tree with its row delegates and the empty state.
/// \param parent Owning widget.
///
ServerDiscoveryWidget::ServerDiscoveryWidget(QWidget *parent)
    : QWidget(parent)
    , _pages(new QStackedWidget(this))
    , _view(new QTreeView(this))
    , _model(new ServerDiscoveryModel(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(_pages);

    _view->setObjectName(QStringLiteral("serverTreeWidget"));
    _view->setModel(_model);
    _view->setHeaderHidden(true);
    _view->setRootIsDecorated(true);
    _view->setUniformRowHeights(false);
    _view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _view->setSelectionMode(QAbstractItemView::SingleSelection);
    _view->setSelectionBehavior(QAbstractItemView::SelectRows);
    _view->setExpandsOnDoubleClick(true);
    _view->setAllColumnsShowFocus(true);
    _view->setItemDelegateForColumn(ServerDiscoveryModel::PrimaryColumn,
                                    new PrimaryColumnDelegate(_view));
    _view->setItemDelegateForColumn(ServerDiscoveryModel::TransportColumn,
                                    new TransportColumnDelegate(_view));

    QHeaderView *header = _view->header();
    header->setStretchLastSection(false);
    header->setSectionResizeMode(ServerDiscoveryModel::PrimaryColumn, QHeaderView::Stretch);
    header->setSectionResizeMode(ServerDiscoveryModel::TransportColumn,
                                 QHeaderView::ResizeToContents);

    _view->setProperty(kHoveredIndexProperty, QVariant::fromValue(QPersistentModelIndex()));
    _view->setMouseTracking(true);
    _view->viewport()->setMouseTracking(true);
    _view->viewport()->installEventFilter(this);

    _pages->addWidget(_view);
    _pages->addWidget(createEmptyPage(_pages));

    connect(_view->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &ServerDiscoveryWidget::currentEndpointChanged);
    connect(_view, &QTreeView::expanded, this, [this](const QModelIndex &index) {
        if (index.isValid() && !index.parent().isValid())
            emit serverExpanded(index.row());
    });
    connect(_view, &QTreeView::doubleClicked, this, [this](const QModelIndex &index) {
        if (_model->isEndpoint(index))
            emit endpointActivated();
    });

    updateEmptyState();
}

///
/// \brief Shows the discovered servers, collapsed and without endpoints.
/// \param servers Servers to display.
///
void ServerDiscoveryWidget::setServers(const QList<ServerInfo> &servers)
{
    _model->setServers(servers);
    for (int row = 0; row < _model->serverCount(); ++row)
        _view->setFirstColumnSpanned(row, {}, true);
    updateEmptyState();
}

///
/// \brief Removes all servers and returns to the empty state.
///
void ServerDiscoveryWidget::clear()
{
    setServers({});
}

///
/// \brief Returns the underlying model so the owner can drive the lazy loading.
/// \return Discovery model.
///
ServerDiscoveryModel *ServerDiscoveryWidget::model() const
{
    return _model;
}

///
/// \brief Reports whether the current row is an endpoint.
/// \return True when an endpoint is selected.
///
bool ServerDiscoveryWidget::hasEndpointSelection() const
{
    return _model->isEndpoint(_view->currentIndex());
}

///
/// \brief Returns the selected endpoint.
/// \return Endpoint for the current row, or a default-constructed value.
///
EndpointInfo ServerDiscoveryWidget::currentEndpoint() const
{
    return _model->endpointAt(_view->currentIndex());
}

///
/// \brief Selects the first endpoint of a server, if it has any.
/// \param serverRow Server row.
///
void ServerDiscoveryWidget::selectFirstEndpoint(int serverRow)
{
    const QModelIndex parent = _model->index(serverRow, ServerDiscoveryModel::PrimaryColumn);
    const QModelIndex first = _model->index(0, ServerDiscoveryModel::PrimaryColumn, parent);
    if (_model->isEndpoint(first))
        _view->setCurrentIndex(first);
}

///
/// \brief Tracks the hovered row from viewport mouse-move and leave events.
/// \param watched Watched object.
/// \param event Event being delivered.
/// \return True if the event was consumed.
///
bool ServerDiscoveryWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == _view->viewport()) {
        if (event->type() == QEvent::MouseMove) {
            const auto *mouseEvent = static_cast<QMouseEvent *>(event);
            setHoveredRow(_view->indexAt(mouseEvent->pos()));
        } else if (event->type() == QEvent::Leave) {
            setHoveredRow({});
        }
    }
    return QWidget::eventFilter(watched, event);
}

///
/// \brief Updates the shared hovered-row property and repaints the viewport.
/// \param index Row under the cursor, or an invalid index for none.
///
void ServerDiscoveryWidget::setHoveredRow(const QModelIndex &index)
{
    const auto hovered = _view->property(kHoveredIndexProperty).value<QPersistentModelIndex>();
    if (hovered == index)
        return;
    _view->setProperty(kHoveredIndexProperty, QVariant::fromValue(QPersistentModelIndex(index)));
    _view->viewport()->update();
}

///
/// \brief Shows the tree once servers exist, and the empty state otherwise.
///
void ServerDiscoveryWidget::updateEmptyState()
{
    _pages->setCurrentIndex(_model->serverCount() > 0 ? 0 : 1);
}
