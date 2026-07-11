// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file addressspacefeature.h
/// \brief Declares the address-space browser UI feature.
///

#pragma once

#include "featureplugin.h"

class AddressSpaceWidget;
class FeatureHost;
class QDockWidget;

///
/// \brief Hosts the address-space tree and node-details docks.
///
class AddressSpaceFeature : public FeaturePlugin
{
public:
    ///
    /// \brief Label used by the feature registry and translated UI surfaces.
    ///
    QString name() const override;

    ///
    /// \brief Creates the address-space docks and wires them to modules contributed by the host.
    ///
    void initialize(FeatureHost &host) override;

    ///
    /// \brief Stores tree/view presentation state, leaving live server data out of settings.
    ///
    void saveState(AppSettings &settings) const override;

    ///
    /// \brief Restores only presentation state saved by saveState().
    ///
    void restoreState(AppSettings &settings) override;

    ///
    /// \brief Clears node data that belongs to the active OPC UA session.
    ///
    void clearRuntimeState() override;

    ///
    /// \brief Adds address-space navigation state to an already collected session payload.
    ///
    void saveSession(SessionData &session) const override;

    ///
    /// \brief Replays saved tree expansion and selection after the session workspace is loaded.
    ///
    void restoreSession(const SessionData &session) override;

private:
    void createDocks(FeatureHost &host);
    void wireModules(FeatureHost &host);
    void contributeLayout(FeatureHost &host);
    void registerCommands(FeatureHost &host);
    void browseAddressSpace();

    QDockWidget *_addressDock = nullptr;
    QDockWidget *_nodeDetailsDock = nullptr;
    AddressSpaceWidget *_widget = nullptr;
};
