// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file logfeature.h
/// \brief Declares the activity-log UI feature.
///

#pragma once

#include "featuremodule.h"

class LogWidget;
class QDockWidget;

///
/// \brief Hosts the activity log dock.
///
class LogFeature : public FeatureModule
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

private:
    QDockWidget *_dock = nullptr;
    LogWidget *_widget = nullptr;
};
