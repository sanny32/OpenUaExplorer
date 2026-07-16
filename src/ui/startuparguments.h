// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file startuparguments.h
/// \brief Declares command-line parsing for application startup.
///

#pragma once

#include <QString>

class QCommandLineParser;

///
/// \brief Adds the application's options and positional arguments to a parser.
/// \param parser Parser to configure.
///
void configureStartupArguments(QCommandLineParser &parser);

///
/// \brief Resolves the session file selected by the parsed startup arguments.
/// \param parser Parser containing successfully parsed arguments.
/// \param sessionFile Destination for the selected path, or an empty string when none was given.
/// \param error Destination for a validation error.
/// \return True when the arguments select at most one session file.
///
bool resolveStartupSessionFile(const QCommandLineParser &parser,
                               QString *sessionFile,
                               QString *error = nullptr);
