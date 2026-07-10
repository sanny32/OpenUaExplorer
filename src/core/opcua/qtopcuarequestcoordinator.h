// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <array>

#include <QHash>
#include <QString>
#include <QtGlobal>

class QtOpcUaRequestCoordinator
{
public:
    /// \brief Categories whose requests supersede only requests of the same category.
    enum class Operation {
        Discovery,
        FindServers,
        Browse,
        BrowseAttributeRead,
        ReferencesBrowse,
        NodeRead,
        ValueRead,
        HistoryRead,
        HistoryEventsRead,
        Write,
        MethodInfo,
        Call,
        SessionNameRead,
        Count
    };

    /// \brief Identifies one request within a connection and operation generation.
    struct Token
    {
        Operation operation;
        quint64 connectionGeneration;
        quint64 operationGeneration;
        QString key;
    };

    /// \brief Starts a request and supersedes the previous request of its category.
    Token begin(Operation operation);

    /// \brief Starts a request that supersedes only the previous request with the same key.
    ///
    /// Categories such as HistoryRead read several nodes in parallel; keying by node
    /// keeps those reads independent while a newer read of one node still cancels its
    /// own in-flight predecessor.
    Token begin(Operation operation, const QString &key);

    /// \brief Reports whether a token can still complete.
    bool isCurrent(const Token &token) const;

    /// \brief Completes a current token exactly once.
    bool settle(const Token &token);

    /// \brief Invalidates requests in one category.
    void cancel(Operation operation);

    /// \brief Invalidates every request and advances the connection generation.
    void cancelAll();
    
    /// \brief Applies the minimum request timeout.
    static int boundedTimeout(int timeoutMs);

private:
    static std::size_t index(Operation operation);

    quint64 _connectionGeneration = 0;
    std::array<quint64, static_cast<std::size_t>(Operation::Count)> _generations{};
    std::array<QHash<QString, quint64>, static_cast<std::size_t>(Operation::Count)> _keyedGenerations{};
};
