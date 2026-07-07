// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file sessionstore.h
/// \brief Declares reading and writing of working-session files.
///

#pragma once

#include <QString>

#include "sessiondata.h"

///
/// \brief Serializes working sessions to and from JSON files.
///
/// The on-disk format is a single JSON document holding the connection profile,
/// user subscriptions, monitored data-access nodes and charted trend nodes. No
/// passwords are written; credentials are resolved from the keychain or prompted
/// for on load.
///
class SessionStore
{
public:
    ///
    /// \brief Current session file schema version.
    ///
    static constexpr int schemaVersion = 1;

    ///
    /// \brief Writes a session to a JSON file.
    /// \param path Destination file path.
    /// \param data Session payload to serialize.
    /// \param error Set to a human-readable message on failure.
    /// \return True on success.
    ///
    static bool save(const QString &path, const SessionData &data, QString *error = nullptr);

    ///
    /// \brief Reads a session from a JSON file.
    /// \param path Source file path.
    /// \param data Destination for the parsed session payload.
    /// \param error Set to a human-readable message on failure.
    /// \return True on success.
    ///
    static bool load(const QString &path, SessionData &data, QString *error = nullptr);
};
