// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file favoritescoordinator.h
/// \brief Declares the main-window favourites popup coordinator.
///

#pragma once

#include <QObject>

#include "opcua/connectionprofile.h"

class ConnectionController;
class FavoritesWidget;
class OpcUaBackend;
class QToolButton;
class QWidget;

///
/// \brief Owns the favourites popup and keeps it in sync with saved profiles.
///
class FavoritesCoordinator : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief Builds a favourites coordinator.
    /// \param controller Connection controller that owns saved profiles.
    /// \param backend Backend used to decide whether a favourite can be added.
    /// \param parent Parent widget and QObject owner.
    ///
    FavoritesCoordinator(ConnectionController *controller,
                         OpcUaBackend *backend,
                         QWidget *parent);

    ///
    /// \brief Opens the popup below the supplied toolbar button.
    /// \param anchor Toolbar button used as popup anchor.
    ///
    void open(QToolButton *anchor);

signals:
    void connectRequested(const ConnectionProfile &profile);
    void editRequested(const ConnectionProfile &profile);
    void addFavoriteRequested();

private:
    void refreshVisible();

    ConnectionController *_controller;
    OpcUaBackend *_backend;
    FavoritesWidget *_widget;
};
