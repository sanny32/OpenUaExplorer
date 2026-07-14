// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file application.h
/// \brief Declares the application class.
///

#pragma once

#include <QApplication>
#include <QString>

#include "appsettings.h"
#include "apptheme.h"

class Application : public QApplication
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the application, installs the platform style, and applies the initial theme.
    /// \param argc Argument count passed to main.
    /// \param argv Argument vector passed to main.
    ///
    Application(int &argc, char **argv);

    ///
    /// \brief Gives access to the application theme manager.
    /// \return Reference to the application theme manager.
    ///
    AppTheme& theme();

    ///
    /// \brief Returns the running application downcast to this type.
    /// \return Pointer to the global Application instance.
    ///
    static Application *instance();

    ///
    /// \brief Persists the OPC UA timestamp display preference and notifies listeners.
    /// \param mode Timestamp mode to apply.
    ///
    void setTimestampMode(AppSettings::TimestampMode mode);

    ///
    /// \brief Hands the main window the session file the shell started the program with.
    ///
    /// A file named on the command line, and on macOS one dropped on the bundle before the
    /// window exists, is held back until this is called, because nothing is listening for
    /// sessionFileRequested() until the main window has been built.
    ///
    void deliverPendingSessionFile();

signals:
    ///
    /// \brief Emitted when the desktop shell asks the application to open a session file.
    /// \param path Path to the session file.
    ///
    void sessionFileRequested(const QString &path);

    ///
    /// \brief Emitted when the OPC UA timestamp display preference changes.
    /// \param mode The newly applied timestamp mode.
    ///
    void timestampModeChanged(AppSettings::TimestampMode mode);

protected:
    bool event(QEvent *event) override;

private:
    void configureCertificateStore();
    void ensureClientCertificate();
    void requestSessionFile(const QString &path);

    AppTheme _theme;
    QString _pendingSessionFile;
    bool _sessionFileHandlerReady = false;
};

///
/// \brief Returns the running application downcast to this type.
/// \return Pointer to the global Application instance.
///
inline Application *theApp()
{
    return static_cast<Application *>(qApp);
}
