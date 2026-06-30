// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include "qtopcuarequestcoordinator.h"

#include <algorithm>

/// \brief Starts a request and supersedes the previous request of its category.
QtOpcUaRequestCoordinator::Token QtOpcUaRequestCoordinator::begin(Operation operation)
{
    quint64 &generation = _generations.at(index(operation));
    return {operation, _connectionGeneration, ++generation, QString()};
}

/// \brief Starts a request that supersedes only the previous request with the same key.
QtOpcUaRequestCoordinator::Token QtOpcUaRequestCoordinator::begin(Operation operation,
                                                                  const QString &key)
{
    quint64 &generation = _keyedGenerations.at(index(operation))[key];
    return {operation, _connectionGeneration, ++generation, key};
}

/// \brief Reports whether a token can still complete.
bool QtOpcUaRequestCoordinator::isCurrent(const Token &token) const
{
    if (token.connectionGeneration != _connectionGeneration)
        return false;
    if (!token.key.isNull()) {
        return token.operationGeneration
            == _keyedGenerations.at(index(token.operation)).value(token.key);
    }
    return token.operationGeneration == _generations.at(index(token.operation));
}

/// \brief Completes a current token exactly once.
bool QtOpcUaRequestCoordinator::settle(const Token &token)
{
    if (!isCurrent(token))
        return false;
    if (!token.key.isNull())
        ++_keyedGenerations.at(index(token.operation))[token.key];
    else
        ++_generations.at(index(token.operation));
    return true;
}

/// \brief Invalidates requests in one category.
void QtOpcUaRequestCoordinator::cancel(Operation operation)
{
    ++_generations.at(index(operation));
    _keyedGenerations.at(index(operation)).clear();
}

/// \brief Invalidates every request and advances the connection generation.
void QtOpcUaRequestCoordinator::cancelAll()
{
    ++_connectionGeneration;
    for (quint64 &generation : _generations)
        ++generation;
    for (auto &keyed : _keyedGenerations)
        keyed.clear();
}

/// \brief Applies the minimum request timeout.
int QtOpcUaRequestCoordinator::boundedTimeout(int timeoutMs)
{
    return std::max(1000, timeoutMs);
}

/// \brief Converts an operation category to its generation-array index.
std::size_t QtOpcUaRequestCoordinator::index(Operation operation)
{
    return static_cast<std::size_t>(operation);
}
