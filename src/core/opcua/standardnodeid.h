// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file standardnodeid.h
/// \brief Declares standard OPC UA NodeId constants.
///

#pragma once

#include <QLatin1String>

///
/// \brief Standard OPC UA NodeId string constants (OPC UA Part 6, Annex A).
///
class StandardNodeId
{
public:
    /// \brief Standard Objects folder, used as the browse root.
    static constexpr char ObjectsFolder[] = "ns=0;i=84";

    /// \brief Server.ServerStatus.CurrentTime, the server's UTC clock.
    static constexpr char ServerCurrentTime[] = "ns=0;i=2258";

    /// \brief Server.ServerDiagnostics.SessionsDiagnosticsSummary.SessionDiagnosticsArray.
    static constexpr char SessionDiagnosticsArray[] = "ns=0;i=3707";

    ///
    /// \brief Returns the Objects-folder NodeId as a QLatin1String.
    /// \return Objects-folder NodeId.
    ///
    static QLatin1String objectsFolder() { return QLatin1String(ObjectsFolder); }

    ///
    /// \brief Returns the server CurrentTime NodeId as a QLatin1String.
    /// \return Server CurrentTime NodeId.
    ///
    static QLatin1String serverCurrentTime() { return QLatin1String(ServerCurrentTime); }

    ///
    /// \brief Returns the SessionDiagnosticsArray NodeId as a QLatin1String.
    /// \return SessionDiagnosticsArray NodeId.
    ///
    static QLatin1String sessionDiagnosticsArray()
    {
        return QLatin1String(SessionDiagnosticsArray);
    }
};
