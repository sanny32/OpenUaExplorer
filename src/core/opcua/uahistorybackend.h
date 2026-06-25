// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QDateTime>
#include <QString>
#include <QVector>

#include "connectionprofile.h"
#include "opcuatypes.h"

///
/// \brief Result of one direct raw HistoryRead request.
///
struct UaHistoryReadResult
{
    QVector<OpcUaHistoryValue> values;
    QString error;
};

///
/// \brief Performs Qt5 raw HistoryRead requests through bundled open62541.
///
class UaHistoryBackend
{
public:
    ///
    /// \brief Reads raw historical Value samples with a short-lived open62541 session.
    /// \param profile Connection profile used by the active Qt OPC UA session.
    /// \param password Username password.
    /// \param privateKeyPassword Private key password; encrypted keys are unsupported.
    /// \param nodeId Node whose Value history is read.
    /// \param start Inclusive range start.
    /// \param end Inclusive range end.
    /// \param maxValues Maximum samples to return, or 0 for no limit.
    /// \param timeoutMs Request timeout in milliseconds.
    /// \return History values or a user-facing error.
    ///
    UaHistoryReadResult readRaw(const ConnectionProfile &profile,
                                const QString &password,
                                const QString &privateKeyPassword,
                                const QString &nodeId,
                                const QDateTime &start,
                                const QDateTime &end,
                                quint32 maxValues,
                                int timeoutMs) const;
};
