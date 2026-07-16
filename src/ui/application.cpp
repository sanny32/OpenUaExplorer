// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file application.cpp
/// \brief Implements the application class.
///

#include <QFileInfo>
#include <QFileOpenEvent>
#include <QTimer>

#include <utility>

#include "appstyle.h"
#include "application.h"
#include "loggingcategories.h"
#include "opcua/pkimanager.h"

#if defined(HAVE_QLEMENTINE_APP_STYLE) && defined(Q_OS_MAC)
#include "macappstyle.h"
#elif defined(HAVE_QLEMENTINE_APP_STYLE)
#include "qlementineappstyle.h"
#endif

///
/// \brief Constructs the application, installs the platform style, and applies the initial theme.
/// \param argc Argument count passed to main.
/// \param argv Argument vector passed to main.
///
Application::Application(int &argc, char **argv)
    : QApplication(argc, argv)
    , _theme(this)
{
#ifdef Q_OS_LINUX
    configureCertificateStore();
#endif

    setApplicationName(QStringLiteral(APP_PRODUCT_NAME));
    setApplicationVersion(QStringLiteral(APP_VERSION));

#if defined(HAVE_QLEMENTINE_APP_STYLE) && defined(Q_OS_MAC)
    setStyle(new MacAppStyle(this));
#elif defined(HAVE_QLEMENTINE_APP_STYLE)
    setStyle(new QlementineAppStyle(this));
#else
    setStyle(new AppStyle(this));
#endif
    _theme.applyInitialScheme();
    ensureClientCertificate();

}

///
/// \brief Hands the main window the session file the shell started the program with.
///
void Application::deliverPendingSessionFile()
{
    _sessionFileHandlerReady = true;

    if (_pendingSessionFile.isEmpty())
        return;

    // Queued rather than emitted straight away: a session that fails to load opens a
    // modal dialog, and a dialog whose exec() runs before the main event loop has
    // started leaves the application quitting the moment it is dismissed, because
    // Qt then sees the last window close before the window it belongs to is up.
    QTimer::singleShot(0, this, [this]() {
        emit sessionFileRequested(std::exchange(_pendingSessionFile, QString()));
    });
}

///
/// \brief Takes the macOS request to open a document into the session-file flow.
/// \param event Event delivered to the application.
/// \return True when the event was consumed here.
///
bool Application::event(QEvent *event)
{
    if (event->type() == QEvent::FileOpen) {
        const QFileOpenEvent *fileEvent = static_cast<QFileOpenEvent *>(event);
        requestSessionFile(fileEvent->file());
        return true;
    }

    return QApplication::event(event);
}

///
/// \brief Queues or emits a request to open a saved application session file.
/// \param path Path to the session file.
///
void Application::requestSessionFile(const QString &path)
{
    if (path.isEmpty())
        return;

    const QString absolutePath = QFileInfo(path).absoluteFilePath();
    if (!_sessionFileHandlerReady) {
        _pendingSessionFile = absolutePath;
        return;
    }

    emit sessionFileRequested(absolutePath);
}

///
/// \brief Gives access to the application theme manager.
/// \return Reference to the application theme manager.
///
AppTheme& Application::theme()
{
    return _theme;
}

///
/// \brief Returns the running application downcast to this type.
/// \return Pointer to the global Application instance.
///
Application *Application::instance()
{
    return static_cast<Application *>(QApplication::instance());
}

///
/// \brief Persists the OPC UA timestamp display preference and notifies listeners.
/// \param mode Timestamp mode to apply.
///
void Application::setTimestampMode(AppSettings::TimestampMode mode)
{
    AppSettings settings;
    if (settings.timestampMode() == mode)
        return;
    settings.setTimestampMode(mode);
    emit timestampModeChanged(mode);
}

#ifdef Q_OS_LINUX
///
/// \brief Points OpenSSL at the trust store of the distribution it is running on.
///
/// The packaged build carries its own OpenSSL, which has /etc/ssl compiled into it as
/// the place to look for CA certificates: that is where Debian, Ubuntu, Astra and
/// openSUSE keep them, and Fedora and RHEL symlink it onto their own. ALT keeps them
/// in /var/lib/ssl and has no such symlink, so there the store has to be found at run
/// time. It is looked up only when the compiled-in path is absent, and never against
/// an environment that already names one.
///
void Application::configureCertificateStore()
{
    if (QFileInfo::exists(QStringLiteral("/etc/ssl/certs")))
        return;

    if (qEnvironmentVariableIsSet("SSL_CERT_FILE") || qEnvironmentVariableIsSet("SSL_CERT_DIR"))
        return;

    static const char *const bundles[] = {
        "/var/lib/ssl/cert.pem",
        "/etc/pki/tls/certs/ca-bundle.crt",
    };
    static const char *const directories[] = {
        "/var/lib/ssl/certs",
        "/etc/pki/tls/certs",
    };

    for (const char *const bundle : bundles) {
        if (QFileInfo::exists(QString::fromLatin1(bundle))) {
            qputenv("SSL_CERT_FILE", bundle);
            break;
        }
    }

    for (const char *const directory : directories) {
        if (QFileInfo::exists(QString::fromLatin1(directory))) {
            qputenv("SSL_CERT_DIR", directory);
            break;
        }
    }
}
#endif

///
/// \brief Generates the application client certificate when the auto-generated pair is absent.
///
void Application::ensureClientCertificate()
{
    PkiManager pki;
    QString certificateFile;
    QString privateKeyFile;
    if (pki.existingClientCertificate(&certificateFile, &privateKeyFile))
        return;

    QString error;
    if (!pki.generateClientCertificate(
            PkiManager::clientCertificateCommonName(),
            PkiManager::applicationUri(),
            &certificateFile,
            &privateKeyFile,
            &error)) {
        qCWarning(lcApp).noquote()
            << tr("Could not generate the client certificate: %1").arg(error);
    }
}
