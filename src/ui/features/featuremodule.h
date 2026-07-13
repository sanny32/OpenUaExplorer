// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file featuremodule.h
/// \brief Declares the base class for UI feature modules.
///

#pragma once

#include <QString>

class AppSettings;
class FeatureHost;
struct SessionData;

///
/// \brief Base class for a dock-style UI feature hosted by MainWindow.
///
class FeatureModule
{
public:
    ///
    /// \brief Destroys the feature module.
    ///
    virtual ~FeatureModule();

    ///
    /// \brief Returns the human-readable feature name.
    /// \return Feature name.
    ///
    virtual QString name() const = 0;

    ///
    /// \brief Creates the feature UI and wires it to host services.
    /// \param host Host services and contribution points.
    ///
    virtual void initialize(FeatureHost &host) = 0;

    ///
    /// \brief Persists feature-owned view state.
    /// \param settings Settings store to write to.
    ///
    virtual void saveState(AppSettings &settings) const;

    ///
    /// \brief Restores feature-owned view state.
    /// \param settings Settings store to read from.
    ///
    virtual void restoreState(AppSettings &settings);

    ///
    /// \brief Clears runtime data when the OPC UA session is no longer usable.
    ///
    virtual void clearRuntimeState();

    ///
    /// \brief Contributes feature state to a saved working session.
    /// \param session Session payload to write to.
    ///
    virtual void saveSession(SessionData &session) const;

    ///
    /// \brief Restores feature state from a loaded working session.
    /// \param session Session payload to read from.
    ///
    virtual void restoreSession(const SessionData &session);
};
