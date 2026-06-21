// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file favoriteswidget.cpp
/// \brief Implements the favourites quick-connect widget.
///

#include <QApplication>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPushButton>
#include <QVBoxLayout>

#include "appcolors.h"
#include "appicons.h"
#include "favoriteswidget.h"
#include "coloredpushbutton.h"
#include "ui_favoriteswidget.h"

namespace {
constexpr int widgetWidth = 560;
constexpr int titleBudget = 320;
constexpr int maxVisibleCards = 8;

/// \brief Drag MIME type carrying the dragged favourite's identifier.
const char favoriteMimeType[] = "application/x-ouaexp-favorite-id";

/// \brief Property name attaching a favourite id to its card widget.
const char favoriteIdProperty[] = "favoriteId";

///
/// \brief Returns a drag/drop/mouse event's position, bridging Qt 5 and Qt 6 APIs.
///
template <class Event>
QPoint eventPosition(const Event *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return event->position().toPoint();
#else
    return event->pos();
#endif
}
}

///
/// \brief Builds the widget from its generated UI and applies themed styling.
/// \param parent Parent widget.
///
FavoritesWidget::FavoritesWidget(QWidget *parent)
    : QFrame(parent, Qt::Popup)
    , ui(new Ui::FavoritesWidget)
{
    ui->setupUi(this);
    applyStyling();

    ui->starIcon->setIcon(QStringLiteral("star"), QSize(20, 20));
    ui->searchEdit->addAction(AppIcons::themed(QStringLiteral("search")),
                              QLineEdit::LeadingPosition);
    connect(ui->searchEdit, &QLineEdit::textChanged, this, [this] { rebuildList(); });
    connect(ui->addButton, &QPushButton::clicked, this, &FavoritesWidget::addFavoriteRequested);

    // The list container receives the dropped cards; a thin line previews the insert point.
    ui->listContainer->setAcceptDrops(true);
    ui->listContainer->installEventFilter(this);
    _dropIndicator = new QWidget(ui->listContainer);
    _dropIndicator->setFixedHeight(2);
    _dropIndicator->setStyleSheet(
        QStringLiteral("background: %1; border-radius: 1px;").arg(AppColors::accent().name()));
    _dropIndicator->hide();
}

///
/// \brief Destroys the widget and its generated UI.
///
FavoritesWidget::~FavoritesWidget()
{
    delete ui;
}

///
/// \brief Applies the theme-aware stylesheets that cannot be expressed in the .ui file.
///
void FavoritesWidget::applyStyling()
{
    setStyleSheet(QStringLiteral(
        "#FavoritesWidget { background: palette(window); border: 1px solid %1;"
        " border-radius: 10px; }"
        "QFrame#favoriteCard { border: 1px solid %1; border-radius: 8px;"
        " background: palette(base); }"
        "QFrame#favoriteCard:hover { border-color: %2; }")
        .arg(AppColors::toCss(AppColors::noticeNeutralBorder()),
             AppColors::toCss(AppColors::header())));

    ui->titleLabel->setStyleSheet(QStringLiteral("font-size: 15px; font-weight: bold; color: %1;")
                                      .arg(AppColors::titleText().name()));
    ui->subtitleLabel->setStyleSheet(QStringLiteral("color: %1;")
                                         .arg(AppColors::subtitleText().name()));
    ui->emptyLabel->setStyleSheet(QStringLiteral("color: %1; padding: 24px;")
                                      .arg(AppColors::subtitleText().name()));
    ui->addButton->setStyleSheet(QStringLiteral(
        "QPushButton { background: %1; color: %2; border: 1px solid %3; border-radius: 6px;"
        " padding: 6px 12px; } QPushButton:hover { background: %4; }"
        " QPushButton:disabled { color: %5; border-color: %5; }")
        .arg(AppColors::toCss(AppColors::noticeNeutralBackground()),
             AppColors::header().name(),
             AppColors::toCss(AppColors::noticeNeutralBorder()),
             AppColors::toCss(AppColors::noticeNeutralBorder()),
             AppColors::hint().name()));
}

///
/// \brief Replaces the displayed favourites, rebuilding the card list.
/// \param favorites Saved connection profiles.
///
void FavoritesWidget::setFavorites(const QList<ConnectionProfile> &favorites)
{
    _favorites = favorites;
    rebuildList();
}

