// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <array>

#include <QtGlobal>

class QtOpcUaRequestCoordinator
{
public:
    /// \brief Categories whose requests supersede only requests of the same category.
    enum class Operation {
        Discovery,
        Browse,
        ReferencesBrowse,
        NodeRead,
        ValueRead,
        Write,
        SessionNameRead,
        Count
    };

    /// \brief Identifies one request within a connection and operation generation.
    struct Token
    {
        Operation operation;
        quint64 connectionGeneration;
        quint64 operationGeneration;
    };

    /// \brief Starts a request and supersedes the previous request of its category.
    Token begin(Operation operation);

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
};
