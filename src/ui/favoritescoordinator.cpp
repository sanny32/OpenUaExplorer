// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file favoritescoordinator.cpp
/// \brief Implements the main-window favourites popup coordinator.
///

#include "favoritescoordinator.h"

#include <algorithm>

#include <QToolButton>

#include "opcua/connectioncontroller.h"
#include "opcua/opcuabackend.h"
#include "widgets/favoriteswidget.h"

///
/// \brief Builds a favourites coordinator.
/// \param controller Connection controller that owns saved profiles.
/// \param backend Backend used to decide whether a favourite can be added.
/// \param parent Parent widget and QObject owner.
///
FavoritesCoordinator::FavoritesCoordinator(ConnectionController *controller,
                                           OpcUaBackend *backend,
                                           QWidget *parent)
    : QObject(parent)
    , _controller(controller)
    , _backend(backend)
    , _widget(new FavoritesWidget(parent))
{
    connect(_widget, &FavoritesWidget::connectRequested,
            this, &FavoritesCoordinator::connectRequested);
    connect(_widget, &FavoritesWidget::editRequested,
            this, &FavoritesCoordinator::editRequested);
    connect(_widget, &FavoritesWidget::removeRequested,
            _controller, &ConnectionController::removeFavorite);
    connect(_widget, &FavoritesWidget::addFavoriteRequested,
            this, &FavoritesCoordinator::addFavoriteRequested);
    connect(_widget, &FavoritesWidget::reorderRequested,
            _controller, &ConnectionController::reorderFavorites);
    connect(_controller, &ConnectionController::profilesChanged,
            this, &FavoritesCoordinator::refreshVisible);
}

///
/// \brief Opens the popup below the supplied toolbar button.
/// \param anchor Toolbar button used as popup anchor.
///
void FavoritesCoordinator::open(QToolButton *anchor)
{
    const QList<ConnectionProfile> profiles = _controller->profiles();
    const ConnectionProfile active = _controller->activeProfile();
    const bool connected = _backend->state() == OpcUaConnectionState::Connected;
    const bool alreadyFavorite = std::any_of(
        profiles.cbegin(), profiles.cend(), [&active](const ConnectionProfile &profile) {
            return profile.isSameEndpoint(active);
        });
    _widget->setCanAddFavorite(
        connected && !active.endpointUrl.isEmpty() && !alreadyFavorite);
    _widget->showFor(profiles, anchor);
}

///
/// \brief Updates the popup contents while it is visible.
///
void FavoritesCoordinator::refreshVisible()
{
    if (_widget->isVisible())
        _widget->setFavorites(_controller->profiles());
}
