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
    /// \brief Returns the human-readable feature name.
    /// \return Feature name.
    ///
    QString name() const override;

    ///
    /// \brief Creates the feature UI and wires it to host services.
    /// \param host Host services and contribution points.
    ///
    void initialize(FeatureHost &host) override;

    ///
    /// \brief Persists feature-owned view state.
    /// \param settings Settings store to write to.
    ///
    void saveState(AppSettings &settings) const override;

    ///
    /// \brief Restores feature-owned view state.
    /// \param settings Settings store to read from.
    ///
    void restoreState(AppSettings &settings) override;

    ///
    /// \brief Clears runtime data when the OPC UA session is no longer usable.
    ///
    void clearRuntimeState() override;

    ///
    /// \brief Saves the expanded tree nodes and selected node into the session.
    /// \param session Session payload to write to.
    ///
    void saveSession(SessionData &session) const override;

    ///
    /// \brief Restores the expanded tree nodes and selected node from the session.
    /// \param session Session payload to read from.
    ///
    void restoreSession(const SessionData &session) override;

private:
    void browseAddressSpace();

    QDockWidget *_addressDock = nullptr;
    QDockWidget *_nodeDetailsDock = nullptr;
    AddressSpaceWidget *_widget = nullptr;
};