///
/// \brief Enables or disables the add-current-connection action.
/// \param enabled True when a current connection exists to add.
///
void FavoritesWidget::setCanAddFavorite(bool enabled)
{
    ui->addButton->setEnabled(enabled);
    ui->addButton->setToolTip(enabled
        ? tr("Add the current connection to favourites")
        : tr("Connect to a server to add it to favourites"));
}

///
/// \brief Populates the favourites and shows the widget right-aligned under a widget.
/// \param favorites Saved connection profiles.
/// \param anchor Widget the popup is positioned beneath.
///
void FavoritesWidget::showFor(const QList<ConnectionProfile> &favorites, QWidget *anchor)
{
    setFixedWidth(widgetWidth);
    setFavorites(favorites);

    if (anchor) {
        const QPoint bottomRight = anchor->mapToGlobal(QPoint(anchor->width(), anchor->height()));
        int x = bottomRight.x() - width();
        const int y = bottomRight.y();
        if (const QWidget *top = anchor->window()) {
            const int winRight = top->mapToGlobal(QPoint(top->width(), 0)).x();
            const int winLeft = top->mapToGlobal(QPoint(0, 0)).x();
            x = qMin(x, winRight - width());
            x = qMax(x, winLeft);
        }
        move(x, y);
    }
    show();
    ui->searchEdit->setFocus();
}

///
/// \brief Rebuilds the card list from the current favourites and search filter.
///
void FavoritesWidget::rebuildList()
{
    while (ui->listLayout->count() > 1) {
        QLayoutItem *item = ui->listLayout->takeAt(0);
        if (QWidget *widget = item->widget())
            widget->deleteLater();
        delete item;
    }

    const QString filter = ui->searchEdit->text().trimmed();
    int shown = 0;
    for (const ConnectionProfile &favorite : _favorites) {
        if (!filter.isEmpty()
            && !favorite.name.contains(filter, Qt::CaseInsensitive)
            && !favorite.endpointUrl.contains(filter, Qt::CaseInsensitive)) {
            continue;
        }
        ui->listLayout->insertWidget(ui->listLayout->count() - 1, createCard(favorite));
        ++shown;
    }
    ui->emptyLabel->setVisible(shown == 0);
    adjustListHeight();
}

///
/// \brief Sizes the scroll area to the visible cards so the widget tracks the favourite count.
///
/// The area is fixed to exactly the shown cards, capped at maxVisibleCards, beyond which it
/// scrolls. It is hidden entirely when there are no cards so only the empty hint remains.
///
void FavoritesWidget::adjustListHeight()
{
    ui->listContainer->adjustSize();

    int cardHeight = 0;
    int cardCount = 0;
    for (int i = 0; i < ui->listLayout->count(); ++i) {
        if (QWidget *card = ui->listLayout->itemAt(i)->widget()) {
            cardHeight = qMax(cardHeight, card->sizeHint().height());
            ++cardCount;
        }
    }

    if (cardCount > 0) {
        const int spacing = ui->listLayout->spacing();
        const int visible = qMin(cardCount, maxVisibleCards);
        ui->scrollArea->setFixedHeight(visible * cardHeight + (visible - 1) * spacing);
    }
    ui->scrollArea->setVisible(cardCount > 0);
    adjustSize();
}

