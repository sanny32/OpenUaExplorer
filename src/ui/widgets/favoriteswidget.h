// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file favoriteswidget.h
/// \brief Declares the favourites quick-connect widget.
///

#pragma once

#include <QFrame>
#include <QList>
#include <QPoint>
#include <QString>
#include <QStringList>

#include "opcua/connectionprofile.h"

class QEvent;
class QLabel;
class QObject;

namespace Ui {
class FavoritesWidget;
}

///
/// \brief Popup listing favourite servers as a searchable card list.
///
/// Opens right-aligned beneath the toolbar's favourites button. Each card shows a saved
/// server with its security policy and mode, a Connect action that connects to the server
/// directly with the saved settings, and a context menu to edit or remove it. A header
/// action adds the current connection to favourites.
///
class FavoritesWidget : public QFrame
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the widget from its generated UI and applies themed styling.
    /// \param parent Parent widget.
    ///
    explicit FavoritesWidget(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the widget and its generated UI.
    ///
    ~FavoritesWidget() override;

    ///
    /// \brief Replaces the displayed favourites, rebuilding the card list.
    /// \param favorites Saved connection profiles.
    ///
    void setFavorites(const QList<ConnectionProfile> &favorites);

    ///
    /// \brief Enables or disables the add-current-connection action.
    /// \param enabled True when a current connection exists to add.
    ///
    void setCanAddFavorite(bool enabled);

    ///
    /// \brief Populates the favourites and shows the widget right-aligned under a widget.
    /// \param favorites Saved connection profiles.
    /// \param anchor Widget the popup is positioned beneath.
    ///
    void showFor(const QList<ConnectionProfile> &favorites, QWidget *anchor);

signals:
    ///
    /// \brief Emitted when the user asks to connect to a favourite.
    /// \param profile Favourite to connect to with its saved settings.
    ///
    void connectRequested(const ConnectionProfile &profile);

    ///
    /// \brief Emitted when the user asks to edit a favourite.
    /// \param profile Favourite the connection dialog should be opened with.
    ///
    void editRequested(const ConnectionProfile &profile);

    ///
    /// \brief Emitted when the user removes a favourite.
    /// \param id Identifier of the favourite to remove.
    ///
    void removeRequested(const QString &id);

    ///
    /// \brief Emitted when the user adds the current connection to favourites.
    ///
    void addFavoriteRequested();

    ///
    /// \brief Emitted when the user reorders favourites by dragging a card.
    /// \param orderedIds Favourite identifiers in their new order.
    ///
    void reorderRequested(const QStringList &orderedIds);

protected:
    ///
    /// \brief Drives card-drag initiation and list-container drop handling.
    /// \param watched Object the event was sent to.
    /// \param event Event being filtered.
    /// \return True when the event is consumed.
    ///
    bool eventFilter(QObject *watched, QEvent *event) override;

    ///
    /// \brief Re-applies theme-aware styling and icons when the palette changes.
    /// \param event Change event being handled.
    ///
    void changeEvent(QEvent *event) override;

private:
    void applyStyling();
    void updateDropIndicatorStyle();
    void rebuildList();
    void adjustListHeight();
    QWidget *createCard(const ConnectionProfile &favorite);
    static QString securityText(const ConnectionProfile &favorite);

    bool draggingEnabled() const;
    void startCardDrag(QWidget *card);
    void updateCardDrag(const QPoint &globalPos);
    void finishCardDrag(const QPoint &globalPos);
    void endCardDrag();
    void restoreDragCursor();
    int dropIndexAt(int y) const;
    void showDropIndicator(int index);
    void hideDropIndicator();
    void moveFavorite(const QString &id, int targetIndex);

    Ui::FavoritesWidget *ui;
    QList<ConnectionProfile> _favorites;

    bool _refreshingTheme = false;
    QWidget *_dropIndicator = nullptr;
    QLabel *_dragPreview = nullptr;
    QWidget *_dragCard = nullptr;
    QPoint _dragStartPos;
    QString _dragId;
    bool _dragging = false;
    bool _dragCursorActive = false;
};
