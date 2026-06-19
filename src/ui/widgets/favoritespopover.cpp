// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file favoritespopover.cpp
/// \brief Implements the favourites quick-connect popover.
///

#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>

#include "appcolors.h"
#include "appicons.h"
#include "favoritespopover.h"
#include "coloredpushbutton.h"
#include "ui_favoritespopover.h"

namespace {
constexpr int popoverWidth = 560;
constexpr int titleBudget = 320;
}

///
/// \brief Builds the popover from its generated UI and applies themed styling.
/// \param parent Parent widget.
///
FavoritesPopover::FavoritesPopover(QWidget *parent)
    : QFrame(parent, Qt::Popup)
    , ui(new Ui::FavoritesPopover)
{
    ui->setupUi(this);
    applyStyling();

    ui->starIcon->setIcon(QStringLiteral("star"), QSize(20, 20));
    ui->searchEdit->addAction(AppIcons::themed(QStringLiteral("search")),
                              QLineEdit::LeadingPosition);
    connect(ui->searchEdit, &QLineEdit::textChanged, this, [this] { rebuildList(); });
    connect(ui->addButton, &QPushButton::clicked, this, &FavoritesPopover::addFavoriteRequested);
    connect(ui->closeButton, &QPushButton::clicked, this, &QFrame::close);
}

///
/// \brief Destroys the popover and its generated UI.
///
FavoritesPopover::~FavoritesPopover()
{
    delete ui;
}

///
/// \brief Applies the theme-aware stylesheets that cannot be expressed in the .ui file.
///
void FavoritesPopover::applyStyling()
{
    setStyleSheet(QStringLiteral(
        "#FavoritesPopover { background: palette(window); border: 1px solid %1;"
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
        " padding: 6px 12px; } QPushButton:hover { background: %4; }")
        .arg(AppColors::toCss(AppColors::noticeNeutralBackground()),
             AppColors::header().name(),
             AppColors::toCss(AppColors::noticeNeutralBorder()),
             AppColors::toCss(AppColors::noticeNeutralBorder())));
}

///
/// \brief Replaces the displayed favourites, rebuilding the card list.
/// \param favorites Saved connection profiles.
///
void FavoritesPopover::setFavorites(const QList<ConnectionProfile> &favorites)
{
    _favorites = favorites;
    rebuildList();
}

///
/// \brief Enables or disables the add-current-connection action.
/// \param enabled True when a current connection exists to add.
///
void FavoritesPopover::setCanAddFavorite(bool enabled)
{
    ui->addButton->setEnabled(enabled);
    ui->addButton->setToolTip(enabled
        ? tr("Add the current connection to favourites")
        : tr("Connect to a server to add it to favourites"));
}

///
/// \brief Populates the favourites and shows the popover right-aligned under a widget.
/// \param favorites Saved connection profiles.
/// \param anchor Widget the popover is positioned beneath.
///
void FavoritesPopover::showFor(const QList<ConnectionProfile> &favorites, QWidget *anchor)
{
    setFavorites(favorites);

    setFixedWidth(popoverWidth);
    adjustSize();
    resize(popoverWidth, qBound(220, height(), 520));

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
void FavoritesPopover::rebuildList()
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
}

///
/// \brief Builds one favourite card with its title, last-used time, and actions.
/// \param favorite Favourite to render.
/// \return The card widget.
///
QWidget *FavoritesPopover::createCard(const ConnectionProfile &favorite)
{
    auto *card = new QFrame(ui->listContainer);
    card->setObjectName(QStringLiteral("favoriteCard"));

    auto *star = new QLabel(card);
    star->setPixmap(AppIcons::themed(QStringLiteral("star")).pixmap(18, 18));

    const QString fullName = favorite.name.isEmpty() ? favorite.endpointUrl : favorite.name;
    auto *name = new QLabel(card);
    name->setToolTip(favorite.endpointUrl);
    name->setText(name->fontMetrics().elidedText(fullName, Qt::ElideMiddle, titleBudget));
    name->setStyleSheet(QStringLiteral("font-weight: bold; color: %1;")
                            .arg(AppColors::titleText().name()));

    auto *subtitle = new QLabel(lastUsedText(favorite), card);
    subtitle->setStyleSheet(QStringLiteral("color: %1; font-size: 11px;")
                                .arg(AppColors::subtitleText().name()));

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
            emit removeRequested(favorite.endpointUrl);
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
/// \brief Formats a favourite's last-used time for display.
/// \param favorite Favourite whose timestamp is described.
/// \return Human-readable last-used line.
///
QString FavoritesPopover::lastUsedText(const ConnectionProfile &favorite)
{
    if (!favorite.lastUsed.isValid())
        return tr("Never used");

    const QDate date = favorite.lastUsed.date();
    const QString time = favorite.lastUsed.time().toString(QStringLiteral("HH:mm:ss"));
    if (date == QDate::currentDate())
        return tr("Last used: Today, %1").arg(time);
    if (date == QDate::currentDate().addDays(-1))
        return tr("Last used: Yesterday, %1").arg(time);
    return tr("Last used: %1, %2")
        .arg(QLocale().toString(date, QLocale::ShortFormat), time);
}
