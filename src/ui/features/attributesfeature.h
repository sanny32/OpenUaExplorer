// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file attributesfeature.h
/// \brief Declares the selected-node attributes UI feature.
///

#pragma once

#include "featureplugin.h"

class AttributesWidget;
class QDockWidget;

///
/// \brief Hosts the selected-node attributes dock.
///
class AttributesFeature : public FeaturePlugin
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

private:
    QDockWidget *_dock = nullptr;
    AttributesWidget *_widget = nullptr;
};
