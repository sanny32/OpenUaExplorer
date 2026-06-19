// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file favoritespopover.h
/// \brief Declares the favourites quick-connect popover.
///

#pragma once

#include <QFrame>
#include <QList>

#include "opcua/connectionprofile.h"

namespace Ui {
class FavoritesPopover;
}

///
/// \brief Popup listing favourite servers as a searchable card list.
///
/// Opens right-aligned beneath the toolbar's favourites button. Each card shows a saved
/// server with its last-used time, a Connect action that opens the connection dialog
/// pre-filled with that server, and a context menu to edit or remove it. A header
/// action adds the current connection to favourites.
///
class FavoritesPopover : public QFrame
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the popover from its generated UI and applies themed styling.
    /// \param parent Parent widget.
    ///
    explicit FavoritesPopover(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the popover and its generated UI.
    ///
    ~FavoritesPopover() override;

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
    /// \brief Populates the favourites and shows the popover right-aligned under a widget.
    /// \param favorites Saved connection profiles.
    /// \param anchor Widget the popover is positioned beneath.
    ///
    void showFor(const QList<ConnectionProfile> &favorites, QWidget *anchor);

signals:
    ///
    /// \brief Emitted when the user asks to connect to a favourite.
    /// \param profile Favourite the connection dialog should be opened with.
    ///
    void connectRequested(const ConnectionProfile &profile);

    ///
    /// \brief Emitted when the user asks to edit a favourite.
    /// \param profile Favourite the connection dialog should be opened with.
    ///
    void editRequested(const ConnectionProfile &profile);

    ///
    /// \brief Emitted when the user removes a favourite.
    /// \param endpointUrl Endpoint URL of the favourite to remove.
    ///
    void removeRequested(const QString &endpointUrl);

    ///
    /// \brief Emitted when the user adds the current connection to favourites.
    ///
    void addFavoriteRequested();

private:
    void applyStyling();
    void rebuildList();
    QWidget *createCard(const ConnectionProfile &favorite);
    static QString lastUsedText(const ConnectionProfile &favorite);

    Ui::FavoritesPopover *ui;
    QList<ConnectionProfile> _favorites;
};