///
/// \brief Builds one favourite card with its title, security settings, and actions.
/// \param favorite Favourite to render.
/// \return The card widget.
///
QWidget *FavoritesWidget::createCard(const ConnectionProfile &favorite)
{
    auto *card = new QFrame(ui->listContainer);
    card->setObjectName(QStringLiteral("favoriteCard"));
    card->setProperty(favoriteIdProperty, favorite.id);
    card->installEventFilter(this);

    auto *star = new QLabel(card);
    star->setPixmap(AppIcons::themed(QStringLiteral("star")).pixmap(18, 18));
    star->setAttribute(Qt::WA_TransparentForMouseEvents);

    const QString fullName = favorite.name.isEmpty() ? favorite.endpointUrl : favorite.name;
    auto *name = new QLabel(card);
    name->setToolTip(favorite.endpointUrl);
    name->setText(name->fontMetrics().elidedText(fullName, Qt::ElideMiddle, titleBudget));
    name->setStyleSheet(QStringLiteral("font-weight: bold; color: %1;")
                            .arg(AppColors::titleText().name()));
    name->setAttribute(Qt::WA_TransparentForMouseEvents);

    auto *subtitle = new QLabel(securityText(favorite), card);
    subtitle->setStyleSheet(QStringLiteral("color: %1; font-size: 11px;")
                                .arg(AppColors::subtitleText().name()));
    subtitle->setAttribute(Qt::WA_TransparentForMouseEvents);

    auto *textColumn = new QVBoxLayout;
    textColumn->setSpacing(2);
    textColumn->addWidget(name);
    textColumn->addWidget(subtitle);

    card->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(card, &QFrame::customContextMenuRequested, this,
            [this, card, favorite](const QPoint &pos) {
        QMenu menu(card);
        QAction *editAction = menu.addAction(tr("Edit..."));
        QAction *removeAction = menu.addAction(tr("Remove from favorites"));
        const QAction *chosen = menu.exec(card->mapToGlobal(pos));
        if (chosen == editAction) {
            emit editRequested(favorite);
            close();
        } else if (chosen == removeAction) {
            emit removeRequested(favorite.id);
        }
    });

    auto *connectButton = new ColoredPushButton(card);
    connectButton->setText(tr("Connect"));
    connectButton->setIcon(AppIcons::themed(QStringLiteral("connect")));
    connectButton->setColors(
        { AppColors::accent(), AppColors::accentHover(), AppColors::accentPressed() });
    connect(connectButton, &QPushButton::clicked, this, [this, favorite] {
        emit connectRequested(favorite);
        close();
    });

    auto *cardLayout = new QHBoxLayout(card);
    cardLayout->setContentsMargins(12, 10, 12, 10);
    cardLayout->setSpacing(12);
    cardLayout->addWidget(star);
    cardLayout->addLayout(textColumn, 1);
    cardLayout->addWidget(connectButton);
    return card;
}

///
/// \brief Formats a favourite's security policy and mode for display.
/// \param favorite Favourite whose security settings are described.
/// \return Human-readable "policy / mode" line.
///
QString FavoritesWidget::securityText(const ConnectionProfile &favorite)
{
    if (favorite.securityPolicy.isEmpty())
        return tr("No security");

    const QString policy = favorite.securityPolicy.section(QLatin1Char('#'), -1);
    QString mode;
    switch (favorite.securityMode) {
    case 2: mode = tr("Sign"); break;
    case 3: mode = tr("Sign & Encrypt"); break;
    default: break; // None / Invalid: the policy name already reads "None".
    }
    return mode.isEmpty() ? policy : QStringLiteral("%1 / %2").arg(policy, mode);
}

///
/// \brief Drives card-drag initiation and list-container drop handling.
/// \param watched Object the event was sent to.
/// \param event Event being filtered.
/// \return True when the event is consumed.
///
bool FavoritesWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->listContainer) {
        switch (event->type()) {
        case QEvent::DragEnter: {
            auto *dragEvent = static_cast<QDragEnterEvent *>(event);
            if (dragEvent->mimeData()->hasFormat(QLatin1String(favoriteMimeType))) {
                dragEvent->acceptProposedAction();
                return true;
            }
            break;
        }
        case QEvent::DragMove: {
            auto *dragEvent = static_cast<QDragMoveEvent *>(event);
            if (dragEvent->mimeData()->hasFormat(QLatin1String(favoriteMimeType))) {
                showDropIndicator(dropIndexAt(eventPosition(dragEvent).y()));
                dragEvent->acceptProposedAction();
                return true;
            }
            break;
        }
        case QEvent::DragLeave:
            hideDropIndicator();
            return true;
        case QEvent::Drop: {
            auto *dropEvent = static_cast<QDropEvent *>(event);
            if (!dropEvent->mimeData()->hasFormat(QLatin1String(favoriteMimeType)))
                break;
            const QString id =
                QString::fromUtf8(dropEvent->mimeData()->data(QLatin1String(favoriteMimeType)));
            const int target = dropIndexAt(eventPosition(dropEvent).y());
            hideDropIndicator();
            moveFavorite(id, target);
            dropEvent->acceptProposedAction();
            return true;
        }
        default:
            break;
        }
        return QFrame::eventFilter(watched, event);
    }

    auto *card = qobject_cast<QWidget *>(watched);
    if (card && card->property(favoriteIdProperty).isValid()) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                _dragStartPos = eventPosition(mouseEvent);
                _dragId = card->property(favoriteIdProperty).toString();
            }
        } else if (event->type() == QEvent::MouseMove) {
            auto *mouseEvent = static_cast<QMouseEvent *>(event);
            if ((mouseEvent->buttons() & Qt::LeftButton) && !_dragId.isEmpty()
                && draggingEnabled()
                && (eventPosition(mouseEvent) - _dragStartPos).manhattanLength()
                       >= QApplication::startDragDistance()) {
                startCardDrag(card);
            }
        }
    }
    return QFrame::eventFilter(watched, event);
}

