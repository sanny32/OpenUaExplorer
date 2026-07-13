// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file featureplugin.cpp
/// \brief Implements the base class for UI feature plugins.
///

#include "featureplugin.h"

#include <QtGlobal>

///
/// \brief Destroys the feature plugin.
///
FeaturePlugin::~FeaturePlugin() = default;

///
/// \brief Persists feature-owned view state.
/// \param settings Settings store to write to.
///
void FeaturePlugin::saveState(AppSettings &settings) const
{
    Q_UNUSED(settings)
}

///
/// \brief Restores feature-owned view state.
/// \param settings Settings store to read from.
///
void FeaturePlugin::restoreState(AppSettings &settings)
{
    Q_UNUSED(settings)
}

///
/// \brief Clears runtime data when the OPC UA session is no longer usable.
///
void FeaturePlugin::clearRuntimeState()
{
}

///
/// \brief Contributes feature state to a saved working session.
/// \param session Session payload to write to.
///
void FeaturePlugin::saveSession(SessionData &session) const
{
    Q_UNUSED(session)
}

///
/// \brief Restores feature state from a loaded working session.
/// \param session Session payload to read from.
///
void FeaturePlugin::restoreSession(const SessionData &session)
{
    Q_UNUSED(session)
}
