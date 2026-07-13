// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file featuremodule.cpp
/// \brief Implements the base class for UI feature modules.
///

#include "featuremodule.h"

#include <QtGlobal>

///
/// \brief Destroys the feature module.
///
FeatureModule::~FeatureModule() = default;

///
/// \brief Persists feature-owned view state.
/// \param settings Settings store to write to.
///
void FeatureModule::saveState(AppSettings &settings) const
{
    Q_UNUSED(settings)
}

///
/// \brief Restores feature-owned view state.
/// \param settings Settings store to read from.
///
void FeatureModule::restoreState(AppSettings &settings)
{
    Q_UNUSED(settings)
}

///
/// \brief Clears runtime data when the OPC UA session is no longer usable.
///
void FeatureModule::clearRuntimeState()
{
}

///
/// \brief Contributes feature state to a saved working session.
/// \param session Session payload to write to.
///
void FeatureModule::saveSession(SessionData &session) const
{
    Q_UNUSED(session)
}

///
/// \brief Restores feature state from a loaded working session.
/// \param session Session payload to read from.
///
void FeatureModule::restoreSession(const SessionData &session)
{
    Q_UNUSED(session)
}
