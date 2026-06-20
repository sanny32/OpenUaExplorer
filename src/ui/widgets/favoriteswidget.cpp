// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file favoriteswidget.cpp
/// \brief Implements the favourites quick-connect widget.
///

#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>

#include "appcolors.h"
#include "appicons.h"
#include "favoriteswidget.h"
#include "coloredpushbutton.h"
#include "ui_favoriteswidget.h"

namespace {
constexpr int popoverWidth = 560;
constexpr int titleBudget = 320;
constexpr int maxVisibleCards = 5;
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
    setFavorites(favorites);

    setFixedWidth(popoverWidth);
    adjustSize();

    // Cap the scroll area so it never shows more than maxVisibleCards before scrolling.
    int cardHeight = 0;
    for (int i = 0; i < ui->listLayout->count(); ++i) {
        if (QWidget *card = ui->listLayout->itemAt(i)->widget())
            cardHeight = qMax(cardHeight, card->sizeHint().height());
    }
    if (cardHeight > 0) {
        const int spacing = ui->listLayout->spacing();
        const int maxListHeight =
            maxVisibleCards * cardHeight + (maxVisibleCards - 1) * spacing;
        ui->scrollArea->setMaximumHeight(maxListHeight);
    } else {
        ui->scrollArea->setMaximumHeight(QWIDGETSIZE_MAX);
    }

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

    auto *star = new QLabel(card);
    star->setPixmap(AppIcons::themed(QStringLiteral("star")).pixmap(18, 18));

    const QString fullName = favorite.name.isEmpty() ? favorite.endpointUrl : favorite.name;
    auto *name = new QLabel(card);
    name->setToolTip(favorite.endpointUrl);
    name->setText(name->fontMetrics().elidedText(fullName, Qt::ElideMiddle, titleBudget));
    name->setStyleSheet(QStringLiteral("font-weight: bold; color: %1;")
                            .arg(AppColors::titleText().name()));

    auto *subtitle = new QLabel(securityText(favorite), card);
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
