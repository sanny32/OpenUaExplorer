// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file startuparguments.cpp
/// \brief Implements command-line parsing for application startup.
///

#include "startuparguments.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>

namespace {

QString tr(const char *text)
{
    return QCoreApplication::translate("StartupArguments", text);
}

}

///
/// \brief Adds the application's options and positional arguments to a parser.
/// \param parser Parser to configure.
///
void configureStartupArguments(QCommandLineParser &parser)
{
    parser.setApplicationDescription(tr("OPC UA client and monitoring application."));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({{QStringLiteral("s"), QStringLiteral("session")},
                      tr("Open the saved application session at <path>."),
                      tr("path")});
    parser.addPositionalArgument(tr("session-file"),
                                 tr("Saved application session file to open."),
                                 QStringLiteral("[session-file]"));
}

///
/// \brief Resolves the session file selected by the parsed startup arguments.
/// \param parser Parser containing successfully parsed arguments.
/// \param sessionFile Destination for the selected path, or an empty string when none was given.
/// \param error Destination for a validation error.
/// \return True when the arguments select at most one session file.
///
bool resolveStartupSessionFile(const QCommandLineParser &parser,
                               QString *sessionFile,
                               QString *error)
{
    const QString optionPath = parser.value(QStringLiteral("session"));
    const QStringList positional = parser.positionalArguments();

    if (positional.size() > 1) {
        if (error)
            *error = tr("Only one session file can be opened at startup.");
        return false;
    }

    if (!optionPath.isEmpty() && !positional.isEmpty()) {
        if (error)
            *error = tr("Specify the session file either with --session or as a positional argument, not both.");
        return false;
    }

    if (sessionFile)
        *sessionFile = optionPath.isEmpty() ? positional.value(0) : optionPath;
    if (error)
        error->clear();
    return true;
}