///
/// \brief Reports whether cards may be reordered: only with several cards and no active filter.
/// \return True when dragging should be allowed.
///
bool FavoritesWidget::draggingEnabled() const
{
    return _favorites.size() > 1 && ui->searchEdit->text().trimmed().isEmpty();
}

///
/// \brief Starts a move drag for a card, carrying its favourite id and a card preview.
/// \param card Card being dragged.
///
void FavoritesWidget::startCardDrag(QWidget *card)
{
    auto *drag = new QDrag(this);
    auto *mime = new QMimeData;
    mime->setData(QLatin1String(favoriteMimeType), _dragId.toUtf8());
    drag->setMimeData(mime);
    drag->setPixmap(card->grab());
    drag->setHotSpot(_dragStartPos);

    drag->exec(Qt::MoveAction);

    hideDropIndicator();
    _dragId.clear();
}

///
/// \brief Maps a vertical position in the list to the index a dropped card would take.
/// \param y Vertical position in list-container coordinates.
/// \return Insertion index between 0 and the card count.
///
int FavoritesWidget::dropIndexAt(int y) const
{
    int index = 0;
    for (int i = 0; i < ui->listLayout->count(); ++i) {
        QWidget *card = ui->listLayout->itemAt(i)->widget();
        if (!card)
            continue;
        if (y < card->y() + card->height() / 2)
            return index;
        ++index;
    }
    return index;
}

///
/// \brief Shows the insertion line at the gap for the given drop index.
/// \param index Insertion index between 0 and the card count.
///
void FavoritesWidget::showDropIndicator(int index)
{
    const int spacing = ui->listLayout->spacing();
    QList<QWidget *> cards;
    for (int i = 0; i < ui->listLayout->count(); ++i) {
        if (QWidget *card = ui->listLayout->itemAt(i)->widget())
            cards.append(card);
    }
    if (cards.isEmpty())
        return;

    int y = 0;
    if (index < cards.size())
        y = cards.at(index)->y() - spacing / 2 - 1;
    else
        y = cards.last()->y() + cards.last()->height() + spacing / 2 - 1;

    _dropIndicator->setGeometry(0, qMax(0, y), ui->listContainer->width(), 2);
    _dropIndicator->show();
    _dropIndicator->raise();
}

///
/// \brief Hides the drop insertion line.
///
void FavoritesWidget::hideDropIndicator()
{
    if (_dropIndicator)
        _dropIndicator->hide();
}

///
/// \brief Moves a favourite to a new position, rebuilds the list, and reports the new order.
/// \param id Favourite being moved.
/// \param targetIndex Insertion index among the current cards.
///
void FavoritesWidget::moveFavorite(const QString &id, int targetIndex)
{
    int from = -1;
    for (int i = 0; i < _favorites.size(); ++i) {
        if (_favorites.at(i).id == id) {
            from = i;
            break;
        }
    }
    if (from < 0)
        return;

    int insert = targetIndex;
    if (from < insert)
        --insert;
    insert = qBound(0, insert, _favorites.size() - 1);
    if (insert == from)
        return;

    _favorites.move(from, insert);
    rebuildList();

    QStringList orderedIds;
    orderedIds.reserve(_favorites.size());
    for (const ConnectionProfile &favorite : _favorites)
        orderedIds.append(favorite.id);
    emit reorderRequested(orderedIds);
}
